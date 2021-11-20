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
#include "module.hpp"
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


struct ValueHeader {
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

    Type type() const
    {
        return (Type)type_;
    }
};


struct Nil {
    ValueHeader hdr_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::nil;
    }

    static void finalizer(Value*)
    {
    }
};


struct Symbol {
    ValueHeader hdr_;
    const char* name_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::symbol;
    }

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
    ValueHeader hdr_;
    s32 value_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::integer;
    }

    static void finalizer(Value*)
    {
    }
};


struct Character {
    ValueHeader hdr_;
    utf8::Codepoint cp;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::character;
    }

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
    ValueHeader hdr_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::cons;
    }

    inline Value* car()
    {
        return dcompr(car_);
    }

    inline Value* cdr()
    {
        return cdr_;
    }

    void set_car(Value* val)
    {
        car_ = compr(val);
    }

    void set_cdr(Value* val)
    {
        cdr_ = val;
    }

    static void finalizer(Value*)
    {
    }

private:
    // NOTE: we want all values in our interpreter to take up eight bytes or
    // less on a 32-bit system. We compress the car pointer, and use a full
    // 32-bit pointer for the cdr. If we need to compress one of the pointers,
    // we might as well optimize the cdr, as there are cases where you just want
    // to iterate to a certain point in a list and do not care what the car is.
    CompressedPtr car_;
    Value* cdr_;
};


struct Function {
    ValueHeader hdr_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::function;
    }

    using CPP_Impl = Value* (*)(int);

    struct Bytecode {
        CompressedPtr bytecode_; // (integeroffset . databuffer)
        CompressedPtr lexical_bindings_;

        Value* bytecode_offset() const;
        Value* databuffer() const;
    };

    struct LispFunction {
        CompressedPtr code_;
        CompressedPtr lexical_bindings_;
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
    ValueHeader hdr_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::data_buffer;
    }

    alignas(ScratchBufferPtr) u8 sbr_mem_[sizeof(ScratchBufferPtr)];

    ScratchBufferPtr value()
    {
        return *reinterpret_cast<ScratchBufferPtr*>(sbr_mem_);
    }

    static void finalizer(Value* buffer);
};


struct String {
    ValueHeader hdr_;
    CompressedPtr data_buffer_;
    u16 offset_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::string;
    }

    const char* value();

    static void finalizer(Value*)
    {
    }
};


struct Error {
    ValueHeader hdr_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::error;
    }

    enum class Code : u8 {
        value_not_callable,
        invalid_argc,
        symbol_table_exhausted,
        undefined_variable_access,
        invalid_argument_type,
        out_of_memory,
        set_in_expression_context,
        mismatched_parentheses,
        invalid_syntax,
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
        case Code::invalid_syntax:
            return "invalid syntax";
        }
        return "Unknown error";
    }

    static void finalizer(Value*)
    {
    }
};


struct UserData {
    ValueHeader hdr_;
    void* obj_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::user_data;
    }

    static void finalizer(Value*)
    {
    }
};


struct __Reserved {
    ValueHeader hdr_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::__reserved;
    }

    // Reserved for future use

    static void finalizer(Value*)
    {
    }
};


struct HeapNode {
    ValueHeader hdr_;
    Value* next_;

    static ValueHeader::Type type()
    {
        return ValueHeader::Type::heap_node;
    }

    static void finalizer(Value*)
    {
        // Should be unreachable.
        while (true)
            ;
    }
};


struct Value {
    ValueHeader hdr_;

    using Type = ValueHeader::Type;

    Type type() const
    {
        return hdr_.type();
    }

    HeapNode& heap_node()
    {
        return *reinterpret_cast<HeapNode*>(this);
    }

    Nil& nil()
    {
        return *reinterpret_cast<Nil*>(this);
    }

    Integer& integer()
    {
        return *reinterpret_cast<Integer*>(this);
    }

    Cons& cons()
    {
        return *reinterpret_cast<Cons*>(this);
    }

    Function& function()
    {
        return *reinterpret_cast<Function*>(this);
    }

    Error& error()
    {
        return *reinterpret_cast<Error*>(this);
    }

    Symbol& symbol()
    {
        return *reinterpret_cast<Symbol*>(this);
    }

    UserData& user_data()
    {
        return *reinterpret_cast<UserData*>(this);
    }

    DataBuffer& data_buffer()
    {
        return *reinterpret_cast<DataBuffer*>(this);
    }

    String& string()
    {
        return *reinterpret_cast<String*>(this);
    }

    Character& character()
    {
        return *reinterpret_cast<Character*>(this);
    }

    template <typename T> T& expect()
    {
        if (this->type() == T::type()) {
            return *reinterpret_cast<T*>(this);
        }

        while (true)
            ;
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
Value* get_op0();
Value* get_op1();
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
    if (var_sym->type() not_eq Value::Type::symbol) {
        return var_sym;
    }

    return set_var(var_sym, value);
}


inline Value* get_var(const char* name)
{
    auto var_sym = make_symbol(name);
    if (var_sym->type() not_eq Value::Type::symbol) {
        return var_sym;
    }

    return get_var(var_sym);
}


// Read an S-expression, leaving the result at the top of the operand stack.
u32 read(const char* code);


// Result on operand stack.
void eval(Value* code);


// Parameter should be a function. Result on operand stack.
void compile(Platform& pfrm, Value* code);


// Load code from a portable bytecode module. Result on operand stack.
void load_module(Module* module);


// Returns the result of the last expression in the string.
Value* dostring(const char* code, ::Function<16, void(Value&)> on_error);


bool is_executing();


const char* intern(const char* string);


#define L_EXPECT_OP(OFFSET, TYPE)                                              \
    if (lisp::Value::Type::TYPE not_eq lisp::Value::Type::error and            \
        lisp::get_op((OFFSET))->type() == lisp::Value::Type::error) {          \
        return lisp::get_op((OFFSET));                                         \
    } else if (lisp::get_op((OFFSET))->type() not_eq                           \
               lisp::Value::Type::TYPE) {                                      \
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
class ProtectedBase {
public:
    ProtectedBase();

    ProtectedBase(const ProtectedBase&) = delete;
    ProtectedBase(ProtectedBase&&) = delete;

    virtual ~ProtectedBase();

    virtual void gc_mark() = 0;

    ProtectedBase* next() const
    {
        return next_;
    }

    ProtectedBase* prev() const
    {
        return prev_;
    }

protected:
    ProtectedBase* prev_;
    ProtectedBase* next_;
};



class Protected : public ProtectedBase {
public:
    Protected(Value* val) : val_(val)
    {
    }

    void gc_mark() override;

    Protected& operator=(Value* val)
    {
        val_ = val;
        return *this;
    }

    void set(Value* val)
    {
        val_ = val;
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
};



template <typename F> void foreach (Value* list, F && fn)
{
    Protected p(list);

    while (true) {

        if (list->type() not_eq Value::Type::cons) {
            break;
        } else {
            fn(list->cons().car());
        }

        list = list->cons().cdr();
    }
}


} // namespace lisp
