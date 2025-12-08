/**
 * @file main.cpp
 * @brief Lua C++ 解释器主程序入口
 *
 * 详细说明：
 * 这是Lua C++解释器的主程序入口，支持两种运行模式：
 * 1. 测试模式：运行单元测试套件（默认模式）
 * 2. 解释器模式：作为完整的Lua解释器运行（未来扩展）
 *
 * 初始化流程（参考lua_c_analysis/src/lua.c）：
 * 1. 创建Lua状态机（LuaState::newState）
 * 2. 初始化全局状态（GlobalState单例）
 * 3. 加载标准库（StandardLibrary::openAll）
 * 4. 执行用户代码或测试
 * 5. 清理资源（RAII自动管理）
 *
 * 使用方法：
 * - 测试模式：直接运行 main.exe
 * - 解释器模式：main.exe <script.lua> (未来实现)
 * - 交互模式：main.exe -i (未来实现)
 *
 * 参考实现：
 * - lua_c_analysis/src/lua.c - Lua 5.1.5官方解释器入口
 * - lua_c_analysis/src/lstate.c - lua_newstate初始化流程
 * - lua_c_analysis/src/linit.c - 标准库加载
 *
 * @author Lua C++ Project
 * @date 2025-12-04
 */

#include "vm/lua_state.hpp"
#include "vm/global_state.hpp"
#include "vm/vm.hpp"
#include "lib/lib_manager.hpp"
#include "lib/baselib.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "compiler/bytecode_printer.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "io/input_stream.hpp"

#include <iostream>
#include <cstring>
#include <memory>
#include <sstream>
#include <fstream>

// 测试框架（可选）
#ifdef ENABLE_TESTS
#include "../tests/unit/framework/test_framework.hpp"
#include "../tests/unit/framework/test_registry.hpp"
using namespace LuaTest;

// 测试注册函数声明
extern void registerValueTests();
extern void registerGCStringTests();
extern void registerTableTests();
extern void registerVMCoreTests();
extern void registerFunctionTests();
extern void registerGCTests();
extern void registerBinaryUnaryExprTests();
extern void registerFunctionCodegenTests();
extern void registerBaselibTests();
extern void registerLuaFunctionTests();
extern void registerMetamethodArithTests();
extern void registerMetamethodCompleteTests();
extern void registerSyntaxSugarTests();
extern void registerFunctionCallTests();
extern void registerLexerNumberTests();
extern void registerLexerLookaheadTests();
extern void registerParserRecursionTests();
extern void registerParserErrorRecoveryTests();
extern void registerParserMemoryPoolTests();
extern void registerDynamicBufferTests();
extern void registerInputStreamStringTests();
extern void registerInputStreamStreamTests();
extern void registerInputStreamFileTests();
#endif

using namespace Lua;

// ============================================================================
// 程序信息和帮助
// ============================================================================

/**
 * @brief 打印版本信息
 */
void printVersion() {
    std::cout << "Lua C++ Interpreter v0.1.0" << std::endl;
    std::cout << "Based on Lua 5.1.5 specification" << std::endl;
    std::cout << "Copyright (C) 2025 Lua C++ Project" << std::endl;
}

/**
 * @brief 打印使用帮助
 */
void printUsage(const char* progname) {
    std::cout << "Usage: " << progname << " [options] [script [args]]" << std::endl;
    std::cout << "Available options are:" << std::endl;
    std::cout << "  -v       show version information" << std::endl;
    std::cout << "  -h       show this help message" << std::endl;
    std::cout << "  -t       run unit tests (default mode)" << std::endl;
    std::cout << "  -i       enter interactive mode (not implemented)" << std::endl;
    std::cout << "  -        execute stdin (not implemented)" << std::endl;
}

// ============================================================================
// Lua虚拟机初始化（参考lua_c_analysis/src/lstate.c）
// ============================================================================

/**
 * @brief 创建并初始化Lua虚拟机
 *
 * 初始化序列（对应lua_newstate + f_luaopen）：
 * 1. 分配LuaState（对应LG结构：lua_State + global_State）
 * 2. 预初始化状态（preinit_state）
 * 3. 初始化全局状态（global_State初始化）
 * 4. 受保护的核心初始化（f_luaopen）：
 *    - 初始化调用栈（stack_init）
 *    - 创建全局表（luaH_new）
 *    - 创建注册表（luaH_new）
 *    - 初始化字符串表（luaS_resize）
 *    - 初始化元方法（luaT_init）
 *    - 初始化词法分析器（luaX_init）
 *    - 固定内存错误消息（luaS_fix）
 *    - 设置GC阈值
 *
 * @return 初始化完成的LuaState指针，失败返回nullptr
 */
