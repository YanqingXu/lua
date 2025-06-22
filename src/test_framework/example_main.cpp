/**
 * @file example_main.cpp
 * @brief Lua测试框架示例主程序
 * 
 * 这个文件演示了如何使用新的Lua测试框架。
 * 它展示了框架的各种功能，包括基础测试、内存检测、错误处理等。
 * 
 * 编译命令:
 * g++ -std=c++17 -I../.. -o example_main example_main.cpp
 * 
 * 运行:
 * ./example_main
 */

#include "test_framework.hpp"
#include "examples/example_test.hpp"
#include <iostream>
#include <exception>

using namespace Lua::TestFramework;
using namespace Lua::TestFramework::Examples;

/**
 * @brief 主函数 - 演示测试框架的使用
 */
int main() {
    try {
        // 显示欢迎信息
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "    Lua Test Framework 2.0 Demo       \n";
        std::cout << "========================================\n";
        std::cout << "\n";
        
        // 初始化测试框架
        std::cout << "Initializing test framework...\n";
        INIT_LUA_TEST_FRAMEWORK();
        
        std::cout << "\n";
        
        // 方式1: 使用便捷宏运行所有测试
        std::cout << "=== Method 1: Using Convenience Macros ===\n";
        RUN_ALL_TESTS(ExampleTestSuite);
        
        std::cout << "\n";
        
        // 方式2: 手动控制测试执行
        std::cout << "=== Method 2: Manual Test Control ===\n";
        
        // 设置不同的主题
        TestUtils::printInfo("Switching to CLASSIC theme...");
        TestUtils::setTheme(ColorTheme::CLASSIC);
        
        // 运行特定的测试组
        TestUtils::printLevelHeader(TestLevel::MAIN, "Manual Test Execution");
        
        RUN_TEST_GROUP("Basic Functionality", []() {
            RUN_TEST(ExampleTestClass, testBasicFunctionality);
            RUN_TEST(ExampleTestClass, testStringOperations);
        });
        
        RUN_TEST_GROUP_WITH_MEMORY_CHECK("Memory Safety", []() {
            RUN_TEST(ExampleTestClass, testMemoryAllocation);
            RUN_TEST(ExampleTestClass, testNoMemoryLeaks);
        });
        
        TestUtils::printLevelFooter(TestLevel::MAIN, "Manual Test Execution Completed");
        
        std::cout << "\n";
        
        // 方式3: 演示错误处理
        std::cout << "=== Method 3: Error Handling Demo ===\n";
        
        TestUtils::setTheme(ColorTheme::MINIMAL);
        
        TestUtils::printLevelHeader(TestLevel::SUITE, "Error Handling Tests");
        
        try {
            RUN_TEST_GROUP("Error Tests", []() {
                RUN_TEST(ExampleTestClass, testExceptionHandling);
                RUN_TEST(ExampleTestClass, testErrorRecovery);
            });
        } catch (const std::exception& e) {
            TestUtils::printError("Caught exception in error handling demo: " + std::string(e.what()));
        }
        
        TestUtils::printLevelFooter(TestLevel::SUITE, "Error Handling Tests Completed");
        
        std::cout << "\n";
        
        // 方式4: 演示内存测试
        std::cout << "=== Method 4: Memory Testing Demo ===\n";
        
        TestUtils::setTheme(ColorTheme::MODERN);
        
        RUN_MEMORY_SAFE_TESTS([]() {
            TestUtils::printInfo("Running comprehensive memory tests...");
            
            RUN_TEST(ExampleTestClass, testMemoryAllocation);
            RUN_TEST(ExampleTestClass, testMemoryDeallocation);
            RUN_TEST(ExampleTestClass, testProperCleanup);
        });
        
        std::cout << "\n";
        
        // 显示测试统计
        std::cout << "=== Test Statistics ===\n";
        auto stats = TestUtils::getTestStatistics();
        TestUtils::printInfo("Total Tests Run: " + std::to_string(stats.totalTests));
        TestUtils::printInfo("Tests Passed: " + std::to_string(stats.passedTests));
        TestUtils::printInfo("Tests Failed: " + std::to_string(stats.failedTests));
        
        if (stats.failedTests == 0) {
            TestUtils::printInfo("🎉 All tests passed successfully!");
        } else {
            TestUtils::printWarning("⚠️  Some tests failed. Please check the output above.");
        }
        
        std::cout << "\n";
        
        // 生成测试报告
        std::cout << "=== Generating Test Report ===\n";
        TestUtils::generateTestReport();
        
        std::cout << "\n";
        std::cout << "========================================\n";
        std::cout << "         Demo Completed Successfully    \n";
        std::cout << "========================================\n";
        
        return stats.failedTests == 0 ? 0 : 1;
        
    } catch (const std::exception& e) {
        std::cerr << "\n";
        std::cerr << "========================================\n";
        std::cerr << "              FATAL ERROR               \n";
        std::cerr << "========================================\n";
        std::cerr << "Exception: " << e.what() << "\n";
        std::cerr << "\n";
        std::cerr << "The demo encountered a fatal error and cannot continue.\n";
        std::cerr << "Please check your test framework installation.\n";
        std::cerr << "========================================\n";
        return 2;
        
    } catch (...) {
        std::cerr << "\n";
        std::cerr << "========================================\n";
        std::cerr << "           UNKNOWN FATAL ERROR         \n";
        std::cerr << "========================================\n";
        std::cerr << "An unknown exception occurred.\n";
        std::cerr << "Please check your test framework installation.\n";
        std::cerr << "========================================\n";
        return 3;
    }
}

/**
 * @brief 演示如何创建自定义测试套件
 * 
 * 这个函数展示了如何创建和组织自定义的测试套件。
 * 虽然在这个示例中没有被调用，但它展示了最佳实践。
 */
void demonstrateCustomTestSuite() {
    class CustomTestSuite {
    public:
        static void runAllTests() {
            TestUtils::printLevelHeader(TestLevel::SUITE, "Custom Test Suite");
            
            RUN_TEST_GROUP("Group 1", runGroup1Tests);
            RUN_TEST_GROUP("Group 2", runGroup2Tests);
            RUN_TEST_GROUP_WITH_MEMORY_CHECK("Memory Group", runMemoryTests);
            
            TestUtils::printLevelFooter(TestLevel::SUITE, "Custom Test Suite Completed");
        }
        
    private:
        static void runGroup1Tests() {
            RUN_TEST(CustomTestClass, test1);
            RUN_TEST(CustomTestClass, test2);
        }
        
        static void runGroup2Tests() {
            RUN_TEST(CustomTestClass, test3);
            RUN_TEST(CustomTestClass, test4);
        }
        
        static void runMemoryTests() {
            RUN_TEST(CustomTestClass, memoryTest1);
            RUN_TEST(CustomTestClass, memoryTest2);
        }
    };
    
    class CustomTestClass {
    public:
        static void test1() { /* 实现测试1 */ }
        static void test2() { /* 实现测试2 */ }
        static void test3() { /* 实现测试3 */ }
        static void test4() { /* 实现测试4 */ }
        static void memoryTest1() { /* 实现内存测试1 */ }
        static void memoryTest2() { /* 实现内存测试2 */ }
    };
    
    // 运行自定义测试套件
    RUN_TEST_SUITE(CustomTestSuite);
}