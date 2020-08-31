#include "lisp.hpp"


#if not defined(__GBA__) and defined(__STANDALONE__)
#include <iostream>

void print_impl(lisp::Value* value)
{
    switch (value->type_) {
    case lisp::Value::Type::nil:
        std::cout << "nil";
        break;

    case lisp::Value::Type::symbol:
        std::cout << '\'' << value->symbol_.name_;
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
        std::cout << "lambda:" << (void*)value->function_.impl_;
        break;

    case lisp::Value::Type::error:
        std::cout << "ERROR: " << lisp::Error::get_string(value->error_.code_)
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

    set_var("double", make_function([](int argc) {
                EXPECT_ARGC(argc, 1);
                EXPECT_OP(0, integer);

                return make_integer(get_op(0)->integer_.value_ * 2);
            }));

    push_op(make_integer(48));
    funcall(get_var("double"), 1);

    EXPECT_OP(0, integer);

    if (get_op(0)->integer_.value_ not_eq 48 * 2) {
        std::cout << "funcall test result check failed!" << std::endl;
        return NIL;
    }

    if (bound_context->operand_stack_.size() not_eq 1) {
        std::cout << "operand stack size check failed!" << std::endl;
        return NIL;
    }

    bound_context->operand_stack_.pop_back();

    std::cout << "funcall test passed!" << std::endl;

    return NIL;
}


static lisp::Value* arithmetic_test()
{
    using namespace lisp;

    push_op(make_integer(48));
    push_op(make_integer(96));
    funcall(get_var("-"), 2);

    EXPECT_OP(0, integer);

    if (get_op(0)->integer_.value_ not_eq 48 - 96) {
        std::cout << "bad arithmetic!" << std::endl;
        return NIL;
    }

    std::cout << "arithmetic test passed!" << std::endl;

    return NIL;
}


static void intern_test()
{
    auto initial = lisp::intern("blah");
    if (strcmp("blah", initial) not_eq 0) {
        std::cout << "interpreter intern failed" << std::endl;
        return;
    }

    // Intern some other junk. We want to re-intern the initial string (above),
    // and make sure that we do not receive a non-matching pointer, which would
    // indicate that we somehow have two copies of the string in the intern
    // table.
    if (strcmp(lisp::intern("dskjflfs"), "dskjflfs") not_eq 0) {
        std::cout << "intern failed" << std::endl;
    }

    if (lisp::intern("blah") not_eq initial) {
        std::cout << "string intern leak" << std::endl;
        return;
    }

    std::cout << "intern test passed!" << std::endl;
}


void do_tests()
{
    auto lat = lisp::make_list(9);

    lisp::set_var("L", lat);
    lisp::set_list(lat, 4, lisp::make_integer(12));

    print(lisp::get_list(lisp::get_var("L"), 4));

    intern_test();
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

        intern_test();
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
                  << ", intern mem: " << lisp::bound_context->string_intern_pos_
                  << std::endl;
        std::cout << ">> ";
    }
}

#endif
