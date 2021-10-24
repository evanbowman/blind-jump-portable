#pragma once


#ifdef __GBA__
// If defined, the system will use fixed pools, and will never call malloc.
#define UNHOSTED
#endif
#define UNHOSTED


#ifdef UNHOSTED
#define USE_COMPRESSED_PTRS
#endif



#ifndef UNHOSTED
#define POOL_USE_HEAP
#endif



#include "function.hpp"
#include "number/numeric.hpp"
#include "platform/scratch_buffer.hpp"
#include "string.hpp"
#include "unicode.hpp"




class Platform;


namespace lisp {


// Call this function to initialize the interpreter, must be done at startup,
// prior to calling any of the library routines below.
void init(Platform& pfrm);


struct Value;


struct Nil {
    static void finalizer(Value*)
    {
    }
};


struct Symbol {
    const char* name_;

    enum class ModeBits {
        requires_intern,
        // If you create a symbol, while promising that the string pointer is
        // stable, the interpreter assumes that the string was previously
        // inserted into the string intern table. It will not perform the
        // slow lookup into the string intern memory region.
        stable_pointer,
    };

    static void finalizer(Value*)
    {
    }
};


struct Integer {
    s32 value_;

    static void finalizer(Value*)
    {
    }
};


struct Character {
    utf8::Codepoint cp;

    static void finalizer(Value*)
    {
    }
};


struct CompressedPtr {
#ifdef USE_COMPRESSED_PTRS
    u16 offset_;
#else
    void* ptr_;
#endif // USE_COMPRESSED_PTRS
};


CompressedPtr compr(Value* value);
Value* dcompr(CompressedPtr ptr);


struct Cons {

    inline Value* car()
    {
        return dcompr(car_);
    }

    inline Value* cdr()
    {
        return dcompr(cdr_);
    }

    void set_car(Value* val)
    {
        car_ = compr(val);
    }

    void set_cdr(Value* val)
    {
        cdr_ = compr(val);
    }

    static void finalizer(Value*)
    {
    }

private:
    CompressedPtr car_;
    CompressedPtr cdr_;
};


struct Function {
    using CPP_Impl = Value* (*)(int);

    struct Bytecode {
        u16 bc_offset_;
        CompressedPtr data_buffer_;
    };

    struct LispFunction {
        CompressedPtr code_;
        CompressedPtr docstring_;
    };

    union {
        CPP_Impl cpp_impl_;
        LispFunction lisp_impl_;
        Bytecode bytecode_impl_;
    };

    enum ModeBits {
        cpp_function,
        lisp_function,
        lisp_bytecode_function,
    };

    static void finalizer(Value*)
    {
    }
};


struct DataBuffer {
    alignas(ScratchBufferPtr) u8 sbr_mem_[sizeof(ScratchBufferPtr)];

    ScratchBufferPtr value()
    {
        return *reinterpret_cast<ScratchBufferPtr*>(sbr_mem_);
    }

    static void finalizer(Value* buffer);
};



struct String {
    CompressedPtr data_buffer_;
    u16 offset_;

    const char* value();

    static void finalizer(Value*)
    {
    }
};



struct Error {
    enum class Code : u8 {
        value_not_callable,
        invalid_argc,
        symbol_table_exhausted,
        undefined_variable_access,
        invalid_argument_type,
        out_of_memory,
        set_in_expression_context,
        mismatched_parentheses,
    } code_;

    CompressedPtr context_;

    static const char* get_string(Code c)
    {
        switch (c) {
        case Code::value_not_callable:
            return "Value not callable";
        case Code::invalid_argc:
            return "Wrong number of arguments passed to function";
        case Code::symbol_table_exhausted:
            return "No more room in symbol table";
        case Code::undefined_variable_access:
            return "Access to undefined variable";
        case Code::invalid_argument_type:
            return "Invalid argument type";
        case Code::out_of_memory:
            return "Out of memory";
        case Code::set_in_expression_context:
            return "\'set\' in expr context";
        case Code::mismatched_parentheses:
            return "mismatched parentheses";
        }
        return "Unknown error";
    }

    static void finalizer(Value*)
    {
    }
};


struct UserData {
    void* obj_;

    static void finalizer(Value*)
    {
    }
};


struct __Reserved {

    // Reserved for future use

    static void finalizer(Value*)
    {
    }
};


struct HeapNode {
    Value* next_;

    static void finalizer(Value*)
    {
        // Should be unreachable.
        while (true) ;
    }
};


struct Value {
    enum Type {
        // When a Value is deallocated, it is converted into a HeapNode, and
        // inserted into a freelist. Therefore, we need no extra space to
        // maintain the freelist.
        heap_node,

        nil,
        integer,
        cons,
        function,
        error,
        symbol,
        user_data,
        data_buffer,
        string,
        character,
        __reserved,
        count,
    };
    u8 type_ : 4;

    bool alive_ : 1;
    bool mark_bit_ : 1;
    u8 mode_bits_ : 2;

    union {
        HeapNode heap_node_;
        Nil nil_;
        Integer integer_;
        Cons cons_;
        Function function_;
        Error error_;
        Symbol symbol_;
        UserData user_data_;
        DataBuffer data_buffer_;
        String string_;
        Character character_;
        __Reserved __reserved_;
    };

