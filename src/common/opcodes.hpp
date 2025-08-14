#pragma once

#include "../common/types.hpp"

namespace Lua {
    // Operation codes used by the VM / compiler - OFFICIAL Lua 5.1 opcodes ONLY
    // Based on lua-5.1.5/src/lopcodes.h - 38 opcodes total
    // NOTE: Helper functions that manipulate Instruction objects are
    // implemented in `vm/instruction.hpp` to avoid circular includes.
    enum class OpCode : u8 {
        // === 官方Lua 5.1操作码 (严格按照lopcodes.h顺序) ===

        MOVE,           // 0:  R(A) := R(B)
        LOADK,          // 1:  R(A) := Kst(Bx)
        LOADBOOL,       // 2:  R(A) := (Bool)B; if (C) pc++
        LOADNIL,        // 3:  R(A) := ... := R(B) := nil
        GETUPVAL,       // 4:  R(A) := UpValue[B]

        GETGLOBAL,      // 5:  R(A) := Gbl[Kst(Bx)]
        GETTABLE,       // 6:  R(A) := R(B)[RK(C)]

        SETGLOBAL,      // 7:  Gbl[Kst(Bx)] := R(A)
        SETUPVAL,       // 8:  UpValue[B] := R(A)
        SETTABLE,       // 9:  R(A)[RK(B)] := RK(C)

        NEWTABLE,       // 10: R(A) := {} (size = B,C)

        SELF,           // 11: R(A+1) := R(B); R(A) := R(B)[RK(C)]

        ADD,            // 12: R(A) := RK(B) + RK(C)
        SUB,            // 13: R(A) := RK(B) - RK(C)
        MUL,            // 14: R(A) := RK(B) * RK(C)
        DIV,            // 15: R(A) := RK(B) / RK(C)
        MOD,            // 16: R(A) := RK(B) % RK(C)
        POW,            // 17: R(A) := RK(B) ^ RK(C)
        UNM,            // 18: R(A) := -R(B)
        NOT,            // 19: R(A) := not R(B)
        LEN,            // 20: R(A) := length of R(B)

        CONCAT,         // 21: R(A) := R(B).. ... ..R(C)

        JMP,            // 22: pc+=sBx

        EQ,             // 23: if ((RK(B) == RK(C)) ~= A) then pc++
        LT,             // 24: if ((RK(B) <  RK(C)) ~= A) then pc++
        LE,             // 25: if ((RK(B) <= RK(C)) ~= A) then pc++

        TEST,           // 26: if not (R(A) <=> C) then pc++
        TESTSET,        // 27: if (R(B) <=> C) then R(A) := R(B) else pc++

        CALL,           // 28: R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
        TAILCALL,       // 29: return R(A)(R(A+1), ... ,R(A+B-1))
        RETURN,         // 30: return R(A), ... ,R(A+B-2)

        FORLOOP,        // 31: R(A)+=R(A+2); if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }
        FORPREP,        // 32: R(A)-=R(A+2); pc+=sBx

        TFORLOOP,       // 33: R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2)); if R(A+3) ~= nil then R(A+2)=R(A+3) else pc++
        SETLIST,        // 34: R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B

        CLOSE,          // 35: close all variables in the stack up to (>=) R(A)
        CLOSURE,        // 36: R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))

        VARARG          // 37: R(A), R(A+1), ..., R(A+B-1) = vararg

        // 总计38个操作码 - 与官方Lua 5.1完全一致
    };
} // namespace Lua
