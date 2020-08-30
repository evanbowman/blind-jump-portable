#include "lisp.hpp"
#include "memory/pool.hpp"
#include "memory/buffer.hpp"
#include <iostream>


inline u32 str_len(const char* str)
{
    const char* s;

    for (s = str; *s; ++s)
        ;
    return (s - str);
}


inline int strcmp(const char* p1, const char* p2)
{
    const unsigned char* s1 = (const unsigned char*)p1;
    const unsigned char* s2 = (const unsigned char*)p2;

    unsigned char c1, c2;

    do {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;

        if (c1 == '\0') {
            return c1 - c2;
        }

    } while (c1 == c2);

    return c1 - c2;
}



namespace lisp {


#define NIL L_NIL


Value nil { lisp::Value::Type::nil, true };


struct Variable {
    const char* name_ = "";
    Value* value_ = NIL;
};


struct Context {
    ObjectPool<Value, 128> memory_;
    Buffer<Value*, 64> operand_stack_;
    Variable globals_[64];
} __context;


Context* bound_context = &__context;


Value* make_function(u8 argc, Function::Impl impl)
{
    if (auto val = bound_context->memory_.get()) {
        val->type_ = Value::Type::function;
        val->mark_bit_ = false;
        val->function_.impl_ = impl;
        val->function_.argc_ = argc;
        return val;
    }
    return NIL;
}


Value* make_cons(Value* car, Value* cdr)
{
    if (auto val = bound_context->memory_.get()) {
        val->type_ = Value::Type::cons;
        val->mark_bit_ = false;
        val->cons_.car_ = car;
        val->cons_.cdr_ = cdr;
        return val;
    }
    return NIL;
}


Value* make_integer(s32 value)
{
    if (auto val = bound_context->memory_.get()) {
        val->type_ = Value::Type::integer;
        val->mark_bit_ = false;
        val->integer_.value_ = value;
        return val;
    }
    return NIL;
}


Value* make_list(u32 length)
{
    auto head = make_cons(NIL, NIL);
    while (length--) {
        auto cell = make_cons(NIL, head);
        head = cell;
    }
    return head;
}


Value* make_error(Error::Code error_code)
{
    if (auto val = bound_context->memory_.get()) {
        val->type_ = Value::Type::error;
        val->mark_bit_ = false;
        val->error_.code_ = error_code;
        return val;
    }
    return NIL;
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
    bound_context->operand_stack_.pop_back();
}


void push_op(Value* operand)
{
    if (not bound_context->operand_stack_.push_back(operand)) {
        while (true) ; // TODO: raise error
    }
}


Value* get_op(u32 offset)
{
    auto& stack = bound_context->operand_stack_;
    if (offset >= stack.size()) {
        return NIL; // TODO: raise error
    }

    return stack[(stack.size() - 1) - offset];
}


#define EXPECT_OP(OFFSET, TYPE)                                           \
    if (lisp::get_op((OFFSET))->type_ not_eq lisp::Value::Type::TYPE) { \
        if (lisp::get_op((OFFSET)) == L_NIL) { \
            return lisp::get_op((OFFSET)); \
        } else { \
            return lisp::make_error(lisp::Error::Code::invalid_argument_type); \
        } \
    }




// The function arguments should be sitting at the top of the operand stack
// prior to calling funcall. The arguments will be consumed, and replaced with
// the result of the function call.
void funcall(Value* obj)
{
    if (obj->type_ not_eq Value::Type::function) {
        push_op(make_error(Error::Code::value_not_callable));
        return;
    }

    if (bound_context->operand_stack_.size() < obj->function_.argc_) {
        push_op(make_error(Error::Code::too_few_arguments));
        return;
    }

    auto result = obj->function_.impl_();
    for (int i = 0; i < obj->function_.argc_; ++i) {
        bound_context->operand_stack_.pop_back();
    }

    push_op(result);
}


Value* set_var(const char* name, Value* value)
{
    for (auto& var : bound_context->globals_) {
        if (strcmp(name, var.name_) == 0) {
            var.name_ = name;
            var.value_ = value;
            return NIL;
        }
    }

    for (auto& var : bound_context->globals_) {
        if (strcmp(var.name_, "") == 0) {
            var.name_ = name;
            var.value_ = value;
            return NIL;
        }
    }

    return make_error(Error::Code::symbol_table_exhausted);
}


Value* get_var(const char* name)
{
    for (auto& var : bound_context->globals_) {
        if (strcmp(name, var.name_) == 0) {
            return var.value_;
        }
    }

    return make_error(Error::Code::undefined_variable_access);
}


void init()
{
    set_var("cons", make_function(2, [] {
        return make_cons(get_op(1), get_op(0));
    }));

    set_var("car", make_function(1, [] {
        EXPECT_OP(0, cons);
        return get_op(0)->cons_.car_;
    }));

    set_var("cdr", make_function(1, [] {
        EXPECT_OP(0, cons);
        return get_op(0)->cons_.cdr_;
    }));

    set_var("+", make_function(2, [] {
        EXPECT_OP(1, integer);
        EXPECT_OP(0, integer);
        return make_integer(get_op(1)->integer_.value_ +
                            get_op(0)->integer_.value_);
    }));

    set_var("-", make_function(2, [] {
        EXPECT_OP(1, integer);
        EXPECT_OP(0, integer);
        return make_integer(get_op(1)->integer_.value_ -
                            get_op(0)->integer_.value_);
    }));

    set_var("*", make_function(2, [] {
        EXPECT_OP(1, integer);
        EXPECT_OP(0, integer);
        return make_integer(get_op(1)->integer_.value_ *
                            get_op(0)->integer_.value_);
    }));

    set_var("/", make_function(2, [] {
        EXPECT_OP(1, integer);
        EXPECT_OP(0, integer);
        return make_integer(get_op(1)->integer_.value_ /
                            get_op(0)->integer_.value_);
    }));
}


static bool is_whitespace(char c)
{
    return c == ' ' or c == '\r' or c == '\n' or c == '\t';
}


// static u32 eval_set(const char* expr, u32 len)
// {
//     std::cout << "here" << std::endl;
//     // (set <NAME> <VALUE>)

//     static const u32 max_var_name = 31;
//     char variable_name[max_var_name];

//     int i = 0;

//     // parse name
//     while (is_whitespace(expr[i])) {
//         ++i;
//     }

//     int start = i;
//     while (i < len and i < max_var_name and not is_whitespace(expr[i])) {
//         variable_name[i - start] = expr[i];
//         ++i;
//     }
//     variable_name[i - start] = '\0';

//     while (is_whitespace(expr[i])) {
//         ++i;
//     }

//     // parse value
//     i += eval(expr + i);

//     set_var(variable_name, get_op(0));

//     return i;
// }


static u32 eval_expr(const char* expr, u32 len)
{
    int i = 0;

    static const int max_fn_name = 31;
    char fn_name[max_fn_name + 1];

    while (is_whitespace(expr[i])) {
        ++i;
    }

    for (; i < len; ++i) {
        int start = i;
        while (i < len and i < max_fn_name and not is_whitespace(expr[i])) {
            fn_name[i - start] = expr[i];
            ++i;
        }
        fn_name[i - start] = '\0';
        break;
    }

    // FIXME: This code doesn't work correctly yet...
    // if (strcmp("set", fn_name) == 0) {
    //     return eval_set(expr + i, len - i) + 2;
    // }

    while (expr[i] not_eq ')') {
        while (is_whitespace(expr[i])) {
            ++i;
        }

        i += eval(expr + i);

        while (is_whitespace(expr[i])) {
            ++i;
        }
    }
    i += 2;

    const auto fn = lisp::get_var(fn_name);
    if (fn == NIL) {
        std::cout << "function " << fn_name << " dne" << std::endl;
        while (true) ;
    }

    funcall(fn);

    return i;
}


static bool is_numeric(char c)
{
    return c == '0'
        or c == '1'
        or c == '2'
        or c == '3'
        or c == '4'
        or c == '5'
        or c == '6'
        or c == '7'
        or c == '8'
        or c == '9';
}


static u32 eval_number(const char* code, u32 len)
{
    u32 result = 0;

    int i = 0;
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

    for (int i = 0; i < len; ++i) {
        if (is_whitespace(code[i]) or code[i] == ')') {
            variable_name[i] = '\0';
            if (strcmp(variable_name, "nil") == 0) {
                push_op(NIL);
            } else {
                std::cout << variable_name << std::endl;
                auto var = lisp::get_var(variable_name);
                if (var == L_NIL) {
                    push_op(make_error(Error::Code::undefined_variable_access));
                } else {
                    push_op(var);
                }
            }
            return i;
        } else {
            variable_name[i] = code[i];
        }
    }

    return max_var_name;
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

    for (int i = 0; i < code_len; ++i) {
        if (is_whitespace(code[i])) {
            continue;
        }
        if (code[i] == '(') {
            return eval_expr(code + i + 1, code_len - (i + 1));
        } else {
            return eval_value(code + i, code_len - i);
        }
    }
    push_op(NIL);
    return code_len;
}


}


