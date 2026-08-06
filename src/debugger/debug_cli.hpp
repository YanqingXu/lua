#pragma once

/**
 * @file debug_cli.hpp
 * @brief Internal command driver used only by debugger development/tests.
 */

#include "debugger/debug_runtime.hpp"

namespace Lua::Debugger {

class DebugCli {
public:
    explicit DebugCli(IDebugRuntime& runtime) : runtime_(runtime) {}

    /** Execute one command without accessing VM-private state. */
    [[nodiscard]] DebugResult<Str> execute(StrView command);

private:
    [[nodiscard]] DebugResult<Str> backtrace();
    [[nodiscard]] DebugResult<Str> locals();

    IDebugRuntime& runtime_;
};

} // namespace Lua::Debugger
