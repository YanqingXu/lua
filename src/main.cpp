/**
 * @file main.cpp
 * @brief Lua C++ 解释器主程序入口
 *
 * 详细说明：
 * 这是Lua C++解释器的主程序入口，支持两种运行模式：
 * 1. 测试模式：运行单元测试套件（默认模式）
 * 2. 解释器模式：作为完整的Lua解释器运行（未来扩展）
 *
 * 初始化流程：
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
 * @author Lua C++ Project
 * @date 2025-12-04
 */

#include "vm/state/lua_state.hpp"
#include "vm/state/global_state.hpp"
#include "vm/vm.hpp"
#include "lib/lib_manager.hpp"
#include "lib/baselib.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include "app/app_options.hpp"
#include "bytecode/bytecode_printer.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "core/value.hpp"
#include "io/file_loader.hpp"
#include "io/input_stream.hpp"
#include "runtime/runtime_services.hpp"
#include "debug/json_trace_sink.hpp"
#include "common/lua_error.hpp"
#include "repl.hpp"
#include "repl/repl_ctx.hpp"
#include "repl/repl_exe.hpp"
#include "repl/repl_prompt.hpp"
#include "vm/vm_constants.hpp"

#include <filesystem>
#include <format>
#include <cctype>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <windows.h>

// 默认测试脚本路径。
// - 为空串：未指定命令行脚本时进入 REPL
// - 非空串：未指定命令行脚本时优先执行该 Lua 脚本
#ifndef LUA_TEST_SCRIPT_PATH
#define LUA_TEST_SCRIPT_PATH ""
#endif

// 测试脚本 Trace 输出路径（配合 LUA_TEST_SCRIPT_PATH 使用）。
// - 为空串：不启用默认脚本 Trace
// - 非空串：执行 LUA_TEST_SCRIPT_PATH 时自动启用 Trace，输出到此路径
//   可在此处直接改为目标路径，或通过编译器定义 /DLUA_TRACE_TEST_SCRIPT_OUTPUT=\"out.jsonl\"
#ifndef LUA_TRACE_TEST_SCRIPT_OUTPUT
#define LUA_TRACE_TEST_SCRIPT_OUTPUT "bin/out.jsonl"
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
    std::cout << "  -e stat  execute string 'stat'" << std::endl;
    std::cout << "  -l name  require library 'name'" << std::endl;
    std::cout << "  -i       enter interactive mode" << std::endl;
    std::cout << "  --trace <file>  write execution trace to JSONL file" << std::endl;
    std::cout << "  --trace-diff <file>  write execution trace with changedRegisters" << std::endl;
    std::cout << "  --       stop handling options" << std::endl;
    std::cout << "  -        execute stdin" << std::endl;
}

// ============================================================================
// Lua虚拟机初始化
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
        // 步骤1-4：LuaState::newState内部完成状态、栈和全局环境初始化
        UPtr<LuaState> L(LuaState::newState());

        if (!L) {
            std::cerr << "[ERROR] Failed to create Lua state: not enough memory" << std::endl;
            return nullptr;
        }

        // 步骤5：加载标准库
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
// 命令行参数支持
// ============================================================================

/**
 * @brief Setup arg table for Lua script to access command-line arguments
 *
 * arg table structure (Lua 5.1.5 standard):
 * - arg[-1]: interpreter name (e.g., "lua.exe")
 * - arg[0]:  script file name
 * - arg[1] ... arg[n]: script arguments
 *
 * Example: lua.exe script.lua a b c
 * - arg[-1] = "lua.exe"
 * - arg[0]  = "script.lua"
 * - arg[1]  = "a"
 * - arg[2]  = "b"
 * - arg[3]  = "c"
 *
 * Implementation details:
 * - Uses index formula: i - scriptIndex (same as official Lua)
 * - Pre-allocates table size for performance
 * - Registers all arguments from argv[0] to argv[argc-1]
 *
 * @param L Lua state pointer
 * @param argc Total number of command-line arguments
 * @param argv Command-line argument array
 * @param scriptIndex Index of script file name in argv
 */
