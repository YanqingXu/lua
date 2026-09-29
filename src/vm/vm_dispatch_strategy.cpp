/**
 * @file vm_dispatch_strategy.cpp
 * @brief 默认 VM 调度策略选择
 */

#include "common/types.hpp"
#include "vm/vm_dispatch_strategy.hpp"

namespace Lua::VM {

CharPtr SwitchDispatch::name() const noexcept {
    return "switch";
}

CharPtr TableDispatch::name() const noexcept {
    return "table";
}

DispatchStrategy& defaultDispatchStrategy() noexcept {
    static SwitchDispatch strategy;
    return strategy;
}

DispatchStrategy& tableDispatchStrategy() noexcept {
    static TableDispatch strategy;
    return strategy;
}

} // namespace Lua::VM
