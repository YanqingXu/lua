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
    iABC, // 三操作数格式
    iABx, // 两操作数格式（大索引）
    iAsBx // 两操作数格式（有符号偏移）
};

// =====================================================================
// 指令布局常量
// =====================================================================

// 各字段的位数
constexpr i32 SIZE_C = 9;
constexpr i32 SIZE_B = 9;
constexpr i32 SIZE_Bx = SIZE_C + SIZE_B; // 18
constexpr i32 SIZE_A = 8;
constexpr i32 SIZE_OP = 6;

// 各字段在指令中的位置
constexpr i32 POS_OP = 0;
constexpr i32 POS_A = POS_OP + SIZE_OP; // 6
constexpr i32 POS_C = POS_A + SIZE_A;   // 14
constexpr i32 POS_B = POS_C + SIZE_C;   // 23
constexpr i32 POS_Bx = POS_C;           // 14

// 参数范围限制
constexpr i32 MAXARG_Bx = (1 << SIZE_Bx) - 1; // 262143
constexpr i32 MAXARG_sBx = MAXARG_Bx >> 1;    // 131071
constexpr i32 MAXARG_A = (1 << SIZE_A) - 1;   // 255
constexpr i32 MAXARG_B = (1 << SIZE_B) - 1;   // 511
constexpr i32 MAXARG_C = (1 << SIZE_C) - 1;   // 511

// RK寻址常量
constexpr i32 BITRK = 1 << (SIZE_B - 1); // 256
constexpr i32 MAXINDEXRK = BITRK - 1;    // 255

// 特殊寄存器
constexpr i32 NO_REG = MAXARG_A; // 255

// =====================================================================
// 操作码枚举
// =====================================================================

/**
 * @brief Lua 5.1虚拟机操作码（38个指令）
 */
enum class OpCode : u8 {
    // 数据移动指令
    /** @brief 将寄存器 R(B) 复制到 R(A)。 */
    MOVE,
    /** @brief 将常量 K(Bx) 加载到 R(A)。 */
    LOADK,
    /** @brief 将 B 转为布尔值写入 R(A)；C 非零时程序计数器递增。 */
    LOADBOOL,
    /** @brief 将 R(A) 到 R(B) 的寄存器设为 nil。 */
    LOADNIL,

    // 变量访问指令
    /** @brief 将上值 B 读取到 R(A)。 */
    GETUPVAL,
    /** @brief 将以 K(Bx) 为键的全局变量读取到 R(A)。 */
    GETGLOBAL,
    /** @brief 将 R(B)[RK(C)] 读取到 R(A)。 */
    GETTABLE,

    // 变量赋值指令
    /** @brief 将 R(A) 写入以 K(Bx) 为键的全局变量。 */
    SETGLOBAL,
    /** @brief 将 R(A) 写入上值 B。 */
    SETUPVAL,
    /** @brief 将 RK(C) 写入 R(A)[RK(B)]。 */
    SETTABLE,

    // 表操作指令
    /** @brief 创建预估数组和哈希大小为 B、C 的表并写入 R(A)。 */
    NEWTABLE,
    /** @brief 为方法调用保存接收者并读取方法：R(A+1) := R(B)，R(A) := R(B)[RK(C)]。 */
    SELF,

    // 算术运算指令
    /** @brief 计算 RK(B) + RK(C) 并写入 R(A)。 */
    ADD,
    /** @brief 计算 RK(B) - RK(C) 并写入 R(A)。 */
    SUB,
    /** @brief 计算 RK(B) * RK(C) 并写入 R(A)。 */
    MUL,
    /** @brief 计算 RK(B) / RK(C) 并写入 R(A)。 */
    DIV,
    /** @brief 计算 RK(B) % RK(C) 并写入 R(A)。 */
    MOD,
    /** @brief 计算 RK(B) 的 RK(C) 次幂并写入 R(A)。 */
    POW,
    /** @brief 对 R(B) 取负并写入 R(A)。 */
    UNM,
    /** @brief 对 R(B) 执行逻辑非并写入 R(A)。 */
    NOT,
    /** @brief 计算 R(B) 的长度并写入 R(A)。 */
    LEN,

    // 字符串操作指令
    /** @brief 拼接 R(B) 到 R(C) 并写入 R(A)。 */
    CONCAT,

