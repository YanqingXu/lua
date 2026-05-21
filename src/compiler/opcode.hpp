#pragma once

/**
 * @file opcode.hpp
 * @brief Lua虚拟机指令集定义（C++版本）
 * 
 * 实现Lua 5.1虚拟机的完整指令集，包括操作码枚举、指令格式和编解码宏。
 * 
 * 核心功能：
 * - 定义38个Lua 5.1操作码
 * - 支持三种指令格式（iABC、iABx、iAsBx）
 * - 提供指令编码/解码工具函数
 * - 支持RK混合寻址模式
 * 
 * 指令格式（32位）：
 * - iABC:  [OP:6][A:8][C:9][B:9]
 * - iABx:  [OP:6][A:8][Bx:18]
 * - iAsBx: [OP:6][A:8][sBx:18] (有符号)
 * 
 * 参考实现：
 * - lua_c_analysis/src/lopcodes.h - Lua 5.1.5 C版本
 */

#include "common/types.hpp"

#include <array>

namespace Lua {

// =====================================================================
// 指令类型定义
// =====================================================================

/**
 * @brief 指令类型（32位无符号整数）
 */
using Instruction = u32;

/**
 * @brief 指令格式枚举
 */
enum class OpMode {
    iABC,   // 三操作数格式
    iABx,   // 两操作数格式（大索引）
    iAsBx   // 两操作数格式（有符号偏移）
};

// =====================================================================
// 指令布局常量
// =====================================================================

// 各字段的位数
constexpr i32 SIZE_C  = 9;
constexpr i32 SIZE_B  = 9;
constexpr i32 SIZE_Bx = SIZE_C + SIZE_B;  // 18
constexpr i32 SIZE_A  = 8;
constexpr i32 SIZE_OP = 6;

// 各字段在指令中的位置
constexpr i32 POS_OP = 0;
constexpr i32 POS_A  = POS_OP + SIZE_OP;  // 6
constexpr i32 POS_C  = POS_A + SIZE_A;    // 14
constexpr i32 POS_B  = POS_C + SIZE_C;    // 23
constexpr i32 POS_Bx = POS_C;             // 14

// 参数范围限制
constexpr i32 MAXARG_Bx  = (1 << SIZE_Bx) - 1;  // 262143
constexpr i32 MAXARG_sBx = MAXARG_Bx >> 1;      // 131071
constexpr i32 MAXARG_A   = (1 << SIZE_A) - 1;   // 255
constexpr i32 MAXARG_B   = (1 << SIZE_B) - 1;   // 511
constexpr i32 MAXARG_C   = (1 << SIZE_C) - 1;   // 511

// RK寻址常量
constexpr i32 BITRK      = 1 << (SIZE_B - 1);   // 256
constexpr i32 MAXINDEXRK = BITRK - 1;           // 255

// 特殊寄存器
constexpr i32 NO_REG = MAXARG_A;  // 255

// =====================================================================
// 操作码枚举
// =====================================================================

/**
 * @brief Lua 5.1虚拟机操作码（38个指令）
 */
enum class OpCode : u8 {
    // 数据移动指令
    MOVE,       // R(A) := R(B)
    LOADK,      // R(A) := K(Bx)
    LOADBOOL,   // R(A) := (Bool)B; if (C) pc++
    LOADNIL,    // R(A) := ... := R(B) := nil
    
    // 变量访问指令
    GETUPVAL,   // R(A) := UpValue[B]
    GETGLOBAL,  // R(A) := Gbl[K(Bx)]
    GETTABLE,   // R(A) := R(B)[RK(C)]
    
    // 变量赋值指令
    SETGLOBAL,  // Gbl[K(Bx)] := R(A)
    SETUPVAL,   // UpValue[B] := R(A)
    SETTABLE,   // R(A)[RK(B)] := RK(C)
    
    // 表操作指令
    NEWTABLE,   // R(A) := {} (size = B,C)
    SELF,       // R(A+1) := R(B); R(A) := R(B)[RK(C)]
    
