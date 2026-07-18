#pragma once

/**
 * @file trace_runtime.hpp
 * @brief Trace/debug state isolated to one runtime context.
 */

#include "common/types.hpp"

namespace Lua {

class ITraceSink;

class TraceRuntime {
public:
    void setSink(ITraceSink* sink) noexcept {
        sink_ = sink;
        sequence_ = 0;
    }

    [[nodiscard]] ITraceSink* sink() const noexcept {
        return sink_;
    }

    [[nodiscard]] u64 nextSequence() noexcept {
        return sequence_++;
    }

    void setDiffEnabled(bool enabled) noexcept {
        diffEnabled_ = enabled;
    }

    [[nodiscard]] bool diffEnabled() const noexcept {
        return diffEnabled_;
    }

    void setDumpBytecode(bool enabled) noexcept {
        dumpBytecode_ = enabled;
    }

    [[nodiscard]] bool dumpBytecode() const noexcept {
        return dumpBytecode_;
    }

private:
    ITraceSink* sink_ = nullptr;
    u64 sequence_ = 0;
    bool dumpBytecode_ = false;
    bool diffEnabled_ = false;
};

} // namespace Lua