UPtr<LuaState> createLuaState() {
    try {
        // 步骤1-4：LuaState::newState内部完成所有初始化
        // 对应C版本的：lua_newstate + preinit_state + f_luaopen
        UPtr<LuaState> L(LuaState::newState());

        if (!L) {
            std::cerr << "[ERROR] Failed to create Lua state: not enough memory" << std::endl;
            return nullptr;
        }

        // 步骤5：加载标准库（对应luaL_openlibs）
        // 注意：在C版本中，这通常在pmain中调用
        StandardLibrary::openAll(L.get());

        return L;

    } catch (const std::bad_alloc& e) {
        std::cerr << "[ERROR] Memory allocation failed: " << e.what() << std::endl;
        return nullptr;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception during initialization: " << e.what() << std::endl;
        return nullptr;
    }
}

// ============================================================================
// 测试模式
// ============================================================================

#ifdef ENABLE_TESTS
/**
 * @brief 打印测试框架标题
 */
void printTestHeader() {
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "Lua C++ Interpreter - Unit Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test Framework: Custom Lightweight Framework" << std::endl;
    std::cout << "Build: Visual Studio 2026 Manual Compilation" << std::endl;
    std::cout << "Date: 2025-12-04" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

/**
 * @brief 打印测试总结
 */
void printTestSummary(int totalTests, int totalFailed) {
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total Tests: " << totalTests << std::endl;
    std::cout << "Passed: " << (totalTests - totalFailed) << std::endl;
    std::cout << "Failed: " << totalFailed << std::endl;
    std::cout << "========================================" << std::endl;

    if (totalFailed == 0) {
        std::cout << "\n[OK] ALL TESTS PASSED!" << std::endl;
    } else {
        std::cout << "\n[FAILED] SOME TESTS FAILED!" << std::endl;
    }
    std::cout << "\n";
}

/**
 * @brief 运行单元测试
 * @return 失败的测试数量
 */
int runTests() {
    printTestHeader();

    // 注册所有测试
    std::cout << "[INFO] Registering tests..." << std::endl;

    registerValueTests();
    registerGCStringTests();
    registerTableTests();
    registerVMCoreTests();
    registerFunctionTests();
    registerGCTests();
    registerBinaryUnaryExprTests();
    registerFunctionCodegenTests();
    registerBaselibTests();
    registerLuaFunctionTests();
    registerMetamethodArithTests();
    registerMetamethodCompleteTests();
    registerSyntaxSugarTests();
    registerFunctionCallTests();
    registerLexerNumberTests();
	registerLexerLookaheadTests();
    registerParserRecursionTests();
    registerParserErrorRecoveryTests();
    registerParserMemoryPoolTests();
    registerDynamicBufferTests();
    registerInputStreamStringTests();
    registerInputStreamStreamTests();
    registerInputStreamFileTests();

    std::cout << "[INFO] All tests registered." << std::endl;
    std::cout << "[INFO] Starting test execution...\n" << std::endl;

    // 运行所有测试
    TestRegistry& registry = TestRegistry::getInstance();
    int failedTests = registry.runAllTests();

    // 打印总结
    int totalTests = 130;  // 更新：添加了5个Token预读测试
    printTestSummary(totalTests, failedTests);

    return failedTests;
}
#endif

// ============================================================================
// 解释器模式（未来实现）
// ============================================================================

/**
 * @brief 读取文件全部内容到字符串
 * @param filename 文件名
 * @return 文件内容字符串
 * @throws std::runtime_error 如果文件打开失败
 */
Str readFileContents(const char* filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error(Str("cannot open ") + filename + ": No such file or directory");
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    Str content(static_cast<usize>(size), '\0');
    if (!file.read(&content[0], size)) {
        throw std::runtime_error(Str("error reading ") + filename);
    }

    return content;
}

/**
 * @brief 执行Lua脚本文件
 *
 * 执行流程：
 * 1. 读取文件内容
 * 2. 解析源码生成AST (Parser)
 * 3. 生成字节码 (CodeGenerator)
 * 4. 创建函数对象并注册到GC
 * 5. 执行字节码 (VM)
 *
 * @param L Lua状态
 * @param filename 脚本文件名
 * @return 执行状态码（0=成功，非0=失败）
 */
int executeScript(LuaState* L, const char* filename) {
    try {
        // 步骤1：读取文件内容
        Str source = readFileContents(filename);

        // 步骤2：解析源码生成AST
        Parser parser(source);
        Chunk chunk = parser.parse();

        // 步骤3：生成字节码
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        if (!proto) {
            std::cerr << filename << ": code generation failed" << std::endl;
            return 1;
        }

        // 步骤4：创建函数对象并注册到GC
        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);

        // 设置函数环境为全局表（确保能访问全局函数）
        func->setEnv(L->getGlobalTable());

        // 步骤5：执行字节码
        VM vm(L);
        vm.execute(func);

        // 清理Proto（Function已经复制了必要的数据）
        delete proto;

        return 0;

    } catch (const ParseError& e) {
        // 语法错误：显示文件名、行号、列号和错误消息
        std::cerr << filename << ":" << e.getLine() << ":" << e.getColumn()
                  << ": " << e.what() << std::endl;
        return 1;

    } catch (const std::runtime_error& e) {
        // 运行时错误或文件错误
        std::cerr << filename << ": " << e.what() << std::endl;
        return 1;

    } catch (const std::exception& e) {
        // 其他异常
        std::cerr << filename << ": unexpected error: " << e.what() << std::endl;
        return 1;
    }
}