void setupArgTable(LuaState* L, i32 argc, char* argv[], i32 scriptIndex) {
    // Create arg table and register to GC
    // Note: In official Lua, table is pre-allocated with lua_createtable(L, narg, n+1)
    // where narg = argc - (scriptIndex + 1) is the script argument count
    Table* argTable = new Table();
    L->getGlobalState().getGC().registerObject(argTable);

    // Populate arg table with all arguments
    // Index formula: i - scriptIndex (matches official Lua implementation)
    // This creates: arg[-scriptIndex] ... arg[0] ... arg[narg-1]
    for (i32 i = 0; i < argc; i++) {
        const char* argText = argv[i];
        Str adjustedArg;
        if (i == 0 && argText != nullptr) {
            adjustedArg = argText;
            for (char& ch : adjustedArg) {
                if (ch == '\\') {
                    ch = '/';
                }
            }
            argText = adjustedArg.c_str();
        } else if (i + 1 < scriptIndex && argv[i] != nullptr && argv[i + 1] != nullptr &&
            std::strcmp(argv[i], "-e") == 0 && std::strcmp(argv[i + 1], "--") == 0) {
            adjustedArg = "-e ";
            argText = adjustedArg.c_str();
        }
        GCString* argStr = L->getGlobalState().getStringPool().intern(argText ? argText : "");
        i32 index = i - scriptIndex;  // Key calculation: same as official Lua
        argTable->set(Value(static_cast<LuaNumber>(index)), Value(argStr));
    }

    // Register arg table as global variable
    L->setGlobal("arg", Value(argTable));
}

// ============================================================================
// 解释器模式
// ============================================================================

namespace {

bool isProjectBinaryChunk(StrView source) {
    return source.size() >= 4 && source.substr(0, 4) == StrView("\x1bLua", 4);
}

StrView skipInitialHashCommentLine(StrView source) {
    if (source.empty() || source.front() != '#') {
        return source;
    }

    usize newline = source.find_first_of("\r\n");
    if (newline == StrView::npos) {
        return source;
    }

    usize next = newline + 1;
    while (next < source.size() && (source[next] == '\r' || source[next] == '\n')) {
        next++;
    }
    return source.substr(next);
}

Str valueToString(const Value& value) {
    if (value.isString()) {
        return value.asString()->getData();
    }
    if (value.isNil()) {
        return "nil";
    }
    if (value.isBoolean()) {
        return value.asBoolean() ? "true" : "false";
    }
    if (value.isNumber()) {
        return std::format("{}", value.asNumber());
    }
    return value.toString();
}

Str makeLuaStringLiteral(StrView text) {
    Str result = "\"";
    for (char ch : text) {
        switch (ch) {
            case '\\': result += "\\\\"; break;
            case '"': result += "\\\""; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result.push_back(ch); break;
        }
    }
    result += "\"";
    return result;
}

Str readAllStdin() {
    std::ostringstream buffer;
    buffer << std::cin.rdbuf();
    return buffer.str();
}

bool isPendingAssignmentName(StrView source) {
    usize first = 0;
    while (first < source.size() &&
           std::isspace(static_cast<unsigned char>(source[first])) != 0) {
        first++;
    }

    usize last = source.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(source[last - 1])) != 0) {
        last--;
    }

    if (first >= last) {
        return false;
    }
    char head = source[first];
    if (!(std::isalpha(static_cast<unsigned char>(head)) != 0 || head == '_')) {
        return false;
    }
    for (usize i = first + 1; i < last; ++i) {
        char ch = source[i];
        if (!(std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_')) {
            return false;
        }
    }
    return true;
}

Vec<Str> collectScriptArgs(const AppOptions& opt) {
    Vec<Str> args;
    if (opt.scriptIndex < 0 || opt.argv == nullptr) {
        return args;
    }

    for (i32 i = opt.scriptIndex + 1; i < opt.argc; ++i) {
        args.emplace_back(opt.argv[i] ? opt.argv[i] : "");
    }
    return args;
}

Function* loadChunkFromSource(LuaState* L, StrView source, StrView chunkName, Str& errorMessage) {
    auto& pool = L->getGlobalState().getStringPool();
    StrView loadSource = source;
    StrView binarySource = skipInitialHashCommentLine(source);
    if (isProjectBinaryChunk(binarySource)) {
        loadSource = binarySource;
    }

    L->setTop(0);
    L->pushString(pool.intern(loadSource.data(), loadSource.size()));
    L->pushString(pool.intern(chunkName.data(), chunkName.size()));

    const i32 nresults = luaB_loadstring(L);
    if (nresults == 1 && L->getTop() >= 1 && L->at(1).isFunction()) {
        Function* func = L->at(1).asFunction();
        L->setTop(0);
        return func;
    }

    if (L->getTop() >= 2) {
        errorMessage = valueToString(L->at(2));
    } else {
        errorMessage = "cannot load chunk";
    }
    L->setTop(0);
    return nullptr;
}

