#pragma once

/**
 * @file compilation_policy.hpp
 * @brief 不可信源码的上下文级限制与单次编译计量策略
 */

#include "common/types.hpp"
#include "runtime/execution_policy.hpp"

#include <chrono>
#include <optional>
#include <stdexcept>

namespace Lua {

/** @brief 不可信源码编译过程的资源策略。 */
struct CompilationPolicy {
    using Clock = std::chrono::steady_clock;

    static constexpr usize DefaultMaxBytes = static_cast<usize>(64) * 1024 * 1024;

    usize maxSourceBytes = DefaultMaxBytes;
    usize maxReaderPieces = 1'000'000;
    usize maxTokens = 1'000'000;
    usize maxAstNodes = 1'000'000;
    usize maxFunctions = 10'000;
    usize maxConstants = 1'000'000;
    usize maxInstructions = 1'000'000;
    usize maxStringBytes = DefaultMaxBytes;
    usize maxNesting = 200;
    std::optional<Clock::time_point> deadline;
};

/** @brief 编译过程超过资源限制时抛出的异常。 */
class CompilationLimitError final : public std::runtime_error {
public:
    explicit CompilationLimitError(const char* message) : std::runtime_error(message) {}
};

/**
 * @brief 单次编译产生的所有嵌套函数原型共享的短期预算
 */
class CompilationBudget {
public:
    explicit CompilationBudget(const CompilationPolicy& policy, ExecutionPolicy* execution = nullptr) noexcept
        : policy_(policy), execution_(execution) {}

    void checkSource(usize bytes) {
        pollDeadline();
        if (bytes > policy_.maxSourceBytes) {
            throw CompilationLimitError("compilation source byte limit exceeded");
        }
        consumeNativeWork(bytes);
    }

    void consumeToken(usize count = 1) {
        consume(count, tokens_, policy_.maxTokens, "compilation token limit exceeded");
    }

    void consumeAstNode(usize count = 1) {
        consume(count, astNodes_, policy_.maxAstNodes, "compilation AST node limit exceeded");
    }

    void consumeFunction(usize count = 1) {
        consume(count, functions_, policy_.maxFunctions, "compilation function limit exceeded");
    }

    void consumeConstant(usize count = 1) {
        consume(count, constants_, policy_.maxConstants, "compilation constant limit exceeded");
    }

    void consumeInstruction(usize count = 1) {
        consume(count, instructions_, policy_.maxInstructions, "compilation instruction limit exceeded");
    }

    void consumeStringBytes(usize count) {
        consume(count, stringBytes_, policy_.maxStringBytes, "compilation string byte limit exceeded");
    }

    [[nodiscard]] usize maxNesting() const noexcept {
        return policy_.maxNesting;
    }

    void pollDeadline() const {
        if (policy_.deadline && CompilationPolicy::Clock::now() >= *policy_.deadline) {
            throw CompilationLimitError("compilation deadline exceeded");
        }
    }

private:
    void consume(usize count, usize& total, usize limit, const char* message) {
        pollDeadline();
        if (count > limit || total > limit - count) {
            throw CompilationLimitError(message);
        }
        total += count;
        consumeNativeWork(count);
    }

    void consumeNativeWork(usize count) {
        if (execution_ != nullptr && execution_->consumeNativeWork(static_cast<ExecutionPolicy::NativeWorkCount>(
                                         count)) != ExecutionStopReason::None) {
            throw CompilationLimitError("compilation interrupted by execution policy");
        }
    }

    CompilationPolicy policy_;
    ExecutionPolicy* execution_ = nullptr;
    usize tokens_ = 0;
    usize astNodes_ = 0;
    usize functions_ = 0;
    usize constants_ = 0;
    usize instructions_ = 0;
    usize stringBytes_ = 0;
};

} // namespace Lua
