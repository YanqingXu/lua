#pragma once

/**
 * @file vm_dispatch_strategy.hpp
 * @brief VM dispatch strategy abstraction.
 */

#include "common/types.hpp"
#include "vm/vm.hpp"

namespace Lua {

struct RuntimeServices;
class LuaState;
class Proto;

namespace VM {

struct VMContext {
    RuntimeServices& services;
    LuaState* state;
    Proto* proto;
    i32 nexeccalls;
};

class DispatchStrategy {
public:
    virtual ~DispatchStrategy() = default;

    virtual ExecResult run(VMContext& context) = 0;
    virtual const char* name() const noexcept = 0;
};

class SwitchDispatch final : public DispatchStrategy {
public:
    ExecResult run(VMContext& context) override;
    const char* name() const noexcept override;
};

DispatchStrategy& defaultDispatchStrategy() noexcept;

}  // namespace VM
}  // namespace Lua
