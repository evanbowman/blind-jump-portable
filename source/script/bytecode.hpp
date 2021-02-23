#pragma once

#include "number/int.h"


// clang-format off


enum class Opcode : u8 {
//  name               | width |  usage
    fatal,              // 1      FATAL
    load_var,           // 3      LOAD_VAR(u16 name_offset)

    push_nil,           // 1      PUSH_NIL
    push_integer,       // 5      PUSH_INTEGER(s32 value)
    push_small_integer, // 2      PUSH_SMALL_INTEGER(s8 value)
    push_0,             // 1      PUSH_0
    push_1,             // 1      PUSH_1
    push_2,             // 1      PUSH_2
    push_symbol,        // 3      PUSH_SYMBOL(u16 name_offset)
    push_list,          // 2      PUSH_LIST(u8 count)

    funcall,            // 2      FUNCALL(u8 argc)
    funcall_1,          // 1      FUNCALL_1
    funcall_2,          // 1      FUNCALL_2
    funcall_3,          // 1      FUNCALL_3

    jump,               // 3      JUMP(u16 offset)
    jump_small,         // 2      JUMP_SMALL(u8 offset)
    jump_if_true,       // 3      JUMP_IF_TRUE(u16 offset)
    jump_small_if_true, // 2      JUMP_SMALL_IF_TRUE(u8 offset)

    pop,                // 1      POP

    ret,                // 1:      RET
};


// clang-format on
