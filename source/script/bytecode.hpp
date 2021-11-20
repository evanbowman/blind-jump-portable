#pragma once

#include "number/endian.hpp"
#include "number/int.h"
#include "platform/scratch_buffer.hpp"


namespace lisp {


using Opcode = u8;


namespace instruction {


struct Header {
    Opcode op_;
};


struct Fatal {
    Header header_;

    static const char* name()
    {
        return "FATAL";
    }

    static constexpr Opcode op()
    {
        // Do not replace the fatal opcode with anything else, this is meant to
        // prevent buffer overruns in the event of errors. (The interpreter
        // initializes all slabs of by bytecode memory to zero.)
        return 0;
    }
};


struct LoadVar {
    Header header_;
    host_u16 name_offset_;

    static const char* name()
    {
        return "LOAD_VAR";
    }

    static constexpr Opcode op()
    {
        return 1;
    }
};


struct PushNil {
    Header header_;

    static const char* name()
    {
        return "PUSH_NIL";
    }

    static constexpr Opcode op()
    {
        return 2;
    }
};


struct PushInteger {
    Header header_;
    host_u32 value_;

    static const char* name()
    {
        return "PUSH_INTEGER";
    }

    static constexpr Opcode op()
    {
        return 3;
    }
};


struct PushSmallInteger {
    Header header_;
    s8 value_;

    static const char* name()
    {
        return "PUSH_SMALL_INTEGER";
    }

    static constexpr Opcode op()
    {
        return 4;
    }
};


struct Push0 {
    Header header_;

    static const char* name()
    {
        return "PUSH_0";
    }

    static constexpr Opcode op()
    {
        return 5;
    }
};


struct Push1 {
    Header header_;

    static const char* name()
    {
        return "PUSH_1";
    }

    static constexpr Opcode op()
    {
        return 6;
    }
};


struct Push2 {
    Header header_;

    static const char* name()
    {
        return "PUSH_2";
    }

    static constexpr Opcode op()
    {
        return 7;
    }
};


struct PushSymbol {
    Header header_;
    host_u16 name_offset_;

    static const char* name()
    {
        return "PUSH_SYMBOL";
    }

    static constexpr Opcode op()
    {
        return 8;
    }
};


struct PushList {
    Header header_;
    u8 element_count_;

    static const char* name()
    {
        return "PUSH_LIST";
    }

    static constexpr Opcode op()
    {
        return 9;
    }
};


struct Funcall {
    Header header_;
    u8 argc_;

    static const char* name()
    {
        return "FUNCALL";
    }

    static constexpr Opcode op()
    {
        return 10;
    }
};


struct Funcall1 {
    Header header_;

    static const char* name()
    {
        return "FUNCALL_1";
    }

    static constexpr Opcode op()
    {
        return 11;
    }
};


struct Funcall2 {
    Header header_;

    static const char* name()
    {
        return "FUNCALL_2";
    }

    static constexpr Opcode op()
    {
        return 12;
    }
};


struct Funcall3 {
    Header header_;

    static const char* name()
    {
        return "FUNCALL_3";
    }

    static constexpr Opcode op()
    {
        return 13;
    }
};


struct Jump {
    Header header_;
    host_u16 offset_;

    static const char* name()
    {
        return "JUMP";
    }

    static constexpr Opcode op()
    {
        return 14;
    }
};


struct SmallJump {
    Header header_;
    u8 offset_;

    static const char* name()
    {
        return "JUMP_SMALL";
    }

    static constexpr Opcode op()
    {
        return 15;
    }
};


struct JumpIfFalse {
    Header header_;
    host_u16 offset_;

    static const char* name()
    {
        return "JUMP_IF_FALSE";
    }

    static constexpr Opcode op()
    {
        return 16;
    }
};


struct SmallJumpIfFalse {
    Header header_;
    u8 offset_;

    static const char* name()
    {
        return "JUMP_SMALL_IF_FALSE";
    }

    static constexpr Opcode op()
    {
        return 17;
    }
};


struct PushLambda {
    Header header_;
    host_u16 lambda_end_;

