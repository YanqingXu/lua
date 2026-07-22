#pragma once

/**
 * @file resource_policy.hpp
 * @brief 脚本控制资源增长所使用的统一上下文级限制
 */

#include "common/types.hpp"

#include <stdexcept>

namespace Lua {

/** @brief 单个运行时上下文的资源上限。 */
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

/** @brief 运行时资源使用超过策略上限时抛出的异常。 */
class ResourceLimitError final : public std::runtime_error {
public:
    explicit ResourceLimitError(const char* message) : std::runtime_error(message) {}
};

} // namespace Lua
