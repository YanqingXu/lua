#pragma once

#include "common/types.hpp"
#include <span>

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
    Str argument;
};

struct AppOptions {
    RunMode mode = RunMode::DefaultBehavior;
    Str programName;
    Vec<Str> arguments;
    Opt<Str> scriptFile;
    Opt<Str> traceFile;
    Opt<Str> errorMessage;
    bool traceDiff = false;
    bool interactive = false;
    i32 scriptIndex = -1;
    Vec<StartupAction> startupActions;
};

AppOptions parseArgs(std::span<char* const> argv);
AppOptions parseArgs(int argc, char** argv);
int runApp(const AppOptions& opt);

} // namespace Lua
