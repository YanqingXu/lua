/**
 * @file app_options.cpp
 * @brief Lua 命令行参数解析的实现
 */

#include "app_options.hpp"

#include <cstring>
#include <utility>

namespace Lua {

AppOptions parseArgs(std::span<char* const> argv) {
    AppOptions opt;
    opt.arguments.reserve(argv.size());
    for (char* arg : argv) {
        opt.arguments.emplace_back(arg != nullptr ? arg : "");
    }
    opt.programName = !opt.arguments.empty() ? opt.arguments.front() : "";

    bool showVersion = false;
    bool showHelp = false;
    bool interactiveMode = false;
    bool hasError = false;

    auto fail = [&](Str message) {
        if (!hasError) {
            opt.errorMessage = std::move(message);
            hasError = true;
        }
    };

    auto addAction = [&](StartupActionKind kind, Str argument) {
        opt.startupActions.push_back(StartupAction{kind, std::move(argument)});
    };

    for (i32 i = 1; i < static_cast<i32>(opt.arguments.size()); ++i) {
        const Str& arg = opt.arguments[static_cast<usize>(i)];

        if (arg == "-v") {
            showVersion = true;
        } else if (arg == "-h") {
            showHelp = true;
        } else if (arg == "-i") {
            interactiveMode = true;
        } else if (arg == "--") {
            if (i + 1 < static_cast<i32>(opt.arguments.size())) {
                opt.scriptFile = opt.arguments[static_cast<usize>(i + 1)];
                opt.scriptIndex = i + 1;
            }
            break;
        } else if (arg == "-") {
            opt.scriptFile = arg;
            opt.scriptIndex = i;
            break;
        } else if (arg == "-e") {
            if (i + 1 >= static_cast<i32>(opt.arguments.size())) {
                fail("'-e' needs argument");
                break;
            }
            if (opt.arguments[static_cast<usize>(i + 1)] == "--") {
                addAction(StartupActionKind::ExecuteChunk, " ");
            } else {
                ++i;
                addAction(StartupActionKind::ExecuteChunk, opt.arguments[static_cast<usize>(i)]);
            }
        } else if (arg.starts_with("-e")) {
            addAction(StartupActionKind::ExecuteChunk, arg.substr(2));
        } else if (arg.starts_with("-l")) {
            Str module = arg.substr(2);
            if (module.empty()) {
                if (i + 1 >= static_cast<i32>(opt.arguments.size())) {
                    fail("'-l' needs argument");
                    break;
                }
                ++i;
                module = opt.arguments[static_cast<usize>(i)];
            }
            addAction(StartupActionKind::RequireModule, std::move(module));
        } else if (arg == "--trace" && i + 1 < static_cast<i32>(opt.arguments.size())) {
            ++i;
            opt.traceFile = opt.arguments[static_cast<usize>(i)];
            opt.traceDiff = false;
        } else if (arg == "--trace-diff" && i + 1 < static_cast<i32>(opt.arguments.size())) {
            ++i;
            opt.traceFile = opt.arguments[static_cast<usize>(i)];
            opt.traceDiff = true;
        } else if (arg == "--trace" || arg == "--trace-diff") {
            fail("trace option needs file argument");
            break;
        } else if (!arg.empty() && arg[0] != '-') {
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
    } else if (opt.scriptFile.has_value()) {
        opt.mode = RunMode::Script;
    } else if (interactiveMode) {
        opt.mode = RunMode::Repl;
    } else if (!opt.startupActions.empty()) {
        opt.mode = RunMode::Script;
    }

    return opt;
}

AppOptions parseArgs(int argc, char** argv) {
    if (argc <= 0 || argv == nullptr) {
        return parseArgs(std::span<char* const>());
    }

    return parseArgs(std::span<char* const>(argv, static_cast<usize>(argc)));
}

} // namespace Lua
