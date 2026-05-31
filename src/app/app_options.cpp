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
    bool hasError = false;

    auto fail = [&](const char* message) {
        if (!hasError) {
            opt.errorMessage = message;
            hasError = true;
        }
    };

    auto addAction = [&](StartupActionKind kind, const char* argument) {
        opt.startupActions.push_back(StartupAction{kind, argument});
    };

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
        } else if (std::strcmp(arg, "--") == 0) {
            if (i + 1 < argc) {
                opt.scriptFile = argv[i + 1];
                opt.scriptIndex = i + 1;
            }
            break;
        } else if (std::strcmp(arg, "-") == 0) {
            opt.scriptFile = arg;
            opt.scriptIndex = i;
            break;
        } else if (std::strcmp(arg, "-e") == 0) {
            if (i + 1 >= argc) {
                fail("'-e' needs argument");
                break;
            }
            if (std::strcmp(argv[i + 1], "--") == 0) {
                addAction(StartupActionKind::ExecuteChunk, " ");
            } else {
                addAction(StartupActionKind::ExecuteChunk, argv[++i]);
            }
        } else if (std::strncmp(arg, "-e", 2) == 0) {
            const char* chunk = arg + 2;
            addAction(StartupActionKind::ExecuteChunk, chunk);
        } else if (std::strncmp(arg, "-l", 2) == 0) {
            const char* module = arg + 2;
            if (*module == '\0') {
                if (i + 1 >= argc) {
                    fail("'-l' needs argument");
                    break;
                }
                module = argv[++i];
            }
            addAction(StartupActionKind::RequireModule, module);
        } else if (std::strcmp(arg, "--trace") == 0 && i + 1 < argc) {
            opt.traceFile = argv[++i];
            opt.traceDiff = false;
        } else if (std::strcmp(arg, "--trace-diff") == 0 && i + 1 < argc) {
            opt.traceFile = argv[++i];
            opt.traceDiff = true;
        } else if (std::strcmp(arg, "--trace") == 0 || std::strcmp(arg, "--trace-diff") == 0) {
            fail("trace option needs file argument");
            break;
        } else if (arg[0] != '-') {
            opt.scriptFile = arg;
            opt.scriptIndex = i;
            break;
        } else {
            fail("unrecognized option");
            break;
        }
    }

    opt.interactive = interactiveMode;

    if (showVersion) {
        opt.mode = RunMode::ShowVersion;
    } else if (hasError) {
        opt.mode = RunMode::Error;
    } else if (showHelp) {
        opt.mode = RunMode::ShowHelp;
    } else if (opt.scriptFile != nullptr) {
        opt.mode = RunMode::Script;
    } else if (interactiveMode) {
        opt.mode = RunMode::Repl;
    } else if (!opt.startupActions.empty()) {
        opt.mode = RunMode::Script;
    }

    return opt;
}

} // namespace Lua
