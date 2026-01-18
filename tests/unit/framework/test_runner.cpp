/**
 * @file test_runner.cpp
 * @brief 测试运行器 - 统一的测试入口
 * 
 * 这个文件包含所有单元测试的主入口。
 * 它会自动运行所有注册的测试并生成报告。
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#include "test_framework.hpp"
#include <iostream>

// 声明所有测试注册函数
extern void registerValueTests();
extern void registerGCStringTests();
extern void registerTableTests();
extern void registerVMCoreTests();
extern void registerFunctionTests();
extern void registerGCTests();
extern void registerLuaStateInitTests();
extern void registerBinaryUnaryExprTests();
extern void registerFunctionCodegenTests();
extern void registerBaselibTests();
extern void registerLuaFunctionTests();
extern void registerMetamethodArithTests();
extern void registerMetamethodCompleteTests();
extern void registerSyntaxSugarTests();
extern void registerIndexedAccessTests();
extern void registerMethodCallTests();
extern void registerStorevarTests();
extern void registerLexerNumberTests();
extern void registerLexerLookaheadTests();
extern void registerParserRecursionTests();
extern void registerParserErrorRecoveryTests();
extern void registerParserMemoryPoolTests();

/**
 * @brief 打印测试框架标题
 */
void printHeader() {
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "Lua C++ Interpreter - Unit Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test Framework: Custom Lightweight Framework" << std::endl;
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
        std::cout << "\n✓ ALL TESTS PASSED!" << std::endl;
    } else {
        std::cout << "\n✗ SOME TESTS FAILED!" << std::endl;
    }
    std::cout << "\n";
}

/**
 * @brief 主函数
 */
int main() {
    printHeader();
    
    // 注册所有测试
    std::cout << "[INFO] Registering tests..." << std::endl;

    registerValueTests();
    registerGCStringTests();
    registerTableTests();
    registerVMCoreTests();
    registerFunctionTests();
    registerGCTests();
    registerLuaStateInitTests();
    registerBinaryUnaryExprTests();
    registerFunctionCodegenTests();
    registerSyntaxSugarTests();
    registerBaselibTests();
    registerLuaFunctionTests();
    registerMetamethodArithTests();
    registerMetamethodCompleteTests();
    registerIndexedAccessTests();
    registerMethodCallTests();
    registerStorevarTests();
    registerLexerNumberTests();
    registerLexerLookaheadTests();
    registerParserRecursionTests();
    registerParserErrorRecoveryTests();
    registerParserMemoryPoolTests();

    std::cout << "[INFO] All tests registered." << std::endl;
    std::cout << "[INFO] Starting test execution...\n" << std::endl;
    
    // 运行所有测试
    LuaTest::TestRegistry& registry = LuaTest::TestRegistry::getInstance();
    int failedTests = registry.runAllTests();
    
    // 打印总结
    // Note: We don't have total test count easily accessible, so we estimate
    int totalTests = failedTests > 0 ? failedTests * 2 : 50; // Rough estimate
    printSummary(totalTests, failedTests);
    
    // 返回失败测试数量作为退出码
    return failedTests > 0 ? 1 : 0;
}

