#include "lisp.hpp"
#include "bulkAllocator.hpp"
#include "memory/buffer.hpp"
#include "memory/pool.hpp"


void english__to_string(int num, char* buffer, int base);


namespace lisp {


struct Variable {
    const char* name_ = "";
    Value* value_ = nullptr; // Careful! A lot of code expects value_ to contain
                             // nil, so make sure that you set value to the nil
                             // sentinel value after defining a variable. Some
                             // variable tables are allocated pior to allocating
                             // nil, otherwise, we would set nil as the default
                             // here, rather than nullptr.
};


static void run_gc();


static const u32 string_intern_table_size = 1000;


struct Context {
    using ValuePool = ObjectPool<Value, 99>;
    using OperandStack = Buffer<CompressedPtr, 594>;
    using Globals = std::array<Variable, 149>;
    using Interns = char[string_intern_table_size];

    Value* nil_ = nullptr;
    Value* oom_ = nullptr;

    int string_intern_pos_ = 0;

    int eval_depth_ = 0;

    Context(Platform& pfrm)
        : value_pools_{allocate_dynamic<ValuePool>(pfrm),
                       allocate_dynamic<ValuePool>(pfrm),
                       allocate_dynamic<ValuePool>(pfrm),
                       allocate_dynamic<ValuePool>(pfrm),
                       allocate_dynamic<ValuePool>(pfrm),
                       allocate_dynamic<ValuePool>(pfrm),
                       allocate_dynamic<ValuePool>(pfrm),
                       allocate_dynamic<ValuePool>(pfrm)},
          operand_stack_(allocate_dynamic<OperandStack>(pfrm)),
          globals_(allocate_dynamic<Globals>(pfrm)),
          interns_(allocate_dynamic<Interns>(pfrm))
    {
        for (auto& pl : value_pools_) {
            if (not pl.obj_) {
                while (true)
                    ; // FIXME: raise error
            }
        }

        if (not operand_stack_.obj_ or not globals_.obj_ or not interns_.obj_) {

            while (true)
                ; // FIXME: raise error
        }
    }

    // We're allocating 8k bytes toward lisp values. Because our simplistic
    // allocation strategy cannot support sizes other than 1k, we need to split
    // our memory for lisp values into regions.
    static constexpr const int value_pool_count = 8;
    DynamicMemory<ValuePool> value_pools_[value_pool_count];

