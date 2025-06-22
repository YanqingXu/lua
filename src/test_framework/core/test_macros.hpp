#ifndef LUA_TEST_FRAMEWORK_TEST_MACROS_HPP
#define LUA_TEST_FRAMEWORK_TEST_MACROS_HPP

#include "test_utils.hpp"
#include "test_memory.hpp"
#include "../formatting/format_formatter.hpp"
#include <exception>
#include <iostream>
#include <memory>

namespace Lua {
namespace TestFramework {

/**
 * @brief 测试级别枚举
 */
enum class TestLevel {
    MAIN,      // 主测试套件（顶级）
    MODULE,    // 模块级别（如 parser, lexer, vm）
    SUITE,     // 测试套件（如 ExprTestSuite, StmtTestSuite）
    GROUP,     // 测试组（如 BinaryExprTest, UnaryExprTest）
    INDIVIDUAL // 单个测试用例
};

} // namespace TestFramework
} // namespace Lua

/**
 * @brief 运行单个测试方法的宏（INDIVIDUAL级别）
 * 
 * 用法: RUN_TEST(TestClass, TestMethod)
 * 示例: RUN_TEST(BinaryExprTest, testAddition)
 * 
 * 此宏用于在测试组内运行单个测试用例。
 * 提供异常处理和结果报告功能。
 * 包含自动内存泄漏检测。
 */
