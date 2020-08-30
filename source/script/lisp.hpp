#include "number/numeric.hpp"



namespace lisp {


// Call this function prior to initialize the interpreter, must be done at
// startup, prior to calling any of the library routines below.
void init();


struct Value;


struct Integer {
    s32 value_;
};


struct Cons {
    Value* car_;
    Value* cdr_;
};


struct Function {
    using Impl = Value*(*)(void);

    Impl impl_;
    u8 argc_;
};


struct Error {
    enum class Code {
        value_not_callable,
        too_few_arguments,
        symbol_table_exhausted,
        undefined_variable_access,
        invalid_argument_type,
    } code_;

    static const char* get_string(Code c)
    {
        switch (c) {
        case Code::value_not_callable: return "Value not callable";
        case Code::too_few_arguments: return "Too few arguments for function";
        case Code::symbol_table_exhausted: return "No more room in symbol table";
        case Code::undefined_variable_access: return "Access to undefined variable";
        case Code::invalid_argument_type: return "Invalid argument type";
        }
        return "Unknown error";
    }
};


struct Value {
    enum class Type : u8 {
        nil, integer, cons, function, error
    } type_ : 7;

    bool mark_bit_ : 1;

    union {
        Integer integer_;
        Cons cons_;
        Function function_;
        Error error_;
    };
};


extern Value nil;
#define L_NIL &lisp::nil


Value* make_function(u8 argc, Function::Impl impl);
Value* make_cons(Value* car, Value* cdr);
Value* make_integer(s32 value);
Value* make_list(u32 length);
Value* make_error(Error::Code error_code);


void set_list(Value* list, u32 position, Value* value);
Value* get_list(Value* list, u32 position);


// For passing parameter to functions. Operands should be pushed in forward
// order, and read in REVERSE ORDER.
void push_op(Value* operand);
Value* get_op(u32 operand_number);
void pop_op();


// Arguments should be pushed onto the operand stack prior to the function
// call. The interpreter will consume the arguments, leaving the result on top
// of the operand stack. i.e., use get_op(0) to read the result. Remember to
// call pop_op() when you are done with the result, otherwise, the result will
// remain on the operand stack, and possibly break the interpreter.
void funcall(Value* obj);


// For named variables. Currently, the interpreter does not support function
// definitions in lisp yet, so all variables are globally scoped.
Value* set_var(const char* name, Value* value);
Value* get_var(const char* name);


// Interpret lisp code, leaving the result at the top of the operand stack
// (similar to funcall). NOTE: this function does not work like a traditional
// eval function in lisp. The input should be a string representation of lisp
// code, not a higher level s-expression (due to memory constraints, we do not
// parse lisp code into lisp data before evaluating).
u32 eval(const char* code);


} // namespace lisp