    // 控制流指令
    /** @brief 将程序计数器增加 sBx。 */
    JMP,
    /** @brief 当“RK(B) == RK(C)”与 A 不一致时跳过下一条指令。 */
    EQ,
    /** @brief 当“RK(B) < RK(C)”与 A 不一致时跳过下一条指令。 */
    LT,
    /** @brief 当“RK(B) <= RK(C)”与 A 不一致时跳过下一条指令。 */
    LE,
    /** @brief 当 R(A) 的真值性与 C 不一致时跳过下一条指令。 */
    TEST,
    /** @brief R(B) 的真值性与 C 一致时写入 R(A)，否则跳过下一条指令。 */
    TESTSET,

    // 函数调用指令
    /** @brief 以 R(A+1) 到 R(A+B-1) 为参数调用 R(A)，并保存 C-1 个结果。 */
    CALL,
    /** @brief 以 R(A+1) 到 R(A+B-1) 为参数尾调用 R(A)。 */
    TAILCALL,
    /** @brief 返回 R(A) 到 R(A+B-2)。 */
    RETURN,

    // 循环控制指令
    /** @brief 推进数值 for 循环索引，未越界时跳转并更新外部索引。 */
    FORLOOP,
    /** @brief 预调整数值 for 循环索引并跳转到循环检查。 */
    FORPREP,
    /** @brief 调用泛型 for 迭代器，结果非 nil 时更新控制变量，否则跳过下一条指令。 */
    TFORLOOP,

    // 表初始化指令
    /** @brief 将 R(A+1) 到 R(A+B) 批量写入表 R(A) 的指定区间。 */
    SETLIST,

    // 栈管理指令
    /** @brief 关闭栈中位置不低于 R(A) 的全部开放变量。 */
    CLOSE,

    // 闭包创建指令
    /** @brief 根据子函数原型 KPROTO[Bx] 与后续上值绑定创建闭包并写入 R(A)。 */
    CLOSURE,

    // 可变参数指令
    /** @brief 将可变参数结果写入从 R(A) 开始的寄存器区间。 */
    VARARG
};

constexpr i32 NUM_OPCODES = static_cast<i32>(OpCode::VARARG) + 1; // 38

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
    return (static_cast<Instruction>(o) << POS_OP) | (static_cast<Instruction>(a) << POS_A) |
           (static_cast<Instruction>(b) << POS_B) | (static_cast<Instruction>(c) << POS_C);
}

/**
 * @brief 创建ABx格式指令
 */
inline Instruction CREATE_ABx(OpCode o, i32 a, i32 bx) {
    return (static_cast<Instruction>(o) << POS_OP) | (static_cast<Instruction>(a) << POS_A) |
           (static_cast<Instruction>(bx) << POS_Bx);
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
    i = (i & ~MASK1(SIZE_A, POS_A)) | ((static_cast<Instruction>(a) << POS_A) & MASK1(SIZE_A, POS_A));
}

/**
 * @brief 设置指令的B参数
 */
inline void SETARG_B(Instruction& i, i32 b) {
    i = (i & ~MASK1(SIZE_B, POS_B)) | ((static_cast<Instruction>(b) << POS_B) & MASK1(SIZE_B, POS_B));
}

/**
 * @brief 设置指令的C参数
 */
inline void SETARG_C(Instruction& i, i32 c) {
    i = (i & ~MASK1(SIZE_C, POS_C)) | ((static_cast<Instruction>(c) << POS_C) & MASK1(SIZE_C, POS_C));
}

/**
 * @brief 设置指令的sBx参数
 */
inline void SETARG_sBx(Instruction& i, i32 sbx) {
    i = (i & ~MASK1(SIZE_Bx, POS_Bx)) |
        ((static_cast<Instruction>(sbx + MAXARG_sBx) << POS_Bx) & MASK1(SIZE_Bx, POS_Bx));
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
    OpArgN, // 参数未使用
    OpArgU, // 参数被使用
    OpArgR, // 寄存器或跳转偏移
    OpArgK  // 常量或寄存器/常量（RK）
};

namespace VM {

/** @brief 操作码的语义分组。 */
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

} // namespace VM

/** @brief 单个操作码的名称、格式与语义元数据。 */
struct OpcodeMetadata {
    OpCode opcode;
    StrView name;
    OpMode mode;
    OpArgMask bMode;
    OpArgMask cMode;
    bool setsA;
    bool isTest;
    VM::OpcodeGroup group;
    bool mayInvokeMetamethod;
};

