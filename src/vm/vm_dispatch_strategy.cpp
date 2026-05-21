/**
 * @file vm_dispatch_strategy.cpp
 * @brief Default VM dispatch strategy selection.
 */

#include "vm/vm_dispatch_strategy.hpp"

namespace Lua::VM {

const char* SwitchDispatch::name() const noexcept {
    return "switch";
}

const char* TableDispatch::name() const noexcept {
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

}  // namespace Lua::VM
