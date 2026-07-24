#pragma once

/**
 * @file execution_policy.hpp
 * @brief 运行时级 Lua 指令、截止时间、取消与终结器治理策略
 */

#include "common/types.hpp"

#include <atomic>
#include <chrono>
#include <limits>
#include <memory>

namespace Lua {

/**
 * @brief VM 执行策略检查点返回的稳定停止原因
 */
enum class ExecutionStopReason : u8 {
    None,
    InstructionBudgetExceeded,
    NativeWorkBudgetExceeded,
    DeadlineExceeded,
    Cancelled,
};

/**
 * @brief 向非所有者线程公开的唯一线程安全执行控制接口
 *
 * 句柄仅公开单向原子请求；配置或清除策略仍属于所有者线程操作。下方共享状态使
 * EngineContext 析构后的迟到请求安全地成为无操作，而不会访问悬空指针。
 */
struct ExecutionCancellationState {
    std::atomic<bool> requested{false};
};

/** @brief 可从宿主侧请求取消执行的共享句柄。 */
class ExecutionCancellationHandle {
public:
    ExecutionCancellationHandle() noexcept = default;

    void requestCancellation() const noexcept {
        if (const std::shared_ptr<ExecutionCancellationState> state = state_.lock()) {
            state->requested.store(true, std::memory_order_relaxed);
        }
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return !state_.expired();
    }

private:
    explicit ExecutionCancellationHandle(const std::shared_ptr<ExecutionCancellationState>& state) noexcept
        : state_(state) {}

    std::weak_ptr<ExecutionCancellationState> state_;

    friend class ExecutionPolicy;
};

/**
 * @brief 单个运行时上下文内所有 LuaState 共享的执行限制
 *
 * 有限指令预算会在 Lua 调用、C 到 Lua 重入、让出与协程恢复之间持续消耗。截止时间使用
 * std::chrono::steady_clock，因此系统时钟调整不会延长或缩短已启动的执行窗口。每轮清理的
 * 终结器预算限制进入用户 __gc 回调的次数，同时保持默认不受限。仅所有者线程可配置策略，
 * 取消是唯一允许的跨线程操作。取消句柄只保留对独立分配状态的弱引用，因此上下文析构后的
 * 请求会安全地成为无操作。
 */
class ExecutionPolicy {
public:
    using Clock = std::chrono::steady_clock;
    using InstructionCount = u64;
    using NativeWorkCount = u64;
    using FinalizerCount = u64;

    static constexpr InstructionCount UnlimitedInstructions = std::numeric_limits<InstructionCount>::max();
    static constexpr NativeWorkCount UnlimitedNativeWork = std::numeric_limits<NativeWorkCount>::max();
    static constexpr FinalizerCount UnlimitedFinalizers = std::numeric_limits<FinalizerCount>::max();

    struct Limits {
        InstructionCount instructionBudget = UnlimitedInstructions;
        Clock::time_point deadline = Clock::time_point::max();
        FinalizerCount finalizerBudgetPerDrain = UnlimitedFinalizers;
        NativeWorkCount nativeWorkBudget = UnlimitedNativeWork;
    };

    ExecutionPolicy() = default;
    ExecutionPolicy(const ExecutionPolicy&) = delete;
    ExecutionPolicy& operator=(const ExecutionPolicy&) = delete;
    ExecutionPolicy(ExecutionPolicy&&) = delete;
    ExecutionPolicy& operator=(ExecutionPolicy&&) = delete;

    /**
     * @brief 启动新的执行窗口并清除先前的取消请求
     * @note 仅限所有者线程；不得与 Lua 执行并发调用。
     */
    void configure(const Limits& limits) noexcept {
        initialInstructions_ = limits.instructionBudget;
        remainingInstructions_ = limits.instructionBudget;
        deadline_ = limits.deadline;
        finalizerBudgetPerDrain_ = limits.finalizerBudgetPerDrain;
        initialNativeWork_ = limits.nativeWorkBudget;
        remainingNativeWork_ = limits.nativeWorkBudget;
        cancellationState_->requested.store(false, std::memory_order_relaxed);
        lastStopReason_ = ExecutionStopReason::None;
    }

    /**
     * @brief 禁用全部限制并清除先前的取消请求
     * @note 仅限所有者线程；不得与 Lua 执行并发调用。
     */
    void reset() noexcept {
        configure(Limits{});
    }

    /**
     * @brief 清除取消请求但不重置预算或截止时间
     * @note 仅限所有者线程；不得与 Lua 执行并发调用。
     */
    void clearCancellation() noexcept {
        cancellationState_->requested.store(false, std::memory_order_relaxed);
    }

