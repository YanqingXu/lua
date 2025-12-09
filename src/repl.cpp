/**
 * @file repl.cpp
 * @brief REPL（交互式解释器）模块实现
 *
 * 详细说明：
 * 本文件实现了 Lua 的交互式 REPL（Read-Eval-Print Loop）功能，
 * 参考官方 Lua 5.1.5 的 lua.c 中的 dotty()、loadline()、pushline() 等函数。
 *
 * 改进功能（参考 lua_with_cpp/src/repl.cpp）：
 * - 信号处理：支持 Ctrl+C 中断
 * - 可配置提示符：从 _PROMPT/_PROMPT2 全局变量读取
 * - exit() 全局函数：支持退出码
 * - _VERSION 全局变量：自动设置
 *
 * @author Lua C++ Project
 * @date 2025-12-04
 */

#include "repl.hpp"
#include "vm/vm.hpp"
#include "vm/global_state.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"

#include <iostream>
#include <string>
#include <csignal>
#include <cstdlib>

namespace Lua {
namespace REPL {

// ============================================================================
// 全局状态（用于信号处理和错误报告）
// ============================================================================

/// 全局 LuaState 指针，用于信号处理器访问
static LuaState* g_currentState = nullptr;

/// 中断标志，由 SIGINT 信号处理器设置
static volatile sig_atomic_t g_interrupted = 0;

/// 程序名（用于错误消息前缀），参考官方 Lua 的 progname
static const char* g_progname = DEFAULT_PROGNAME;

// ============================================================================
// 错误报告函数（参考 lua_c_analysis/src/lua.c 的 l_message 和 report）
// ============================================================================

void setProgName(const char* name) {
    if (name != nullptr && name[0] != '\0') {
        // 提取基本文件名（去除路径）
        const char* p = name;
        const char* lastSep = nullptr;
        while (*p) {
            if (*p == '/' || *p == '\\') {
                lastSep = p;
            }
            p++;
        }
        g_progname = lastSep ? lastSep + 1 : name;
    } else {
        g_progname = DEFAULT_PROGNAME;
    }
}

const char* getProgName() {
    return g_progname;
}

void reportError(const char* msg) {
    // 参考官方 Lua 的 l_message() 函数
    if (g_progname) {
        std::cerr << g_progname << ": ";
    }
    std::cerr << msg << std::endl;
}

void reportError(const char* source, int line, const char* msg) {
    // 格式：progname: source:line: message
    std::cerr << g_progname << ": " << source << ":" << line << ": " << msg << std::endl;
}

// ============================================================================
// 内部辅助函数（匿名命名空间）
// ============================================================================

namespace {

/**
 * @brief SIGINT (Ctrl+C) 信号处理器
 *
 * 当用户按下 Ctrl+C 时，设置中断标志而不是终止程序。
 * 这允许 REPL 优雅地处理中断，取消当前输入并继续运行。
 *
 * @param signal 信号编号
 */
void signalHandler([[maybe_unused]] int signal) {
    g_interrupted = 1;
    // 在 Windows 上，需要重新注册信号处理器
#ifdef _WIN32
    std::signal(SIGINT, signalHandler);
#endif
}

/**
 * @brief 安装信号处理器
 */
void installSignalHandler() {
    std::signal(SIGINT, signalHandler);
}

/**
 * @brief 恢复默认信号处理
 */
void restoreSignalHandler() {
    std::signal(SIGINT, SIG_DFL);
}

/**
 * @brief 检查是否被中断
 * @return true 如果收到 Ctrl+C 信号
 */
bool wasInterrupted() {
    return g_interrupted != 0;
}

/**
 * @brief 清除中断标志
 */
void clearInterruptFlag() {
    g_interrupted = 0;
}

/**
 * @brief 检测输入是否因为不完整而导致解析失败
 *
 * 参考 lua_c_analysis/src/lua.c 的 incomplete() 函数：
 * 如果错误消息以 "<eof>" 结尾，说明输入不完整（需要更多输入）
 *
 * @param errorMessage 解析错误消息
 * @return true 如果输入不完整，需要更多输入
 */
bool isIncompleteInput(const Str& errorMessage) {
    // 官方 Lua 的不完整输入错误以 "'<eof>'" 或 "<eof>" 结尾
    const char* eofPatterns[] = {
        "<eof>",
        "'end' expected",
        "Expected 'end'",
        "'until' expected",
        "Expected 'until'",
        "unexpected end of input",
        "Unexpected token in expression",
        "to close function",
        "to close 'if'",
        "to close 'while'",
        "to close 'for'",
        "to close 'do'",
    };

    for (const char* pattern : eofPatterns) {
        if (errorMessage.find(pattern) != Str::npos) {
            return true;
        }
    }

    return false;
}

/**
 * @brief 获取提示符（支持用户自定义）
 *
 * 从 Lua 全局变量 _PROMPT 或 _PROMPT2 读取提示符。
 * 如果全局变量未设置或不是字符串，返回默认值。
 *
 * 这允许用户通过设置 _PROMPT = ">>> " 来自定义提示符。
 *
 * @param L Lua 状态机
 * @param firstLine 是否是第一行（true 返回主提示符，false 返回续行提示符）
 * @return 提示符字符串
 */
Str getPrompt(LuaState* L, bool firstLine) {
    // 缓存提示符字符串，避免每次都创建
    static Str cachedPrompt1;
    static Str cachedPrompt2;

    const char* varName = firstLine ? "_PROMPT" : "_PROMPT2";
    const char* defaultPrompt = firstLine ? DEFAULT_PROMPT1 : DEFAULT_PROMPT2;
    Str& cachedPrompt = firstLine ? cachedPrompt1 : cachedPrompt2;

    try {
        // getGlobal 使用 const Str& 参数
        Value val = L->getGlobal(varName);

        if (val.isString()) {
            cachedPrompt = val.asString()->c_str();
            return cachedPrompt;
        }
    } catch (...) {
        // 忽略错误，使用默认提示符
    }

    return defaultPrompt;
}

/**
 * @brief 读取一行用户输入
 *
 * @param prompt 显示的提示符
 * @param line [out] 读取的行
 * @return true 如果成功读取，false 如果 EOF 或被中断
 */
bool readLine(const Str& prompt, Str& line) {
    std::cout << prompt << std::flush;

    // 检查是否被中断
    if (wasInterrupted()) {
        clearInterruptFlag();
        std::cout << std::endl;
        line.clear();
        return true;  // 返回 true 但 line 为空，让主循环继续
    }

    if (!std::getline(std::cin, line)) {
        // 检查是否是因为中断导致的读取失败
        if (wasInterrupted()) {
            clearInterruptFlag();
            std::cin.clear();  // 清除 EOF 状态
            std::cout << std::endl;
            line.clear();
            return true;
        }
        return false;  // EOF (Ctrl+D on Unix, Ctrl+Z on Windows)
    }

    // 处理 Windows 的 \r\n 换行符
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }

    return true;
}

/**
 * @brief 检查并处理 "=" 前缀的快速表达式求值
 *
 * 官方 Lua 的行为：
 * - 如果输入以 "=" 开头，将其视为 "return <表达式>"
 * - 这允许用户快速求值表达式并打印结果
 *
 * @param source 原始输入
 * @param wasExplicitReturn [out] 是否使用了 "=" 前缀
 * @return 转换后的源码（如果需要），否则返回原始源码
 */
Str tryAsExpression(const Str& source, bool& wasExplicitReturn) {
    if (!source.empty() && source[0] == '=') {
        wasExplicitReturn = true;
        return "return " + source.substr(1);
    }
    wasExplicitReturn = false;
    return source;
}

/**
 * @brief 执行 REPL 输入并打印结果
 *
 * @param L Lua 状态
 * @param source 源代码
 * @param isExpression 是否是表达式（需要打印结果）
 * @return 执行状态码（0=成功）
 */
int executeREPLInput(LuaState* L, const Str& source, bool isExpression) {
    try {
        // 解析源码
        Parser parser(source);
        Chunk chunk = parser.parse();

        // 生成字节码
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        if (!proto) {
            std::cerr << "code generation failed" << std::endl;
            return 1;
        }

        // 创建函数对象并注册到 GC
        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);

        // 设置函数环境为全局表
        func->setEnv(L->getGlobalTable());

        // 记录执行前的栈大小
        usize stackSizeBefore = L->getStack().size();

        // 执行字节码
        VM vm(L);
        vm.execute(func);

        // 如果是表达式，打印返回值
        if (isExpression) {
            usize stackSizeAfter = L->getStack().size();

            // 计算新增的返回值数量
            if (stackSizeAfter > stackSizeBefore) {
                usize nresults = stackSizeAfter - stackSizeBefore;

                // 打印所有返回值
                for (usize i = 0; i < nresults; ++i) {
                    usize idx = stackSizeBefore + i;
                    if (idx < L->getStack().size()) {
                        const Value& v = L->getStack()[idx];
                        // 转换为字符串并打印
                        if (v.isNil()) {
                            std::cout << "nil";
                        } else if (v.isBoolean()) {
                            std::cout << (v.asBoolean() ? "true" : "false");
                        } else if (v.isNumber()) {
                            std::cout << v.asNumber();
                        } else if (v.isString()) {
                            std::cout << v.asString()->c_str();
                        } else if (v.isTable()) {
                            std::cout << "table: " << v.asTable();
                        } else if (v.isFunction()) {
                            std::cout << "function: " << v.asFunction();
                        } else {
                            std::cout << v.toString();
                        }
                        if (i < nresults - 1) {
                            std::cout << "\t";  // 多值用 tab 分隔
                        }
                    }
                }
                std::cout << std::endl;
            }
        }

        // 清理 Proto
        delete proto;

        return 0;

    } catch (const ParseError& e) {
        // 使用官方 Lua 风格的错误格式：progname: source:line: message
        reportError("stdin", e.getLine(), e.what());
        return 1;

    } catch (const std::runtime_error& e) {
        reportError(e.what());
        return 1;

    } catch (const std::exception& e) {
        reportError(e.what());
        return 1;
    }
}

/**
 * @brief exit() 函数的 C 函数实现
 *
 * 支持可选的退出码参数：
 * - exit() - 退出码为 0
 * - exit(n) - 退出码为 n
 *
 * @param L Lua 状态机
 * @return 不返回（调用 std::exit）
 */
int luaB_exit(LuaState* L) {
    int exitCode = 0;

    // 检查是否有参数
    if (L->getTop() > 0) {
        Value arg = L->at(-1);
        if (arg.isNumber()) {
            exitCode = static_cast<int>(arg.asNumber());
        } else if (arg.isBoolean()) {
            exitCode = arg.asBoolean() ? 0 : 1;
        }
    }

    std::exit(exitCode);
    return 0;  // 永远不会到达
}

}  // anonymous namespace