    DynamicMemory<OperandStack> operand_stack_;
    DynamicMemory<Globals> globals_;
    DynamicMemory<Interns> interns_;
};


static std::optional<Context> bound_context;


Value* get_nil()
{
    if (not bound_context->nil_) {
        // Someone has likely called git_nil() before calling init().
        while (true) ;
    }
    return bound_context->nil_;
}


const char* intern(const char* string)
{
    const auto len = str_len(string);

    if (len + 1 >
        string_intern_table_size - bound_context->string_intern_pos_) {
        while (true)
            ; // TODO: raise error, table full...
    }

    auto& ctx = bound_context;

    const char* search = *ctx->interns_.obj_;
    for (int i = 0; i < ctx->string_intern_pos_;) {
        if (strcmp(search + i, string) == 0) {
            return search + i;
        } else {
            while (search[i] not_eq '\0') {
                ++i;
            }
            ++i;
        }
    }

    auto result = *ctx->interns_.obj_ + ctx->string_intern_pos_;

    for (u32 i = 0; i < len; ++i) {
        (*ctx->interns_.obj_)[ctx->string_intern_pos_++] = string[i];
    }
    (*ctx->interns_.obj_)[ctx->string_intern_pos_++] = '\0';

    return result;
}


CompressedPtr compr(Value* val)
{
    for (int pool_id = 0; pool_id < Context::value_pool_count; ++pool_id) {
        auto& cells = bound_context->value_pools_[pool_id].obj_->cells();
        if ((char*)val >= (char*)cells.data() and
            (char*)val < (char*)(cells.data() + cells.size())) {

            static_assert((1 << CompressedPtr::source_pool_bits) - 1 >=
                          Context::value_pool_count - 1,
                          "Source pool bits in compressed ptr insufficient "
                          "to address all value pools");

            static_assert((1 << CompressedPtr::offset_bits) - 1 >=
                          sizeof(Context::ValuePool::Cells),
                          "Compressed pointer offset insufficient to address "
                          "all memory in value pool");

            CompressedPtr result;
            result.source_pool_ = pool_id;
            result.offset_ = (char*)val - (char*)cells.data();

            return result;
        }
    }

    while (true) ; // Attempt to compress invalid pointer
}


Value* dcompr(CompressedPtr ptr)
{
    auto& cells = bound_context->value_pools_[ptr.source_pool_].obj_->cells();
    return (Value*)((char*)cells.data() + ptr.offset_);
}


int length(Value* lat)
{
    int len = 0;
    while (true) {
        ++len;
        lat = lat->cons_.cdr();
        if (lat->type_ not_eq Value::Type::cons) {
            if (lat not_eq get_nil()) {
                return 0; // not a well-formed list
            }
            break;
        }
    }
    return len;
}


static Value* alloc_value()
{
    auto init_val = [](Value* val) {
        val->mark_bit_ = false;
        val->alive_ = true;
        return val;
    };

    for (auto& pl : bound_context->value_pools_) {
        if (not pl.obj_->empty()) {
            return init_val(pl.obj_->get());
        }
    }

    run_gc();

    // Hopefully, we've freed up enough memory...
    for (auto& pl : bound_context->value_pools_) {
        if (not pl.obj_->empty()) {
            return init_val(pl.obj_->get());
        }
    }

    return nullptr;
}


Value* make_function(Function::Impl impl)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::function;
        val->function_.impl_ = impl;
        return val;
    }
    return bound_context->oom_;
}


Value* make_cons(Value* car, Value* cdr)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::cons;
        val->cons_.set_car(car);
        val->cons_.set_cdr(cdr);
        return val;
    }
    return bound_context->oom_;
}


Value* make_integer(s32 value)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::integer;
        val->integer_.value_ = value;
        return val;
    }
    return bound_context->oom_;
}


Value* make_list(u32 length)
{
    if (length == 0) {
        return get_nil();
    }
    auto head = make_cons(get_nil(), get_nil());
    while (--length) {
        push_op(head); // To keep head from being collected, in case make_cons()
                       // triggers the gc.
        auto cell = make_cons(get_nil(), head);
        pop_op(); // head

        head = cell;
    }
    return head;
}


Value* make_error(Error::Code error_code)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::error;
        val->error_.code_ = error_code;
        return val;
    }
    return bound_context->oom_;
}


Value* make_symbol(const char* name)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::symbol;
        val->symbol_.name_ = intern(name);
        return val;
    }
    return bound_context->oom_;
}


Value* make_userdata(void* obj)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::user_data;
        val->user_data_.obj_ = obj;
        return val;
    }
    return bound_context->oom_;
}


void set_list(Value* list, u32 position, Value* value)
{
    while (position--) {
        if (list->type_ not_eq Value::Type::cons) {
            // TODO: raise error
            return;
        }
        list = list->cons_.cdr();
    }

    if (list->type_ not_eq Value::Type::cons) {
        // TODO: raise error
        return;
    }

    list->cons_.set_car(value);
}


Value* get_list(Value* list, u32 position)
{
    while (position--) {
        if (list->type_ not_eq Value::Type::cons) {
            // TODO: raise error
            return get_nil();
        }
        list = list->cons_.cdr();
    }

    if (list->type_ not_eq Value::Type::cons) {
        // TODO: raise error
        return get_nil();
    }

    return list->cons_.car();
}


void pop_op()
{
    bound_context->operand_stack_.obj_->pop_back();
}


void push_op(Value* operand)
{
    if (not bound_context->operand_stack_.obj_->push_back(compr(operand))) {
        while (true)
            ; // TODO: raise error
    }
}