int executeFunction(LuaState* L, Function* func, const Vec<Str>& args) {
    auto& pool = L->getGlobalState().getStringPool();

    L->setTop(0);
    L->pushFunction(func);
    for (const Str& arg : args) {
        L->pushString(pool.intern(arg.data(), arg.size()));
    }

    const i32 status = L->pcall(static_cast<i32>(args.size()), MULTRET, 0);
    if (status != LUA_OK) {
        Str message = L->getTop() >= 1 ? valueToString(L->at(1)) : "runtime error";
        REPL::reportError(message.c_str());
        L->setTop(0);
        return 1;
    }

    L->setTop(0);
    return 0;
}

int executeSource(LuaState* L, StrView source, StrView chunkName, const Vec<Str>& args = {}) {
    try {
        Str errorMessage;
        Function* func = loadChunkFromSource(L, source, chunkName, errorMessage);
        if (func == nullptr) {
            REPL::reportError(errorMessage.c_str());
            return 1;
        }

        return executeFunction(L, func, args);
    } catch (const std::exception& e) {
        REPL::reportError(e.what());
        L->setTop(0);
        return 1;
    }
}

int executeScript(LuaState* L, const char* filename, const Vec<Str>& args = {}) {
    try {
        Str source = readWholeFile(filename);
        Str chunkName = Str("@") + filename;
        return executeSource(L, StrView(source.data(), source.size()),
                             StrView(chunkName.data(), chunkName.size()), args);
    } catch (const std::exception& e) {
        REPL::reportError(e.what());
        return 1;
    }
}

int executeStdinScript(LuaState* L, const Vec<Str>& args) {
    Str source = readAllStdin();
    constexpr StrView chunkName("=stdin");
    return executeSource(L, StrView(source.data(), source.size()), chunkName, args);
}

int executeStartupAction(LuaState* L, const StartupAction& action) {
    const Str argument = action.argument ? action.argument : "";

    if (action.kind == StartupActionKind::ExecuteChunk) {
        constexpr StrView chunkName("=(command line)");
        return executeSource(L, StrView(argument.data(), argument.size()), chunkName);
    }

    if (std::filesystem::exists(argument)) {
        return executeScript(L, argument.c_str());
    }

    const Str source = "require(" + makeLuaStringLiteral(argument) + ")";
    constexpr StrView chunkName("=(command line)");
    return executeSource(L, StrView(source.data(), source.size()), chunkName);
}

int executeStartupActions(LuaState* L, const AppOptions& opt) {
    for (const StartupAction& action : opt.startupActions) {
        int status = executeStartupAction(L, action);
        if (status != 0) {
            return status;
        }
    }
    return 0;
}

int runQuietInteractive(LuaState* L) {
    REPL::detail::ReplContext& context = REPL::detail::globalContext();
    REPL::detail::ErrorColorContextGuard colorGuard(context);

    Str inputBuffer;
    bool bufferIsExpression = false;
    bool isFirstLine = true;
    usize currentLine = 1;

    auto resetInput = [&]() {
        inputBuffer.clear();
        bufferIsExpression = false;
        isFirstLine = true;
        currentLine = 1;
    };

    while (true) {
        std::cout << REPL::detail::getPrompt(L, isFirstLine, currentLine) << std::flush;

        Str line;
        if (!std::getline(std::cin, line)) {
            std::cout << std::endl;
            return 0;
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (isFirstLine && line.empty()) {
            continue;
        }

        if (isFirstLine) {
            bool wasExplicitReturn = false;
            inputBuffer = REPL::detail::tryAsExpression(line, wasExplicitReturn);
            bufferIsExpression = wasExplicitReturn;
        } else {
            inputBuffer += "\n" + line;
        }

        auto prepared =
            REPL::detail::prepareInputForExecution(L, inputBuffer, bufferIsExpression);
        if (!prepared) {
            const ParseError& error = prepared.error();
            if (REPL::detail::isIncompleteInput(error.what()) ||
                (isFirstLine && isPendingAssignmentName(inputBuffer))) {
                isFirstLine = false;
                currentLine += 1;
                continue;
            }

            REPL::detail::reportError(context, std::cerr, "stdin", error.getLine(), error.what(), false);
            resetInput();
            continue;
        }

        REPL::detail::executePreparedInput(context, L, std::move(*prepared), std::cout, std::cerr);
        resetInput();
    }
}

} // namespace

