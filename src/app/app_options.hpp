#pragma once

#include "common/types.hpp"

namespace Lua {

enum class RunMode {
    ShowVersion,
    ShowHelp,
    Repl,
    Script,
    DefaultBehavior
};

struct AppOptions {
    RunMode mode = RunMode::DefaultBehavior;
    const char* programName = nullptr;
    int argc = 0;
    char** argv = nullptr;
    const char* scriptFile = nullptr;
    const char* traceFile = nullptr;
    bool traceDiff = false;
    i32 scriptIndex = -1;
};

AppOptions parseArgs(int argc, char** argv);
int runApp(const AppOptions& opt);

} // namespace Lua
