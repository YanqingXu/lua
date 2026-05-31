#pragma once

#include "common/types.hpp"

namespace Lua {

enum class RunMode {
    ShowVersion,
    ShowHelp,
    Error,
    Repl,
    Script,
    DefaultBehavior
};

enum class StartupActionKind {
    ExecuteChunk,
    RequireModule
};

struct StartupAction {
    StartupActionKind kind = StartupActionKind::ExecuteChunk;
    const char* argument = nullptr;
};

struct AppOptions {
    RunMode mode = RunMode::DefaultBehavior;
    const char* programName = nullptr;
    int argc = 0;
    char** argv = nullptr;
    const char* scriptFile = nullptr;
    const char* traceFile = nullptr;
    const char* errorMessage = nullptr;
    bool traceDiff = false;
    bool interactive = false;
    i32 scriptIndex = -1;
    Vec<StartupAction> startupActions;
};

AppOptions parseArgs(int argc, char** argv);
int runApp(const AppOptions& opt);

} // namespace Lua