#ifndef __GBA__


void print_impl(lisp::Value* value)
{
    switch (value->type_) {
    case lisp::Value::Type::nil:
        std::cout << "nil";
        break;

    case lisp::Value::Type::integer:
        std::cout << value->integer_.value_;
        break;

    case lisp::Value::Type::cons:
        std::cout << '(';
        print_impl(value->cons_.car_);
        std::cout << " . ";
        print_impl(value->cons_.cdr_);
        std::cout << ')';
        break;

    case lisp::Value::Type::function:
        std::cout << "fn<" << (int)value->function_.argc_ << '>';
        break;

    case lisp::Value::Type::error:
        std::cout << "ERROR: "
                  << lisp::Error::get_string(value->error_.code_)
                  << std::endl;
        break;
    }
}


void print(lisp::Value* value)
{
    print_impl(value);
    std::cout << std::endl;
}


static lisp::Value* function_test()
{
    using namespace lisp;

    set_var("double", make_function(1, []() {
        EXPECT_OP(0, integer);

        return make_integer(get_op(0)->integer_.value_ * 2);
    }));

    push_op(make_integer(48));
    funcall(get_var("double"));

    EXPECT_OP(0, integer);

    if (get_op(0)->integer_.value_ not_eq 48 * 2) {
        std::cout << "funcall test result check failed!" << std::endl;
        return L_NIL;
    }

    if (bound_context->operand_stack_.size() not_eq 1) {
        std::cout << "operand stack size check failed!" << std::endl;
        return L_NIL;
    }

    bound_context->operand_stack_.pop_back();

    std::cout << "funcall test passed!" << std::endl;

    return L_NIL;
}


