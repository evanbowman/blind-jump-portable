#pragma once

#include "number/int.h"


// clang-format off


enum class Opcode : u8 {
//  name                | width |  usage

// OPCODE VALUE 0 TRIGGERS A FATAL ERROR IF CALLED. THE BYTECODE COMPILER WILL
// SET ALL NEWLY-ALLOCATED PROGRAM MEMORY TO ZERO, SO ERRONEOUS JUMP
// INSTRUCTIONS OR ERRORS IN THE BYTECODE OUTPUT ARE LIKELY TO EXIT THE VM,
// RATHER THAN OVERRUN THE CODE BUFFER.
    fatal,               // 1      FATAL

// VARIABLE LOADING OPCODES
    load_var,            // 3      LOAD_VAR(u16 name_offset)

// OPCODES FOR PUSHING LITERALS
    push_nil,            // 1      PUSH_NIL
    push_integer,        // 5      PUSH_INTEGER(s32 value)
    push_small_integer,  // 2      PUSH_SMALL_INTEGER(s8 value)
    push_0,              // 1      PUSH_0
    push_1,              // 1      PUSH_1
    push_2,              // 1      PUSH_2
    push_symbol,         // 3      PUSH_SYMBOL(u16 name_offset)
    push_list,           // 2      PUSH_LIST(u8 count)

// FUNCtiON CALL OPCODES
    funcall,             // 2      FUNCALL(u8 argc)
    funcall_1,           // 1      FUNCALL_1
    funcall_2,           // 1      FUNCALL_2
    funcall_3,           // 1      FUNCALL_3

// OPCODES THAT MODIFY THE VM's PROGRAM COUNTER
    jump,                // 3      JUMP(u16 offset)
    jump_small,          // 2      JUMP_SMALL(u8 offset)
    jump_if_false,       // 3      JUMP_IF_FALSE(u16 offset)
    jump_small_if_false, // 2      JUMP_SMALL_IF_FALSE(u8 offset)
    push_lambda,         // 1      PUSH_LAMBDA(u16 lambda_end)

// MISC OPCODES FOR THE OPERAND STACK
    pop,                 // 1      POP
    popn,                // 2      POPN
    dup,                 // 1      DUP

// RETURN OPCODES
    ret,                 // 1      RET
};


// clang-format on