    // 算术运算指令
    ADD,        // R(A) := RK(B) + RK(C)
    SUB,        // R(A) := RK(B) - RK(C)
    MUL,        // R(A) := RK(B) * RK(C)
    DIV,        // R(A) := RK(B) / RK(C)
    MOD,        // R(A) := RK(B) % RK(C)
    POW,        // R(A) := RK(B) ^ RK(C)
    UNM,        // R(A) := -R(B)
    NOT,        // R(A) := not R(B)
    LEN,        // R(A) := length of R(B)
    
    // 字符串操作指令
    CONCAT,     // R(A) := R(B).. ... ..R(C)
    
    // 控制流指令
    JMP,        // pc += sBx
    EQ,         // if ((RK(B) == RK(C)) ~= A) then pc++
    LT,         // if ((RK(B) <  RK(C)) ~= A) then pc++
    LE,         // if ((RK(B) <= RK(C)) ~= A) then pc++
    TEST,       // if not (R(A) <=> C) then pc++
    TESTSET,    // if (R(B) <=> C) then R(A) := R(B) else pc++
    
    // 函数调用指令
    CALL,       // R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
    TAILCALL,   // return R(A)(R(A+1), ... ,R(A+B-1))
    RETURN,     // return R(A), ... ,R(A+B-2)
    
    // 循环控制指令
    FORLOOP,    // R(A)+=R(A+2); if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A) }
    FORPREP,    // R(A)-=R(A+2); pc+=sBx
    TFORLOOP,   // R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2)); if R(A+3) ~= nil then R(A+2)=R(A+3) else pc++
    
    // 表初始化指令
    SETLIST,    // R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B
    
    // 栈管理指令
    CLOSE,      // close all variables in the stack up to (>=) R(A)
    
    // 闭包创建指令
    CLOSURE,    // R(A) := closure(KPROTO[Bx], R(A), ... ,R(A+n))
    
