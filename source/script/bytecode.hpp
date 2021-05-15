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
        return "FIRST";
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
        return "REST";
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


// Just a utility intended for the compiler, not to be used by the vm.
inline Header* load_instruction(ScratchBuffer& buffer, int index)
{
    int offset = 0;

    while (true) {
        switch (buffer.data_[offset]) {
        case Fatal::op():
            return nullptr;

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
            MATCH(PushSymbol)
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
            MATCH(Funcall)
            MATCH(Funcall1)
            MATCH(Funcall2)
            MATCH(Funcall3)
            MATCH(PushList)
            MATCH(Pop)
            MATCH(Ret)
            MATCH(Dup)
            MATCH(MakePair)
            MATCH(First)
            MATCH(Rest)
            MATCH(Arg)
        }
    }
    return nullptr;
}
} // namespace instruction

} // namespace lisp