Value* get_op(u32 offset)
{
    auto& stack = bound_context->operand_stack_;
    if (offset >= stack.obj_->size()) {
        return get_nil(); // TODO: raise error
    }

    return dcompr((*stack.obj_)[(stack.obj_->size() - 1) - offset]);
}


// The function arguments should be sitting at the top of the operand stack
// prior to calling funcall. The arguments will be consumed, and replaced with
// the result of the function call.
void funcall(Value* obj, u8 argc)
{
    if (obj->type_ not_eq Value::Type::function) {
        push_op(make_error(Error::Code::value_not_callable));
        return;
    }

    if (bound_context->operand_stack_.obj_->size() < argc) {
        push_op(make_error(Error::Code::invalid_argc));
        return;
    }

    auto result = obj->function_.impl_(argc);
    for (int i = 0; i < argc; ++i) {
        bound_context->operand_stack_.obj_->pop_back();
    }

    push_op(result);
}


Value* set_var(const char* name, Value* value)
{
    auto& globals = *bound_context->globals_.obj_;
    for (u32 i = 0; i < globals.size(); ++i) {
        if (strcmp(name, globals[i].name_) == 0) {
            std::swap(globals[i], globals[0]);
            globals[0].value_ = value;
            return get_nil();
        }
    }

    for (auto& var : *bound_context->globals_.obj_) {
        if (strcmp(var.name_, "") == 0) {
            var.name_ = intern(name);
            var.value_ = value;
            return get_nil();
        }
    }

    return make_error(Error::Code::symbol_table_exhausted);
}


Value* get_var(const char* name)
{
    auto& globals = *bound_context->globals_.obj_;
    for (u32 i = 0; i < globals.size(); ++i) {
        if (strcmp(name, globals[i].name_) == 0) {
            std::swap(globals[i], globals[0]);
            return globals[0].value_;
        }
    }

    return make_error(Error::Code::undefined_variable_access);
}


static bool is_whitespace(char c)
{
    return c == ' ' or c == '\r' or c == '\n' or c == '\t';
}


static int eat_whitespace(const char* c)
{
    int i = 0;
    while (is_whitespace(c[i])) {
        ++i;
        if (c[i] == '\0') {
            return i;
        }
    }

    while (c[i] == ';') { // Skip comments
        while (c[i] not_eq '\n') {
            ++i;
            if (c[i] == '\0') {
                return i;
            }
        }
        while (is_whitespace(c[i])) {
            ++i;
            if (c[i] == '\0') {
                return i;
            }
        }
    }

    return i;
}


static u32 expr_len(const char* str, u32 script_len)
{
    int paren_count = 0;
    u32 i = 0;
    for (; i < script_len; ++i) {
        if (str[i] == '(') {
            paren_count = 1;
            ++i;
            break;
        }
    }
    for (; i < script_len; ++i) {
        if (str[i] == '(') {
            ++paren_count;
        }
        if (str[i] == ')') {
            --paren_count;
        }
        if (paren_count == 0) {
            return i + 1;
        }
    }
    return i;
}


static bool is_boolean_true(Value* val)
{
    switch (val->type_) {
    case Value::Type::integer:
        return val->integer_.value_ not_eq 0;

    default:
        break;
    }

    return val not_eq get_nil();
}


static u32 eval_while(const char* expr, u32 len)
{
    // (while <COND> <EXPR>)

    push_op(get_nil()); // The return value, will be replaced if the loop
                        // branches into the body expression.

 TOP:
    int i = 0;

    i += eval(expr) + 1;

    const bool done = not is_boolean_true(get_op(0));
    pop_op();

    i += eat_whitespace(expr + i);

    if (done) {
        if (expr[i] == '(') {
            i += expr_len(expr + i, str_len(expr + i));
        } else {
            while (expr[i] not_eq ' ') {
                ++i;
                if (expr[i] == ')') {
                    break;
                }
            }
        }
        return i + 7;
    } else {
        pop_op();
        eval(expr + i);
        goto TOP;
    }
}


