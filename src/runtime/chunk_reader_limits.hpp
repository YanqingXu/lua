#pragma once

/**
 * @file chunk_reader_limits.hpp
 * @brief Hard limits applied while deserializing an untrusted Lua chunk.
 */

#include "common/types.hpp"
#include "runtime/resource_policy.hpp"

namespace Lua {

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
