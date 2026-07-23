/**
 * @file json_trace_sink.hpp
 * @brief JSONL 格式的追踪输出实现
 *
 * 将 VM 执行事件逐行写入 .jsonl 文件。
 * 每行一个完整 JSON 对象，便于流式解析和离线可视化。
 */

#pragma once

#include "debug/trace_sink.hpp"
#include <fstream>
#include <string>

namespace Lua {

/**
 * @brief JSON 行格式追踪输出端
 *
 * 构造时打开输出文件，析构时自动关闭。
 * 线程不安全——仅供单线程 VM 使用。
 */
class JsonTraceSink final : public ITraceSink {
public:
    /**
     * @brief 构造函数
     * @param filePath 输出文件路径
     * @param maxEvents 最大事件数（0 = 不限制，默认 1000000）
     */
    explicit JsonTraceSink(const Str& filePath, u64 maxEvents = 1000000);

    ~JsonTraceSink() override;

    // 禁止拷贝
    JsonTraceSink(const JsonTraceSink&) = delete;
    JsonTraceSink& operator=(const JsonTraceSink&) = delete;

    void onInstruction(const TraceEvent& evt) override;
    void onCall(const TraceEvent& evt) override;
    void onReturn(const TraceEvent& evt) override;
    void onError(const TraceEvent& evt) override;
    void flush() override;

    /** @brief 检查文件是否成功打开 */
    bool isOpen() const {
        return file_.is_open();
    }

    /** @brief 获取已写入的事件数 */
    u64 getEventCount() const {
        return eventCount_;
    }

private:
    std::ofstream file_;
    u64 maxEvents_;
    u64 eventCount_ = 0;

    /** @brief 检查是否达到事件上限 */
    bool reachedLimit() const {
        return maxEvents_ > 0 && eventCount_ >= maxEvents_;
    }
};

} // namespace Lua
