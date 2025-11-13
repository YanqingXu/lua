/**
 * @file opcode.cpp
 * @brief Lua虚拟机指令集实现
 */

#include "compiler/opcode.hpp"

namespace Lua {

// =====================================================================
// 操作码属性表
// =====================================================================

/**
 * @brief 操作码属性编码
 * 
 * 位域分布：
 * - 位0-1：指令格式（OpMode）
 * - 位2-3：C参数类型（OpArgMask）
 * - 位4-5：B参数类型（OpArgMask）
 * - 位6：是否设置A寄存器
 * - 位7：是否是测试指令
 */
static const u8 opModes[NUM_OPCODES] = {
    // 格式化：opmode(T, A, B, C, mode)
    // T=测试指令, A=设置A, B=B参数类型, C=C参数类型, mode=指令格式
    
    #define opmode(t,a,b,c,m) \
        (static_cast<u8>((t)<<7 | (a)<<6 | (static_cast<u8>(b))<<4 | (static_cast<u8>(c))<<2 | (static_cast<u8>(m))))
    
    #define N OpArgMask::OpArgN
    #define U OpArgMask::OpArgU
    #define R OpArgMask::OpArgR
    #define K OpArgMask::OpArgK
    
    opmode(0, 1, R, N, OpMode::iABC),   // OP_MOVE
    opmode(0, 1, K, N, OpMode::iABx),   // OP_LOADK
    opmode(0, 1, U, U, OpMode::iABC),   // OP_LOADBOOL
    opmode(0, 1, R, N, OpMode::iABC),   // OP_LOADNIL
    opmode(0, 1, U, N, OpMode::iABC),   // OP_GETUPVAL
    opmode(0, 1, K, N, OpMode::iABx),   // OP_GETGLOBAL
    opmode(0, 1, R, K, OpMode::iABC),   // OP_GETTABLE
    opmode(0, 0, K, N, OpMode::iABx),   // OP_SETGLOBAL
    opmode(0, 0, U, N, OpMode::iABC),   // OP_SETUPVAL
    opmode(0, 0, K, K, OpMode::iABC),   // OP_SETTABLE
    opmode(0, 1, U, U, OpMode::iABC),   // OP_NEWTABLE
    opmode(0, 1, R, K, OpMode::iABC),   // OP_SELF
    opmode(0, 1, K, K, OpMode::iABC),   // OP_ADD
    opmode(0, 1, K, K, OpMode::iABC),   // OP_SUB
    opmode(0, 1, K, K, OpMode::iABC),   // OP_MUL
    opmode(0, 1, K, K, OpMode::iABC),   // OP_DIV
    opmode(0, 1, K, K, OpMode::iABC),   // OP_MOD
    opmode(0, 1, K, K, OpMode::iABC),   // OP_POW
    opmode(0, 1, R, N, OpMode::iABC),   // OP_UNM
    opmode(0, 1, R, N, OpMode::iABC),   // OP_NOT
    opmode(0, 1, R, N, OpMode::iABC),   // OP_LEN
    opmode(0, 1, R, R, OpMode::iABC),   // OP_CONCAT
    opmode(0, 0, R, N, OpMode::iAsBx),  // OP_JMP
    opmode(1, 0, K, K, OpMode::iABC),   // OP_EQ
    opmode(1, 0, K, K, OpMode::iABC),   // OP_LT
    opmode(1, 0, K, K, OpMode::iABC),   // OP_LE
    opmode(1, 1, R, U, OpMode::iABC),   // OP_TEST
    opmode(1, 1, R, U, OpMode::iABC),   // OP_TESTSET
    opmode(0, 1, U, U, OpMode::iABC),   // OP_CALL
    opmode(0, 1, U, U, OpMode::iABC),   // OP_TAILCALL
    opmode(0, 0, U, N, OpMode::iABC),   // OP_RETURN
    opmode(0, 1, R, N, OpMode::iAsBx),  // OP_FORLOOP
    opmode(0, 1, R, N, OpMode::iAsBx),  // OP_FORPREP
    opmode(1, 0, N, U, OpMode::iABC),   // OP_TFORLOOP
    opmode(0, 0, U, U, OpMode::iABC),   // OP_SETLIST
    opmode(0, 0, N, N, OpMode::iABC),   // OP_CLOSE
    opmode(0, 1, U, N, OpMode::iABx),   // OP_CLOSURE
    opmode(0, 1, U, N, OpMode::iABC),   // OP_VARARG
    
    #undef opmode
    #undef N
    #undef U
    #undef R
    #undef K
};

// =====================================================================
// 操作码名称表
// =====================================================================

static const char* opNames[NUM_OPCODES] = {
    "MOVE", "LOADK", "LOADBOOL", "LOADNIL",
    "GETUPVAL", "GETGLOBAL", "GETTABLE",
    "SETGLOBAL", "SETUPVAL", "SETTABLE",
    "NEWTABLE", "SELF",
    "ADD", "SUB", "MUL", "DIV", "MOD", "POW",
    "UNM", "NOT", "LEN",
    "CONCAT",
    "JMP",
    "EQ", "LT", "LE",
    "TEST", "TESTSET",
    "CALL", "TAILCALL", "RETURN",
    "FORLOOP", "FORPREP", "TFORLOOP",
    "SETLIST",
    "CLOSE",
    "CLOSURE",
    "VARARG"
};

// =====================================================================
// 属性访问函数实现
// =====================================================================

OpMode getOpMode(OpCode op) {
    return static_cast<OpMode>(opModes[static_cast<i32>(op)] & 3);
}

OpArgMask getBMode(OpCode op) {
    return static_cast<OpArgMask>((opModes[static_cast<i32>(op)] >> 4) & 3);
}

OpArgMask getCMode(OpCode op) {
    return static_cast<OpArgMask>((opModes[static_cast<i32>(op)] >> 2) & 3);
}

bool testAMode(OpCode op) {
    return (opModes[static_cast<i32>(op)] & (1 << 6)) != 0;
}

bool testTMode(OpCode op) {
    return (opModes[static_cast<i32>(op)] & (1 << 7)) != 0;
}

const char* getOpName(OpCode op) {
    i32 idx = static_cast<i32>(op);
    if (idx >= 0 && idx < NUM_OPCODES) {
        return opNames[idx];
    }
    return "UNKNOWN";
}

}  // namespace Lua

