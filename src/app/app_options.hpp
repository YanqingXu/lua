#pragma once

/**
 * @file app_options.hpp
 * @brief Lua 命令行程序的运行模式与参数解析接口
 */

#include "common/types.hpp"
#include <span>

namespace Lua {

/** @brief 命令行程序最终选择的运行模式。 */
enum class RunMode {
    ShowVersion,
    ShowHelp,
    Error,
    Repl,
    Script,
    DefaultBehavior
};

/** @brief 脚本执行前启动动作的类型。 */
enum class StartupActionKind {
    ExecuteChunk,
    RequireModule
};

/** @brief 一项按命令行顺序执行的启动动作。 */
struct StartupAction {
    StartupActionKind kind = StartupActionKind::ExecuteChunk;
    Str argument;
};

/** @brief 命令行解析后的完整应用配置。 */
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

/** @brief 解析命令行参数视图。 */
AppOptions parseArgs(std::span<char* const> argv);
/** @brief 解析传统 argc/argv 命令行参数。 */
AppOptions parseArgs(int argc, char** argv);
/** @brief 根据已解析配置运行命令行程序。 */
int runApp(const AppOptions& opt);

} // namespace Lua
