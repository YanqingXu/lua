/**
 * @file trace_types.hpp
 * @brief 虚拟机追踪事件类型定义
 *
 * 定义执行追踪系统使用的事件类型枚举和事件结构体。
 * 所有事件使用扁平结构体和类型标签，避免继承开销。
 */

#pragma once

#include "common/types.hpp"
#include "compiler/opcode.hpp"

namespace Lua {

// 前向声明
class Proto;
class Value;

/**
 * @brief 追踪事件类型
 */
enum class TraceEventKind : u8 {
    /** @brief 指令执行 */
    Instruction,
    /** @brief 函数调用 */
    Call,
    /** @brief 函数返回 */
    Return,
    /** @brief 运行时错误 */
    Error
};

/**
 * @brief 一条指令对单个寄存器槽产生的变更
 *
 * 旧值与新值已序列化为 JSON 值片段，因此事件跨层传递后，输出端无需重新读取虚拟机
 * 栈状态即可写出它们。
 */
struct TraceRegisterChange {
    i32 slot = 0;
    bool hasName = false;
    Str name;
    Str oldValue;
    Str newValue;
    const char* oldType = "nil";
    const char* newType = "nil";
};

/**
 * @brief 追踪事件结构体
 *
 * 扁平结构，根据事件类型字段决定哪些字段有效。
 * - 指令事件：序号、类型、程序计数器、操作码、操作数、来源、行号、调用深度和函数名
 * - 调用事件：序号、类型、来源、行号、调用深度和函数名
 * - 返回事件：序号、类型、来源、行号、调用深度和函数名
 * - 错误事件：序号、类型、来源、行号、调用深度、函数名和错误消息
 */
struct TraceEvent {
    /** @brief 全局递增序号 */
    u64 seq = 0;
    TraceEventKind kind = TraceEventKind::Instruction;

    // ---- 指令信息 ----
    /** @brief 程序计数器（0-based） */
    i32 pc = 0;
    OpCode op = OpCode::MOVE;
    i32 a = 0;
    i32 b = 0;
    i32 c = 0;
    i32 bx = 0;
    i32 sbx = 0;

    // ---- 位置信息 ----
    /** @brief 源码行号 */
    i32 line = 0;
    /** @brief 源文件名（借用指针，不拥有） */
    const char* source = "?";

    // ---- 调用信息 ----
    /** @brief 当前调用深度 */
    i32 callDepth = 0;
    /** @brief 可读函数名或 source:line 标签 */
    Str funcName = "?";

    // ---- 寄存器快照上下文（由输出端读取，不拥有内存）----
    /** @brief 寄存器基地址 */
    Value* base = nullptr;
    /** @brief 栈帧大小 */
    i32 maxStack = 0;
    /** @brief 当前函数原型（用于获取局部变量名） */
    Proto* proto = nullptr;

    // ---- 追踪差异模式 ----
    bool includeChangedRegisters = false;
    Vec<TraceRegisterChange> changedRegisters;

    // ---- 错误信息 ----
    /** @brief 错误事件的错误消息。 */
    const char* errorMsg = nullptr;
};

} // namespace Lua
