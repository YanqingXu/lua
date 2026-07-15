#pragma once

/**
 * @file execution_policy.hpp
 * @brief Runtime-wide Lua instruction, deadline, cancellation, and finalizer governance.
 */

#include "common/types.hpp"

#include <atomic>
#include <chrono>
#include <limits>

namespace Lua {

/**
 * @brief Stable reason returned by the VM execution-policy checkpoint.
 */
enum class ExecutionStopReason : u8 {
    None,
    InstructionBudgetExceeded,
    DeadlineExceeded,
    Cancelled,
};

/**
 * @brief The only thread-safe execution control exposed to non-owner threads.
 *
 * The handle is non-owning and must not outlive its EngineContext. It deliberately
 * exposes only a one-way atomic request: configuring or clearing policy remains an
 * owner-thread operation performed while that context is not executing Lua code.
 */
class ExecutionCancellationHandle {
public:
    ExecutionCancellationHandle() noexcept = default;

    void requestCancellation() const noexcept {
        if (requested_ != nullptr) {
            requested_->store(true, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return requested_ != nullptr;
    }

private:
    explicit ExecutionCancellationHandle(std::atomic<bool>* requested) noexcept : requested_(requested) {}

    std::atomic<bool>* requested_ = nullptr;

    friend class ExecutionPolicy;
};

/**
 * @brief Shared execution limits for every LuaState in one runtime context.
 *
 * A finite instruction budget is consumed across Lua calls, C-to-Lua re-entry,
 * yields, and coroutine resumes. Deadlines use std::chrono::steady_clock so wall
 * clock adjustments cannot extend or shorten an armed run. A per-drain finalizer
 * budget bounds entry into user __gc callbacks without changing the unlimited
 * default. Policy configuration is owner-thread-only; cancellation is the sole
 * cross-thread operation.
 */
class ExecutionPolicy {
public:
    using Clock = std::chrono::steady_clock;
    using InstructionCount = u64;
    using FinalizerCount = u64;

    static constexpr InstructionCount UnlimitedInstructions = std::numeric_limits<InstructionCount>::max();
    static constexpr FinalizerCount UnlimitedFinalizers = std::numeric_limits<FinalizerCount>::max();

    struct Limits {
        InstructionCount instructionBudget = UnlimitedInstructions;
        Clock::time_point deadline = Clock::time_point::max();
        FinalizerCount finalizerBudgetPerDrain = UnlimitedFinalizers;
    };

    ExecutionPolicy() noexcept = default;
    ExecutionPolicy(const ExecutionPolicy&) = delete;
    ExecutionPolicy& operator=(const ExecutionPolicy&) = delete;
    ExecutionPolicy(ExecutionPolicy&&) = delete;
    ExecutionPolicy& operator=(ExecutionPolicy&&) = delete;

    /**
     * @brief Arm a fresh execution window and clear any earlier cancellation.
     * @note Owner-thread-only; do not call concurrently with Lua execution.
     */
    void configure(const Limits& limits) noexcept {
        initialInstructions_ = limits.instructionBudget;
        remainingInstructions_ = limits.instructionBudget;
        deadline_ = limits.deadline;
        finalizerBudgetPerDrain_ = limits.finalizerBudgetPerDrain;
        cancellationRequested_.store(false, std::memory_order_relaxed);
    }

    /**
     * @brief Disable all limits and clear any earlier cancellation request.
     * @note Owner-thread-only; do not call concurrently with Lua execution.
     */
    void reset() noexcept {
        configure(Limits{});
    }

    /**
     * @brief Clear cancellation without resetting budget or deadline.
     * @note Owner-thread-only; do not call concurrently with Lua execution.
     */
    void clearCancellation() noexcept {
        cancellationRequested_.store(false, std::memory_order_relaxed);
    }

    [[nodiscard]] ExecutionCancellationHandle cancellationHandle() noexcept {
        return ExecutionCancellationHandle(&cancellationRequested_);
    }

    [[nodiscard]] InstructionCount initialInstructionBudget() const noexcept {
        return initialInstructions_;
    }

    [[nodiscard]] InstructionCount remainingInstructions() const noexcept {
        return remainingInstructions_;
    }

    [[nodiscard]] InstructionCount consumedInstructions() const noexcept {
        if (initialInstructions_ == UnlimitedInstructions) {
            return 0;
        }
        return initialInstructions_ - remainingInstructions_;
    }

    [[nodiscard]] Clock::time_point deadline() const noexcept {
        return deadline_;
    }

    [[nodiscard]] bool hasInstructionBudget() const noexcept {
        return remainingInstructions_ != UnlimitedInstructions;
    }

    [[nodiscard]] bool hasDeadline() const noexcept {
        return deadline_ != Clock::time_point::max();
    }

    /**
     * @brief Maximum __gc callbacks entered by one collector/finalize drain.
     *
     * This limit is replenished for each drain rather than consumed across the
     * execution window. A finite value slices ordinary GC finalization and
     * bounds close-time entry into user finalizers; zero suppresses callbacks
     * for that drain while shutdown still destroys all owned storage.
     */
    [[nodiscard]] FinalizerCount finalizerBudgetPerDrain() const noexcept {
        return finalizerBudgetPerDrain_;
    }

    [[nodiscard]] bool hasFinalizerBudget() const noexcept {
        return finalizerBudgetPerDrain_ != UnlimitedFinalizers;
    }

    [[nodiscard]] bool isCancellationRequested() const noexcept {
        return cancellationRequested_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Consume permission for the next Lua VM instruction.
     *
     * Cancellation has priority over deadline, which has priority over budget.
     * Exactly N instructions may execute for a configured budget of N.
     */
    [[nodiscard]] ExecutionStopReason consumeInstruction() noexcept {
        if (cancellationRequested_.load(std::memory_order_relaxed)) [[unlikely]] {
            return ExecutionStopReason::Cancelled;
        }

        if (deadline_ != Clock::time_point::max() && Clock::now() >= deadline_) [[unlikely]] {
            return ExecutionStopReason::DeadlineExceeded;
        }

        if (remainingInstructions_ != UnlimitedInstructions) {
            if (remainingInstructions_ == 0) [[unlikely]] {
                return ExecutionStopReason::InstructionBudgetExceeded;
            }
            --remainingInstructions_;
        }

        return ExecutionStopReason::None;
    }

private:
    std::atomic<bool> cancellationRequested_{false};
    InstructionCount initialInstructions_ = UnlimitedInstructions;
    InstructionCount remainingInstructions_ = UnlimitedInstructions;
    Clock::time_point deadline_ = Clock::time_point::max();
    FinalizerCount finalizerBudgetPerDrain_ = UnlimitedFinalizers;
};

} // namespace Lua
