/**
 * @file trace_sink.hpp
 * @brief 虚拟机追踪输出接口
 *
 * 定义追踪输出端抽象接口和默认的空输出端。
 * 虚拟机通过此接口输出事件，完全解耦于具体的输出实现。
 */

#pragma once

#include "debug/trace_types.hpp"

namespace Lua {

/**
 * @brief 追踪事件输出接口
 *
 * 虚拟机主循环持有追踪输出端指针，在关键执行点调用对应方法。
 * 实现类决定如何处理事件（写文件、转发、丢弃等）。
 */
class ITraceSink {
public:
    virtual ~ITraceSink() = default;

    /** @brief 指令执行事件 */
    virtual void onInstruction(const TraceEvent& evt) = 0;

    /** @brief 函数调用事件 */
    virtual void onCall(const TraceEvent& evt) = 0;

    /** @brief 函数返回事件 */
    virtual void onReturn(const TraceEvent& evt) = 0;

    /** @brief 运行时错误事件 */
    virtual void onError(const TraceEvent& evt) = 0;

    /** @brief 刷新缓冲区 */
    virtual void flush() = 0;
};

/**
 * @brief 空追踪输出端（默认实现，零开销）
 *
 * 所有方法为空实现，用于未启用追踪时。
 */
class NullTraceSink final : public ITraceSink {
public:
    void onInstruction(const TraceEvent&) override {}
    void onCall(const TraceEvent&) override {}
    void onReturn(const TraceEvent&) override {}
    void onError(const TraceEvent&) override {}
    void flush() override {}
};

} // namespace Lua