    // 可变参数指令
    VARARG      // R(A), R(A+1), ..., R(A+B-1) = vararg
};

constexpr i32 NUM_OPCODES = static_cast<i32>(OpCode::VARARG) + 1;  // 38

// =====================================================================
// 指令编解码函数
// =====================================================================

/**
 * @brief 创建位掩码
 */
constexpr Instruction MASK1(i32 n, i32 p) {
    return ((~((~static_cast<Instruction>(0)) << n)) << p);
}

/**
 * @brief 从指令中提取操作码
 */
inline OpCode GET_OPCODE(Instruction i) {
    return static_cast<OpCode>((i >> POS_OP) & MASK1(SIZE_OP, 0));
}

/**
 * @brief 从指令中提取A参数
 */
inline i32 GETARG_A(Instruction i) {
    return static_cast<i32>((i >> POS_A) & MASK1(SIZE_A, 0));
}

/**
 * @brief 从指令中提取B参数
 */
inline i32 GETARG_B(Instruction i) {
    return static_cast<i32>((i >> POS_B) & MASK1(SIZE_B, 0));
}

/**
 * @brief 从指令中提取C参数
 */
inline i32 GETARG_C(Instruction i) {
    return static_cast<i32>((i >> POS_C) & MASK1(SIZE_C, 0));
}

/**
 * @brief 从指令中提取Bx参数
 */
inline i32 GETARG_Bx(Instruction i) {
    return static_cast<i32>((i >> POS_Bx) & MASK1(SIZE_Bx, 0));
}

/**
 * @brief 从指令中提取sBx参数（有符号）
 */
inline i32 GETARG_sBx(Instruction i) {
    return GETARG_Bx(i) - MAXARG_sBx;
}

/**
 * @brief 创建ABC格式指令
 */
inline Instruction CREATE_ABC(OpCode o, i32 a, i32 b, i32 c) {
    return (static_cast<Instruction>(o) << POS_OP)
         | (static_cast<Instruction>(a) << POS_A)
         | (static_cast<Instruction>(b) << POS_B)
         | (static_cast<Instruction>(c) << POS_C);
}

/**
 * @brief 创建ABx格式指令
 */
inline Instruction CREATE_ABx(OpCode o, i32 a, i32 bx) {
    return (static_cast<Instruction>(o) << POS_OP)
         | (static_cast<Instruction>(a) << POS_A)
         | (static_cast<Instruction>(bx) << POS_Bx);
}

/**
 * @brief 创建AsBx格式指令
 */
inline Instruction CREATE_AsBx(OpCode o, i32 a, i32 sbx) {
    return CREATE_ABx(o, a, sbx + MAXARG_sBx);
}

/**
 * @brief 设置指令的A参数
 */
inline void SETARG_A(Instruction& i, i32 a) {
    i = (i & ~MASK1(SIZE_A, POS_A))
      | ((static_cast<Instruction>(a) << POS_A) & MASK1(SIZE_A, POS_A));
}

/**
 * @brief 设置指令的B参数
 */
inline void SETARG_B(Instruction& i, i32 b) {
    i = (i & ~MASK1(SIZE_B, POS_B))
      | ((static_cast<Instruction>(b) << POS_B) & MASK1(SIZE_B, POS_B));
}

/**
 * @brief 设置指令的C参数
 */
inline void SETARG_C(Instruction& i, i32 c) {
    i = (i & ~MASK1(SIZE_C, POS_C))
      | ((static_cast<Instruction>(c) << POS_C) & MASK1(SIZE_C, POS_C));
}

/**
 * @brief 设置指令的sBx参数
 */
inline void SETARG_sBx(Instruction& i, i32 sbx) {
    i = (i & ~MASK1(SIZE_Bx, POS_Bx))
      | ((static_cast<Instruction>(sbx + MAXARG_sBx) << POS_Bx) & MASK1(SIZE_Bx, POS_Bx));
}

// =====================================================================
// RK寻址辅助函数
// =====================================================================

/**
 * @brief 判断操作数是否为常量
 */
inline bool ISK(i32 x) {
    return (x & BITRK) != 0;
}

/**
 * @brief 从RK操作数中提取常量索引
 */
inline i32 INDEXK(i32 r) {
    return r & ~BITRK;
}

/**
 * @brief 将常量索引编码为RK操作数
 */
inline i32 RKASK(i32 x) {
    return x | BITRK;
}

// =====================================================================
// 操作码属性
// =====================================================================

/**
 * @brief 操作数类型枚举
 */
enum class OpArgMask {
    OpArgN,  // 参数未使用
    OpArgU,  // 参数被使用
    OpArgR,  // 寄存器或跳转偏移
    OpArgK   // 常量或寄存器/常量（RK）
};

namespace VM {

enum class OpcodeGroup : u8 {
    Unknown,
    DataMove,
    Global,
    Upvalue,
    Table,
    Arithmetic,
    Unary,
    Branch,
    Comparison,
    Call,
    Loop,
    Closure,
    Vararg,
};

}  // namespace VM

struct OpcodeMetadata {
    OpCode opcode;
    const char* name;
    OpMode mode;
    OpArgMask bMode;
    OpArgMask cMode;
    bool setsA;
    bool isTest;
    VM::OpcodeGroup group;
    bool mayInvokeMetamethod;
};

namespace detail {

constexpr OpcodeMetadata makeOpcodeMetadata(OpCode opcode, const char* name, OpMode mode,
                                            OpArgMask bMode, OpArgMask cMode, bool setsA,
                                            bool isTest, VM::OpcodeGroup group,
                                            bool mayInvokeMetamethod) noexcept {
    return OpcodeMetadata{opcode, name, mode, bMode, cMode, setsA, isTest, group, mayInvokeMetamethod};
}

}  // namespace detail

inline constexpr std::array<OpcodeMetadata, static_cast<usize>(NUM_OPCODES)> kOpcodeMetadata = {{
    detail::makeOpcodeMetadata(OpCode::MOVE, "MOVE", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgN,
                               true, false, VM::OpcodeGroup::DataMove, false),
    detail::makeOpcodeMetadata(OpCode::LOADK, "LOADK", OpMode::iABx, OpArgMask::OpArgK, OpArgMask::OpArgN,
                               true, false, VM::OpcodeGroup::DataMove, false),
    detail::makeOpcodeMetadata(OpCode::LOADBOOL, "LOADBOOL", OpMode::iABC, OpArgMask::OpArgU,
                               OpArgMask::OpArgU, true, false, VM::OpcodeGroup::DataMove, false),
    detail::makeOpcodeMetadata(OpCode::LOADNIL, "LOADNIL", OpMode::iABC, OpArgMask::OpArgR,
                               OpArgMask::OpArgN, true, false, VM::OpcodeGroup::DataMove, false),
    detail::makeOpcodeMetadata(OpCode::GETUPVAL, "GETUPVAL", OpMode::iABC, OpArgMask::OpArgU,
                               OpArgMask::OpArgN, true, false, VM::OpcodeGroup::Upvalue, false),
    detail::makeOpcodeMetadata(OpCode::GETGLOBAL, "GETGLOBAL", OpMode::iABx, OpArgMask::OpArgK,
                               OpArgMask::OpArgN, true, false, VM::OpcodeGroup::Global, false),
    detail::makeOpcodeMetadata(OpCode::GETTABLE, "GETTABLE", OpMode::iABC, OpArgMask::OpArgR,
                               OpArgMask::OpArgK, true, false, VM::OpcodeGroup::Table, true),
    detail::makeOpcodeMetadata(OpCode::SETGLOBAL, "SETGLOBAL", OpMode::iABx, OpArgMask::OpArgK,
                               OpArgMask::OpArgN, false, false, VM::OpcodeGroup::Global, false),
    detail::makeOpcodeMetadata(OpCode::SETUPVAL, "SETUPVAL", OpMode::iABC, OpArgMask::OpArgU,
                               OpArgMask::OpArgN, false, false, VM::OpcodeGroup::Upvalue, false),
    detail::makeOpcodeMetadata(OpCode::SETTABLE, "SETTABLE", OpMode::iABC, OpArgMask::OpArgK,
                               OpArgMask::OpArgK, false, false, VM::OpcodeGroup::Table, true),
    detail::makeOpcodeMetadata(OpCode::NEWTABLE, "NEWTABLE", OpMode::iABC, OpArgMask::OpArgU,
                               OpArgMask::OpArgU, true, false, VM::OpcodeGroup::Table, false),
    detail::makeOpcodeMetadata(OpCode::SELF, "SELF", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgK,
                               true, false, VM::OpcodeGroup::Table, true),
    detail::makeOpcodeMetadata(OpCode::ADD, "ADD", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK,
                               true, false, VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::SUB, "SUB", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK,
                               true, false, VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::MUL, "MUL", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK,
                               true, false, VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::DIV, "DIV", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK,
                               true, false, VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::MOD, "MOD", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK,
                               true, false, VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::POW, "POW", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK,
                               true, false, VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::UNM, "UNM", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgN,
                               true, false, VM::OpcodeGroup::Unary, true),
    detail::makeOpcodeMetadata(OpCode::NOT, "NOT", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgN,
                               true, false, VM::OpcodeGroup::Unary, false),
    detail::makeOpcodeMetadata(OpCode::LEN, "LEN", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgN,
                               true, false, VM::OpcodeGroup::Unary, true),
    detail::makeOpcodeMetadata(OpCode::CONCAT, "CONCAT", OpMode::iABC, OpArgMask::OpArgR,
                               OpArgMask::OpArgR, true, false, VM::OpcodeGroup::Unary, true),
    detail::makeOpcodeMetadata(OpCode::JMP, "JMP", OpMode::iAsBx, OpArgMask::OpArgR, OpArgMask::OpArgN,
                               false, false, VM::OpcodeGroup::Branch, false),
    detail::makeOpcodeMetadata(OpCode::EQ, "EQ", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK,
                               false, true, VM::OpcodeGroup::Comparison, true),
    detail::makeOpcodeMetadata(OpCode::LT, "LT", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK,
                               false, true, VM::OpcodeGroup::Comparison, true),
    detail::makeOpcodeMetadata(OpCode::LE, "LE", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK,
                               false, true, VM::OpcodeGroup::Comparison, true),
    detail::makeOpcodeMetadata(OpCode::TEST, "TEST", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgU,
                               true, true, VM::OpcodeGroup::Branch, false),
    detail::makeOpcodeMetadata(OpCode::TESTSET, "TESTSET", OpMode::iABC, OpArgMask::OpArgR,
                               OpArgMask::OpArgU, true, true, VM::OpcodeGroup::Branch, false),
    detail::makeOpcodeMetadata(OpCode::CALL, "CALL", OpMode::iABC, OpArgMask::OpArgU, OpArgMask::OpArgU,
                               true, false, VM::OpcodeGroup::Call, true),
    detail::makeOpcodeMetadata(OpCode::TAILCALL, "TAILCALL", OpMode::iABC, OpArgMask::OpArgU,
                               OpArgMask::OpArgU, true, false, VM::OpcodeGroup::Call, true),
    detail::makeOpcodeMetadata(OpCode::RETURN, "RETURN", OpMode::iABC, OpArgMask::OpArgU,
                               OpArgMask::OpArgN, false, false, VM::OpcodeGroup::Call, false),
    detail::makeOpcodeMetadata(OpCode::FORLOOP, "FORLOOP", OpMode::iAsBx, OpArgMask::OpArgR,
                               OpArgMask::OpArgN, true, false, VM::OpcodeGroup::Loop, false),
    detail::makeOpcodeMetadata(OpCode::FORPREP, "FORPREP", OpMode::iAsBx, OpArgMask::OpArgR,
                               OpArgMask::OpArgN, true, false, VM::OpcodeGroup::Loop, false),
    detail::makeOpcodeMetadata(OpCode::TFORLOOP, "TFORLOOP", OpMode::iABC, OpArgMask::OpArgN,
                               OpArgMask::OpArgU, false, true, VM::OpcodeGroup::Loop, false),
    detail::makeOpcodeMetadata(OpCode::SETLIST, "SETLIST", OpMode::iABC, OpArgMask::OpArgU,
                               OpArgMask::OpArgU, false, false, VM::OpcodeGroup::Table, false),
    detail::makeOpcodeMetadata(OpCode::CLOSE, "CLOSE", OpMode::iABC, OpArgMask::OpArgN, OpArgMask::OpArgN,
                               false, false, VM::OpcodeGroup::Branch, false),
    detail::makeOpcodeMetadata(OpCode::CLOSURE, "CLOSURE", OpMode::iABx, OpArgMask::OpArgU,
                               OpArgMask::OpArgN, true, false, VM::OpcodeGroup::Closure, false),
    detail::makeOpcodeMetadata(OpCode::VARARG, "VARARG", OpMode::iABC, OpArgMask::OpArgU,
                               OpArgMask::OpArgN, true, false, VM::OpcodeGroup::Vararg, false),
}};

inline constexpr OpcodeMetadata kUnknownOpcodeMetadata = {
    static_cast<OpCode>(0),
    "UNKNOWN",
    OpMode::iABC,
    OpArgMask::OpArgN,
    OpArgMask::OpArgN,
    false,
    false,
    VM::OpcodeGroup::Unknown,
    false,
};

static_assert(kOpcodeMetadata.size() == static_cast<usize>(NUM_OPCODES),
              "opcode metadata must cover every opcode");

constexpr bool isValidOpcode(OpCode op) noexcept {
    return static_cast<usize>(op) < kOpcodeMetadata.size();
}

constexpr const OpcodeMetadata& opcodeMetadata(OpCode op) noexcept {
    return isValidOpcode(op) ? kOpcodeMetadata[static_cast<usize>(op)] : kUnknownOpcodeMetadata;
}

/**
 * @brief 获取指令格式
 */
OpMode getOpMode(OpCode op);

/**
 * @brief 获取B参数类型
 */
OpArgMask getBMode(OpCode op);

/**
 * @brief 获取C参数类型
 */
OpArgMask getCMode(OpCode op);

/**
 * @brief 测试指令是否设置A寄存器
 */
bool testAMode(OpCode op);

/**
 * @brief 测试指令是否是测试指令
 */
bool testTMode(OpCode op);

/**
 * @brief 获取操作码名称
 */
const char* getOpName(OpCode op);

// 表构造器批处理大小
constexpr i32 LFIELDS_PER_FLUSH = 50;

}  // namespace Lua

