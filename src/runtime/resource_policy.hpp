#pragma once

/**
 * @file resource_policy.hpp
 * @brief Canonical per-context limits for script-controlled resource growth.
 */

#include "common/types.hpp"

#include <stdexcept>

namespace Lua {

struct ResourcePolicy {
    static constexpr usize DefaultMaxBytes = static_cast<usize>(64) * 1024 * 1024;

    usize maxStringBytes = DefaultMaxBytes;
    usize maxOutputBytes = DefaultMaxBytes;
    usize maxSourceBytes = DefaultMaxBytes;
    usize maxProtoBytes = DefaultMaxBytes;
    usize maxTableArraySlots = 1'000'000;
    usize maxTableHashEntries = 1'000'000;
    usize maxStackSlots = 1'000'000;
    usize maxReturnValues = 1'000'000;
    usize maxSortElements = 1'000'000;
    usize maxSortComparisons = 32'000'000;
    usize maxPatternSteps = 16'000'000;
    usize maxReaderPieces = 1'000'000;
};

class ResourceLimitError final : public std::runtime_error {
public:
    explicit ResourceLimitError(const char* message) : std::runtime_error(message) {}
};

} // namespace Lua
