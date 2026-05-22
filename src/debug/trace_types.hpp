/**
 * @file trace_types.hpp
 * @brief VM Trace 事件类型定义
 *
 * 定义执行追踪系统使用的事件类型枚举和事件结构体。
 * 所有事件使用扁平 struct + kind 标签，避免继承开销。
 */

#pragma once

#include "common/types.hpp"
#include "compiler/opcode.hpp"

namespace Lua {

// 前向声明
class Proto;
class Value;

/**
 * @brief Trace 事件类型
 */
enum class TraceEventKind : u8 {
    Instruction,    ///< 指令执行
    Call,           ///< 函数调用
    Return,         ///< 函数返回
    Error           ///< 运行时错误
};

/**
 * @brief One register slot changed by an instruction.
 *
 * oldValue/newValue are already serialized as JSON value fragments so sinks can
 * write them without re-reading VM stack state after the event crosses layers.
 */
struct TraceRegisterChange {
    i32         slot       = 0;
    bool        hasName    = false;
    Str         name;
    Str         oldValue;
    Str         newValue;
    const char* oldType    = "nil";
    const char* newType    = "nil";
};

/**
 * @brief Trace 事件结构体
 *
 * 扁平结构，根据 kind 字段决定哪些字段有效。
 * - Instruction: 全部字段有效
 * - Call:        seq, kind, source, line, callDepth, funcName
 * - Return:      seq, kind, callDepth
 * - Error:       seq, kind, source, line, callDepth, errorMsg
 */
struct TraceEvent {
    u64             seq       = 0;          ///< 全局递增序号
    TraceEventKind  kind      = TraceEventKind::Instruction;

    // ---- 指令信息 ----
    i32             pc        = 0;          ///< 程序计数器（0-based）
    OpCode          op        = OpCode::MOVE;
    i32             a         = 0;
    i32             b         = 0;
    i32             c         = 0;
    i32             bx        = 0;
    i32             sbx       = 0;

    // ---- 位置信息 ----
    i32             line      = 0;          ///< 源码行号
    const char*     source    = "?";        ///< 源文件名（借用指针，不拥有）

    // ---- 调用信息 ----
    i32             callDepth = 0;          ///< 当前调用深度
    const char*     funcName  = nullptr;    ///< 函数名（Call 事件用）

    // ---- 寄存器快照上下文（由 sink 读取，不拥有内存）----
    Value*          base      = nullptr;    ///< 寄存器基地址
    i32             maxStack  = 0;          ///< 栈帧大小
    Proto*          proto     = nullptr;    ///< 当前函数原型（用于获取局部变量名）

    // ---- 差异模式（--trace-diff）----
    bool            includeChangedRegisters = false;
    Vec<TraceRegisterChange> changedRegisters;

    // ---- 错误信息 ----
    const char*     errorMsg  = nullptr;    ///< Error 事件的错误消息
};

} // namespace Lua
