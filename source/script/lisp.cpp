#include "lisp.hpp"
#include "bulkAllocator.hpp"
#include "bytecode.hpp"
#include "listBuilder.hpp"
#include "localization.hpp"
#include "memory/buffer.hpp"
#include "memory/pool.hpp"
#include <complex>
#ifdef __GBA__
#define HEAP_DATA __attribute__((section(".ewram")))
#else
#define HEAP_DATA
#endif


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


static int run_gc();


static const u32 string_intern_table_size = 1999;


#define VALUE_POOL_SIZE 9000


static HEAP_DATA Value value_pool_data[VALUE_POOL_SIZE];
static Value* value_pool = nullptr;


void value_pool_init()
{
    for (int i = 0; i < VALUE_POOL_SIZE; ++i) {
        Value* v = value_pool_data + i;

        v->alive_ = false;
        v->mark_bit_ = false;
        v->type_ = Value::Type::heap_node;

        v->heap_node_.next_ = value_pool;
        value_pool = v;
    }
}


Value* value_pool_alloc()
{
    if (value_pool) {
        auto ret = value_pool;
        value_pool = ret->heap_node_.next_;
        return (Value*)ret;
    }
    return nullptr;
}


void value_pool_free(Value* value)
{
    value->type_ = Value::Type::heap_node;
    value->alive_ = false;
    value->mark_bit_ = false;

    value->heap_node_.next_ = value_pool;
    value_pool = value;
}


struct Context {
    using OperandStack = Buffer<CompressedPtr, 994>;

    using Interns = char[string_intern_table_size];

    Context(Platform& pfrm)
        : operand_stack_(allocate_dynamic<OperandStack>(pfrm)),
          interns_(allocate_dynamic<Interns>(pfrm)), pfrm_(pfrm)
    {
        if (not operand_stack_ or not interns_) {

            while (true)
                ; // FIXME: raise error
        }
    }

    DynamicMemory<OperandStack> operand_stack_;
    DynamicMemory<Interns> interns_;

    u16 arguments_break_loc_;
    u8 current_fn_argc_ = 0;
    Value* this_ = nullptr;


    Value* nil_ = nullptr;
    Value* oom_ = nullptr;
    Value* string_buffer_ = nullptr;
    Value* globals_tree_ = nullptr;


    const IntegralConstant* constants_ = nullptr;
    u16 constants_count_ = 0;

    int string_intern_pos_ = 0;
    int eval_depth_ = 0;
    int interp_entry_count_ = 0;

    Platform& pfrm_;
};


static std::optional<Context> bound_context;


// Globals tree node:
// ((key . value) . (left-child . right-child))
//
// i.e.: Each global variable binding uses three cons cells.


static void globals_tree_insert(Value* key, Value* value)
{
    auto& ctx = *bound_context;

    Protected new_kvp(make_cons(key, value));

    if (ctx.globals_tree_ == get_nil()) {
        // The empty set of left/right children
        push_op(make_cons(get_nil(), get_nil()));

        auto new_tree = make_cons(new_kvp, get_op(0));
        pop_op();

        ctx.globals_tree_ = new_tree;

    } else {
        // Ok, if the tree exists, now we need to scan the tree, looking for the
        // key. If it exists, replace the existing value with our new
        // value. Otherwise, insert key at the terminal point.

        Protected current(ctx.globals_tree_);
        Protected prev(ctx.globals_tree_);
        bool insert_left = true;

        while (current not_eq get_nil()) {

            auto current_key = current->cons_.car()->cons_.car();

            if (current_key->symbol_.name_ == key->symbol_.name_) {
                // The key alreay exists, overwrite the previous value.
                current->cons_.car()->cons_.set_cdr(value);
                return;

            } else {
                prev = (Value*)current;

                if (current_key->symbol_.name_ < key->symbol_.name_) {
                    // Continue loop through left subtree
                    insert_left = true;
                    current = current->cons_.cdr()->cons_.car();
                } else {
                    // Continue loop through right subtree
                    insert_left = false;
                    current = current->cons_.cdr()->cons_.cdr();
                }
            }
        }

        if (insert_left) {
            push_op(make_cons(get_nil(), get_nil()));

            auto new_tree = make_cons(new_kvp, get_op(0));
            pop_op();

            prev->cons_.cdr()->cons_.set_car(new_tree);

        } else {
            push_op(make_cons(get_nil(), get_nil()));

            auto new_tree = make_cons(new_kvp, get_op(0));
            pop_op();

            prev->cons_.cdr()->cons_.set_cdr(new_tree);
        }
    }
}


using GlobalsTreeVisitor = ::Function<24, void(Value&, Value&)>;


static Value* left_subtree(Value* tree)
{
    return tree->cons_.cdr()->cons_.car();
}


static Value* right_subtree(Value* tree)
{
    return tree->cons_.cdr()->cons_.cdr();
}


static void set_right_subtree(Value* tree, Value* value)
{
    tree->cons_.cdr()->cons_.set_cdr(value);
}


// Invokes callback with (key . value) for each global var definition.
// In place traversal, using Morris algorithm.
static void globals_tree_traverse(Value* root, GlobalsTreeVisitor callback)
{
    if (root == get_nil()) {
        return;
    }

    auto current = root;
    auto prev = get_nil();

    while (current not_eq get_nil()) {

        if (left_subtree(current) == get_nil()) {
            callback(*current->cons_.car(), *current);
            current = right_subtree(current);
        } else {
            prev = left_subtree(current);

            while (right_subtree(prev) not_eq get_nil() and
                   right_subtree(prev) not_eq current) {
                prev = right_subtree(prev);
            }

            if (right_subtree(prev) == get_nil()) {
                set_right_subtree(prev, current);
                current = left_subtree(current);
            } else {
                set_right_subtree(prev, get_nil());
                callback(*current->cons_.car(), *current);
                current = right_subtree(current);
            }
        }
    }
}


static void globals_tree_erase(Value* key)
{
    auto& ctx = *bound_context;

    if (ctx.globals_tree_ == get_nil()) {
        return;
    }

    auto current = ctx.globals_tree_;
    auto prev = current;
    bool erase_left = true;

    while (current not_eq get_nil()) {

        auto current_key = current->cons_.car()->cons_.car();

        if (current_key->symbol_.name_ == key->symbol_.name_) {

            Protected erased(current);

            if (current == prev) {
                ctx.globals_tree_ = get_nil();
            } else {
                if (erase_left) {
                    prev->cons_.cdr()->cons_.set_car(get_nil());
                } else {
                    prev->cons_.cdr()->cons_.set_cdr(get_nil());
                }
            }

            auto reattach_child = [](Value& kvp, Value&) {
                globals_tree_insert(kvp.cons_.car(), kvp.cons_.cdr());
            };

            auto left_child = erased->cons_.cdr()->cons_.car();
            if (left_child not_eq get_nil()) {
                globals_tree_traverse(left_child, reattach_child);
            }

            auto right_child = erased->cons_.cdr()->cons_.cdr();
            if (right_child not_eq get_nil()) {
                globals_tree_traverse(right_child, reattach_child);
            }

            return;
        }

        prev = current;

        if (current_key->symbol_.name_ < key->symbol_.name_) {
            erase_left = true;
            current = current->cons_.cdr()->cons_.car();
        } else {
            erase_left = false;
            current = current->cons_.cdr()->cons_.cdr();
        }
    }
}