// ============================================================================
// REPL 公共接口实现
// ============================================================================

void initialize(LuaState* L) {
    StringPool& pool = StringPool::getInstance();

    // 设置 _VERSION 全局变量
    GCString* versionVal = pool.intern(LUA_VERSION);
    L->setGlobal("_VERSION", Value(versionVal));

    // 设置默认提示符（可被用户修改）
    GCString* prompt1Val = pool.intern(DEFAULT_PROMPT1);
    L->setGlobal("_PROMPT", Value(prompt1Val));

    GCString* prompt2Val = pool.intern(DEFAULT_PROMPT2);
    L->setGlobal("_PROMPT2", Value(prompt2Val));

    // 注册 exit() 函数
    Function* exitFunc = new Function(luaB_exit);
    L->getGlobalState().getGC().registerObject(exitFunc);
    L->setGlobal("exit", Value(exitFunc));
}

int run(LuaState* L) {
    // 安装信号处理器
    installSignalHandler();
    g_currentState = L;

    // 显示欢迎信息
    std::cout << VERSION << "  " << COPYRIGHT << std::endl;
    std::cout << "Type 'exit' or press Ctrl+D to quit." << std::endl;

    Str inputBuffer;  // 累积的输入
    bool isFirstLine = true;

    while (true) {
        // 检查中断标志
        if (wasInterrupted()) {
            clearInterruptFlag();
            std::cout << std::endl;
            inputBuffer.clear();
            isFirstLine = true;
            continue;
        }

        // 获取提示符（支持用户自定义）
        Str prompt = getPrompt(L, isFirstLine);

        // 读取一行输入
        Str line;
        if (!readLine(prompt, line)) {
            // EOF，退出 REPL
            std::cout << std::endl;
            break;
        }

        // 检查中断（读取过程中可能被中断）
        if (wasInterrupted()) {
            clearInterruptFlag();
            inputBuffer.clear();
            isFirstLine = true;
            continue;
        }

        // 检查退出命令（仅在首行时检查）
        if (isFirstLine && (line == "exit" || line == "quit")) {
            break;
        }

        // 跳过空行（仅在首行时）
        if (isFirstLine && line.empty()) {
            continue;
        }

        // 累积输入
        bool wasExplicitReturn = false;
        if (isFirstLine) {
            inputBuffer = tryAsExpression(line, wasExplicitReturn);
        } else {
            inputBuffer += "\n" + line;
        }

        // 尝试解析输入
        bool parseSuccess = false;
        bool isExpression = wasExplicitReturn;
        Str sourceToExecute;

        try {
            // 官方 Lua 5.1.5 行为：
            // - 只有 "=expr" 语法才会打印表达式结果
            // - 普通输入直接作为语句处理，不自动包装为表达式
            Parser parser(inputBuffer);
            parser.parse();
            sourceToExecute = inputBuffer;
            isExpression = wasExplicitReturn;  // 只有使用了 "=" 前缀才打印结果
            parseSuccess = true;
        } catch (const ParseError& e) {
            // 检查是否是不完整输入
            if (isIncompleteInput(e.what())) {
                // 需要更多输入，继续读取
                isFirstLine = false;
                continue;
            }

            // 真正的语法错误 - 使用官方 Lua 风格
            reportError("stdin", e.getLine(), e.what());
            inputBuffer.clear();
            isFirstLine = true;
            continue;
        }

        // 执行代码
        if (parseSuccess) {
            executeREPLInput(L, sourceToExecute, isExpression);
        }

        // 重置状态，准备下一个输入
        inputBuffer.clear();
        isFirstLine = true;
    }

    // 恢复默认信号处理
    restoreSignalHandler();
    g_currentState = nullptr;

    std::cout << "Goodbye!" << std::endl;
    return 0;
}

}  // namespace REPL
}  // namespace Lua

