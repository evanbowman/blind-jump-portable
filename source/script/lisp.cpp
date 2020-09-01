#include "lisp.hpp"
#include "bulkAllocator.hpp"
#include "memory/buffer.hpp"
#include "memory/pool.hpp"


void english__to_string(int num, char* buffer, int base);


namespace lisp {


#define NIL L_NIL


Value nil{lisp::Value::Type::nil, true, true};


static Value oom{lisp::Value::Type::error, true, true};


struct Variable {
    const char* name_ = "";
    Value* value_ = NIL;
};


static void run_gc();


static const u32 string_intern_table_size = 1000;


struct Context {
    using ValuePool = ObjectPool<Value, 74>;
    using OperandStack = Buffer<Value*, 297>;
    using Globals = std::array<Variable, 149>;
    using Interns = char[string_intern_table_size];

    int string_intern_pos_ = 0;

    int eval_depth_ = 0;

    Context(Platform& pfrm)
        : value_pools_{allocate_dynamic<ValuePool>(pfrm),
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

    // We're allocating 4k bytes toward lisp values. Because our simplistic
    // allocation strategy cannot support sizes other than 1k, we need to split
    // our memory for lisp values into regions.
    DynamicMemory<ValuePool> value_pools_[4];

    DynamicMemory<OperandStack> operand_stack_;
    DynamicMemory<Globals> globals_;
    DynamicMemory<Interns> interns_;
};


static std::optional<Context> bound_context;


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
    return &oom;
}


Value* make_cons(Value* car, Value* cdr)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::cons;
        val->cons_.car_ = car;
        val->cons_.cdr_ = cdr;
        return val;
    }
    return &oom;
}


Value* make_integer(s32 value)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::integer;
        val->integer_.value_ = value;
        return val;
    }
    return &oom;
}


Value* make_list(u32 length)
{
    if (length == 0) {
        return NIL;
    }
    auto head = make_cons(NIL, NIL);
    while (--length) {
        push_op(head); // To keep head from being collected, in case make_cons()
                       // triggers the gc.
        auto cell = make_cons(NIL, head);
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
    return &oom;
}


Value* make_symbol(const char* name)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::symbol;
        val->symbol_.name_ = intern(name);
        return val;
    }
    return &oom;
}


Value* make_userdata(void* obj)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::user_data;
        val->user_data_.obj_ = obj;
        return val;
    }
    return &oom;
}


void set_list(Value* list, u32 position, Value* value)
{
    while (position--) {
        if (list->type_ not_eq Value::Type::cons) {
            // TODO: raise error
            return;
        }
        list = list->cons_.cdr_;
    }

    if (list->type_ not_eq Value::Type::cons) {
        // TODO: raise error
        return;
    }

    list->cons_.car_ = value;
}


Value* get_list(Value* list, u32 position)
{
    while (position--) {
        if (list->type_ not_eq Value::Type::cons) {
            // TODO: raise error
            return NIL;
        }
        list = list->cons_.cdr_;
    }

    if (list->type_ not_eq Value::Type::cons) {
        // TODO: raise error
        return NIL;
    }

    return list->cons_.car_;
}


void pop_op()
{
    bound_context->operand_stack_.obj_->pop_back();
}


void push_op(Value* operand)
{
    if (not bound_context->operand_stack_.obj_->push_back(operand)) {
        while (true)
            ; // TODO: raise error
    }
}


Value* get_op(u32 offset)
{
    auto& stack = bound_context->operand_stack_;
    if (offset >= stack.obj_->size()) {
        return NIL; // TODO: raise error
    }

    return (*stack.obj_)[(stack.obj_->size() - 1) - offset];
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
    for (auto& var : *bound_context->globals_.obj_) {
        if (strcmp(name, var.name_) == 0) {
            var.value_ = value;
            return NIL;
        }
    }

    for (auto& var : *bound_context->globals_.obj_) {
        if (strcmp(var.name_, "") == 0) {
            var.name_ = intern(name);
            var.value_ = value;
            return NIL;
        }
    }

    return make_error(Error::Code::symbol_table_exhausted);
}


