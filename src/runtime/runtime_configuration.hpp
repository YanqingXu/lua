#pragma once

/**
 * @file runtime_configuration.hpp
 * @brief 创建根 State 前应用到 EngineContext 的内部配置快照
 */

#include "runtime/compilation_policy.hpp"
#include "runtime/execution_policy.hpp"
#include "runtime/resource_policy.hpp"
#include "runtime/sandbox_policy.hpp"

namespace Lua {

struct RuntimeConfiguration {
    SandboxProfile sandbox = SandboxProfile::unrestricted();
    ExecutionPolicy::Limits execution{};
    ResourcePolicy resources{};
    CompilationPolicy compilation{};
};

} // namespace Lua
