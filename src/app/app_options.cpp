#include "app_options.hpp"

#include <cstring>

namespace Lua {

AppOptions parseArgs(int argc, char** argv) {
    AppOptions opt;
    opt.argc = argc;
    opt.argv = argv;
    opt.programName = (argc > 0 && argv != nullptr) ? argv[0] : nullptr;

    bool showVersion = false;
    bool showHelp = false;
    bool interactiveMode = false;

    for (i32 i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        if (arg == nullptr) {
            continue;
        }

        if (std::strcmp(arg, "-v") == 0) {
            showVersion = true;
        } else if (std::strcmp(arg, "-h") == 0) {
            showHelp = true;
        } else if (std::strcmp(arg, "-i") == 0) {
            interactiveMode = true;
        } else if (std::strcmp(arg, "--trace") == 0 && i + 1 < argc) {
            opt.traceFile = argv[++i];
        } else if (arg[0] != '-') {
            opt.scriptFile = arg;
            opt.scriptIndex = i;
            break;
        }
    }

    if (showVersion) {
        opt.mode = RunMode::ShowVersion;
    } else if (showHelp) {
        opt.mode = RunMode::ShowHelp;
    } else if (opt.scriptFile != nullptr) {
        opt.mode = RunMode::Script;
    } else if (interactiveMode) {
        opt.mode = RunMode::Repl;
    }

    return opt;
}

} // namespace Lua