#define RUN_TEST(TestClass, TestMethod) \
    do { \
        MEMORY_LEAK_TEST_GUARD(#TestClass "::" #TestMethod); \
        try { \
            Lua::TestFramework::TestUtils::printInfo("Running " #TestClass "::" #TestMethod "..."); \
            TestClass::TestMethod(); \
            Lua::TestFramework::TestUtils::printTestResult(#TestClass "::" #TestMethod, true); \
        } catch (const std::exception& e) { \
            Lua::TestFramework::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::TestFramework::TestUtils::printException(e, #TestClass "::" #TestMethod); \
            throw; \
        } catch (...) { \
            Lua::TestFramework::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::TestFramework::TestUtils::printUnknownException(#TestClass "::" #TestMethod); \
            throw; \
        } \
    } while(0)

/**
 * @brief 运行主测试的宏（MAIN级别）
 * 
 * 用法: RUN_MAIN_TEST(TestName, TestFunction)
 * 示例: RUN_MAIN_TEST("All Tests", runAllTests)
 * 
 * 这是运行整个测试套件的最高级别宏。
 * 只应在主测试入口点使用。
 */
#define RUN_MAIN_TEST(TestName, TestFunction) \
    do { \
        try { \
            Lua::TestFramework::TestUtils::printLevelHeader(Lua::Tests::TestFormatting::TestLevel::MAIN, TestName, "Running complete test suite"); \
            TestFunction(); \
            Lua::TestFramework::TestUtils::printLevelFooter(Lua::Tests::TestFormatting::TestLevel::MAIN, "All tests completed successfully"); \
        } catch (const std::exception& e) { \
            Lua::TestFramework::TestUtils::printException(e, "Main test"); \
            throw; \
        } catch (...) { \
            Lua::TestFramework::TestUtils::printUnknownException("Main test"); \
            throw; \
        } \
    } while(0)

/**
 * @brief 运行模块测试的宏（MODULE级别）
 * 
 * 用法: RUN_TEST_MODULE(ModuleName, ModuleTestClass)
 * 示例: RUN_TEST_MODULE("Parser Module", ParserTestSuite)
 * 
 * 此宏用于运行特定模块的测试（如 parser, lexer, vm）。
 * 提供不同功能模块之间的清晰分离。
 */
#define RUN_TEST_MODULE(ModuleName, ModuleTestClass) \
    do { \
        try { \
            Lua::TestFramework::TestUtils::printLevelHeader(Lua::Tests::TestFormatting::TestLevel::MODULE, ModuleName, "Running module tests"); \
            ModuleTestClass::runAllTests(); \
            Lua::TestFramework::TestUtils::printLevelFooter(Lua::Tests::TestFormatting::TestLevel::MODULE, ModuleName " module tests completed successfully"); \
        } catch (const std::exception& e) { \
            Lua::TestFramework::TestUtils::printException(e, ModuleName " module"); \
            throw; \
        } catch (...) { \
            Lua::TestFramework::TestUtils::printUnknownException(ModuleName " module"); \
            throw; \
        } \
    } while(0)

/**
 * @brief 运行测试套件的宏（SUITE级别）
 * 
 * 用法: RUN_TEST_SUITE(TestSuiteName)
 * 示例: RUN_TEST_SUITE(ExprTestSuite)
 * 
 * 此宏用于运行模块内的特定测试套件。
 * 测试套件将相关功能测试分组（如表达式测试、语句测试）。
 * 应在模块级测试类内调用。
 */
#define RUN_TEST_SUITE(TestSuite) \
    do { \
        try { \
            Lua::TestFramework::TestUtils::printLevelHeader(Lua::Tests::TestFormatting::TestLevel::SUITE, #TestSuite " Test Suite"); \
            TestSuite::runAllTests(); \
            Lua::TestFramework::TestUtils::printLevelFooter(Lua::Tests::TestFormatting::TestLevel::SUITE, #TestSuite " tests completed successfully"); \
        } catch (const std::exception& e) { \
            Lua::TestFramework::TestUtils::printException(e, #TestSuite " test suite"); \
            throw; \
        } catch (...) { \
            Lua::TestFramework::TestUtils::printUnknownException(#TestSuite " test suite"); \
            throw; \
        } \
    } while(0)

/**
 * @brief 运行测试组的宏（GROUP级别）
 * 
 * 用法: RUN_TEST_GROUP(GroupName, GroupFunction)
 * 示例: RUN_TEST_GROUP("Binary Expression Tests", testBinaryExpressions)
 * 
 * 此宏用于运行测试套件内的相关测试组。
 * 测试组按特定功能或特性区域组织测试。
 * 应在测试套件类内调用。
 */
#define RUN_TEST_GROUP(GroupName, GroupFunction) \
    do { \
        try { \
            Lua::TestFramework::TestUtils::printLevelHeader(Lua::Tests::TestFormatting::TestLevel::GROUP, GroupName); \
            GroupFunction(); \
            Lua::TestFramework::TestUtils::printLevelFooter(Lua::Tests::TestFormatting::TestLevel::GROUP, GroupName " completed"); \
        } catch (const std::exception& e) { \
            Lua::TestFramework::TestUtils::printException(e, GroupName); \
            throw; \
        } catch (...) { \
            Lua::TestFramework::TestUtils::printUnknownException(GroupName); \
            throw; \
        } \
    } while(0)

/**
 * @brief 带内存检查的测试组宏
 * 
 * 用法: RUN_TEST_GROUP_WITH_MEMORY_CHECK(GroupName, GroupFunction)
 * 
 * 在测试组执行前后进行内存检查，确保没有内存泄漏。
 */
#define RUN_TEST_GROUP_WITH_MEMORY_CHECK(GroupName, GroupFunction) \
    do { \
        MEMORY_LEAK_TEST_GUARD(GroupName); \
        try { \
            Lua::TestFramework::TestUtils::printLevelHeader(Lua::Tests::TestFormatting::TestLevel::GROUP, GroupName); \
            GroupFunction(); \
            Lua::TestFramework::TestUtils::printLevelFooter(Lua::Tests::TestFormatting::TestLevel::GROUP, GroupName " completed"); \
        } catch (const std::exception& e) { \
            Lua::TestFramework::TestUtils::printException(e, GroupName); \
            throw; \
        } catch (...) { \
            Lua::TestFramework::TestUtils::printUnknownException(GroupName); \
            throw; \
        } \
    } while(0)

/**
 * @brief 带内存检查的测试套件宏
 * 
 * 用法: RUN_TEST_SUITE_WITH_MEMORY_CHECK(TestSuite)
 * 
 * 在测试套件执行前后进行内存检查。
 */
#define RUN_TEST_SUITE_WITH_MEMORY_CHECK(TestSuite) \
    do { \
        MEMORY_LEAK_TEST_GUARD(#TestSuite); \
        try { \
            Lua::TestFramework::TestUtils::printLevelHeader(Lua::Tests::TestFormatting::TestLevel::SUITE, #TestSuite " Test Suite"); \
            TestSuite::runAllTests(); \
            Lua::TestFramework::TestUtils::printLevelFooter(Lua::Tests::TestFormatting::TestLevel::SUITE, #TestSuite " tests completed successfully"); \
        } catch (const std::exception& e) { \
            Lua::TestFramework::TestUtils::printException(e, #TestSuite " test suite"); \
            throw; \
        } catch (...) { \
            Lua::TestFramework::TestUtils::printUnknownException(#TestSuite " test suite"); \
            throw; \
        } \
    } while(0)

/**
 * @brief 安全的单个测试执行宏（INDIVIDUAL级别）
 * 
 * 用法: RUN_SAFE_TEST(TestName, TestFunction)
 * 
 * 提供更安全的测试执行，包含完整的异常处理和内存检查。
 */
#define RUN_SAFE_TEST(TestName, TestFunction) \
    do { \
        MEMORY_LEAK_TEST_GUARD(TestName); \
        try { \
            Lua::TestFramework::TestUtils::printInfo("Running " TestName "..."); \
            TestFunction(); \
            Lua::TestFramework::TestUtils::printTestResult(TestName, true); \
        } catch (const std::exception& e) { \
            Lua::TestFramework::TestUtils::printTestResult(TestName, false); \
            Lua::TestFramework::TestUtils::printException(e, TestName); \
            throw; \
        } catch (...) { \
            Lua::TestFramework::TestUtils::printTestResult(TestName, false); \
            Lua::TestFramework::TestUtils::printUnknownException(TestName); \
            throw; \
        } \
    } while(0)

/**
 * @brief 内存泄漏检测宏
 * 
 * 用法: MEMORY_LEAK_TEST_GUARD("TestName")
 * 在作用域内自动进行内存泄漏检测。
 */
#ifndef MEMORY_LEAK_TEST_GUARD
#define MEMORY_LEAK_TEST_GUARD(testName) \
    Lua::TestFramework::MemoryTestUtils::MemoryGuard memoryGuard(testName)
#endif

/**
 * @brief 带超时的内存检测宏
 * 
 * 用法: MEMORY_LEAK_TEST_GUARD_WITH_TIMEOUT("TestName", 5000)
 * 
 * 在指定超时时间内进行内存泄漏检测。
 */
#define MEMORY_LEAK_TEST_GUARD_WITH_TIMEOUT(testName, timeoutMs) \
    do { \
        int oldTimeout = Lua::TestFramework::MemoryTestUtils::getTimeoutMs(); \
        Lua::TestFramework::MemoryTestUtils::setTimeoutMs(timeoutMs); \
        Lua::TestFramework::MemoryTestUtils::MemoryGuard memoryGuard(testName); \
        Lua::TestFramework::MemoryTestUtils::setTimeoutMs(oldTimeout); \
    } while(0)

/**
 * @brief 条件内存检测宏
 * 
 * 用法: CONDITIONAL_MEMORY_LEAK_TEST_GUARD(condition, "TestName")
 * 
 * 只有在条件为真时才进行内存检测。
 */
#define CONDITIONAL_MEMORY_LEAK_TEST_GUARD(condition, testName) \
    std::unique_ptr<Lua::TestFramework::MemoryTestUtils::MemoryGuard> memoryGuard; \
    if (condition) { \
        memoryGuard = std::make_unique<Lua::TestFramework::MemoryTestUtils::MemoryGuard>(testName); \
    }

/**
 * @brief 内存使用情况报告宏
 * 
 * 用法: MEMORY_USAGE_REPORT("TestPhase")
 * 
 * 打印当前内存使用情况。
 */
#define MEMORY_USAGE_REPORT(phase) \
    do { \
        if (Lua::TestFramework::MemoryTestUtils::isEnabled()) { \
            size_t usage = Lua::TestFramework::MemoryTestUtils::getCurrentMemoryUsage(); \
            std::cout << "[MEMORY] " << phase << ": " << usage << " bytes" << std::endl; \
        } \
    } while(0)

/**
 * @brief 全局便捷宏定义
 * 
 * 这些宏提供了更简洁的测试框架使用方式
 */

// 快速启动测试框架
#define INIT_TEST_FRAMEWORK() \
    do { \
        Lua::TestFramework::TestUtils::setColorEnabled(true); \
        Lua::TestFramework::TestUtils::setTheme("modern"); \
    } while(0)

// 运行所有测试的便捷宏
#define RUN_ALL_TESTS(TestSuiteClass) \
    do { \
        INIT_TEST_FRAMEWORK(); \
        Lua::TestFramework::TestUtils::printLevelHeader(Lua::Tests::TestFormatting::TestLevel::MAIN, "Starting All Tests"); \
        RUN_TEST_SUITE(TestSuiteClass); \
        Lua::TestFramework::TestUtils::printLevelFooter(Lua::Tests::TestFormatting::TestLevel::MAIN, "All Tests Completed"); \
    } while(0)

// 运行模块测试的便捷宏
#define RUN_MODULE_TESTS(ModuleName, TestSuiteClass) \
    do { \
        INIT_TEST_FRAMEWORK(); \
        Lua::TestFramework::TestUtils::printLevelHeader(Lua::Tests::TestFormatting::TestLevel::MODULE, "Module: " #ModuleName); \
        RUN_TEST_SUITE(TestSuiteClass); \
        Lua::TestFramework::TestUtils::printLevelFooter(Lua::Tests::TestFormatting::TestLevel::MODULE, "Module " #ModuleName " Completed"); \
    } while(0)

// 快速测试宏（用于CI/CD）
#define RUN_QUICK_TESTS(TestSuiteClass) \
    do { \
        Lua::TestFramework::TestUtils::setColorEnabled(false); \
        Lua::TestFramework::TestUtils::printInfo("Running Quick Tests..."); \
        RUN_TEST_SUITE(TestSuiteClass); \
        Lua::TestFramework::TestUtils::printInfo("Quick Tests Completed"); \
    } while(0)

// 内存安全测试宏
#define RUN_MEMORY_SAFE_TESTS(TestSuiteClass) \
    do { \
        INIT_TEST_FRAMEWORK(); \
        Lua::TestFramework::TestUtils::printLevelHeader(Lua::Tests::TestFormatting::TestLevel::MAIN, "Memory Safe Tests"); \
        MEMORY_LEAK_TEST_GUARD("Full Test Suite"); \
        RUN_TEST_SUITE(TestSuiteClass); \
        Lua::TestFramework::TestUtils::printLevelFooter(Lua::Tests::TestFormatting::TestLevel::MAIN, "Memory Safe Tests Completed"); \
    } while(0)

// 全局初始化宏
#define INIT_LUA_TEST_FRAMEWORK() Lua::TestFramework::Initializer::initialize()
#define QUICK_INIT_LUA_TEST_FRAMEWORK() Lua::TestFramework::Initializer::quickInit()

#endif // LUA_TEST_FRAMEWORK_TEST_MACROS_HPP