static Value* globals_tree_find(Value* key)
{
    auto& ctx = *bound_context;

    if (ctx.globals_tree_ == get_nil()) {
        return get_nil();
    }

    auto current = ctx.globals_tree_;

    while (current not_eq get_nil()) {

        auto current_key = current->cons_.car()->cons_.car();

        if (current_key->symbol_.name_ == key->symbol_.name_) {
            return current->cons_.car()->cons_.cdr();
        }

        if (current_key->symbol_.name_ < key->symbol_.name_) {
            current = current->cons_.cdr()->cons_.car();
        } else {
            current = current->cons_.cdr()->cons_.cdr();
        }
    }

    StringBuffer<31> hint("[var: ");
    hint += key->symbol_.name_;
    hint += "]";

    return make_error(Error::Code::undefined_variable_access,
                      make_string(bound_context->pfrm_, hint.c_str()));
}


void set_constants(const IntegralConstant* constants, u16 count)
{
    if (not bound_context) {
        return;
    }

    bound_context->constants_ = constants;
    bound_context->constants_count_ = count;
}


u16 symbol_offset(const char* symbol)
{
    return symbol - *bound_context->interns_;
}


const char* symbol_from_offset(u16 offset)
{
    return *bound_context->interns_ + offset;
}


Value* get_nil()
{
    if (not bound_context->nil_) {
        // Someone has likely called git_nil() before calling init().
        while (true)
            ;
    }
    return bound_context->nil_;
}


void get_interns(::Function<24, void(const char*)> callback)
{
    auto& ctx = bound_context;

    const char* search = *ctx->interns_;
    for (int i = 0; i < ctx->string_intern_pos_;) {
        callback(search + i);
        while (search[i] not_eq '\0') {
            ++i;
        }
        ++i;
    }

    for (u16 i = 0; i < bound_context->constants_count_; ++i) {
        callback((const char*)bound_context->constants_[i].name_);
    }
}


void get_env(::Function<24, void(const char*)> callback)
{
    auto& ctx = bound_context;

    globals_tree_traverse(ctx->globals_tree_, [&callback](Value& val, Value&) {
        callback((const char*)val.cons_.car()->symbol_.name_);
    });

    for (u16 i = 0; i < bound_context->constants_count_; ++i) {
        callback((const char*)bound_context->constants_[i].name_);
    }
}


Value* get_arg(u16 n)
{
    auto br = bound_context->arguments_break_loc_;
    auto argc = bound_context->current_fn_argc_;
    if (br >= ((argc - 1) - n)) {
        return dcompr((*bound_context->operand_stack_)[br - ((argc - 1) - n)]);
    } else {
        return get_nil();
    }
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

    const char* search = *ctx->interns_;
    for (int i = 0; i < ctx->string_intern_pos_;) {
        if (str_cmp(search + i, string) == 0) {
            return search + i;
        } else {
            while (search[i] not_eq '\0') {
                ++i;
            }
            ++i;
        }
    }

    auto result = *ctx->interns_ + ctx->string_intern_pos_;

    for (u32 i = 0; i < len; ++i) {
        (*ctx->interns_)[ctx->string_intern_pos_++] = string[i];
    }
    (*ctx->interns_)[ctx->string_intern_pos_++] = '\0';

    return result;
}


CompressedPtr compr(Value* val)
{
    CompressedPtr result;

#ifdef USE_COMPRESSED_PTRS
    static_assert(sizeof(Value) % 2 == 0);
    result.offset_ = ((u8*)val - (u8*)value_pool_data) / sizeof(Value);
#else
    result.ptr_ = val;
#endif

    return result;
}


Value* dcompr(CompressedPtr ptr)
{
#ifdef USE_COMPRESSED_PTRS
    auto ret = (Value*)(((ptr.offset_ * sizeof(Value)) + (u8*)value_pool_data));
    return ret;
#else
    return ptr.ptr_;
#endif // USE_COMPRESSED_PTRS
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

    if (auto val = value_pool_alloc()) {
        return init_val(val);
    }

    run_gc();

    // Hopefully, we've freed up enough memory...
    if (auto val = value_pool_alloc()) {
        return init_val(val);
    }

    return nullptr;
}


Value* make_function(Function::CPP_Impl impl)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::function;
        val->function_.cpp_impl_ = impl;
        val->mode_bits_ = Function::ModeBits::cpp_function;
        return val;
    }
    return bound_context->oom_;
}


static Value* make_lisp_function(Value* impl)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::function;
        val->function_.lisp_impl_.code_ = compr(impl);
        val->function_.lisp_impl_.docstring_ = compr(get_nil());
        val->mode_bits_ = Function::ModeBits::lisp_function;
        return val;
    }
    return bound_context->oom_;
}


Value* make_bytecode_function(Value* buffer)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::function;
        val->function_.bytecode_impl_.data_buffer_ = compr(buffer);
        val->mode_bits_ = Function::ModeBits::lisp_bytecode_function;
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


Value* make_error(Error::Code error_code, Value* context)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::error;
        val->error_.code_ = error_code;
        val->error_.context_ = compr(context);
        return val;
    }
    return bound_context->oom_;
}


Value* make_symbol(const char* name, Symbol::ModeBits mode)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::symbol;
        val->symbol_.name_ = [mode, name] {
            switch (mode) {
            case Symbol::ModeBits::requires_intern:
                break;

            case Symbol::ModeBits::stable_pointer:
                return name;
            }
            return intern(name);
        }();
        return val;
    }
    return bound_context->oom_;
}


