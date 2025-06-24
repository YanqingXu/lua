#ifndef LUA_TEST_FRAMEWORK_TEST_MACROS_HPP
#define LUA_TEST_FRAMEWORK_TEST_MACROS_HPP

#include "test_utils.hpp"
#include "test_memory.hpp"
#include "../formatting/format_formatter.hpp"
#include "../../common/types.hpp"
#include <exception>
#include <iostream>

namespace Lua {
namespace TestFramework {

/**
 * @brief Test level enumeration
 */
// TestLevel definition has been moved to formatting/format_define.hpp
// Use Lua::Tests::TestFormatting::TestLevel

} // namespace TestFramework
} // namespace Lua

/**
 * @brief Macro for running individual test methods (INDIVIDUAL level)
 * 
 * Usage: RUN_TEST(TestClass, TestMethod)
 * Example: RUN_TEST(BinaryExprTest, testAddition)
 * 
 * This macro is used to run individual test cases within a test group.
 * Provides exception handling and result reporting functionality.
 * Includes automatic memory leak detection.
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
 * @brief Macro for running main tests (MAIN level)
 * 
 * Usage: RUN_MAIN_TEST(TestName, TestFunction)
 * Example: RUN_MAIN_TEST("All Tests", runAllTests)
 * 
 * This is the highest level macro for running the entire test suite.
 * Should only be used at the main test entry point.
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
 * @brief Macro for running module tests (MODULE level)
 * 
 * Usage: RUN_TEST_MODULE(ModuleName, ModuleTestClass)
 * Example: RUN_TEST_MODULE("Parser Module", ParserTestSuite)
 * 
 * This macro is used to run tests for specific modules (such as parser, lexer, vm).
 * Provides clear separation between different functional modules.
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
 * @brief Macro for running test suites (SUITE level)
 * 
 * Usage: RUN_TEST_SUITE(TestSuiteName)
 * Example: RUN_TEST_SUITE(ExprTestSuite)
 * 
 * This macro is used to run specific test suites within a module.
 * Test suites group related functionality tests (such as expression tests, statement tests).
 * Should be called within module-level test classes.
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
 * @brief Macro for running test groups (GROUP level)
 * 
 * Usage: RUN_TEST_GROUP(GroupName, GroupFunction)
 * Example: RUN_TEST_GROUP("Binary Expression Tests", testBinaryExpressions)
 * 
 * This macro is used to run related test groups within a test suite.
 * Test groups organize tests by specific functionality or feature areas.
 * Should be called within test suite classes.
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
 * @brief Safe test execution macro with memory leak detection and timeout (INDIVIDUAL level)
 * 
 * Usage: SAFE_RUN_TEST(TestClass, TestMethod, timeoutMs)
 * Example: SAFE_RUN_TEST(BinaryExprTest, testAddition, 5000)
 * 
 * This macro provides comprehensive test execution with:
 * - Automatic memory leak detection
 * - Timeout monitoring
 * - Complete exception handling
 * - Test result reporting
 * 
 * @param TestClass The test class containing the test method
 * @param TestMethod The test method to execute
 * @param timeoutMs Timeout in milliseconds (0 for no timeout)
 */
#define SAFE_RUN_TEST(TestClass, TestMethod, timeoutMs) \
    do { \
        int oldTimeout = Lua::TestFramework::MemoryTestUtils::getTimeoutMs(); \
        if (timeoutMs > 0) { \
            Lua::TestFramework::MemoryTestUtils::setTimeoutMs(timeoutMs); \
        } \
        MEMORY_LEAK_TEST_GUARD(#TestClass "::" #TestMethod); \
        try { \
            Lua::TestFramework::TestUtils::printInfo("Running " #TestClass "::" #TestMethod " (timeout: " + std::to_string(timeoutMs > 0 ? timeoutMs : oldTimeout) + "ms)..."); \
            TestClass::TestMethod(); \
            Lua::TestFramework::TestUtils::printTestResult(#TestClass "::" #TestMethod, true); \
            if (Lua::TestFramework::MemoryTestUtils::hasTimeoutOccurred()) { \
                Lua::TestFramework::TestUtils::printWarning("Timeout detected during test execution: " #TestClass "::" #TestMethod); \
            } \
        } catch (const std::exception& e) { \
            Lua::TestFramework::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::TestFramework::TestUtils::printException(e, #TestClass "::" #TestMethod); \
            if (timeoutMs > 0) { \
                Lua::TestFramework::MemoryTestUtils::setTimeoutMs(oldTimeout); \
            } \
            throw; \
        } catch (...) { \
            Lua::TestFramework::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::TestFramework::TestUtils::printUnknownException(#TestClass "::" #TestMethod); \
            if (timeoutMs > 0) { \
                Lua::TestFramework::MemoryTestUtils::setTimeoutMs(oldTimeout); \
            } \
            throw; \
        } \
        if (timeoutMs > 0) { \
            Lua::TestFramework::MemoryTestUtils::setTimeoutMs(oldTimeout); \
        } \
    } while(0)

/**
 * @brief Memory leak detection macro
 * 
 * Usage: MEMORY_LEAK_TEST_GUARD("TestName")
 * Automatically performs memory leak detection within the scope.
 */
#ifndef MEMORY_LEAK_TEST_GUARD
#define MEMORY_LEAK_TEST_GUARD(testName) \
    Lua::TestFramework::MemoryTestUtils::MemoryGuard memoryGuard(testName)
#endif

#endif // LUA_TEST_FRAMEWORK_TEST_MACROS_HPP