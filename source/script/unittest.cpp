#include "lisp.hpp"
#include "platform/platform.hpp"


#include <fstream>
#include <iostream>


static lisp::Value* function_test()
{
    using namespace lisp;

    set_var("double", make_function([](int argc) {
                L_EXPECT_ARGC(argc, 1);
                L_EXPECT_OP(0, integer);

                return make_integer(get_op(0)->integer().value_ * 2);
            }));

    push_op(make_integer(48));
    funcall(get_var("double"), 1);

    L_EXPECT_OP(0, integer);

    if (get_op(0)->integer().value_ not_eq 48 * 2) {
        std::cout << "funcall test result check failed!" << std::endl;
        return L_NIL;
    }

    // if (bound_context->operand_stack_.size() not_eq 1) {
    //     std::cout << "operand stack size check failed!" << std::endl;
    //     return L_NIL;
    // }


    pop_op();

    std::cout << "funcall test passed!" << std::endl;

    return L_NIL;
}


static lisp::Value* arithmetic_test()
{
    using namespace lisp;

    push_op(make_integer(48));
    push_op(make_integer(96));
    funcall(get_var("-"), 2);

    L_EXPECT_OP(0, integer);

    if (get_op(0)->integer().value_ not_eq 48 - 96) {
        std::cout << "bad arithmetic!" << std::endl;
        return L_NIL;
    }

    std::cout << "arithmetic test passed!" << std::endl;

    return L_NIL;
}


static void intern_test()
{
    auto initial = lisp::intern("blah");
    if (str_cmp("blah", initial) not_eq 0) {
        std::cout << "interpreter intern failed" << std::endl;
        return;
    }

    // Intern some other junk. We want to re-intern the initial string (above),
    // and make sure that we do not receive a non-matching pointer, which would
    // indicate that we somehow have two copies of the string in the intern
    // table.
    if (str_cmp(lisp::intern("dskjflfs"), "dskjflfs") not_eq 0) {
        std::cout << "intern failed" << std::endl;
    }

    if (lisp::intern("blah") not_eq initial) {
        std::cout << "string intern leak" << std::endl;
        return;
    }

    std::cout << "intern test passed!" << std::endl;
}

class Printer : public lisp::Printer {
public:
    void put_str(const char* str) override
    {
        std::cout << str;
    }
};


void do_tests()
{
    auto lat = lisp::make_list(9);

    lisp::set_var("L", lat);
    lisp::set_list(lat, 4, lisp::make_integer(12));

    Printer p;
    lisp::format(lisp::get_list(lisp::get_var("L"), 4), p);

    intern_test();
    function_test();
    arithmetic_test();
}


const char* utilities =
    "(set 'bisect-impl\n"
    "     (compile\n"
    "      (lambda\n"
    "        (if (not $1)\n"
    "            (cons (reverse $2) $0)\n"
    "          (if (not (cdr $1))\n"
    "              (cons (reverse $2) $0)\n"
    "            ((this)\n"
    "             (cdr $0)\n"
    "             (cdr (cdr $1))\n"
    "             (cons (car $0) $2)))))))\n"
    "\n"
    "\n"
    "(set 'bisect (lambda (bisect-impl $0 $0 '())))\n"
    "\n"
    "\n"
    "(set 'merge\n"
    "     (compile\n"
    "      (lambda\n"
    "        (if (not $0)\n"
    "            $1\n"
    "          (if (not $1)\n"
    "              $0\n"
    "            (if (< (car $0) (car $1))\n"
    "                (cons (car $0) ((this) (cdr $0) $1))\n"
    "              (cons (car $1) ((this) $0 (cdr $1)))))))))\n"
    "\n"
    "\n"
    "\n"
    "(set 'sort\n"
    "     (lambda\n"
    "       (if (not (cdr $0))\n"
    "           $0\n"
    "         (let ((temp (bisect $0)))\n"
    "           (merge (sort (car temp))\n"
    "                  (sort (cdr temp)))))))\n"
    "";


int main(int argc, char** argv)
{
    Platform pfrm;

    lisp::init(pfrm);

    lisp::dostring(utilities, [](lisp::Value& err) {});

    std::string line;
    std::cout << ">> ";
    while (std::getline(std::cin, line)) {

        lisp::read(line.c_str());
        Printer p;
        auto result = lisp::get_op(0);
        // format(result, p);
        // std::cout << std::endl;
        lisp::pop_op();
        lisp::eval(result);
        result = lisp::get_op(0);
        lisp::pop_op();
        format(result, p);
        // std::cout << "stack size: "
        //           << lisp::bound_context->operand_stack_.size()
        //           << ", object pool: "
        //           << lisp::bound_context->memory_.remaining()
        //           << ", intern mem: " << lisp::bound_context->string_intern_pos_
        //           << std::endl;
        std::cout << std::endl << ">> ";
    }
}
