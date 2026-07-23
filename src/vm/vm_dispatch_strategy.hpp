#pragma once

/**
 * @file vm_dispatch_strategy.hpp
 * @brief 虚拟机调度策略抽象
 */

#include "common/types.hpp"
#include "vm/vm.hpp"

namespace Lua {

struct RuntimeServices;
class LuaState;
class Proto;

namespace VM {

/** @brief 操作码调度后端共享的虚拟机执行上下文。 */
struct VMContext {
    RuntimeServices& services;
    LuaState* state;
    Proto* proto;
    i32 nexeccalls;
};

/** @brief 虚拟机操作码调度策略的抽象接口。 */
class DispatchStrategy {
public:
    virtual ~DispatchStrategy() = default;

    virtual ExecResult run(VMContext& context) = 0;
    virtual const char* name() const noexcept = 0;
};

/** @brief 基于 switch 语句的操作码调度策略。 */
class SwitchDispatch final : public DispatchStrategy {
public:
    ExecResult run(VMContext& context) override;
    const char* name() const noexcept override;
};

/** @brief 基于处理器表的操作码调度策略。 */
class TableDispatch final : public DispatchStrategy {
public:
    ExecResult run(VMContext& context) override;
    const char* name() const noexcept override;
};

DispatchStrategy& defaultDispatchStrategy() noexcept;
DispatchStrategy& tableDispatchStrategy() noexcept;

} // namespace VM
} // namespace Lua
