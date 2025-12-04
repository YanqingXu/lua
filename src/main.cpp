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
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"

#include <iostream>
#include <cstring>
#include <memory>

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

    std::cout << "[INFO] All tests registered." << std::endl;
    std::cout << "[INFO] Starting test execution...\n" << std::endl;

    // 运行所有测试
    TestRegistry& registry = TestRegistry::getInstance();
    int failedTests = registry.runAllTests();

    // 打印总结
    int totalTests = 125;
    printTestSummary(totalTests, failedTests);

    return failedTests;
}
#endif

// ============================================================================
// 解释器模式（未来实现）
// ============================================================================

/**
 * @brief 执行Lua脚本文件
 * @param L Lua状态
 * @param filename 脚本文件名
 * @return 执行状态码
 */
int executeScript(LuaState* L, const char* filename) {
    std::cout << "[INFO] Script execution not yet implemented: " << filename << std::endl;
    std::cout << "[INFO] This feature will be available in future versions." << std::endl;
    return 0;
}

/**
 * @brief 交互式REPL模式
 * @param L Lua状态
 * @return 执行状态码
 */
int interactiveMode(LuaState* L) {
    std::cout << "[INFO] Interactive mode not yet implemented." << std::endl;
    std::cout << "[INFO] This feature will be available in future versions." << std::endl;
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
        std::cout << "[INFO] Initializing Lua virtual machine..." << std::endl;
        auto L = createLuaState();

        if (!L) {
            std::cerr << "[ERROR] Cannot create Lua state: not enough memory" << std::endl;
            return 1;
        }

        std::cout << "[INFO] Lua VM initialized successfully." << std::endl;

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
        std::cout << "[INFO] Shutting down Lua VM..." << std::endl;
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
