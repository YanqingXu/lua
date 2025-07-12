#pragma once

#include "../common/types.hpp"

namespace Lua {
    // Operation codes used by the VM / compiler (subset of Lua 5.1).
    // NOTE: Helper functions that manipulate Instruction objects are
    // implemented in `vm/instruction.hpp` to avoid circular includes.
    enum class OpCode : u8 {
        // Basic moves / loads
        MOVE,
        LOADK,
        LOADBOOL,
        LOADNIL,

        // Table / global access
        GETGLOBAL,
        SETGLOBAL,
        GETTABLE,
        SETTABLE,
        NEWTABLE,

        // Arithmetic
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        POW,
        UNM,
        NOT,
        LEN,

        // Stack ops
        POP,

        // Comparisons
        EQ,
        LT,
        LE,

        // Jump / branching
        JMP,
        TEST,

        // Function call / return
        CALL,
        RETURN,

        // Closures & misc
        CLOSURE,
        GETUPVAL,
        SETUPVAL,
        CONCAT,
        CLOSE,

        // Metamethod-aware operations
        GETTABLE_MM,    // Table access with __index metamethod
        SETTABLE_MM,    // Table assignment with __newindex metamethod
        CALL_MM,        // Function call with __call metamethod
        ADD_MM,         // Addition with __add metamethod
        SUB_MM,         // Subtraction with __sub metamethod
        MUL_MM,         // Multiplication with __mul metamethod
        DIV_MM,         // Division with __div metamethod
        MOD_MM,         // Modulo with __mod metamethod
        POW_MM,         // Power with __pow metamethod
        UNM_MM,         // Unary minus with __unm metamethod
        CONCAT_MM,      // Concatenation with __concat metamethod
        EQ_MM,          // Equality with __eq metamethod
        LT_MM,          // Less than with __lt metamethod
        LE_MM           // Less than or equal with __le metamethod
    };
} // namespace Lua