Value* get_var(const char* name)
{
    for (auto& var : *bound_context->globals_.obj_) {
        if (strcmp(name, var.name_) == 0) {
            return var.value_;
        }
    }

    return make_error(Error::Code::undefined_variable_access);
}


static bool is_whitespace(char c)
{
    return c == ' ' or c == '\r' or c == '\n' or c == '\t';
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


static u32 eval_if(const char* expr, u32 len)
{
    // (if <COND> <EXPR1> <EXPR2>)

    int i = 0;

    i += eval(expr) + 1;

    const bool branch = [&] {
        switch (get_op(0)->type_) {
        case Value::Type::integer:
            return get_op(0)->integer_.value_ not_eq 0;

        default:
            break;
        }

        return get_op(0) not_eq NIL;
    }();

    pop_op();

    // Now we'll need to skip over one of the branches. If the next
    // non-whitespace character is not a '(' or a ''', then should be easy,
    // because we can just eat chars until we hit some whitespace or a ')'. If
    // then next non-whitespace character is a '(', we'll need to count
    // opening/closing parens to determine where the expression ends.

    while (is_whitespace(expr[i])) {
        ++i;
    }

    if (not branch) {
        if (expr[i] == '(') {
            i += expr_len(expr + i, str_len(expr + i));
        } else {
            while (expr[i] not_eq ' ') {
                ++i;
                if (expr[i] == ')') {
                    while (true)
                        ; // TODO: support single expr if statements...
                }
            }
        }
    } else {
        i += eval(expr + i);
    }

    while (is_whitespace(expr[i])) {
        ++i;
    }

    if (not branch) {
        // We've skipped the first expression, and want to eval the second one.
        const auto consumed = eval(expr + i);

        i += consumed;

        while (is_whitespace(expr[i])) {
            ++i;
        }

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


    while (is_whitespace(expr[i])) {
        ++i;
    }

    return i + 4;
}


static u32 eval_set(const char* expr, u32 len)
{
    // (set <NAME> <VALUE>)

    static const u32 max_var_name = 31;
    char variable_name[max_var_name];

    u32 i = 0;

    // parse name
    while (is_whitespace(expr[i])) {
        ++i;
    }

    int start = i;
    while (i < len and i < max_var_name and not is_whitespace(expr[i])) {
        variable_name[i - start] = expr[i];
        ++i;
    }
    variable_name[i - start] = '\0';

    while (is_whitespace(expr[i])) {
        ++i;
    }

    // parse value
    i += eval(expr + i);

    if (bound_context->eval_depth_ <= 1) {
        set_var(variable_name, get_op(0));
    }

    pop_op();

    if (bound_context->eval_depth_ <= 1) {
        push_op(NIL);
    } else {
        push_op(make_error(Error::Code::set_in_expression_context));
    }

    return i;
}


static u32 eval_expr(const char* expr, u32 len)
{
    u32 i = 0;

    static const int max_fn_name = 31;
    char fn_name[max_fn_name + 1];

    while (is_whitespace(expr[i])) {
        ++i;
    }

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

    if (strcmp("set", fn_name) == 0) {
        return eval_set(expr + i, len - i) + 2;
    } else if (strcmp("if", fn_name) == 0) {
        return eval_if(expr + i, len - i);
    }

    while (is_whitespace(expr[i])) {
        ++i;
    }

    int param_count = 0;

    while (expr[i] not_eq ')') {
        while (is_whitespace(expr[i])) {
            ++i;
        }

        i += eval(expr + i);

        ++param_count;

        while (is_whitespace(expr[i])) {
            ++i;
        }
    }
    i += 2;

    const auto fn = get_var(fn_name);
    if (fn == NIL) {
        for (int i = 0; i < param_count; ++i) {
            pop_op();
        }
        push_op(make_error(Error::Code::undefined_variable_access));
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
        push_op(NIL);
    } else {
        auto var = lisp::get_var(variable_name);
        if (var == NIL) {
            push_op(make_error(Error::Code::undefined_variable_access));
        } else {
            push_op(var);
        }
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

    for (u32 i = 0; i < code_len; ++i) {
        if (is_whitespace(code[i])) {
            continue;
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
    push_op(NIL);
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

        auto len = expr_len(code + i, script_len);

        // TODO: check for errors!

        i += len;

        lisp::pop_op();
    }
}


void format_impl(Value* value, StringBuffer<28>& buffer)
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
        format_impl(value->cons_.car_, buffer);
        if (value->cons_.cdr_->type_ not_eq Value::Type::cons) {
            buffer += " . ";
            format_impl(value->cons_.cdr_, buffer);
        } else {
            auto current = value;
            while (true) {
                if (current->cons_.cdr_->type_ == Value::Type::cons) {
                    buffer += " ";
                    format_impl(current->cons_.cdr_->cons_.car_, buffer);
                    current = current->cons_.cdr_;
                } else if (current->cons_.cdr_ not_eq NIL) {
                    buffer += " ";
                    format_impl(current->cons_.cdr_, buffer);
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


StringBuffer<28> format(Value* value)
{
    StringBuffer<28> result;
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
        if (value->cons_.cdr_->type_ == Value::Type::cons) {
            auto current = value;

            current->mark_bit_ = true; // possibly redundant

            while (current->cons_.cdr_->type_ == Value::Type::cons) {
                gc_mark_value(current->cons_.car_);
                current = current->cons_.cdr_;
                current->mark_bit_ = true;
            }
            gc_mark_value(value->cons_.car_);
            gc_mark_value(value->cons_.cdr_);
        } else {
            gc_mark_value(value->cons_.car_);
            gc_mark_value(value->cons_.cdr_);
        }
        break;

    default:
        break;
    }

    value->mark_bit_ = true;
}


static void gc_mark()
{
    auto& ctx = bound_context;

    for (auto elem : *ctx->operand_stack_.obj_) {
        gc_mark_value(elem);
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
    oom.error_.code_ = Error::Code::out_of_memory;

    bound_context.emplace(pfrm);

    // We cannot be sure that the memory will be zero-initialized, so make sure
    // that all of the alive bits in the value pool entries are zero. This needs
    // to be done at least once, otherwise, gc will not work correctly.
    for (auto& pl : bound_context->value_pools_) {
        pl.obj_->scan_cells([&pl](Value* val) {
            val->alive_ = false;
            val->mark_bit_ = false;
        });
    }

    set_var("cons", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                return make_cons(get_op(1), get_op(0));
            }));

    set_var("car", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, cons);
                return get_op(0)->cons_.car_;
            }));

    set_var("cdr", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, cons);
                return get_op(0)->cons_.cdr_;
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
                while (lat not_eq NIL) {
                    if (lat->type_ not_eq Value::Type::cons) {
                        return make_error(Error::Code::invalid_argument_type);
                    }
                    ++apply_argc;
                    push_op(lat->cons_.car_);

                    lat = lat->cons_.cdr_;
                }

                funcall(fn, apply_argc);

                auto result = get_op(0);
                pop_op();

                return result;
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

    set_var("div", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(1, integer);
                L_EXPECT_OP(0, integer);
                return make_integer(get_op(1)->integer_.value_ /
                                    get_op(0)->integer_.value_);
            }));

    set_var("interp-stat", make_function([](int argc) {
                auto lat = make_list(3);
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
                pop_op(); // lat

                return lat;
            }));

    set_var("range", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 3);
                L_EXPECT_OP(2, integer);
                L_EXPECT_OP(1, integer);
                L_EXPECT_OP(0, integer);

                const auto start = get_op(2)->integer_.value_;
                const auto end = get_op(1)->integer_.value_;
                const auto incr = get_op(0)->integer_.value_;

                auto lat = make_list((end - start) / incr);

                for (int i = start; i < end; i += incr) {
                    push_op(lat);
                    set_list(lat, i, make_integer(i));
                    pop_op();
                }

                return lat;
            }));

    set_var("gc", make_function([](int argc) {
                run_gc();
                return NIL;
            }));
}


} // namespace lisp
