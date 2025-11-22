/**
 * @file main.cpp
 * @brief 主程序入口 - 用于Visual Studio IDE手动编译测试
 *
 * 这个文件用于在Visual Studio 2026 IDE中进行手动编译和调试。
 * 它复用了 tests/unit/ 目录下的测试框架和测试用例，避免代码重复。
 *
 * 功能：
 * - 包含测试框架（test_framework.hpp）
 * - 调用所有测试注册函数
 * - 运行所有单元测试
 * - 输出测试报告
 *
 * 使用方法：
 * 1. 在Visual Studio 2026中打开此文件
 * 2. 配置项目包含目录：lua/src 和 lua/tests/unit
 * 3. 添加所有必要的.cpp文件到项目：
 *    - src/core/*.cpp
 *    - src/gc/*.cpp
 *    - src/vm/*.cpp
 *    - src/compiler/*.cpp
 *    - src/lib/*.cpp
 *    - tests/unit/test_*.cpp (所有测试文件)
 * 4. 编译并运行
 *
 * @note 这个文件复用测试框架，与build_tests.bat使用相同的测试代码
 * @author Lua C++ Project
 * @date 2025-11-14
 */

// 包含测试框架（相对路径：从src/到tests/unit/）
#include "../tests/unit/test_framework.hpp"
#include "../tests/unit/test_registry.hpp"

#include <iostream>

using namespace LuaTest;

/**
 * @brief 打印测试框架标题
 */
void printHeader() {
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "Lua C++ Interpreter - Unit Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test Framework: Custom Lightweight Framework" << std::endl;
    std::cout << "Build: Visual Studio 2026 Manual Compilation" << std::endl;
    std::cout << "Date: 2025-11-14" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

/**
 * @brief 打印测试总结
 */
void printSummary(int totalTests, int totalFailed) {
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
 * @brief 主函数
 */
int main() {
    try {
        printHeader();

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
        registerMetamethodCompleteTests();  // 新增：完整元方法测试

        std::cout << "[INFO] All tests registered." << std::endl;
        std::cout << "[INFO] Starting test execution...\n" << std::endl;

        // 运行所有测试
        TestRegistry& registry = TestRegistry::getInstance();
        int failedTests = registry.runAllTests();

        // 打印总结
        int totalTests = 125; // 更新测试总数 (111 + 14新增测试)
        printSummary(totalTests, failedTests);

        // 返回失败测试数量作为退出码
        return failedTests > 0 ? 1 : 0;

    } catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Exception caught: " << e.what() << std::endl;
        return 2;
    } catch (...) {
        std::cerr << "\n[ERROR] Unknown exception caught" << std::endl;
        return 3;
    }
}