    template <typename T> T& expect()
    {
        if constexpr (std::is_same<T, Integer>()) {
            if (type_ == Type::integer) {
                return integer_;
            } else {
                while (true)
                    ; // TODO: fatal error...
            }
        } else if constexpr (std::is_same<T, Cons>()) {
            if (type_ == Type::cons) {
                return cons_;
            } else {
                while (true)
                    ;
            }
        } else if constexpr (std::is_same<T, Function>()) {
            if (type_ == Type::function) {
                return function_;
            } else {
                while (true)
                    ;
            }
        } else if constexpr (std::is_same<T, Error>()) {
            if (type_ == Type::error) {
                return error_;
            } else {
                while (true)
                    ;
            }
        } else if constexpr (std::is_same<T, Symbol>()) {
            if (type_ == Type::symbol) {
                return symbol_;
            } else {
                while (true)
                    ;
            }
        } else if constexpr (std::is_same<T, UserData>()) {
            if (type_ == Type::user_data) {
                return user_data_;
            } else {
                while (true)
                    ;
            }
        } else {
            // TODO: how to put a static assert here?
            return nullptr;
        }
    }
};


struct IntegralConstant {
    const char* const name_;
    const int value_;
};


void set_constants(const IntegralConstant* array, u16 count);


Value* make_function(Function::CPP_Impl impl);
Value* make_cons(Value* car, Value* cdr);
Value* make_integer(s32 value);
Value* make_list(u32 length);
Value* make_error(Error::Code error_code, Value* context);
Value* make_userdata(void* obj);
Value* make_symbol(const char* name,
                   Symbol::ModeBits mode = Symbol::ModeBits::requires_intern);
Value* make_databuffer(Platform& pfrm);
Value* make_string(Platform& pfrm, const char* str);
Value* make_character(Platform& pfrm, utf8::Codepoint cp);


void get_interns(::Function<24, void(const char*)> callback);
void get_env(::Function<24, void(const char*)> callback);


Value* get_nil();
#define L_NIL lisp::get_nil()


void set_list(Value* list, u32 position, Value* value);
Value* get_list(Value* list, u32 position);
int length(Value* lat);


// For passing parameter to functions. Operands should be pushed in forward
// order, and read in REVERSE ORDER.
void push_op(Value* operand);
Value* get_op(u32 operand_number);
void pop_op();
Value* get_arg(u16 arg);


Value* get_this();
u8 get_argc();


// Arguments should be pushed onto the operand stack prior to the function
// call. The interpreter will consume the arguments, leaving the result on top
// of the operand stack. i.e., use get_op(0) to read the result. Remember to
// call pop_op() when you are done with the result, otherwise, the result will
// remain on the operand stack, and possibly break the interpreter.
//
// You also need to indicate, in the argc parameter, the number of arguments
// that you pushed onto the operand stack.
void funcall(Value* fn, u8 argc);


Value* set_var(Value* sym, Value* value);
Value* get_var(Value* sym);


// Provided for convenience.
inline Value* set_var(const char* name, Value* value)
{
    auto var_sym = make_symbol(name);
    if (var_sym->type_ not_eq Value::Type::symbol) {
        return var_sym;
    }

    return set_var(var_sym, value);
}


inline Value* get_var(const char* name)
{
    auto var_sym = make_symbol(name);
    if (var_sym->type_ not_eq Value::Type::symbol) {
        return var_sym;
    }

    return get_var(var_sym);
}


template <typename T> T& loadv(const char* name)
{
    return get_var(name)->expect<T>();
}


// Read an S-expression, leaving the result at the top of the operand stack.
u32 read(const char* code);


void eval(Value* code);


void compile(Platform& pfrm, Value* code);


// Returns the result of the last expression in the string.
Value* dostring(const char* code, ::Function<16, void(Value&)> on_error);


bool is_executing();


int paren_balance(const char* ptr);


const char* intern(const char* string);


#define L_EXPECT_OP(OFFSET, TYPE)                                              \
    if (lisp::Value::Type::TYPE not_eq lisp::Value::Type::error and            \
        lisp::get_op((OFFSET))->type_ == lisp::Value::Type::error) {           \
        return lisp::get_op((OFFSET));                                         \
    } else if (lisp::get_op((OFFSET))->type_ not_eq lisp::Value::Type::TYPE) { \
        if (lisp::get_op((OFFSET)) == L_NIL) {                                 \
            return lisp::get_op((OFFSET));                                     \
        } else {                                                               \
            return lisp::make_error(lisp::Error::Code::invalid_argument_type,  \
                                    L_NIL);                                    \
        }                                                                      \
    }


#define L_EXPECT_ARGC(ARGC, EXPECTED)                                          \
    if (ARGC not_eq EXPECTED)                                                  \
        return lisp::make_error(lisp::Error::Code::invalid_argc, L_NIL);


class Printer {
public:
    virtual void put_str(const char* c) = 0;
    virtual ~Printer()
    {
    }
};


class DefaultPrinter : public lisp::Printer {
public:
    void put_str(const char* str) override
    {
        fmt_ += str;
    }

    StringBuffer<1024> fmt_;
};



void format(Value* value, Printer& p);


// Protected objects will not be collected until the Protected wrapper goes out
// of scope.
class Protected {
public:
    Protected(Value* val);

    Protected& operator=(Value* val)
    {
        val_ = val;
        return *this;
    }

    Protected(const Protected&) = delete;
    Protected(Protected&&) = delete;

    ~Protected();

    void set(Value* val)
    {
        val_ = val;
    }

    Protected* next() const
    {
        return next_;
    }

    Protected* prev() const
    {
        return prev_;
    }

    operator Value*()
    {
        return val_;
    }

    Value* operator->()
    {
        return val_;
    }

protected:
    Value* val_;
    Protected* prev_;
    Protected* next_;
};


template <typename F> void foreach (Value* list, F && fn)
{
    Protected p(list);

    while (true) {

        if (list->type_ not_eq Value::Type::cons) {
            break;
        } else {
            fn(list->cons_.car());
        }

        list = list->cons_.cdr();
    }
}


} // namespace lisp
