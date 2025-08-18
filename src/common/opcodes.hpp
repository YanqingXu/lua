#pragma once

#include "../common/types.hpp"

namespace Lua {
    // === 官方Lua 5.1指令格式枚举 ===
    enum class OpMode : u8 {
        iABC = 0,   // 指令格式: A(8) B(9) C(9)
        iABx = 1,   // 指令格式: A(8) Bx(18)
        iAsBx = 2   // 指令格式: A(8) sBx(18)
    };

    // === 官方Lua 5.1参数模式枚举 ===
    enum class OpArgMask : u8 {
        OpArgN = 0,  // 参数未使用
        OpArgU = 1,  // 参数被使用但不是寄存器/常量
        OpArgR = 2,  // 参数是寄存器或跳转偏移
        OpArgK = 3   // 参数是常量或寄存器/常量(RK)
    };

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

    // === 官方Lua 5.1指令模式信息 ===
    // 基于lua-5.1.5/src/lopcodes.c的luaP_opmodes数组
    // 位域定义：
    // bits 0-1: 指令格式 (OpMode)
    // bits 2-3: C参数模式 (OpArgMask)
    // bits 4-5: B参数模式 (OpArgMask)
    // bit 6:    是否设置寄存器A
    // bit 7:    是否为测试指令

    constexpr u8 NUM_OPCODES = 38;

    // 指令模式编码宏（与官方Lua 5.1一致）
    constexpr u8 opmode(bool t, bool a, OpArgMask b, OpArgMask c, OpMode m) {
        return (static_cast<u8>(t) << 7) |
               (static_cast<u8>(a) << 6) |
               (static_cast<u8>(b) << 4) |
               (static_cast<u8>(c) << 2) |
               static_cast<u8>(m);
    }

    // 官方Lua 5.1指令模式数组（与lopcodes.c完全一致）
    extern const u8 luaP_opmodes[NUM_OPCODES];

    // 指令模式查询函数（与官方Lua 5.1一致）
    inline OpMode getOpMode(OpCode op) {
        return static_cast<OpMode>(luaP_opmodes[static_cast<u8>(op)] & 3);
    }

    inline OpArgMask getBMode(OpCode op) {
        return static_cast<OpArgMask>((luaP_opmodes[static_cast<u8>(op)] >> 4) & 3);
    }

    inline OpArgMask getCMode(OpCode op) {
        return static_cast<OpArgMask>((luaP_opmodes[static_cast<u8>(op)] >> 2) & 3);
    }

    inline bool testAMode(OpCode op) {
        return (luaP_opmodes[static_cast<u8>(op)] & (1 << 6)) != 0;
    }

    inline bool testTMode(OpCode op) {
        return (luaP_opmodes[static_cast<u8>(op)] & (1 << 7)) != 0;
    }

    // 操作码名称数组（用于调试）
    extern const char* const luaP_opnames[NUM_OPCODES + 1];

} // namespace Lua
