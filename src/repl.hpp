/**
 * @file repl.hpp
 * @brief REPL（交互式解释器）模块头文件
 *
 * 详细说明：
 * 本模块实现了 Lua 的交互式 REPL（Read-Eval-Print Loop）功能，
 * 提供交互式读取、补全多行输入、执行和错误展示流程。
 *
 * 主要功能：
 * - 显示欢迎信息和提示符
 * - 读取用户输入（支持多行输入）
 * - 检测输入是否完整
 * - 解析并执行代码
 * - 打印表达式结果
 * - 处理错误并继续运行
 * - 支持 Ctrl+C 中断信号
 * - 支持默认行号提示符和可配置的提示符 (_PROMPT, _PROMPT2)
 * @author Lua C++ 项目
 * @date 2025-12-04
 */

#ifndef LUA_REPL_HPP
#define LUA_REPL_HPP

#include <iosfwd>

#include "common/types.hpp"
#include "vm/state/lua_state.hpp"

namespace Lua {

/**
 * @brief REPL 模块命名空间
 *
 * 包含所有 REPL 相关的函数和常量。
 */
namespace REPL {

// ============================================================================
// REPL 常量定义
// ============================================================================

/**
 * @brief 默认主提示符（对应 LUA_PROMPT）
 */
constexpr const char* DEFAULT_PROMPT1 = "> ";

/**
 * @brief 默认续行提示符（对应 LUA_PROMPT2）
 */
constexpr const char* DEFAULT_PROMPT2 = ">> ";

/**
 * @brief 版本信息
 */
constexpr const char* VERSION = "Lua 5.1.5";

/**
 * @brief 版权信息
 */
constexpr const char* COPYRIGHT = "Copyright (C) 1994-2012 Lua.org, PUC-Rio";

/**
 * @brief Lua 版本字符串（用于 _VERSION 全局变量）
 */
constexpr const char* LUA_VERSION = "Lua 5.1";

/**
 * @brief 默认程序名（用于错误消息前缀）
 */
constexpr const char* DEFAULT_PROGNAME = "lua";

/**
 * @brief 默认持久化历史文件名
 */
constexpr const char* DEFAULT_HISTORY_FILE = ".lua_history";

/** @brief 交互式解释器元命令类型。 */
enum class MetaCommandKind {
    None,
    Help,
    Bytecode,
    Ast,
    Gc,
    Unknown,
};

/** @brief 解析后的交互式解释器元命令。 */
struct MetaCommand {
    MetaCommandKind kind = MetaCommandKind::None;
    Str argument;
};

/** @brief 输入补全候选及其替换范围。 */
struct CompletionResult {
    Str completedLine;
    Vec<Str> candidates;
};

/** @brief 错误消息的颜色输出模式。 */
enum class ErrorColorMode {
    Auto,
    Never,
    Always,
};

// ============================================================================
// REPL 公共接口
// ============================================================================

/**
 * @brief 解析 REPL 元命令（以 . 开头的首行输入）
 */
MetaCommand parseMetaCommand(const Str& line);

/**
 * @brief 打印 REPL 帮助文本
 */
void printHelp(std::ostream& out);

/**
 * @brief 记录一条历史输入（忽略空行）
 */
void recordHistory(Vec<Str>& history, const Str& line);

/**
 * @brief 从文件读取历史记录
 */
bool loadHistory(const Str& path, Vec<Str>& history);

/**
 * @brief 将历史记录保存到文件
 */
bool saveHistory(const Str& path, const Vec<Str>& history);

/**
 * @brief 计算 REPL 行尾 Tab 补全结果
 */
CompletionResult completeInput(LuaState* L, const Str& line);

/**
 * @brief 编译 REPL 输入并打印字节码
 */
int printBytecode(LuaState* L, const Str& source, std::ostream& out, std::ostream& err);

/**
 * @brief 解析 REPL 输入并打印 AST
 */
int printAst(LuaState* L, const Str& source, std::ostream& out, std::ostream& err);

/**
 * @brief 打印或触发 GC 相关 REPL 信息
 */
int printGc(LuaState* L, const Str& argument, std::ostream& out, std::ostream& err);

/**
 * @brief 执行已解析的 REPL 元命令
 */
int runMetaCommand(LuaState* L, const MetaCommand& command, std::ostream& out, std::ostream& err);

/**
 * @brief 设置程序名（用于错误消息前缀）
 *
 * 参考官方 Lua 的 progname 全局变量。
 * 错误消息格式：progname: source:line: message
 *
 * @param name 程序名（通常为 argv[0]）
 */
void setProgName(const char* name);

/**
 * @brief 获取程序名
 * @return 程序名字符串
 */
const char* getProgName();

/**
 * @brief 设置错误输出颜色模式
 */
void setErrorColorMode(ErrorColorMode mode);

/**
 * @brief 获取当前错误输出颜色模式
 */
ErrorColorMode getErrorColorMode();

/**
 * @brief 输出错误消息到 stderr
 *
 * 参考官方 Lua 的 l_message() 函数。
 * 格式：progname: message
 *
 * @param msg 错误消息
 * @param showProgName 是否显示程序名前缀（默认 true）
 */
void reportError(const char* msg, bool showProgName = true);

/**
 * @brief 输出带源位置的错误消息
 *
 * 格式（脚本模式）：progname: source:line: message
 * 格式（REPL 模式）：source:line: message
 *
 * @param source 源文件名或 "stdin"
 * @param line 行号
 * @param msg 错误消息
 * @param showProgName 是否显示程序名前缀（默认 true，脚本模式）
 */
void reportError(const char* source, int line, const char* msg, bool showProgName = true);

/**
 * @brief 初始化 REPL 环境
 *
 * 设置 REPL 所需的全局变量和函数：
 * - _VERSION: Lua 版本信息
 * - _PROMPT: 主提示符（默认值启用 lua:N> 行号显示，可由用户修改）
 * - _PROMPT2: 续行提示符（默认值启用 lua:N>> 行号显示，可由用户修改）
 * - exit(): 退出 REPL 的函数
 *
 * @param L Lua 状态机指针
 */
void initialize(LuaState* L);

/**
 * @brief 运行交互式 REPL 模式
 *
 * 实现完整的 REPL（Read-Eval-Print Loop）功能：
 * 1. 显示欢迎信息
 * 2. 显示提示符并读取用户输入
 * 3. 检测输入是否完整（支持多行输入）
 * 4. 解析并执行代码
 * 5. 打印表达式结果
 * 6. 处理错误并继续运行
 *
 * 支持的特性：
 * - Ctrl+C 中断当前输入
 * - 默认行号提示符，可通过 _PROMPT 和 _PROMPT2 全局变量覆盖
 * - exit() 函数退出
 *
 * 参考官方 Lua 的 dotty() 函数实现。
 *
 * @param L Lua 状态机指针
 * @return 执行状态码（0=成功）
 */
int run(LuaState* L);

} // namespace REPL

} // namespace Lua

#endif // LUA_REPL_HPP
