#pragma once

#include "number/int.h"
#include "number/endian.hpp"


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


struct JumpSmall {
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


struct JumpSmallIfFalse {
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


}

}
