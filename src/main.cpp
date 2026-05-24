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
#include "io/file_loader.hpp"
#include "io/input_stream.hpp"
#include "runtime/runtime_services.hpp"
#include "debug/json_trace_sink.hpp"
#include "common/lua_error.hpp"
#include "repl.hpp"

#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
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
    std::cout << "  -h       show this help message" << std::endl;
    std::cout << "  -i       enter interactive mode" << std::endl;
    std::cout << "  --trace <file>  write execution trace to JSONL file" << std::endl;
    std::cout << "  --trace-diff <file>  write execution trace with changedRegisters" << std::endl;
    std::cout << "  -        execute stdin (not implemented)" << std::endl;
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
        GCString* argStr = L->getGlobalState().getStringPool().intern(argv[i]);
        i32 index = i - scriptIndex;  // Key calculation: same as official Lua
        argTable->set(Value(static_cast<LuaNumber>(index)), Value(argStr));
    }

    // Register arg table as global variable
    L->setGlobal("arg", Value(argTable));
}

// ============================================================================
// 解释器模式
// ============================================================================

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
        Str source = readWholeFile(filename);

        // 步骤2：解析源码生成AST
        RuntimeServices services(L->getGlobalState());

        Parser parser(source, services);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);

        // 步骤3：生成字节码
        CodeGenerator codegen(services);
        Proto* proto = codegen.generate(chunk, filename);

        if (!proto) {
            const Str message = std::format("{}: code generation failed", filename);
            REPL::reportError(message.c_str());
            return 1;
        }

        // 步骤4：创建函数对象并注册到GC
        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);

        // 设置函数环境为全局表（确保能访问全局函数）
        func->setEnv(L->getGlobalTable());

        // 步骤5：执行字节码
        VM::execute(services, L, func);

        // Proto由GC管理，并通过Function的标记路径保持可达。

        return 0;

    } catch (const ParseError& e) {
        // 语法错误 - 使用官方 Lua 风格：progname: source:line: message
        REPL::reportError(filename, e.getLine(), e.what());
        return 1;

    } catch (const LuaError& e) {
        // VM / runtime 层错误
        REPL::reportError(e.what());
        return 1;

    } catch (const std::runtime_error& e) {
        // 运行时错误或文件错误
        REPL::reportError(e.what());
        return 1;

    } catch (const std::exception& e) {
        // 其他异常
        REPL::reportError(e.what());
        return 1;
    }
}

int Lua::runApp(const AppOptions& opt) {
    const char* programName = opt.programName ? opt.programName : "lua";

    REPL::setProgName(programName);

    switch (opt.mode) {
    case RunMode::ShowVersion:
        printVersion();
        return 0;

    case RunMode::ShowHelp:
        printUsage(programName);
        return 0;

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
    case RunMode::Script:
        setupArgTable(L.get(), static_cast<i32>(opt.argc), opt.argv, opt.scriptIndex);
        status = executeScript(L.get(), opt.scriptFile);
        break;

    case RunMode::Repl:
        std::cout << "[INFO] 进入 REPL 模式。" << std::endl;
        REPL::initialize(L.get());
        status = REPL::run(L.get());
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