static u32 eval_if(const char* expr, u32 len)
{
    // (if <COND> <EXPR1> <EXPR2>)

    int i = 0;

    i += eval(expr) + 1;

    const bool branch = is_boolean_true(get_op(0));

    pop_op();

    // Now we'll need to skip over one of the branches. If the next
    // non-whitespace character is not a '(' or a ''', then should be easy,
    // because we can just eat chars until we hit some whitespace or a ')'. If
    // then next non-whitespace character is a '(', we'll need to count
    // opening/closing parens to determine where the expression ends.

    i += eat_whitespace(expr + i);

    if (not branch) {
        if (expr[i] == '(') {
            i += expr_len(expr + i, str_len(expr + i));
        } else {
            while (expr[i] not_eq ' ') {
                ++i;
                if (expr[i] == ')') {
                    break;
                }
            }
        }
    } else {
        i += eval(expr + i);
    }

    i += eat_whitespace(expr + i);

    if (expr[i] == ')') { // if without an else branch
        if (not branch) {
            push_op(get_nil());
        }
        return i + 4;
    }

    if (not branch) {
        // We've skipped the first expression, and want to eval the second one.
        const auto consumed = eval(expr + i);

        i += consumed;

        i += eat_whitespace(expr + i);

        return i + 4;
    } else {
        // We've evaulated the first expresion, and want to skip the second one.
        if (expr[i] == '(' or expr[i] == '\'') {
            i += expr_len(expr + i, str_len(expr + i));
        } else {
            while (expr[i] not_eq ' ' and expr[i] not_eq ')') {
                ++i;
            }
        }
    }

    i += eat_whitespace(expr + i);

    return i + 4;
}


static u32 eval_expr(const char* expr, u32 len)
{
    u32 i = 0;

    static const int max_fn_name = 31;
    char fn_name[max_fn_name + 1];
    fn_name[0] = '\0';

    i += eat_whitespace(expr + i);

    for (; i < len; ++i) {
        int start = i;
        while (i < len and i < max_fn_name and (not is_whitespace(expr[i])) and
               expr[i] not_eq ')') {

            fn_name[i - start] = expr[i];
            ++i;
        }
        fn_name[i - start] = '\0';
        break;
    }

    if (strcmp("if", fn_name) == 0) {
        return eval_if(expr + i, len - i);
    } else if (strcmp("while", fn_name) == 0) {
        return eval_while(expr + i, len - i);
    }

    i += eat_whitespace(expr + i);

    int param_count = 0;

    while (expr[i] not_eq ')') {
        i += eat_whitespace(expr + i);

        i += eval(expr + i);

        ++param_count;

        i += eat_whitespace(expr + i);
    }
    i += 2;

    const auto fn = get_var(fn_name);
    if (fn->type_ not_eq Value::Type::function) {
        for (int i = 0; i < param_count; ++i) {
            pop_op();
        }
        push_op(fn);
        return i;
    }

    funcall(fn, param_count);

    return i;
}


static bool is_numeric(char c)
{
    return c == '0' or c == '1' or c == '2' or c == '3' or c == '4' or
           c == '5' or c == '6' or c == '7' or c == '8' or c == '9';
}


static u32 eval_number(const char* code, u32 len)
{
    u32 result = 0;

    u32 i = 0;
    for (; i < len; ++i) {
        if (is_whitespace(code[i]) or code[i] == ')') {
            break;
        } else {
            result = result * 10 + (code[i] - '0');
        }
    }

    lisp::push_op(lisp::make_integer(result));
    return i;
}


static u32 eval_variable(const char* code, u32 len)
{
    static const u32 max_var_name = 31;
    char variable_name[max_var_name];

    u32 i = 0;

    for (; i < len; ++i) {
        if (is_whitespace(code[i]) or code[i] == ')') {
            break;
        } else {
            variable_name[i] = code[i];
        }
    }

    variable_name[i] = '\0';

    // FIXME: actually support quoted stuff...
    if (variable_name[0] == '\'') {
        push_op(make_symbol(variable_name + 1));
        return i;
    }

    if (strcmp(variable_name, "nil") == 0) {
        push_op(get_nil());
    } else {
        auto var = lisp::get_var(variable_name);
        push_op(var);
    }
    return i;
}


