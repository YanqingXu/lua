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
        CONCAT,
        CLOSE
    };
} // namespace Lua