    static const char* name()
    {
        return "PUSH_LAMBDA";
    }

    static constexpr Opcode op()
    {
        return 18;
    }
};


struct Pop {
    Header header_;

    static const char* name()
    {
        return "POP";
    }

    static constexpr Opcode op()
    {
        return 19;
    }
};


struct Dup {
    Header header_;

    static const char* name()
    {
        return "DUP";
    }

    static constexpr Opcode op()
    {
        return 20;
    }
};


struct Ret {
    Header header_;

    static const char* name()
    {
        return "RET";
    }

    static constexpr Opcode op()
    {
        return 21;
    }
};


struct MakePair {
    Header header_;

    static const char* name()
    {
        return "MAKE_PAIR";
    }

    static constexpr Opcode op()
    {
        return 22;
    }
};


struct First {
    Header header_;

    static const char* name()
    {
        return "CAR";
    }

    static constexpr Opcode op()
    {
        return 23;
    }
};


struct Rest {
    Header header_;

    static const char* name()
    {
        return "CDR";
    }

    static constexpr Opcode op()
    {
        return 24;
    }
};


struct Arg {
    Header header_;

    static const char* name()
    {
        return "ARGN";
    }

    static constexpr Opcode op()
    {
        return 27;
    }
};


struct TailCall {
    Header header_;
    u8 argc_;

    static const char* name()
    {
        return "TAILCALL";
    }

    static constexpr Opcode op()
    {
        return 28;
    }
};


struct TailCall1 {
    Header header_;

    static const char* name()
    {
        return "TAILCALL1";
    }

    static constexpr Opcode op()
    {
        return 29;
    }
};


struct TailCall2 {
    Header header_;

    static const char* name()
    {
        return "TAILCALL2";
    }

    static constexpr Opcode op()
    {
        return 30;
    }
};


struct TailCall3 {
    Header header_;

    static const char* name()
    {
        return "TAILCALL3";
    }

    static constexpr Opcode op()
    {
        return 31;
    }
};


struct PushThis {
    Header header_;

    static const char* name()
    {
        return "PUSH_THIS";
    }

    static constexpr Opcode op()
    {
        return 32;
    }
};


struct Arg0 {
    Header header_;

    static const char* name()
    {
        return "ARG0";
    }

    static constexpr Opcode op()
    {
        return 33;
    }
};


struct Arg1 {
    Header header_;

    static const char* name()
    {
        return "ARG1";
    }

    static constexpr Opcode op()
    {
        return 34;
    }
};


struct Arg2 {
    Header header_;

    static const char* name()
    {
        return "ARG2";
    }

    static constexpr Opcode op()
    {
        return 35;
    }
};


// By convention, we distinguish between a return from the end of a function,
// and a return from the middle of a function. Both opcodes do the same thing,
// but we want to be able to determine where a function ends, mostly because a
// unique terminating opcode at the very end of a function makes the
// disassembler easier to write, otherwise, we'd need to store the bytecode
// length.
struct EarlyRet {
    Header header_;

    static const char* name()
    {
        return "RET";
    }

    static constexpr Opcode op()
    {
        return 36;
    }
};


struct Not {
    Header header_;

    static const char* name()
    {
        return "NOT";
    }

    static constexpr Opcode op()
    {
        return 37;
    }
};


struct LexicalDef {
    Header header_;
    host_u16 name_offset_;

    static const char* name()
    {
        return "LEXICAL_DEF";
    }

    static constexpr Opcode op()
    {
        return 38;
    }
};


struct LexicalFramePush {
    Header header_;

    static const char* name()
    {
        return "LEXICAL_FRAME_PUSH";
    }

    static constexpr Opcode op()
    {
        return 39;
    }
};


struct LexicalFramePop {
    Header header_;

    static const char* name()
    {
        return "LEXICAL_FRAME_POP";
    }

    static constexpr Opcode op()
    {
        return 40;
    }
};


struct LexicalVarLoad {
    Header header_;
    u8 frame_;
    u8 slot_;

    static const char* name()
    {
        return "LEXICAL_VAR_LOAD";
    }