static u32 eval_value(const char* code, u32 len)
{
    if (is_numeric(code[0])) {
        return eval_number(code, len);
    } else {
        return eval_variable(code, len);
    }
}


u32 eval(const char* code)
{
    const auto code_len = str_len(code);

    u32 i = 0;
    i += eat_whitespace(code);

    for (; i < code_len; ++i) {
        if (is_whitespace(code[i])) {
            while (true) ;
        }
        if (code[i] == '(') {
            bound_context->eval_depth_ += 1;
            auto result = eval_expr(code + i + 1, code_len - (i + 1));
            bound_context->eval_depth_ -= 1;
            return result;
        } else {
            return eval_value(code + i, code_len - i);
        }
    }
    push_op(get_nil());
    return code_len;
}


void dostring(const char* code)
{
    const auto script_len = str_len(code);

    // I designed the eval function to read a single expression. Find where each
    // expression begins and ends, and skip ahead to the next expression in the
    // file after reading the current one. Ideally, I would simply fix whatever
    // bug in the eval function causes incorrect result offsets...

    u32 i = 0;
    while (i < script_len) {
        lisp::eval(code + i);

        auto len = expr_len(code + i, script_len - i);

        // TODO: check for errors!

        i += len;

        lisp::pop_op();
    }
}


void format_impl(Value* value, StringBuffer<47>& buffer)
{
    switch (value->type_) {
    case lisp::Value::Type::nil:
        buffer += "nil";
        break;

    case lisp::Value::Type::symbol:
        buffer += value->symbol_.name_;
        break;

    case lisp::Value::Type::integer: {
        char str[32];
        english__to_string(value->integer_.value_, str, 10);
        buffer += str;
        break;
    }

    case lisp::Value::Type::cons:
        buffer.push_back('(');
        format_impl(value->cons_.car(), buffer);
        if (value->cons_.cdr()->type_ not_eq Value::Type::cons) {
            buffer += " . ";
            format_impl(value->cons_.cdr(), buffer);
        } else {
            auto current = value;
            while (true) {
                if (current->cons_.cdr()->type_ == Value::Type::cons) {
                    buffer += " ";
                    format_impl(current->cons_.cdr()->cons_.car(), buffer);
                    current = current->cons_.cdr();
                } else if (current->cons_.cdr() not_eq get_nil()) {
                    buffer += " ";
                    format_impl(current->cons_.cdr(), buffer);
                    break;
                } else {
                    break;
                }
            }
        }
        buffer.push_back(')');
        break;

    case lisp::Value::Type::function:
        buffer += "<lambda>";
        break;

    case lisp::Value::Type::user_data:
        buffer += "<ud>";
        break;

    case lisp::Value::Type::error:
        buffer += "ERR: ";
        buffer += lisp::Error::get_string(value->error_.code_);
        ;
        break;
    }
}


StringBuffer<47> format(Value* value)
{
    StringBuffer<47> result;
    format_impl(value, result);
    return result;
}


// Garbage Collection:
//
// Each object already contains a mark bit. We will need to trace the global
// variable table and the operand stack, and deal with all of the gc
// roots. Then, we'll need to scan through the raw slab of memory allocated
// toward each memory pool used for lisp::Value instances (not the
// freelist!). For any cell in the pool with an unset mark bit, we'll add that
// node back to the pool.


static void gc_mark_value(Value* value)
{
    switch (value->type_) {
    case Value::Type::cons:
        if (value->cons_.cdr()->type_ == Value::Type::cons) {
            auto current = value;

            current->mark_bit_ = true; // possibly redundant

            while (current->cons_.cdr()->type_ == Value::Type::cons) {
                gc_mark_value(current->cons_.car());
                current = current->cons_.cdr();
                current->mark_bit_ = true;
            }
            gc_mark_value(value->cons_.car());
            gc_mark_value(value->cons_.cdr());
        } else {
            gc_mark_value(value->cons_.car());
            gc_mark_value(value->cons_.cdr());
        }
        break;

    default:
        break;
    }

    value->mark_bit_ = true;
}