static Value* intern_to_symbol(const char* already_interned_str)
{
    if (auto val = alloc_value()) {
        val->type_ = Value::Type::symbol;
        val->symbol_.name_ = already_interned_str;
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


Value* make_databuffer(Platform& pfrm)
{
    if (not pfrm.scratch_buffers_remaining()) {
        // Collect any data buffers that may be lying around.
        run_gc();
    }

    if (auto val = alloc_value()) {
        val->type_ = Value::Type::data_buffer;
        new ((ScratchBufferPtr*)val->data_buffer_.sbr_mem_)
            ScratchBufferPtr(pfrm.make_scratch_buffer());
        return val;
    }
    return bound_context->oom_;
}


void live_values(::Function<24, void(Value&)> callback);


Value* make_string(Platform& pfrm, const char* string)
{
    auto len = str_len(string);

    Value* existing_buffer = nullptr;
    decltype(len) free = 0;

    if (bound_context->string_buffer_ not_eq L_NIL) {
        auto buffer = bound_context->string_buffer_;
        free = 0;
        for (int i = SCRATCH_BUFFER_SIZE - 1; i > 0; --i) {
            if (buffer->data_buffer_.value()->data_[i] == '\0') {
                ++free;
            } else {
                break;
            }
        }
        if (free > len + 1) { // +1 for null term, > for other null term
            existing_buffer = buffer;
        } else {
            bound_context->string_buffer_ = L_NIL;
        }
    }


    if (existing_buffer) {
        const auto offset = (SCRATCH_BUFFER_SIZE - free) + 1;

        auto write_ptr = existing_buffer->data_buffer_.value()->data_ + offset;

        while (*string) {
            *write_ptr++ = *string++;
        }

        if (auto val = alloc_value()) {
            val->type_ = Value::Type::string;
            val->string_.data_buffer_ = compr(existing_buffer);
            val->string_.offset_ = offset;
            return val;
        } else {
            return bound_context->oom_;
        }
    } else {
        auto buffer = make_databuffer(pfrm);

        if (buffer == bound_context->oom_) {
            return bound_context->oom_;
        }

        Protected p(buffer);
        bound_context->string_buffer_ = buffer;

        for (int i = 0; i < SCRATCH_BUFFER_SIZE; ++i) {
            buffer->data_buffer_.value()->data_[i] = '\0';
        }
        auto write_ptr = buffer->data_buffer_.value()->data_;

        while (*string) {
            *write_ptr++ = *string++;
        }

        if (auto val = alloc_value()) {
            val->type_ = Value::Type::string;
            val->string_.data_buffer_ = compr(buffer);
            val->string_.offset_ = 0;
            return val;
        } else {
            return bound_context->oom_;
        }
    }
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
    bound_context->operand_stack_->pop_back();
}


void push_op(Value* operand)
{
#ifdef UNHOSTED
    if (not bound_context->operand_stack_->push_back(compr(operand))) {
        while (true)
            ; // TODO: raise error
    }
#else
    bound_context->operand_stack_->push_back(compr(operand));
#endif
}


void insert_op(u32 offset, Value* operand)
{
    auto& stack = bound_context->operand_stack_;
    auto pos = stack->end() - offset;
    stack->insert(pos, compr(operand));
}


Value* get_op(u32 offset)
{
    auto& stack = bound_context->operand_stack_;
    if (offset >= stack->size()) {
        return get_nil(); // TODO: raise error
    }

    return dcompr((*stack)[(stack.obj_->size() - 1) - offset]);
}


void vm_execute(Value* code, int start_offset);


// The function arguments should be sitting at the top of the operand stack
// prior to calling funcall. The arguments will be consumed, and replaced with
// the result of the function call.
void funcall(Value* obj, u8 argc)
{
    auto pop_args = [&argc] {
        for (int i = 0; i < argc; ++i) {
            bound_context->operand_stack_->pop_back();
        }
    };

    // NOTE: The callee must be somewhere on the operand stack, so it's safe
    // to store this unprotected var here.
    Value* prev_this = get_this();

    auto& ctx = *bound_context;
    auto prev_arguments_break_loc = ctx.arguments_break_loc_;
    auto prev_argc = ctx.current_fn_argc_;

    switch (obj->type_) {
    case Value::Type::function: {
        if (bound_context->operand_stack_->size() < argc) {
            pop_args();
            push_op(make_error(Error::Code::invalid_argc, obj));
            break;
        }

        switch (obj->mode_bits_) {
        case Function::ModeBits::cpp_function: {
            auto result = obj->function_.cpp_impl_(argc);
            pop_args();
            push_op(result);
            break;
        }

        case Function::ModeBits::lisp_function: {
            auto& ctx = *bound_context;
            const auto break_loc = ctx.operand_stack_->size() - 1;
            auto expression_list = dcompr(obj->function_.lisp_impl_.code_);
            auto result = get_nil();
            push_op(result);
            while (expression_list not_eq get_nil()) {
                if (expression_list->type_ not_eq Value::Type::cons) {
                    break;
                }
                pop_op(); // result
                ctx.arguments_break_loc_ = break_loc;
                ctx.current_fn_argc_ = argc;
                ctx.this_ = obj;
                eval(expression_list->cons_.car()); // new result
                expression_list = expression_list->cons_.cdr();
            }
            result = get_op(0);
            pop_op(); // result
            pop_args();
            push_op(result);
            break;
        }

        case Function::ModeBits::lisp_bytecode_function: {
            auto& ctx = *bound_context;
            const auto break_loc = ctx.operand_stack_->size() - 1;
            ctx.arguments_break_loc_ = break_loc;
            ctx.current_fn_argc_ = argc;
            ctx.this_ = obj;
            vm_execute(dcompr(obj->function_.bytecode_impl_.data_buffer_),
                       obj->function_.bytecode_impl_.bc_offset_);
            auto result = get_op(0);
            pop_op();
            pop_args();
            push_op(result);
            break;
        }
        }
        break;
    }

    default:
        push_op(make_error(Error::Code::value_not_callable, L_NIL));
        break;
    }

    bound_context->this_ = prev_this;
    ctx.arguments_break_loc_ = prev_arguments_break_loc;
    ctx.current_fn_argc_ = prev_argc;
}


u8 get_argc()
{
    return bound_context->current_fn_argc_;
}


Value* get_this()
{
    return bound_context->this_;
}


Value* get_var_stable(const char* intern_str)
{
    return get_var(make_symbol(intern_str, Symbol::ModeBits::stable_pointer));
}


Value* get_var(Value* symbol)
{
    if (symbol->symbol_.name_[0] == '$') {
        s32 argn = 0;
        for (u32 i = 1; symbol->symbol_.name_[i] not_eq '\0'; ++i) {
            argn = argn * 10 + (symbol->symbol_.name_[i] - '0');
        }

        return get_arg(argn);
    }

    auto found = globals_tree_find(symbol);

    if (found->type_ not_eq Value::Type::error) {
        return found;
    } else {
        for (u16 i = 0; i < bound_context->constants_count_; ++i) {
            const auto& k = bound_context->constants_[i];
            if (str_cmp(k.name_, symbol->symbol_.name_) == 0) {
                return lisp::make_integer(k.value_);
            }
        }
        return found;
    }
}


Value* set_var(Value* symbol, Value* val)
{
    globals_tree_insert(symbol, val);
    return get_nil();
}


bool is_boolean_true(Value* val)
{
    switch (val->type_) {
    case Value::Type::integer:
        return val->integer_.value_ not_eq 0;

    default:
        break;
    }

    return val not_eq get_nil();
}


static const long hextable[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,  1,  2,  3,  4,  5,  6,  7,  8,
    9,  -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1};


long hexdec(unsigned const char* hex)
{
    long ret = 0;
    while (*hex && ret >= 0) {
        ret = (ret << 4) | hextable[*hex++];
    }
    return ret;
}


bool is_executing()
{
    if (bound_context) {
        return bound_context->interp_entry_count_;
    }

    return false;
}


Value* dostring(const char* code, ::Function<16, void(Value&)> on_error)
{
    if (code == nullptr) {
        on_error(*L_NIL);
    }

    ++bound_context->interp_entry_count_;

    int i = 0;

    Protected result(get_nil());

    while (true) {
        i += read(code + i);
        auto reader_result = get_op(0);
        if (reader_result == get_nil()) {
            pop_op();
            break;
        }
        eval(reader_result);
        auto expr_result = get_op(0);
        result.set(expr_result);
        pop_op(); // expression result
        pop_op(); // reader result

        if (expr_result->type_ == Value::Type::error) {
            push_op(expr_result);
            on_error(*expr_result);
            pop_op();
            break;
        }
    }

    --bound_context->interp_entry_count_;

    return result;
}


void format_impl(Value* value, Printer& p, int depth)
{
    bool prefix_quote = false;

    switch ((lisp::Value::Type)value->type_) {
    case lisp::Value::Type::heap_node:
        // We should never reach here.
        while (true)
            ;
        break;

    case lisp::Value::Type::nil:
        if (depth == 0) {
            p.put_str("'()");
        } else {
            p.put_str("()");
        }

        break;

    case lisp::Value::Type::__reserved:
        break;

    case lisp::Value::Type::character:
        // TODO...
        break;

    case lisp::Value::Type::string:
        p.put_str("\"");
        p.put_str(value->string_.value());
        p.put_str("\"");
        break;

    case lisp::Value::Type::symbol:
        p.put_str(value->symbol_.name_);
        break;

    case lisp::Value::Type::integer: {
        p.put_str(to_string<32>(value->integer_.value_).c_str());
        break;
    }

    case lisp::Value::Type::cons:
        if (depth == 0 and not prefix_quote) {
            p.put_str("'");
            prefix_quote = true;
        }
        p.put_str("(");
        format_impl(value->cons_.car(), p, depth + 1);
        if (value->cons_.cdr()->type_ == Value::Type::nil) {
            // ...
        } else if (value->cons_.cdr()->type_ not_eq Value::Type::cons) {
            p.put_str(" . ");
            format_impl(value->cons_.cdr(), p, depth + 1);
        } else {
            auto current = value;
            while (true) {
                if (current->cons_.cdr()->type_ == Value::Type::cons) {
                    p.put_str(" ");
                    format_impl(
                        current->cons_.cdr()->cons_.car(), p, depth + 1);
                    current = current->cons_.cdr();
                } else if (current->cons_.cdr() not_eq get_nil()) {
                    p.put_str(" ");
                    format_impl(current->cons_.cdr(), p, depth + 1);
                    break;
                } else {
                    break;
                }
            }
        }
        p.put_str(")");
        break;

    case lisp::Value::Type::function:
        p.put_str("<lambda>");
        break;

    case lisp::Value::Type::user_data:
        p.put_str("<ud>");
        break;

    case lisp::Value::Type::error:
        p.put_str("[ERR: ");
        p.put_str(lisp::Error::get_string(value->error_.code_));
        p.put_str(" : ");
        format_impl(dcompr(value->error_.context_), p, 0);
        p.put_str("]");
        break;

    case lisp::Value::Type::data_buffer:
        p.put_str("<sbr>");
        break;

    case lisp::Value::Type::count:
        break;
    }
}


const char* String::value()
{
    return dcompr(data_buffer_)->data_buffer_.value()->data_ + offset_;
}


void format(Value* value, Printer& p)
{
    format_impl(value, p, 0);
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
    if (value->mark_bit_) {
        return;
    }

    switch (value->type_) {
    case Value::Type::function:
        if (value->mode_bits_ == Function::ModeBits::lisp_function) {
            gc_mark_value((dcompr(value->function_.lisp_impl_.code_)));
            gc_mark_value((dcompr(value->function_.lisp_impl_.docstring_)));
        } else if (value->mode_bits_ ==
                   Function::ModeBits::lisp_bytecode_function) {
            gc_mark_value(
                (dcompr(value->function_.bytecode_impl_.data_buffer_)));
        }
        break;

    case Value::Type::string:
        gc_mark_value(dcompr(value->string_.data_buffer_));
        break;

    case Value::Type::error:
        gc_mark_value(dcompr(value->error_.context_));
        break;

    case Value::Type::cons:
        if (value->cons_.cdr()->type_ == Value::Type::cons) {
            auto current = value;

            while (current->cons_.cdr()->type_ == Value::Type::cons) {
                gc_mark_value(current->cons_.car());
                current = current->cons_.cdr();
                current->mark_bit_ = true;
            }

            gc_mark_value(current->cons_.car());
            gc_mark_value(current->cons_.cdr());

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


static Protected* __protected_values = nullptr;


Protected::Protected(Value* val) : val_(val), next_(nullptr)
{
    auto plist = __protected_values;
    if (plist) {
        plist->next_ = this;
        prev_ = plist;
    } else {
        prev_ = nullptr;
    }
    plist = this;
}

Protected::~Protected()
{
    if (next_) {
        next_->prev_ = prev_;
    }
    if (prev_) {
        prev_->next_ = next_;
    }
}


static void gc_mark()
{
    gc_mark_value(bound_context->nil_);

    if (bound_context->oom_) {
        gc_mark_value(bound_context->oom_);
    }

    auto& ctx = bound_context;

    for (auto elem : *ctx->operand_stack_) {
        gc_mark_value(dcompr(elem));
    }

    globals_tree_traverse(ctx->globals_tree_, [](Value& car, Value& node) {
        node.mark_bit_ = true;
        node.cons_.cdr()->mark_bit_ = true;
        gc_mark_value(&car);
    });

    gc_mark_value(ctx->this_);

    auto p_list = __protected_values;
    while (p_list) {
        gc_mark_value(*p_list);
        p_list = p_list->next();
    }
}


using Finalizer = void (*)(Value*);

struct FinalizerTableEntry {
    FinalizerTableEntry(Finalizer fn) : fn_(fn)
    {
    }

    Finalizer fn_;
};


static void invoke_finalizer(Value* value)
{
    // NOTE: This ordering should match the Value::Type enum.
    static const std::array<FinalizerTableEntry, Value::Type::count> table = {
        HeapNode::finalizer,
        Nil::finalizer,
        Integer::finalizer,
        Cons::finalizer,
        Function::finalizer,
        Error::finalizer,
        Symbol::finalizer,
        UserData::finalizer,
        DataBuffer::finalizer,
        String::finalizer,
        Character::finalizer,
        __Reserved::finalizer,
    };

    table[value->type_].fn_(value);
}


void DataBuffer::finalizer(Value* buffer)
{
    reinterpret_cast<ScratchBufferPtr*>(buffer->data_buffer_.sbr_mem_)
        ->~ScratchBufferPtr();
}


static int gc_sweep()
{
    if (not bound_context->string_buffer_->mark_bit_) {
        bound_context->string_buffer_ = L_NIL;
    }

    int collect_count = 0;

    for (int i = 0; i < VALUE_POOL_SIZE; ++i) {

        Value* val = &value_pool_data[i];

        if (val->alive_) {
            if (val->mark_bit_) {
                val->mark_bit_ = false;
            } else {
                invoke_finalizer(val);
                value_pool_free(val);
                ++collect_count;
            }
        }
    }

    return collect_count;
}


void live_values(::Function<24, void(Value&)> callback)
{
    for (int i = 0; i < VALUE_POOL_SIZE; ++i) {

        Value* val = &value_pool_data[i];

        if (val->alive_) {
            callback(*val);
        }
    }
}


static int run_gc()
{
    return gc_mark(), gc_sweep();
}


using EvalBuffer = StringBuffer<900>;


namespace {
class EvalPrinter : public Printer {
public:
    EvalPrinter(EvalBuffer& buffer) : buffer_(buffer)
    {
    }

    void put_str(const char* str) override
    {
        buffer_ += str;
    }

private:
    EvalBuffer& buffer_;
};
} // namespace


template <typename F> void foreach_string_intern(F&& fn)
{
    char* const interns = *bound_context->interns_;
    char* str = interns;

    while (static_cast<u32>(str - interns) < string_intern_table_size and
           static_cast<s32>(str - interns) <
               bound_context->string_intern_pos_ and
           *str not_eq '\0') {

        fn(str);

        str += str_len(str) + 1;
    }
}


static u32 read_list(const char* code)
{
    int i = 0;

    auto result = get_nil();
    push_op(get_nil());

    bool dotted_pair = false;

    while (true) {
        switch (code[i]) {
        case '\r':
        case '\n':
        case '\t':
        case ' ':
            ++i;
            break;

        case '.':
            i += 1;
            if (dotted_pair or result == get_nil()) {
                push_op(lisp::make_error(Error::Code::mismatched_parentheses,
                                         L_NIL));
                return i;
            } else {
                dotted_pair = true;
                i += read(code + i);
                result->cons_.set_cdr(get_op(0));
                pop_op();
            }
            break;

        case ';':
            while (true) {
                if (code[i] == '\0' or code[i] == '\r' or code[i] == '\n') {
                    break;
                } else {
                    ++i;
                }
            }
            break;

        case ')':
            ++i;
            return i;

        case '\0':
            pop_op();
            push_op(
                lisp::make_error(Error::Code::mismatched_parentheses, L_NIL));
            return i;
            break;

        default:
            if (dotted_pair) {
                push_op(lisp::make_error(Error::Code::mismatched_parentheses,
                                         L_NIL));
                return i;
            }
            i += read(code + i);

            if (result == get_nil()) {
                result = make_cons(get_op(0), get_nil());
                pop_op(); // the result from read()
                pop_op(); // nil
                push_op(result);
            } else {
                auto next = make_cons(get_op(0), get_nil());
                pop_op();
                result->cons_.set_cdr(next);
                result = next;
            }
            break;
        }
    }
}


static u32 read_string(const char* code)
{
    auto temp = bound_context->pfrm_.make_scratch_buffer();
    auto write = temp->data_;

    int i = 0;
    while (*code not_eq '"') {
        if (*code == '\0' or i == SCRATCH_BUFFER_SIZE - 1) {
            // FIXME: correct error code.
            push_op(
                lisp::make_error(Error::Code::mismatched_parentheses, L_NIL));
        }
        *(write++) = *(code++);
        i++;
    }

    if (*code == '"') {
        ++i;
        ++code;
    }

    push_op(make_string(bound_context->pfrm_, temp->data_));

    return i;
}


static u32 read_symbol(const char* code)
{
    int i = 0;

    StringBuffer<64> symbol;

    if (code[0] == '\'') {
        push_op(make_symbol("'", Symbol::ModeBits::stable_pointer));
        return 1;
    }

    while (true) {
        switch (code[i]) {
        case '(':
        case ')':
        case ' ':
        case '\r':
        case '\n':
        case '\t':
        case '\0':
        case ';':
            goto FINAL;

        default:
            symbol.push_back(code[i++]);
            break;
        }
    }

FINAL:

    if (symbol == "nil") {
        push_op(get_nil());
    } else {
        push_op(make_symbol(symbol.c_str()));
    }

    return i;
}


static u32 read_number(const char* code)
{
    int i = 0;

    StringBuffer<64> num_str;

    while (true) {
        switch (code[i]) {
        case 'x':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            num_str.push_back(code[i++]);
            break;

        default:
            goto FINAL;
        }
    }

FINAL:

    if (num_str.length() > 1 and num_str[1] == 'x') {
        lisp::push_op(
            lisp::make_integer(hexdec((const u8*)num_str.begin() + 2)));
    } else {
        s32 result = 0;
        for (u32 i = 0; i < num_str.length(); ++i) {
            result = result * 10 + (num_str[i] - '0');
        }

        lisp::push_op(lisp::make_integer(result));
    }

    return i;
}


u32 read(const char* code)
{
    int i = 0;

    push_op(get_nil());

    while (true) {
        switch (code[i]) {
        case '\0':
            return i;

        case '(':
            ++i;
            pop_op(); // nil
            i += read_list(code + i);
            // list now at stack top.
            return i;

        case ';':
            while (true) {
                if (code[i] == '\0' or code[i] == '\r' or code[i] == '\n') {
                    break;
                } else {
                    ++i;
                }
            }
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            pop_op(); // nil
            i += read_number(code + i);
            // number now at stack top.
            return i;

        case '\n':
        case '\r':
        case '\t':
        case ' ':
            ++i;
            break;

        case '"':
            pop_op(); // nil
            i += read_string(code + i + 1);
            return i + 1;

        default:
            pop_op(); // nil
            i += read_symbol(code + i);
            // symbol now at stack top.

            // Ok, so for quoted expressions, we're going to put the value into
            // a cons, where the car holds the quote symbol, and the cdr holds
            // the value. Not sure how else to support top-level quoted
            // values outside of s-expressions.
            if (get_op(0)->type_ == Value::Type::symbol and
                str_cmp(get_op(0)->symbol_.name_, "'") == 0) {

                auto pair = make_cons(get_op(0), get_nil());
                push_op(pair);
                i += read(code + i);
                pair->cons_.set_cdr(get_op(0));
                pop_op(); // result of read()
                pop_op(); // pair
                pop_op(); // symbol
                push_op(pair);
            }
            return i;
        }
    }
}


static void eval_let(Value* code)
{
    // Overview:
    // Push the previous values of all of the let binding vars onto the stack.
    // Overwrite the current contents of the global vars. Pop the previous
    // contents off of the operand stack, and re-assign the var to the stashed
    // value.

    if (code->type_ not_eq Value::Type::cons) {
        push_op(lisp::make_error(Error::Code::mismatched_parentheses, L_NIL));
        return;
    }

    Value* bindings = code->cons_.car();

    Protected result(get_nil());

    int stashed_vars = 0;

    foreach (bindings, [&](Value* val) {
        if (result not_eq get_nil()) {
            return;
        }
        if (val->type_ == Value::Type::cons) {
            auto sym = val->cons_.car();
            auto bind = val->cons_.cdr();
            if (sym->type_ == Value::Type::symbol and
                bind->type_ == Value::Type::cons) {
                auto prev = get_var(sym);

                push_op(prev);
                ++stashed_vars;

                eval(bind->cons_.car());

                set_var(sym, get_op(0));
                pop_op();

            } else {
                result = lisp::make_error(Error::Code::mismatched_parentheses,
                                          L_NIL);
            }
        } else {
            result =
                lisp::make_error(Error::Code::mismatched_parentheses, L_NIL);
        }
    })
        ;

    auto clear_stash = [&] {
        for (int i = 0; i < stashed_vars; ++i) {
            pop_op();
        }
    };

    if (result not_eq get_nil()) {
        clear_stash();
        push_op(result);
        return;
    }

    foreach (code->cons_.cdr(), [&](Value* val) {
        eval(val);
        result.set(get_op(0));
        pop_op();
    })
        ;

    int i = 0;
    foreach (bindings, [&i, stashed_vars](Value* val) {
        auto value = get_op((stashed_vars - 1) - i);

        auto sym = val->cons_.car();

        if (value->type_ not_eq Value::Type::error) {
            set_var(sym, value);
        } else {
            globals_tree_erase(sym);
        }
        ++i;
    })
        ;

    clear_stash();
    push_op(result);
}


static void eval_if(Value* code)
{
    if (code->type_ not_eq Value::Type::cons) {
        push_op(lisp::make_error(Error::Code::mismatched_parentheses, L_NIL));
        return;
    }

    auto cond = code->cons_.car();

    auto true_branch = get_nil();
    auto false_branch = get_nil();

    if (code->cons_.cdr()->type_ == Value::Type::cons) {
        true_branch = code->cons_.cdr()->cons_.car();

        if (code->cons_.cdr()->cons_.cdr()->type_ == Value::Type::cons) {
            false_branch = code->cons_.cdr()->cons_.cdr()->cons_.car();
        }
    }

    eval(cond);
    if (is_boolean_true(get_op(0))) {
        eval(true_branch);
    } else {
        eval(false_branch);
    }

    auto result = get_op(0);
    pop_op(); // result
    pop_op(); // cond
    push_op(result);
}


static void eval_lambda(Value* code)
{
    // todo: argument list...

    push_op(make_lisp_function(code));
}


void eval(Value* code)
{
    ++bound_context->interp_entry_count_;

    // NOTE: just to protect this from the GC, in case the user didn't bother to
    // do so.
    push_op(code);

    if (code->type_ == Value::Type::symbol) {
        pop_op();
        push_op(get_var(code));
    } else if (code->type_ == Value::Type::cons) {
        auto form = code->cons_.car();
        if (form->type_ == Value::Type::symbol) {
            if (str_cmp(form->symbol_.name_, "if") == 0) {
                eval_if(code->cons_.cdr());
                auto result = get_op(0);
                pop_op(); // result
                pop_op(); // code
                push_op(result);
                --bound_context->interp_entry_count_;
                return;
            } else if (str_cmp(form->symbol_.name_, "lambda") == 0) {
                eval_lambda(code->cons_.cdr());
                auto result = get_op(0);
                pop_op(); // result
                pop_op(); // code
                push_op(result);
                --bound_context->interp_entry_count_;
                return;
            } else if (str_cmp(form->symbol_.name_, "'") == 0) {
                pop_op(); // code
                push_op(code->cons_.cdr());
                --bound_context->interp_entry_count_;
                return;
            } else if (str_cmp(form->symbol_.name_, "let") == 0) {
                eval_let(code->cons_.cdr());
                auto result = get_op(0);
                pop_op();
                pop_op();
                push_op(result);
                --bound_context->interp_entry_count_;
                return;
            }
        }

        eval(code->cons_.car());
        auto function = get_op(0);
        pop_op();

        int argc = 0;

        auto clear_args = [&] {
            while (argc) {
                pop_op();
                --argc;
            }
        };

        auto arg_list = code->cons_.cdr();
        while (true) {
            if (arg_list == get_nil()) {
                break;
            }
            if (arg_list->type_ not_eq Value::Type::cons) {
                clear_args();
                pop_op();
                push_op(make_error(Error::Code::value_not_callable, arg_list));
                --bound_context->interp_entry_count_;
                return;
            }

            eval(arg_list->cons_.car());
            ++argc;

            arg_list = arg_list->cons_.cdr();
        }

        funcall(function, argc);
        auto result = get_op(0);
        if (result->type_ == Value::Type::error and
            dcompr(result->error_.context_) == L_NIL) {
            result->error_.context_ = compr(code);
        }
        pop_op(); // result
        pop_op(); // protected expr (see top)
        push_op(result);
        --bound_context->interp_entry_count_;
        return;
    }
}


Platform* interp_get_pfrm()
{
    return &bound_context->pfrm_;
}


void init(Platform& pfrm)
{
    if (bound_context) {
        return;
    }

    bound_context.emplace(pfrm);

    value_pool_init();
    bound_context->nil_ = alloc_value();
    bound_context->nil_->type_ = Value::Type::nil;
    bound_context->globals_tree_ = bound_context->nil_;
    bound_context->this_ = bound_context->nil_;

    bound_context->oom_ = alloc_value();
    bound_context->oom_->type_ = Value::Type::error;
    bound_context->oom_->error_.code_ = Error::Code::out_of_memory;
    bound_context->oom_->error_.context_ = compr(bound_context->nil_);

    bound_context->string_buffer_ = bound_context->nil_;

    if (dcompr(compr(get_nil())) not_eq get_nil()) {
        error(pfrm, "pointer compression test failed");
        while (true)
            ;
    }

    lisp::set_var("*pfrm*", lisp::make_userdata(&pfrm));

    intern("'");

    set_var("set", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(1, symbol);

                lisp::set_var(get_op(1), get_op(0));

                return L_NIL;
            }));

    set_var("cons", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                auto car = get_op(1);
                auto cdr = get_op(0);

                if (car->type_ == lisp::Value::Type::error) {
                    return car;
                }

                if (cdr->type_ == lisp::Value::Type::error) {
                    return cdr;
                }

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
                    auto val = get_op((argc - 1) - i);
                    if (val->type_ == Value::Type::error) {
                        return val;
                    }
                    set_list(lat, i, val);
                }
                return lat;
            }));

    set_var("arg", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, integer);
                return get_arg(get_op(0)->integer_.value_);
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

    set_var("any-true", make_function([](int argc) {
                for (int i = 0; i < argc; ++i) {
                    if (is_boolean_true(get_op(i))) {
                        return get_op(i);
                    }
                }
                return L_NIL;
            }));

    set_var("all-true", make_function([](int argc) {
                for (int i = 0; i < argc; ++i) {
                    if (not is_boolean_true(get_op(i))) {
                        return L_NIL;
                    }
                }
                return make_integer(1);
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

                case Value::Type::string:
                    return str_cmp(get_op(0)->string_.value(),
                                   get_op(1)->string_.value()) == 0;
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
                        return make_error(Error::Code::invalid_argument_type,
                                          lat);
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

    set_var("gen", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(1, integer);

                auto result = make_list(get_op(1)->integer_.value_);
                auto fn = get_op(0);
                const int count = get_op(1)->integer_.value_;
                push_op(result);
                for (int i = 0; i < count; ++i) {
                    push_op(make_integer(i));
                    funcall(fn, 1);
                    set_list(result, i, get_op(0));
                    pop_op(); // result from funcall
                }
                pop_op(); // result
                return result;
            }));

    set_var("length", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);

                if (get_op(0)->type_ == Value::Type::nil) {
                    return make_integer(0);
                }

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
                auto& ctx = bound_context;

                int values_remaining = 0;
                Value* current = value_pool;
                while (current) {
                    ++values_remaining;
                    current = current->heap_node_.next_;
                }

                ListBuilder lat;

                auto make_stat = [&](const char* name, int value) {
                    auto c = make_cons(get_nil(), get_nil());
                    if (c == bound_context->oom_) {
                        return c;
                    }
                    push_op(c); // gc protect

                    c->cons_.set_car(
                        make_symbol(name, Symbol::ModeBits::stable_pointer));
                    c->cons_.set_cdr(make_integer(value));

                    pop_op(); // gc unprotect
                    return c;
                };

                lat.push_front(make_stat("vars", [&] {
                    int symb_tab_used = 0;
                    globals_tree_traverse(
                        ctx->globals_tree_,
                        [&symb_tab_used](Value&, Value&) { ++symb_tab_used; });
                    return symb_tab_used;
                }()));

                lat.push_front(make_stat("stk", ctx->operand_stack_->size()));
                lat.push_front(make_stat("internb", ctx->string_intern_pos_));
                lat.push_front(make_stat("free", values_remaining));

                int databuffers = 0;

                for (int i = 0; i < VALUE_POOL_SIZE; ++i) {
                    Value* val = &value_pool_data[i];
                    if (val->alive_ and
                        val->type_ == Value::Type::data_buffer) {
                        ++databuffers;
                    }
                }

                lat.push_front(make_stat("sbr", databuffers));

                return lat.result();
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
                    return lisp::make_error(lisp::Error::Code::invalid_argc,
                                            L_NIL);
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

                globals_tree_erase(get_op(0));

                return get_nil();
            }));


    set_var("symbol", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, string);

                return make_symbol(get_op(0)->string_.value());
            }));


    set_var("string", make_function([](int argc) {
                EvalBuffer b;
                EvalPrinter p(b);

                for (int i = argc - 1; i > -1; --i) {
                    auto val = get_op(i);
                    if (val->type_ == Value::Type::string) {
                        p.put_str(val->string_.value());
                    } else {
                        format_impl(val, p, 0);
                    }
                }

                if (auto pfrm = interp_get_pfrm()) {
                    return make_string(*pfrm, b.c_str());
                }
                return L_NIL;
            }));

    set_var("bound", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, symbol);

                auto found = globals_tree_find(get_op(0));
                return make_integer(found not_eq get_nil() and
                                    found->type_ not_eq
                                        lisp::Value::Type::error);
            }));


    set_var("filter", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(0, cons);
                L_EXPECT_OP(1, function);

                auto fn = get_op(1);
                Value* result = make_cons(L_NIL, L_NIL);
                auto prev = result;
                auto current = result;

                foreach (get_op(0), [&](Value* val) {
                    push_op(result); // gc protect

                    push_op(val);
                    funcall(fn, 1);
                    auto funcall_result = get_op(0);

                    if (is_boolean_true(funcall_result)) {
                        current->cons_.set_car(val);
                        auto next = make_cons(L_NIL, L_NIL);
                        if (next == bound_context->oom_) {
                            current = result;
                            return;
                        }
                        current->cons_.set_cdr(next);
                        prev = current;
                        current = next;
                    }
                    pop_op(); // funcall result

                    pop_op(); // gc unprotect
                })
                    ;

                if (current == result) {
                    return L_NIL;
                }

                prev->cons_.set_cdr(L_NIL);

                return result;
            }));


    set_var(
        "map", make_function([](int argc) {
            if (argc < 2) {
                return get_nil();
            }
            if (lisp::get_op(argc - 1)->type_ not_eq Value::Type::function and
                lisp::get_op(argc - 1)->type_ not_eq Value::Type::cons) {
                return lisp::make_error(
                    lisp::Error::Code::invalid_argument_type, L_NIL);
            }

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

    set_var("reverse", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, cons);

                Value* result = get_nil();
                foreach (get_op(0), [&](Value* car) {
                    push_op(result);
                    result = make_cons(car, result);
                    pop_op();
                })
                    ;

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

    set_var("gc",
            make_function([](int argc) { return make_integer(run_gc()); }));

    set_var("get", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(1, cons);
                L_EXPECT_OP(0, integer);

                return get_list(get_op(1), get_op(0)->integer_.value_);
            }));

#ifdef __GBA__
    set_var("write-u8", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(0, integer);
                L_EXPECT_OP(1, integer);
                const u8 val = get_op(1)->integer_.value_;
                *((u8*)(intptr_t)get_op(0)->integer_.value_) = val;
                return get_nil();
            }));

    set_var("write-u16", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(0, integer);
                L_EXPECT_OP(1, integer);
                const u16 val = get_op(1)->integer_.value_;
                *((u16*)(intptr_t)get_op(0)->integer_.value_) = val;
                return get_nil();
            }));

    set_var("write-u32", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 2);
                L_EXPECT_OP(0, integer);
                L_EXPECT_OP(1, integer);
                const u32 val = get_op(1)->integer_.value_;
                *((u32*)(intptr_t)get_op(0)->integer_.value_) = val;
                return get_nil();
            }));

    set_var("read-u8", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, integer);
                return lisp::make_integer(
                    *((u8*)(intptr_t)get_op(0)->integer_.value_));
            }));

    set_var("read-u16", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, integer);
                return lisp::make_integer(
                    *((u16*)(intptr_t)get_op(0)->integer_.value_));
            }));

    set_var("read-u32", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, integer);
                return lisp::make_integer(
                    *((u32*)(intptr_t)get_op(0)->integer_.value_));
            }));