namespace detail {

constexpr OpcodeMetadata makeOpcodeMetadata(OpCode opcode, StrView name, OpMode mode, OpArgMask bMode, OpArgMask cMode,
                                            bool setsA, bool isTest, VM::OpcodeGroup group,
                                            bool mayInvokeMetamethod) noexcept {
    return OpcodeMetadata{opcode, name, mode, bMode, cMode, setsA, isTest, group, mayInvokeMetamethod};
}

} // namespace detail

inline constexpr std::array<OpcodeMetadata, static_cast<usize>(NUM_OPCODES)> kOpcodeMetadata = {{
    detail::makeOpcodeMetadata(OpCode::MOVE, "MOVE", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgN, true, false,
                               VM::OpcodeGroup::DataMove, false),
    detail::makeOpcodeMetadata(OpCode::LOADK, "LOADK", OpMode::iABx, OpArgMask::OpArgK, OpArgMask::OpArgN, true, false,
                               VM::OpcodeGroup::DataMove, false),
    detail::makeOpcodeMetadata(OpCode::LOADBOOL, "LOADBOOL", OpMode::iABC, OpArgMask::OpArgU, OpArgMask::OpArgU, true,
                               false, VM::OpcodeGroup::DataMove, false),
    detail::makeOpcodeMetadata(OpCode::LOADNIL, "LOADNIL", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgN, true,
                               false, VM::OpcodeGroup::DataMove, false),
    detail::makeOpcodeMetadata(OpCode::GETUPVAL, "GETUPVAL", OpMode::iABC, OpArgMask::OpArgU, OpArgMask::OpArgN, true,
                               false, VM::OpcodeGroup::Upvalue, false),
    detail::makeOpcodeMetadata(OpCode::GETGLOBAL, "GETGLOBAL", OpMode::iABx, OpArgMask::OpArgK, OpArgMask::OpArgN, true,
                               false, VM::OpcodeGroup::Global, false),
    detail::makeOpcodeMetadata(OpCode::GETTABLE, "GETTABLE", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgK, true,
                               false, VM::OpcodeGroup::Table, true),
    detail::makeOpcodeMetadata(OpCode::SETGLOBAL, "SETGLOBAL", OpMode::iABx, OpArgMask::OpArgK, OpArgMask::OpArgN,
                               false, false, VM::OpcodeGroup::Global, false),
    detail::makeOpcodeMetadata(OpCode::SETUPVAL, "SETUPVAL", OpMode::iABC, OpArgMask::OpArgU, OpArgMask::OpArgN, false,
                               false, VM::OpcodeGroup::Upvalue, false),
    detail::makeOpcodeMetadata(OpCode::SETTABLE, "SETTABLE", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK, false,
                               false, VM::OpcodeGroup::Table, true),
    detail::makeOpcodeMetadata(OpCode::NEWTABLE, "NEWTABLE", OpMode::iABC, OpArgMask::OpArgU, OpArgMask::OpArgU, true,
                               false, VM::OpcodeGroup::Table, false),
    detail::makeOpcodeMetadata(OpCode::SELF, "SELF", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgK, true, false,
                               VM::OpcodeGroup::Table, true),
    detail::makeOpcodeMetadata(OpCode::ADD, "ADD", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK, true, false,
                               VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::SUB, "SUB", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK, true, false,
                               VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::MUL, "MUL", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK, true, false,
                               VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::DIV, "DIV", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK, true, false,
                               VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::MOD, "MOD", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK, true, false,
                               VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::POW, "POW", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK, true, false,
                               VM::OpcodeGroup::Arithmetic, true),
    detail::makeOpcodeMetadata(OpCode::UNM, "UNM", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgN, true, false,
                               VM::OpcodeGroup::Unary, true),
    detail::makeOpcodeMetadata(OpCode::NOT, "NOT", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgN, true, false,
                               VM::OpcodeGroup::Unary, false),
    detail::makeOpcodeMetadata(OpCode::LEN, "LEN", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgN, true, false,
                               VM::OpcodeGroup::Unary, true),
    detail::makeOpcodeMetadata(OpCode::CONCAT, "CONCAT", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgR, true,
                               false, VM::OpcodeGroup::Unary, true),
    detail::makeOpcodeMetadata(OpCode::JMP, "JMP", OpMode::iAsBx, OpArgMask::OpArgR, OpArgMask::OpArgN, false, false,
                               VM::OpcodeGroup::Branch, false),
    detail::makeOpcodeMetadata(OpCode::EQ, "EQ", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK, false, true,
                               VM::OpcodeGroup::Comparison, true),
    detail::makeOpcodeMetadata(OpCode::LT, "LT", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK, false, true,
                               VM::OpcodeGroup::Comparison, true),
    detail::makeOpcodeMetadata(OpCode::LE, "LE", OpMode::iABC, OpArgMask::OpArgK, OpArgMask::OpArgK, false, true,
                               VM::OpcodeGroup::Comparison, true),
    detail::makeOpcodeMetadata(OpCode::TEST, "TEST", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgU, true, true,
                               VM::OpcodeGroup::Branch, false),
    detail::makeOpcodeMetadata(OpCode::TESTSET, "TESTSET", OpMode::iABC, OpArgMask::OpArgR, OpArgMask::OpArgU, true,
                               true, VM::OpcodeGroup::Branch, false),
    detail::makeOpcodeMetadata(OpCode::CALL, "CALL", OpMode::iABC, OpArgMask::OpArgU, OpArgMask::OpArgU, true, false,
                               VM::OpcodeGroup::Call, true),
    detail::makeOpcodeMetadata(OpCode::TAILCALL, "TAILCALL", OpMode::iABC, OpArgMask::OpArgU, OpArgMask::OpArgU, true,
                               false, VM::OpcodeGroup::Call, true),
    detail::makeOpcodeMetadata(OpCode::RETURN, "RETURN", OpMode::iABC, OpArgMask::OpArgU, OpArgMask::OpArgN, false,
                               false, VM::OpcodeGroup::Call, false),
    detail::makeOpcodeMetadata(OpCode::FORLOOP, "FORLOOP", OpMode::iAsBx, OpArgMask::OpArgR, OpArgMask::OpArgN, true,
                               false, VM::OpcodeGroup::Loop, false),
    detail::makeOpcodeMetadata(OpCode::FORPREP, "FORPREP", OpMode::iAsBx, OpArgMask::OpArgR, OpArgMask::OpArgN, true,
                               false, VM::OpcodeGroup::Loop, false),
    detail::makeOpcodeMetadata(OpCode::TFORLOOP, "TFORLOOP", OpMode::iABC, OpArgMask::OpArgN, OpArgMask::OpArgU, false,
                               true, VM::OpcodeGroup::Loop, false),
    detail::makeOpcodeMetadata(OpCode::SETLIST, "SETLIST", OpMode::iABC, OpArgMask::OpArgU, OpArgMask::OpArgU, false,
                               false, VM::OpcodeGroup::Table, false),
    detail::makeOpcodeMetadata(OpCode::CLOSE, "CLOSE", OpMode::iABC, OpArgMask::OpArgN, OpArgMask::OpArgN, false, false,
                               VM::OpcodeGroup::Branch, false),
    detail::makeOpcodeMetadata(OpCode::CLOSURE, "CLOSURE", OpMode::iABx, OpArgMask::OpArgU, OpArgMask::OpArgN, true,
                               false, VM::OpcodeGroup::Closure, false),
    detail::makeOpcodeMetadata(OpCode::VARARG, "VARARG", OpMode::iABC, OpArgMask::OpArgU, OpArgMask::OpArgN, true,
                               false, VM::OpcodeGroup::Vararg, false),
}};

inline constexpr OpcodeMetadata kUnknownOpcodeMetadata = {
    static_cast<OpCode>(0),   "UNKNOWN", OpMode::iABC, OpArgMask::OpArgN, OpArgMask::OpArgN, false, false,
    VM::OpcodeGroup::Unknown, false,
};

static_assert(kOpcodeMetadata.size() == static_cast<usize>(NUM_OPCODES), "opcode metadata must cover every opcode");

namespace detail {

consteval bool opcodeMetadataMatchesEnumOrder() {
    for (usize index = 0; index < kOpcodeMetadata.size(); ++index) {
        const auto expected = static_cast<OpCode>(index);
        if (kOpcodeMetadata[index].opcode != expected) {
            return false;
        }
        if (kOpcodeMetadata[index].name.empty()) {
            return false;
        }
        if (kOpcodeMetadata[index].group == VM::OpcodeGroup::Unknown) {
            return false;
        }
    }
    return true;
}

} // namespace detail

static_assert(detail::opcodeMetadataMatchesEnumOrder(),
              "kOpcodeMetadata must stay in exact OpCode enum order and contain complete teaching metadata");

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

} // namespace Lua
