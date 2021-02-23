#pragma once

#include "number/int.h"


enum class Opcode : u8 {
    nop,                // 1: NOP
    load_var,           // 3: LOAD_VAR(u16 name_offset)
    set_var,            // 3: SET_VAR(u16 name_offset)

    // These functions use inline-caching to improve variable assignment/lookup
    // performance. Any calls to bind/unbind will invalidate all cache entries
    load_cached,        // 3: LOAD_CACHED(u16 var_tab_offset)
    set_cached,         // 3: SET_CACHED(u16 var_tab_offset)

    push_nil,           // 1: PUSH_NIL
    push_integer,       // 5: PUSH_INTEGER(s32 value)
    push_small_integer, //
    push_symbol,        // 3: PUSH_SYMBOL(u16 name_offset)
    push_list,          // 2: PUSH_LIST(u8 count)
    pop,                // 1: POP

    funcall,            // 2: FUNCALL(u8 argc)

    jump,               // 3: JUMP(u16 offset)
    small_jump,         // 2: SMALL_JUMP(u8 offset)

    ret,                // 1: RET
};