#endif // __GBA__


    set_var("read", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, string);
                read(get_op(0)->string_.value());
                auto result = get_op(0);
                pop_op();
                return result;
            }));


    set_var("eval", make_function([](int argc) {
                if (argc < 1) {
                    return lisp::make_error(lisp::Error::Code::invalid_argc,
                                            L_NIL);
                }

                eval(get_op(0));
                auto result = get_op(0);
                pop_op(); // result

                return result;
            }));

    set_var("globals", make_function([](int argc) {
                return bound_context->globals_tree_;
            }));

    set_var("this",
            make_function([](int argc) { return bound_context->this_; }));

    set_var("argc", make_function([](int argc) {
                // NOTE: This works because native functions do not assign
                // current_fn_argc_.
                return make_integer(bound_context->current_fn_argc_);
            }));

    set_var("env", make_function([](int argc) {
                auto pfrm = interp_get_pfrm();

                Value* result = make_cons(get_nil(), get_nil());
                push_op(result); // protect from the gc

                Value* current = result;

                get_env([&current, pfrm](const char* str) {
                    current->cons_.set_car(intern_to_symbol(str));
                    auto next = make_cons(get_nil(), get_nil());
                    if (next not_eq bound_context->oom_) {
                        current->cons_.set_cdr(next);
                        current = next;
                    }
                });

                pop_op(); // result

                return result;
            }));

    set_var(
        "compile", make_function([](int argc) {
            auto pfrm = interp_get_pfrm();

            L_EXPECT_ARGC(argc, 1);
            L_EXPECT_OP(0, function);

            if (get_op(0)->mode_bits_ == Function::ModeBits::lisp_function) {
                compile(*pfrm, dcompr(get_op(0)->function_.lisp_impl_.code_));
                auto ret = get_op(0);
                pop_op();
                return ret;
            } else {
                return get_op(0);
            }
        }));

    set_var(
        "disassemble", make_function([](int argc) {
            L_EXPECT_ARGC(argc, 1);
            L_EXPECT_OP(0, function);

            if (get_op(0)->mode_bits_ ==
                Function::ModeBits::lisp_bytecode_function) {

                Platform::RemoteConsole::Line out;

                u8 depth = 0;

                auto buffer =
                    dcompr(get_op(0)->function_.bytecode_impl_.data_buffer_);

                auto data = buffer->data_buffer_.value();

                const auto start_offset =
                    get_op(0)->function_.bytecode_impl_.bc_offset_;

                for (int i = start_offset; i < SCRATCH_BUFFER_SIZE;) {

                    const auto offset = to_string<10>(i - start_offset);
                    if (offset.length() < 4) {
                        for (u32 i = 0; i < 4 - offset.length(); ++i) {
                            out.push_back('0');
                        }
                    }

                    out += offset;
                    out += ": ";

                    using namespace instruction;

                    switch ((Opcode)(*data).data_[i]) {
                    case Fatal::op():
                        return get_nil();

                    case LoadVar::op():
                        i += 1;
                        out += "LOAD_VAR(";
                        out += symbol_from_offset(
                            ((HostInteger<s16>*)(data->data_ + i))->get());
                        out += ")";
                        i += 2;
                        break;

                    case PushSymbol::op():
                        i += 1;
                        out += "PUSH_SYMBOL(";
                        out += to_string<10>(
                            ((HostInteger<s16>*)(data->data_ + i))->get());
                        out += ")";
                        i += 2;
                        break;

                    case PushNil::op():
                        out += "PUSH_NIL";
                        i += 1;
                        break;

                    case Push0::op():
                        i += 1;
                        out += "PUSH_0";
                        break;

                    case Push1::op():
                        i += 1;
                        out += "PUSH_1";
                        break;

                    case Push2::op():
                        i += 1;
                        out += "PUSH_2";
                        break;

                    case PushInteger::op():
                        i += 1;
                        out += "PUSH_INTEGER(";
                        out += to_string<10>(
                            ((HostInteger<s32>*)(data->data_ + i))->get());
                        out += ")";
                        i += 4;
                        break;

                    case PushSmallInteger::op():
                        out += "PUSH_SMALL_INTEGER(";
                        out += to_string<10>(*(data->data_ + i + 1));
                        out += ")";
                        i += 2;
                        break;

                    case JumpIfFalse::op():
                        out += "JUMP_IF_FALSE(";
                        out += to_string<10>(
                            ((HostInteger<u16>*)(data->data_ + i + 1))->get());
                        out += ")";
                        i += 3;
                        break;

                    case Jump::op():
                        out += "JUMP(";
                        out += to_string<10>(
                            ((HostInteger<u16>*)(data->data_ + i + 1))->get());
                        out += ")";
                        i += 3;
                        break;

                    case SmallJumpIfFalse::op():
                        out += "SMALL_JUMP_IF_FALSE(";
                        out += to_string<10>(*(data->data_ + i + 1));
                        out += ")";
                        i += 2;
                        break;

                    case SmallJump::op():
                        out += "SMALL_JUMP(";
                        out += to_string<10>(*(data->data_ + i + 1));
                        out += ")";
                        i += 2;
                        break;

                    case PushLambda::op():
                        out += "PUSH_LAMBDA(";
                        out += to_string<10>(
                            ((HostInteger<u16>*)(data->data_ + i + 1))->get());
                        out += ")";
                        i += 3;
                        ++depth;
                        break;

                    case PushThis::op():
                        out += PushThis::name();
                        i += sizeof(PushThis);
                        break;

                    case Arg::op():
                        out += Arg::name();
                        i += sizeof(Arg);
                        break;

                    case Arg0::op():
                        out += Arg0::name();
                        i += sizeof(Arg0);
                        break;

                    case Arg1::op():
                        out += Arg1::name();
                        i += sizeof(Arg1);
                        break;

                    case Arg2::op():
                        out += Arg2::name();
                        i += sizeof(Arg2);
                        break;

                    case TailCall::op():
                        out += TailCall::name();
                        out += "(";
                        out += to_string<10>(*(data->data_ + i + 1));
                        out += ")";
                        i += 2;
                        break;

                    case TailCall1::op():
                        out += TailCall1::name();
                        ++i;
                        break;

                    case TailCall2::op():
                        out += TailCall2::name();
                        ++i;
                        break;

                    case TailCall3::op():
                        out += TailCall3::name();
                        ++i;
                        break;

                    case Funcall::op():
                        out += "FUNCALL(";
                        out += to_string<10>(*(data->data_ + i + 1));
                        out += ")";
                        i += 2;
                        break;

                    case PushList::op():
                        out += "PUSH_LIST(";
                        out += to_string<10>(*(data->data_ + i + 1));
                        out += ")";
                        i += 2;
                        break;

                    case Funcall1::op():
                        out += "FUNCALL_1";
                        i += 1;
                        break;

                    case Funcall2::op():
                        out += "FUNCALL_2";
                        i += 1;
                        break;

                    case Funcall3::op():
                        out += "FUNCALL_3";
                        i += 1;
                        break;

                    case Pop::op():
                        out += "POP";
                        i += 1;
                        break;

                    case MakePair::op():
                        out += "MAKE_PAIR";
                        i += 1;
                        break;

                    case Not::op():
                        out += Not::name();
                        i += sizeof(Not);
                        break;

                    case First::op():
                        out += First::name();
                        i += sizeof(First);
                        break;

                    case Rest::op():
                        out += Rest::name();
                        i += sizeof(Rest);
                        break;

                    case Dup::op():
                        out += Dup::name();
                        i += 1;
                        break;

                    case EarlyRet::op():
                        out += EarlyRet::name();
                        i += sizeof(EarlyRet);
                        break;

                    case Ret::op(): {
                        if (depth == 0) {
                            out += "RET\r\n";
                            auto pfrm = interp_get_pfrm();
                            pfrm->remote_console().printline(out.c_str(),
                                                             false);
                            ((Platform*)pfrm)->sleep(80);
                            return get_nil();
                        } else {
                            --depth;
                            out += "RET";
                            i += 1;
                        }
                        break;
                    }

                    default:
                        interp_get_pfrm()->remote_console().printline(
                            out.c_str(), false);
                        interp_get_pfrm()->sleep(80);
                        return get_nil();
                    }
                    out += "\r\n";
                }
                return get_nil();
            } else if (get_op(0)->mode_bits_ ==
                       Function::ModeBits::lisp_function) {
                auto expression_list =
                    dcompr(get_op(0)->function_.lisp_impl_.code_);

                DefaultPrinter p;
                format(expression_list, p);

                interp_get_pfrm()->remote_console().printline(p.fmt_.c_str(),
                                                              false);
                interp_get_pfrm()->sleep(80);
                return get_nil();

            } else {
                return get_nil();
            }
        }));
}


int paren_balance(const char* ptr)
{
    int balance = 0;
    while (*ptr not_eq '\0') {
        if (*ptr == '(') {
            ++balance;
        } else if (*ptr == ')') {
            --balance;
        }
        ++ptr;
    }
    return balance;
}


} // namespace lisp
