#pragma once

/**
 * @file ring_trace_sink.hpp
 * @brief 容量有界且感知分配器的内存跟踪输出端
 */

#include "debug/trace_sink.hpp"
#include "runtime/lua_allocator.hpp"

#include <stdexcept>

namespace Lua {

/** @brief 将最近的追踪事件保存在固定容量环形缓冲区中的输出端。 */
class RingTraceSink final : public ITraceSink {
public:
    explicit RingTraceSink(usize capacity, const LuaAllocator* allocator = nullptr)
        : events_(LuaSnapshotStdAllocator<TraceEvent>(allocator)), capacity_(capacity) {
        events_.reserve(capacity_);
    }

    void onInstruction(const TraceEvent& event) override { append(event); }
    void onCall(const TraceEvent& event) override { append(event); }
    void onReturn(const TraceEvent& event) override { append(event); }
    void onError(const TraceEvent& event) override { append(event); }
    void flush() override {}

    [[nodiscard]] usize size() const noexcept { return events_.size(); }
    [[nodiscard]] usize capacity() const noexcept { return capacity_; }
    [[nodiscard]] u64 totalEvents() const noexcept { return totalEvents_; }

    [[nodiscard]] const TraceEvent& at(usize chronologicalIndex) const {
        if (chronologicalIndex >= events_.size()) {
            throw std::out_of_range("trace ring index out of range");
        }
        if (events_.size() < capacity_) {
            return events_[chronologicalIndex];
        }
        return events_[(writeCursor_ + chronologicalIndex) % capacity_];
    }

private:
    void append(const TraceEvent& event) {
        ++totalEvents_;
        if (capacity_ == 0) {
            return;
        }
        if (events_.size() < capacity_) {
            events_.push_back(event);
            writeCursor_ = events_.size() % capacity_;
            return;
        }
        events_[writeCursor_] = event;
        writeCursor_ = (writeCursor_ + 1) % capacity_;
    }

    LuaOwnedVector<TraceEvent> events_;
    usize capacity_ = 0;
    usize writeCursor_ = 0;
    u64 totalEvents_ = 0;
};

} // namespace Lua
