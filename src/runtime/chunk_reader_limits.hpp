#pragma once

/**
 * @file chunk_reader_limits.hpp
 * @brief 反序列化不可信 Lua 代码块时应用的硬限制
 */

#include "common/types.hpp"
#include "runtime/resource_policy.hpp"

namespace Lua {

/** @brief 二进制代码块读取器的资源限制。 */
struct ChunkReaderLimits {
    usize maxInputBytes = ResourcePolicy::DefaultMaxBytes;
    usize maxProtoDepth = 200;
    usize maxProtoCount = 10'000;
    usize maxInstructionCount = 1'000'000;
    usize maxConstantCount = 1'000'000;
    usize maxStringBytes = ResourcePolicy::DefaultMaxBytes;
    usize maxDebugEntries = 1'000'000;

    [[nodiscard]] static ChunkReaderLimits fromResourcePolicy(const ResourcePolicy& policy) noexcept {
        ChunkReaderLimits limits;
        limits.maxInputBytes = policy.maxProtoBytes;
        limits.maxStringBytes = policy.maxStringBytes;
        return limits;
    }
};

} // namespace Lua
