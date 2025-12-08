/**
 * @file repl.cpp
 * @brief REPL（交互式解释器）模块实现
 *
 * 详细说明：
 * 本文件实现了 Lua 的交互式 REPL（Read-Eval-Print Loop）功能，
 * 参考官方 Lua 5.1.5 的 lua.c 中的 dotty()、loadline()、pushline() 等函数。
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

#include <iostream>
#include <string>

namespace Lua {
namespace REPL {

// ============================================================================
// 内部辅助函数（匿名命名空间）
// ============================================================================

namespace {

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
 * @brief 读取一行用户输入
 *
 * @param prompt 显示的提示符
 * @param line [out] 读取的行
 * @return true 如果成功读取，false 如果 EOF
 */
bool readLine(const char* prompt, Str& line) {
    std::cout << prompt << std::flush;

    if (!std::getline(std::cin, line)) {
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
 * @brief 检查输入是否是语句而不是表达式
 *
 * 在 Lua 中，以下是有效的语句：
 * - 赋值语句（x = 10）
 * - 函数调用语句（print("Hello")）
 * - 控制结构（if, while, for, etc.）
 *
 * @param source 源代码
 * @return true 如果是语句（不应打印结果）
 */
bool isStatementNotExpression(const Str& source) {
    // 检查是否是赋值语句（包含 =）
    // 注意：需要排除 ==, ~=, <=, >= 这些比较运算符
    usize pos = 0;
    while ((pos = source.find('=', pos)) != Str::npos) {
        if (pos > 0) {
            char before = source[pos - 1];
            if (before == '=' || before == '~' || before == '<' || before == '>') {
                pos++;
                continue;
            }
        }
        if (pos + 1 < source.size() && source[pos + 1] == '=') {
            pos++;
            continue;
        }
        return true;  // 找到了赋值 =
    }

    // 检查是否以控制结构关键字开头
    const char* stmtKeywords[] = {
        "if ", "while ", "for ", "repeat ", "function ", "local ", "do ", "return "
    };
    for (const char* kw : stmtKeywords) {
        if (source.find(kw) == 0) {
            return true;
        }
    }

    // 检查是否是函数调用语句
    if (!source.empty()) {
        usize i = 0;
        while (i < source.size() && (source[i] == ' ' || source[i] == '\t')) {
            i++;
        }

        if (i < source.size() && (std::isalpha(source[i]) || source[i] == '_')) {
            while (i < source.size() && (std::isalnum(source[i]) || source[i] == '_')) {
                i++;
            }
            while (i < source.size() && (source[i] == ' ' || source[i] == '\t')) {
                i++;
            }

            if (i < source.size()) {
                char next = source[i];
                if (next == '(' || next == '"' || next == '\'' || next == '{' || next == ':') {
                    return true;
                }
                if (next == '.') {
                    usize j = i + 1;
                    while (j < source.size() && (std::isalnum(source[j]) || source[j] == '_' || source[j] == '.')) {
                        j++;
                    }
                    while (j < source.size() && (source[j] == ' ' || source[j] == '\t')) {
                        j++;
                    }
                    if (j < source.size() && (source[j] == '(' || source[j] == '"' || source[j] == '\'' || source[j] == '{' || source[j] == ':')) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

/**
 * @brief 尝试将代码作为表达式编译执行
 *
 * 策略（参考官方 Lua REPL）：
 * 1. 如果输入明显是语句（赋值、控制结构等），按语句处理
 * 2. 否则，首先尝试添加 "return" 前缀（作为表达式）
 * 3. 如果失败，尝试按原样编译（可能是函数调用语句）
 *
 * @param source 源代码
 * @param isExpression [out] 是否应该打印结果
 * @return 转换后的源码
 */
Str wrapAsExpressionIfNeeded(const Str& source, bool& isExpression) {
    // 如果明显是语句，不尝试作为表达式
    if (isStatementNotExpression(source)) {
        isExpression = false;
        return source;
    }

    // 首先尝试作为表达式（添加 return 前缀）
    Str exprSource = "return " + source;

    try {
        Parser parser(exprSource);
        parser.parse();  // 如果成功，则是表达式
        isExpression = true;
        return exprSource;
    } catch (const ParseError&) {
        // 表达式解析失败，尝试作为语句
    }

    // 尝试作为语句解析
    try {
        Parser parser(source);
        parser.parse();
        isExpression = false;
        return source;
    } catch (const ParseError&) {
        // 都失败了，按原样处理（会在执行时报错）
        isExpression = false;
        return source;
    }
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
        std::cerr << "stdin:" << e.getLine() << ": " << e.what() << std::endl;
        return 1;

    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;

    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }
}

}  // anonymous namespace

// ============================================================================
// REPL 公共接口实现
// ============================================================================

int run(LuaState* L) {
    // 显示欢迎信息
    std::cout << VERSION << "  " << COPYRIGHT << std::endl;
    std::cout << "Type 'exit' or press Ctrl+D to quit." << std::endl;

    Str inputBuffer;  // 累积的输入
    bool isFirstLine = true;

    while (true) {
        // 选择提示符
        const char* prompt = isFirstLine ? PROMPT1 : PROMPT2;

        // 读取一行输入
        Str line;
        if (!readLine(prompt, line)) {
            // EOF，退出 REPL
            std::cout << std::endl;
            break;
        }

        // 检查退出命令
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
            if (wasExplicitReturn) {
                // 使用了 "=" 前缀，直接作为表达式处理
                Parser parser(inputBuffer);
                parser.parse();
                sourceToExecute = inputBuffer;
                isExpression = true;
                parseSuccess = true;
            } else {
                // 首先尝试作为表达式（添加 return）
                bool tryExpr = false;
                Str exprSource = wrapAsExpressionIfNeeded(inputBuffer, tryExpr);

                if (tryExpr) {
                    // 成功作为表达式解析
                    sourceToExecute = exprSource;
                    isExpression = true;
                    parseSuccess = true;
                } else {
                    // 尝试作为语句解析
                    Parser parser(inputBuffer);
                    parser.parse();
                    sourceToExecute = inputBuffer;
                    isExpression = false;
                    parseSuccess = true;
                }
            }
        } catch (const ParseError& e) {
            // 检查是否是不完整输入
            if (isIncompleteInput(e.what())) {
                // 需要更多输入，继续读取
                isFirstLine = false;
                continue;
            }

            // 真正的语法错误
            std::cerr << "stdin:" << e.getLine() << ": " << e.what() << std::endl;
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

    std::cout << "Goodbye!" << std::endl;
    return 0;
}

}  // namespace REPL
}  // namespace Lua

