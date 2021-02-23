#pragma once

#include "number/int.h"


enum class Opcode : u8 {
    fatal,              // 1: FATAL
    load_var,           // 3: LOAD_VAR(u16 name_offset)

    push_nil,           // 1: PUSH_NIL
    push_integer,       // 5: PUSH_INTEGER(s32 value)
    push_symbol,        // 3: PUSH_SYMBOL(u16 name_offset)
    push_list,          // 2: PUSH_LIST(u8 count)
    pop,                // 1: POP

    funcall,            // 2: FUNCALL(u8 argc)

    jump,               // 3: JUMP(u16 offset)

    ret,                // 1: RET
};