static lisp::Value* arithmetic_test()
{
    using namespace lisp;

    push_op(make_integer(48));
    push_op(make_integer(96));
    funcall(get_var("-"));

    EXPECT_OP(0, integer);

    if (get_op(0)->integer_.value_ not_eq 48 - 96) {
        std::cout << "bad arithmetic!" << std::endl;
        return L_NIL;;
    }

    std::cout << "arithmetic test passed!" << std::endl;

    return L_NIL;
}


void do_tests()
{
    auto lat = lisp::make_list(9);

    lisp::set_var("L", lat);
    lisp::set_list(lat, 4, lisp::make_integer(12));

    print(lisp::get_list(lisp::get_var("L"), 4));

    function_test();
    arithmetic_test();
}


int main(int argc, char** argv)
{
    lisp::init();

    if (argc == 1) {
        auto lat = lisp::make_list(9);

        lisp::set_var("L", lat);
        lisp::set_list(lat, 4, lisp::make_integer(12));

        print(lisp::get_list(lisp::get_var("L"), 4));

        function_test();
        arithmetic_test();

        return 0;
    }
    // TODO: real argument parsing...

    std::string line;
    std::cout << ">> ";
    while (std::getline(std::cin, line)) {
        lisp::eval(line.c_str());
        print(lisp::get_op(0));
        lisp::pop_op();
        std::cout << "stack size: "
                  << lisp::bound_context->operand_stack_.size()
                  << ", object pool: "
                  << lisp::bound_context->memory_.remaining()
                  << std::endl;
        std::cout << ">> ";
    }
}

#endif // __GBA__