static void gc_mark()
{
    gc_mark_value(bound_context->nil_);
    gc_mark_value(bound_context->oom_);

    auto& ctx = bound_context;

    for (auto elem : *ctx->operand_stack_.obj_) {
        gc_mark_value(dcompr(elem));
    }

    for (auto& var : *ctx->globals_.obj_) {
        gc_mark_value(var.value_);
    }
}


static void gc_sweep()
{
    for (auto& pl : bound_context->value_pools_) {
        pl.obj_->scan_cells([&pl](Value* val) {
            if (val->alive_) {
                if (val->mark_bit_) {
                    val->mark_bit_ = false;
                } else {
                    // This value should be unreachable, let's collect it.
                    val->alive_ = false;
                    pl.obj_->post(val);
                }
            }
        });
    }
}


static void run_gc()
{
    gc_mark();
    gc_sweep();
}


void init(Platform& pfrm)
{
    bound_context.emplace(pfrm);

    // We cannot be sure that the memory will be zero-initialized, so make sure
    // that all of the alive bits in the value pool entries are zero. This needs
    // to be done at least once, otherwise, gc will not work correctly.
    for (auto& pl : bound_context->value_pools_) {
        pl.obj_->scan_cells([](Value* val) {
            val->alive_ = false;
            val->mark_bit_ = false;
        });
    }

    bound_context->nil_ = alloc_value();
    bound_context->nil_->type_ = Value::Type::nil;

    bound_context->oom_ = alloc_value();
    bound_context->oom_->type_ = Value::Type::error;
    bound_context->oom_->error_.code_ = Error::Code::out_of_memory;

    for (auto& var : *bound_context->globals_.obj_) {
        var.value_ = get_nil();
    }

    if (dcompr(compr(get_nil())) not_eq get_nil()) {
        error(pfrm, "pointer compression test failed");
        while (true) ;
    }

    // For optimization purposes, the commonly used loop iteration variables are
    // placed at the beginning of the string intern table, which makes setting
    // the contents of variables bound to i or j faster than usual.
    intern("i");
    intern("j");

    set_var("set", make_function([](int argc) {
        L_EXPECT_ARGC(argc, 2);
        L_EXPECT_OP(1, symbol);

        const char* name = get_op(1)->symbol_.name_;
        lisp::set_var(name, get_op(0));

        return L_NIL;
    }));

    set_var("cons", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                return make_cons(get_op(1), get_op(0));
            }));

    set_var("car", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, cons);
                return get_op(0)->cons_.car();
            }));

    set_var("cdr", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, cons);
                return get_op(0)->cons_.cdr();
            }));

    set_var("list", make_function([](int argc) {
                auto lat = make_list(argc);
                for (int i = 0; i < argc; ++i) {
                    set_list(lat, i, get_op((argc - 1) - i));
                }
                return lat;
            }));

    set_var(
        "progn", make_function([](int argc) {
            // I could have defined progn at the language level, but because all of
            // the expressions are evaluated anyway, much easier to define progn as
            // a function.
            //
            // Drawbacks: (1) Defining progn takes up a small amount of memory for
            // the function object. (2) Extra use of the operand stack.
            return get_op(0);
        }));

    set_var("not", make_function([](int argc) {
        L_EXPECT_ARGC(argc, 1);
        return make_integer(not is_boolean_true(get_op(0)));
    }));

    set_var(
        "equal", make_function([](int argc) {
            L_EXPECT_ARGC(argc, 2);

            if (get_op(0)->type_ not_eq get_op(1)->type_) {
                return make_integer(0);
            }

            return make_integer([] {
                switch (get_op(0)->type_) {
                case Value::Type::integer:
                    return get_op(0)->integer_.value_ ==
                           get_op(1)->integer_.value_;

                case Value::Type::cons:
                    // TODO!
                    // This comparison needs to be done as efficiently as possible...
                    break;

                case Value::Type::function:
                    return get_op(0) == get_op(1);

                case Value::Type::error:
                    break;

                case Value::Type::symbol:
                    return get_op(0)->symbol_.name_ == get_op(1)->symbol_.name_;

                case Value::Type::user_data:
                    return get_op(0)->user_data_.obj_ ==
                           get_op(1)->user_data_.obj_;
                }
                return false;
            }());
        }));

    set_var("apply", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(0, cons);
                L_EXPECT_OP(1, function);

                auto lat = get_op(0);
                auto fn = get_op(1);

                int apply_argc = 0;
                while (lat not_eq get_nil()) {
                    if (lat->type_ not_eq Value::Type::cons) {
                        return make_error(Error::Code::invalid_argument_type);
                    }
                    ++apply_argc;
                    push_op(lat->cons_.car());

                    lat = lat->cons_.cdr();
                }

                funcall(fn, apply_argc);

                auto result = get_op(0);
                pop_op();

                return result;
            }));

    set_var("fill", make_function([](int argc) {
        L_EXPECT_ARGC(argc, 2);
        L_EXPECT_OP(1, integer);

        auto result = make_list(get_op(1)->integer_.value_);
        for (int i = 0; i < get_op(1)->integer_.value_; ++i) {
            set_list(result, i, get_op(0));
        }

        return result;
    }));

    set_var("length", make_function([](int argc) {
        L_EXPECT_ARGC(argc, 1);
        L_EXPECT_OP(0, cons);

        return make_integer(length(get_op(0)));
    }));

    set_var("<", make_function([](int argc) {
        L_EXPECT_ARGC(argc, 2);
        L_EXPECT_OP(0, integer);
        L_EXPECT_OP(1, integer);
        return make_integer(get_op(1)->integer_.value_ <
                            get_op(0)->integer_.value_);
    }));

    set_var(">", make_function([](int argc) {
        L_EXPECT_ARGC(argc, 2);
        L_EXPECT_OP(0, integer);
        L_EXPECT_OP(1, integer);
        return make_integer(get_op(1)->integer_.value_ >
                            get_op(0)->integer_.value_);
    }));

    set_var("+", make_function([](int argc) {
                int accum = 0;
                for (int i = 0; i < argc; ++i) {
                    L_EXPECT_OP(i, integer);
                    accum += get_op(i)->integer_.value_;
                }
                return make_integer(accum);
            }));

    set_var("-", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(1, integer);
                L_EXPECT_OP(0, integer);
                return make_integer(get_op(1)->integer_.value_ -
                                    get_op(0)->integer_.value_);
            }));

    set_var("*", make_function([](int argc) {
                int accum = 1;
                for (int i = 0; i < argc; ++i) {
                    L_EXPECT_OP(i, integer);
                    accum *= get_op(i)->integer_.value_;
                }
                return make_integer(accum);
            }));

    set_var("/", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(1, integer);
                L_EXPECT_OP(0, integer);
                return make_integer(get_op(1)->integer_.value_ /
                                    get_op(0)->integer_.value_);
            }));

    set_var("interp-stat", make_function([](int argc) {
                auto lat = make_list(4);
                auto& ctx = bound_context;
                int values_remaining = 0;
                for (auto& pl : ctx->value_pools_) {
                    values_remaining += pl.obj_->remaining();
                }

                push_op(lat); // for the gc
                set_list(lat, 0, make_integer(values_remaining));
                set_list(lat, 1, make_integer(ctx->string_intern_pos_));
                set_list(
                    lat, 2, make_integer(ctx->operand_stack_.obj_->size()));
                set_list(lat, 3, make_integer([&] {
                    int symb_tab_used = 0;
                    for (u32 i = 0; i < ctx->globals_.obj_->size(); ++i) {
                        if (strcmp("", (*ctx->globals_.obj_)[i].name_)) {
                            ++symb_tab_used;
                        }
                    }
                    return symb_tab_used;
                }()));
                pop_op(); // lat

                return lat;
            }));

    set_var("range", make_function([](int argc) {
                int start = 0;
                int end = 0;
                int incr = 1;

                if (argc == 2) {

                    L_EXPECT_OP(1, integer);
                    L_EXPECT_OP(0, integer);

                    start = get_op(1)->integer_.value_;
                    end = get_op(0)->integer_.value_;

                } else if (argc == 3) {

                    L_EXPECT_OP(2, integer);
                    L_EXPECT_OP(1, integer);
                    L_EXPECT_OP(0, integer);

                    start = get_op(2)->integer_.value_;
                    end = get_op(1)->integer_.value_;
                    incr = get_op(0)->integer_.value_;
                } else {
                    L_EXPECT_ARGC(argc, 2);
                }

                if (incr == 0) {
                    return get_nil();
                }

                auto lat = make_list((end - start) / incr);

                for (int i = start; i < end; i += incr) {
                    push_op(lat);
                    set_list(lat, (i - start) / incr, make_integer(i));
                    pop_op();
                }

                return lat;
            }));

    set_var("unbind", make_function([](int argc) {
        L_EXPECT_ARGC(argc, 1);
        L_EXPECT_OP(0, symbol);

        for (auto& var : *bound_context->globals_.obj_) {
            if (strcmp(get_op(0)->symbol_.name_, var.name_) == 0) {
                var.value_ = get_nil();
                var.name_ = "";
                return get_nil();
            }
        }

        return get_nil();
    }));

    set_var("bound", make_function([](int argc) {
        L_EXPECT_ARGC(argc, 1);
        L_EXPECT_OP(0, symbol);

        for (auto& var : *bound_context->globals_.obj_) {
            if (strcmp(get_op(0)->symbol_.name_, var.name_) == 0) {
                return make_integer(1);
            }
        }

        return make_integer(0);
    }));

    set_var("map", make_function([](int argc) {
        if (argc < 2) {
            return get_nil();
        }
        L_EXPECT_OP(argc - 1, function);

        // I've never seen map used with so many input lists, but who knows,
        // someone might try to call this with more than six inputs...
        Buffer<Value*, 6> inp_lats;

        if (argc < static_cast<int>(inp_lats.size())) {
            return get_nil(); // TODO: return error
        }

        for (int i = 0; i < argc - 1; ++i) {
            L_EXPECT_OP(i, cons);
            inp_lats.push_back(get_op(i));
        }

        const auto len = length(inp_lats[0]);
        if (len == 0) {
            return get_nil();
        }
        for (auto& l : inp_lats) {
            if (length(l) not_eq len) {
                return get_nil(); // return error instead!
            }
        }

        auto fn = get_op(argc - 1);

        int index = 0;

        Value* result = make_list(len);
        push_op(result); // protect from the gc

        // Because the length function returned a non-zero value, we've
        // already succesfully scanned the list, so we don't need to do any
        // type checking.

        while (index < len) {

            for (auto& lat : reversed(inp_lats)) {
                push_op(lat->cons_.car());
                lat = lat->cons_.cdr();
            }
            funcall(fn, inp_lats.size());

            set_list(result, index, get_op(0));
            pop_op();

            ++index;
        }

        pop_op(); // the protected result list

        return result;
    }));

    set_var("select", make_function([](int argc) {
        L_EXPECT_ARGC(argc, 2);
        L_EXPECT_OP(0, cons);
        L_EXPECT_OP(1, cons);

        const auto len = length(get_op(0));
        if (not len or len not_eq length(get_op(1))) {
            return get_nil();
        }

        auto input_list = get_op(1);
        auto selection_list = get_op(0);

        auto result = get_nil();
        for (int i = len - 1; i > -1; --i) {
            if (is_boolean_true(get_list(selection_list, i))) {
                push_op(result);
                auto next = make_cons(get_list(input_list, i), result);
                result = next;
                pop_op(); // result
            }
        }

        return result;
    }));

    set_var("gc", make_function([](int argc) {
                run_gc();
                return get_nil();
            }));
}


} // namespace lisp