    static constexpr Opcode op()
    {
        return 41;
    }
};


struct PushString {
    Header header_;
    u8 length_;
    // u8 bytes_[length_];

    static const char* name()
    {
        return "PUSH_STRING";
    }

    static constexpr Opcode op()
    {
        return 42;
    }
};


// NOTE: These relocatable instructions exist for portable bytecode. The
// non-relocatable versions of these instructions refer to an index in the
// interpreter's string intern table. We cannot assume anything about the
// ordering of the string intern table in the interpreter that ultimately loads
// the bytecode, so instead, we ship a symbol table along with the portable
// bytecode. When first seeing this instruction, the VM loads the symbol string
// from the bytecode's symbol table, and searches the host interpreter's symbol
// table for the symbol string. The vm then swaps the relocatable instruction
// for the non-relocatable version, using the symbol offset into the host symbol
// table. Loading relocatable symbols is a bit slow the first time, but
// optimized out after the first load.
struct LoadVarRelocatable : public LoadVar {
    static const char* name()
    {
        return "LOAD_VAR_RELOCATABLE";
    }

    static constexpr Opcode op()
    {
        return 43;
    }
}; static_assert(sizeof(LoadVarRelocatable) == sizeof(LoadVar));


struct PushSymbolRelocatable : public PushSymbol {
    static const char* name()
    {
        return "PUSH_SYMBOL_RELOCATABLE";
    }

    static constexpr Opcode op()
    {
        return 44;
    }
}; static_assert(sizeof(PushSymbolRelocatable) == sizeof(PushSymbol));


struct LexicalDefRelocatable : public LexicalDef {
    static const char* name()
    {
        return "LEXICAL_DEF_RELOCATABLE";
    }

    static constexpr Opcode op()
    {
        return 45;
    }
}; static_assert(sizeof(LexicalDefRelocatable) == sizeof(LexicalDef));



// Just a utility intended for the compiler, not to be used by the vm.
inline Header* load_instruction(ScratchBuffer& buffer, int index)
{
    int offset = 0;

    while (true) {
        switch (buffer.data_[offset]) {
        case Fatal::op():
            return nullptr;

        case PushString::op():
            if (index == 0) {
                return (Header*)(buffer.data_ + offset);
            } else {
                index--;
                offset += sizeof(PushString) +
                    ((PushString*)(buffer.data_ + offset))->length_;
            }
            break;

#define MATCH(NAME)                                                            \
    case NAME::op():                                                           \
        if (index == 0) {                                                      \
            return (Header*)(buffer.data_ + offset);                           \
        } else {                                                               \
            index--;                                                           \
            offset += sizeof(NAME);                                            \
        }                                                                      \
        break;

            MATCH(LoadVar)
            MATCH(LoadVarRelocatable)
            MATCH(PushSymbol)
            MATCH(PushSymbolRelocatable)
            MATCH(PushNil)
            MATCH(Push0)
            MATCH(Push1)
            MATCH(Push2)
            MATCH(PushInteger)
            MATCH(PushSmallInteger)
            MATCH(JumpIfFalse)
            MATCH(Jump)
            MATCH(SmallJumpIfFalse)
            MATCH(SmallJump)
            MATCH(PushLambda)
            MATCH(TailCall)
            MATCH(TailCall1)
            MATCH(TailCall2)
            MATCH(TailCall3)
            MATCH(Funcall)
            MATCH(Funcall1)
            MATCH(Funcall2)
            MATCH(Funcall3)
            MATCH(PushList)
            MATCH(Pop)
            MATCH(Ret)
            MATCH(EarlyRet)
            MATCH(Dup)
            MATCH(MakePair)
            MATCH(First)
            MATCH(Rest)
            MATCH(Arg)
            MATCH(Arg0)
            MATCH(Arg1)
            MATCH(Arg2)
            MATCH(PushThis)
            MATCH(Not)
            MATCH(LexicalDef)
            MATCH(LexicalDefRelocatable)
            MATCH(LexicalFramePush)
            MATCH(LexicalFramePop)
            MATCH(LexicalVarLoad)
        }
    }
    return nullptr;
}
} // namespace instruction

} // namespace lisp