// ============================================================================
// REPL 辅助函数
// ============================================================================

/**
 * @brief REPL 常量定义
 */
namespace REPL {
    constexpr const char* PROMPT1 = "> ";      // 主提示符
    constexpr const char* PROMPT2 = ">> ";     // 续行提示符
    constexpr const char* VERSION = "Lua 5.1 (C++ Implementation)";
    constexpr const char* COPYRIGHT = "Copyright (c) 2025 Lua C++ Project";
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
    // 例如: "Syntax error: '<eof>' expected near '<eof>'"
    // 或者 "Unexpected token: <eof>"

    // 检查常见的不完整输入模式
    const char* eofPatterns[] = {
        "<eof>",
        "'end' expected",
        "Expected 'end'",              // 我们的解析器格式
        "'until' expected",
        "Expected 'until'",            // 我们的解析器格式
        "unexpected end of input",
        "Unexpected token in expression",  // 我们的解析器使用这个
        "to close function",           // 函数未关闭
        "to close 'if'",               // if 未关闭
        "to close 'while'",            // while 未关闭
        "to close 'for'",              // for 未关闭
        "to close 'do'",               // do 未关闭
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
    // 如果以 "=" 开头，转换为 "return ..."
    // 例如: "=1+2" -> "return 1+2"
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
 * 策略：如果输入可以作为语句成功解析，并且不是纯表达式，则是语句。
 *
 * @param source 源代码
 * @return true 如果是语句（不应打印结果）
 */
bool isStatementNotExpression(const Str& source) {
    // 检查是否是赋值语句（包含 =）
    // 注意：需要排除 ==, ~=, <=, >= 这些比较运算符
    usize pos = 0;
    while ((pos = source.find('=', pos)) != Str::npos) {
        // 检查是否是比较运算符
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
        // 找到了赋值 =
        return true;
    }

    // 检查是否以控制结构关键字开头（这些是语句）
    const char* stmtKeywords[] = {
        "if ", "while ", "for ", "repeat ", "function ", "local ", "do ", "return "
    };
    for (const char* kw : stmtKeywords) {
        if (source.find(kw) == 0) {
            return true;
        }
    }

    // 检查是否是函数调用语句（以标识符开头，后面跟着括号或字符串/表）
    // 例如：print("Hello"), foo(1, 2), obj:method()
    // 这需要更复杂的检测，我们使用启发式方法：
    // 如果输入以标识符开头，后面跟着 ( 或 " 或 ' 或 { 或 :，可能是函数调用
    if (!source.empty()) {
        // 跳过前导空白
        usize i = 0;
        while (i < source.size() && (source[i] == ' ' || source[i] == '\t')) {
            i++;
        }

        // 检查是否以标识符开头
        if (i < source.size() && (std::isalpha(source[i]) || source[i] == '_')) {
            // 找到标识符结尾
            usize start = i;
            while (i < source.size() && (std::isalnum(source[i]) || source[i] == '_')) {
                i++;
            }

            // 跳过空白
            while (i < source.size() && (source[i] == ' ' || source[i] == '\t')) {
                i++;
            }

            // 检查后面是否是函数调用的标志
            if (i < source.size()) {
                char next = source[i];
                // 直接函数调用：print("Hello"), print 'Hello', print {1,2,3}
                if (next == '(' || next == '"' || next == '\'' || next == '{') {
                    return true;
                }
                // 方法调用：obj:method()
                if (next == ':') {
                    return true;
                }
                // 成员访问后的函数调用：obj.method()
                if (next == '.') {
                    // 继续检查是否最终是函数调用
                    // 简化处理：如果有 . 后面跟着标识符和 (，认为是函数调用
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
        // 成功作为语句解析（如函数调用），不打印结果
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

        // 清理：不清理栈，保持全局变量等状态

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

/**
 * @brief 交互式 REPL 模式
 *
 * 实现完整的 REPL（Read-Eval-Print Loop）功能：
 * 1. 显示欢迎信息
 * 2. 显示提示符并读取用户输入
 * 3. 检测输入是否完整（支持多行输入）
 * 4. 解析并执行代码
 * 5. 打印表达式结果
 * 6. 处理错误并继续运行
 *
 * @param L Lua 状态
 * @return 执行状态码
 */
int interactiveMode(LuaState* L) {
    // 显示欢迎信息
    std::cout << REPL::VERSION << "  " << REPL::COPYRIGHT << std::endl;
    std::cout << "Type 'exit' or press Ctrl+D to quit." << std::endl;

    Str inputBuffer;  // 累积的输入
    bool isFirstLine = true;

    while (true) {
        // 选择提示符
        const char* prompt = isFirstLine ? REPL::PROMPT1 : REPL::PROMPT2;

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
        bool isExpression = wasExplicitReturn;  // 如果使用了 "=" 前缀，一定是表达式
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

// ============================================================================
// 主函数（参考lua_c_analysis/src/lua.c的main函数）
// ============================================================================

/**
 * @brief 主函数
 *
 * 执行流程（参考Lua 5.1.5官方实现）：
 * 1. 解析命令行参数
 * 2. 创建Lua状态机（lua_open/lua_newstate）
 * 3. 检查状态机创建是否成功
 * 4. 在保护模式下执行主程序（lua_cpcall/pmain）
 * 5. 报告执行结果
 * 6. 关闭状态机（lua_close）
 * 7. 返回退出码
 *
 * 退出码：
 * - 0: 成功
 * - 1: 执行失败
 * - 2: 异常错误
 *
 * @param argc 参数数量
 * @param argv 参数数组
 * @return 退出码
 */
int main(int argc, char** argv) {
    try {
        // 步骤1：解析命令行参数
        bool enableTestMode = true;  // 默认运行测试
        bool showVersion = false;
        bool showHelp = false;
        const char* scriptFile = nullptr;

        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "-v") == 0) {
                showVersion = true;
            } else if (std::strcmp(argv[i], "-h") == 0) {
                showHelp = true;
            } else if (std::strcmp(argv[i], "-t") == 0) {
                enableTestMode = true;
            } else if (std::strcmp(argv[i], "-i") == 0) {
                enableTestMode = false;
                // 交互模式
            } else if (argv[i][0] != '-') {
                enableTestMode = false;
                scriptFile = argv[i];
                break;
            }
        }

        // 显示版本或帮助
        if (showVersion) {
            printVersion();
            return 0;
        }

        if (showHelp) {
            printUsage(argv[0]);
            return 0;
        }

        // 步骤2-3：创建并初始化Lua状态机
        // 对应C版本的：lua_State *L = lua_open();
        auto L = createLuaState();

        if (!L) {
            std::cerr << "cannot create Lua state: not enough memory" << std::endl;
            return 1;
        }

        // 步骤4-5：执行主程序
        int status = 0;

#ifdef ENABLE_TESTS
        if (enableTestMode) {
            // 测试模式
            status = runTests();
        } else
#endif
        if (scriptFile) {
            // 脚本执行模式
            status = executeScript(L.get(), scriptFile);
        } else {
            // 交互模式
            status = interactiveMode(L.get());
        }

        // 步骤6：清理资源（RAII自动管理）
        L.reset();  // 显式释放，对应lua_close(L)

        // 步骤7：返回退出码
        return status > 0 ? 1 : 0;

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Exception caught: " << e.what() << std::endl;
        return 2;
    } catch (...) {
        std::cerr << "\n[ERROR] Unknown exception caught" << std::endl;
        return 2;
    }
}