int Lua::runApp(const AppOptions& opt) {
    const char* programName = opt.programName ? opt.programName : "lua";

    REPL::setProgName(programName);

    switch (opt.mode) {
    case RunMode::ShowVersion:
        printVersion();
        return 0;

    case RunMode::Error:
        REPL::reportError(opt.errorMessage ? opt.errorMessage : "invalid command line");
        return 1;

    case RunMode::ShowHelp:
        printUsage(programName);
        return 1;

    case RunMode::Script:
    case RunMode::Repl:
    case RunMode::DefaultBehavior:
        break;
    }

    auto L = createLuaState();
    if (!L) {
        REPL::reportError("cannot create state: not enough memory");
        return 1;
    }

    if (opt.interactive) {
        REPL::initialize(L.get());
    }

    i32 status = 0;

    VM::setTraceSink(nullptr);
    VM::setTraceDiffEnabled(false);

    UPtr<JsonTraceSink> traceSink;
    if (opt.traceFile) {
        traceSink = makeUnique<JsonTraceSink>(opt.traceFile);
        if (traceSink->isOpen()) {
            VM::setTraceDiffEnabled(opt.traceDiff);
            VM::setTraceSink(traceSink.get());
            std::cout << (opt.traceDiff ? "[TRACE] Trace diff enabled → " : "[TRACE] Trace enabled → ")
                      << opt.traceFile << std::endl;
        } else {
            std::cerr << "[TRACE] Warning: cannot open trace file, trace disabled." << std::endl;
            VM::setTraceDiffEnabled(false);
            traceSink.reset();
        }
    }

    switch (opt.mode) {
    case RunMode::Script: {
        status = executeStartupActions(L.get(), opt);
        if (status != 0) {
            break;
        }

        if (opt.scriptFile != nullptr) {
            setupArgTable(L.get(), static_cast<i32>(opt.argc), opt.argv, opt.scriptIndex);
            Vec<Str> scriptArgs = collectScriptArgs(opt);
            if (std::strcmp(opt.scriptFile, "-") == 0) {
                status = executeStdinScript(L.get(), scriptArgs);
            } else {
                status = executeScript(L.get(), opt.scriptFile, scriptArgs);
            }
        }

        if (status == 0 && opt.interactive) {
            status = runQuietInteractive(L.get());
        }
        break;
    }

    case RunMode::Repl:
        status = executeStartupActions(L.get(), opt);
        if (status == 0) {
            status = runQuietInteractive(L.get());
        }
        break;

    case RunMode::DefaultBehavior: {
        constexpr const char* kTestScriptPath = LUA_TEST_SCRIPT_PATH;

        if (kTestScriptPath[0] == '\0') {
            std::cout << "[INFO] 未指定脚本，进入 REPL 模式。" << std::endl;
            REPL::initialize(L.get());
            status = REPL::run(L.get());
        } else if (std::filesystem::exists(kTestScriptPath)) {
            constexpr const char* kTestTraceOutput = LUA_TRACE_TEST_SCRIPT_OUTPUT;
            UPtr<JsonTraceSink> testTraceSink;
            if (kTestTraceOutput[0] != '\0' && !traceSink) {
                testTraceSink = makeUnique<JsonTraceSink>(kTestTraceOutput);
                if (testTraceSink->isOpen()) {
                    VM::setTraceDiffEnabled(false);
                    VM::setTraceSink(testTraceSink.get());
                    std::cout << "[TRACE] Test trace enabled \u2192 " << kTestTraceOutput << std::endl;
                } else {
                    testTraceSink.reset();
                }
            }

            std::cout << "[INFO] 执行测试脚本: " << kTestScriptPath << std::endl;
            status = executeScript(L.get(), kTestScriptPath);

            if (testTraceSink) {
                testTraceSink->flush();
                VM::setTraceSink(nullptr);
                VM::setTraceDiffEnabled(false);
                std::cout << "[TRACE] Test trace complete: " << testTraceSink->getEventCount() << " events written."
                          << std::endl;
            }
        } else {
            const Str message = std::format("test script not found: {}", kTestScriptPath);
            REPL::reportError(message.c_str());
            status = 1;
        }
        break;
    }

    case RunMode::ShowVersion:
    case RunMode::ShowHelp:
    case RunMode::Error:
        break;
    }

    if (traceSink) {
        traceSink->flush();
        VM::setTraceSink(nullptr);
        VM::setTraceDiffEnabled(false);
        std::cout << "[TRACE] Trace complete: " << traceSink->getEventCount() << " events written." << std::endl;
        traceSink.reset();
    }

    L.reset();
    return status > 0 ? 1 : 0;
}

// ============================================================================
// 主函数
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

#ifdef _WIN32
    // 设置控制台 UTF-8 编码（解决 Windows 中文乱码问题）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
	

    try {
        return runApp(parseArgs(argc, argv));

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Exception caught: " << e.what() << std::endl;
        return 2;
    } catch (...) {
        std::cerr << "\n[ERROR] Unknown exception caught" << std::endl;
        return 2;
    }
}