    [[nodiscard]] ExecutionCancellationHandle cancellationHandle() noexcept {
        return ExecutionCancellationHandle(cancellationState_);
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

    [[nodiscard]] NativeWorkCount remainingNativeWork() const noexcept {
        return remainingNativeWork_;
    }

    [[nodiscard]] NativeWorkCount initialNativeWorkBudget() const noexcept {
        return initialNativeWork_;
    }

    [[nodiscard]] NativeWorkCount consumedNativeWork() const noexcept {
        if (initialNativeWork_ == UnlimitedNativeWork) {
            return 0;
        }
        return initialNativeWork_ - remainingNativeWork_;
    }

    /**
     * @brief 单轮收集器或终结清理允许进入的 __gc 回调数上限
     *
     * 此限制会在每轮清理时补充，而不会跨执行窗口持续消耗。有限值会对常规垃圾回收终结过程
     * 分片，并限制关闭期间进入用户终结器的次数；零会抑制本轮回调，但关闭流程仍会销毁所有
     * 拥有的存储。
     */
    [[nodiscard]] FinalizerCount finalizerBudgetPerDrain() const noexcept {
        return finalizerBudgetPerDrain_;
    }

    [[nodiscard]] bool hasFinalizerBudget() const noexcept {
        return finalizerBudgetPerDrain_ != UnlimitedFinalizers;
    }

    [[nodiscard]] bool isCancellationRequested() const noexcept {
        return cancellationState_->requested.load(std::memory_order_relaxed);
    }

    [[nodiscard]] ExecutionStopReason lastStopReason() const noexcept {
        return lastStopReason_;
    }

    /**
     * @brief 轮询同样适用于原生回调内部的停止条件
     *
     * 此检查点特意不消耗 Lua 指令预算。原生回调在有界工作分片之间协作调用它；只有 VM
     * 操作码调度会计量 Lua 指令。
     */
    [[nodiscard]] ExecutionStopReason pollStop() const noexcept {
        if (cancellationState_->requested.load(std::memory_order_relaxed)) [[unlikely]] {
            lastStopReason_ = ExecutionStopReason::Cancelled;
            return lastStopReason_;
        }

        if (deadline_ != Clock::time_point::max() && Clock::now() >= deadline_) [[unlikely]] {
            lastStopReason_ = ExecutionStopReason::DeadlineExceeded;
            return lastStopReason_;
        }

        return ExecutionStopReason::None;
    }

    /**
     * @brief 消耗执行下一条 Lua VM 指令的许可
     *
     * 取消的优先级高于截止时间，截止时间又高于预算。配置为 N 的预算恰好允许执行 N 条指令。
     */
    [[nodiscard]] ExecutionStopReason consumeInstruction() noexcept {
        const ExecutionStopReason stop = pollStop();
        if (stop != ExecutionStopReason::None) [[unlikely]] {
            return stop;
        }

        if (remainingInstructions_ != UnlimitedInstructions) {
            if (remainingInstructions_ == 0) [[unlikely]] {
                lastStopReason_ = ExecutionStopReason::InstructionBudgetExceeded;
                return lastStopReason_;
            }
            --remainingInstructions_;
        }

        return ExecutionStopReason::None;
    }

    /**
     * @brief 计量原生库代码内部执行的有界工作
     *
     * 此预算特意独立于 VM 指令。原生操作要么获得全部请求单位，要么耗尽剩余配额并报告稳定的
     * 停止原因。
     */
    [[nodiscard]] ExecutionStopReason consumeNativeWork(NativeWorkCount units = 1) noexcept {
        const ExecutionStopReason stop = pollStop();
        if (stop != ExecutionStopReason::None) [[unlikely]] {
            return stop;
        }

        if (remainingNativeWork_ != UnlimitedNativeWork) {
            if (units > remainingNativeWork_) [[unlikely]] {
                remainingNativeWork_ = 0;
                lastStopReason_ = ExecutionStopReason::NativeWorkBudgetExceeded;
                return lastStopReason_;
            }
            remainingNativeWork_ -= units;
        }
        return ExecutionStopReason::None;
    }

private:
    std::shared_ptr<ExecutionCancellationState> cancellationState_ = std::make_shared<ExecutionCancellationState>();
    InstructionCount initialInstructions_ = UnlimitedInstructions;
    InstructionCount remainingInstructions_ = UnlimitedInstructions;
    NativeWorkCount initialNativeWork_ = UnlimitedNativeWork;
    NativeWorkCount remainingNativeWork_ = UnlimitedNativeWork;
    Clock::time_point deadline_ = Clock::time_point::max();
    FinalizerCount finalizerBudgetPerDrain_ = UnlimitedFinalizers;
    mutable ExecutionStopReason lastStopReason_ = ExecutionStopReason::None;
};

} // namespace Lua
