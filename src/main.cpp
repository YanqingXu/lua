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
#include "repl.hpp"

#include <iostream>
#include <cstring>
#include <memory>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <windows.h>

// 默认测试脚本路径。
// - 为空串：未指定命令行脚本时进入 REPL
// - 非空串：未指定命令行脚本时优先执行该 Lua 脚本
#ifndef LUA_TEST_SCRIPT_PATH
#define LUA_TEST_SCRIPT_PATH "E:/Programming2/lua_in_cpp/lua/tests/lua/functions/test_call_success.lua"
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
// 命令行参数支持
// ============================================================================

/**
 * @brief Setup arg table for Lua script to access command-line arguments
 *
 * Reference implementation: lua_c_analysis/src/lua.c:getargs()
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
            REPL::reportError((Str(filename) + ": code generation failed").c_str());
            return 1;
        }

        // 步骤4：创建函数对象并注册到GC
        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);

        // 设置函数环境为全局表（确保能访问全局函数）
        func->setEnv(L->getGlobalTable());

        // 步骤5：执行字节码
        VM::execute(L, func);

        // 清理Proto（Function已经复制了必要的数据）
        delete proto;

        return 0;

    } catch (const ParseError& e) {
        // 语法错误 - 使用官方 Lua 风格：progname: source:line: message
        REPL::reportError(filename, e.getLine(), e.what());
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

#ifdef _WIN32
    // 设置控制台 UTF-8 编码（解决 Windows 中文乱码问题）
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
	

    try {
        // 设置程序名（用于错误消息），参考官方 Lua 的 progname
        REPL::setProgName(argv[0]);

        // 步骤1：解析命令行参数
        bool showVersion = false;
        bool showHelp = false;
        bool interactiveMode = false;
        const char* scriptFile = nullptr;
        i32 scriptIndex = -1;  // 脚本文件在argv中的索引（用于arg表）

        for (i32 i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "-v") == 0) {
                showVersion = true;
            } else if (std::strcmp(argv[i], "-h") == 0) {
                showHelp = true;
            } else if (std::strcmp(argv[i], "-i") == 0) {
                interactiveMode = true;
            } else if (argv[i][0] != '-') {
                scriptFile = argv[i];
                scriptIndex = i;  // 记录脚本文件索引
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
            REPL::reportError("cannot create state: not enough memory");
            return 1;
        }

        // 步骤4-5：执行主程序
        i32 status = 0;

        if (scriptFile) {
            // 设置arg表（使脚本可通过arg[0], arg[1]...访问命令行参数）
            setupArgTable(L.get(), argc, argv, scriptIndex);

            // 脚本执行模式
            status = executeScript(L.get(), scriptFile);
        } else if (interactiveMode) {
            std::cout << "[INFO] 进入 REPL 模式。" << std::endl;
            REPL::initialize(L.get());
            status = REPL::run(L.get());
        } else {
            constexpr const char* kTestScriptPath = LUA_TEST_SCRIPT_PATH;

            if (kTestScriptPath[0] == '\0') {
                std::cout << "[INFO] 未指定脚本，进入 REPL 模式。" << std::endl;
                REPL::initialize(L.get());
                status = REPL::run(L.get());
            } else if (std::filesystem::exists(kTestScriptPath)) {
                std::cout << "[INFO] 执行测试脚本: " << kTestScriptPath << std::endl;
                status = executeScript(L.get(), kTestScriptPath);
            } else {
                std::ostringstream oss;
                oss << "test script not found: " << kTestScriptPath;
                REPL::reportError(oss.str().c_str());
                status = 1;
            }
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
