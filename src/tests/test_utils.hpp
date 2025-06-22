#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include "formatting/test_formatter.hpp"
#include "formatting/test_config.hpp"
#include "../common/memory_leak_detector.hpp"
#include "../common/timeout_memory_detector.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

/**
 * Test utility functions for consistent formatting across all test suites
 * This is a simplified facade interface that delegates to the formatting module
 */
class TestUtils {
private:
    static TestFormatting::TestFormatter& getFormatter() {
        return TestFormatting::TestFormatter::getInstance();
    }

public:
    // Type aliases for convenience
    using TestLevel = TestFormatting::TestLevel;
    
    // Backward compatible interface
    /**
     * Print a standardized section header with the given section name
     * @param sectionName The name of the test section
     */
    static void printSectionHeader(const std::string& sectionName) {
        getFormatter().printSectionHeader(sectionName);
    }

    /**
     * Print a standardized section footer indicating completion
     */
    static void printSectionFooter() {
        getFormatter().printSectionFooter("Section completed");
    }

    /**
     * Print a simple section header with equals formatting (alternative style)
     * @param sectionName The name of the test section
     */
    static void printSimpleSectionHeader(const std::string& sectionName) {
        getFormatter().printSimpleSectionHeader(sectionName);
    }

    /**
     * Print a simple section footer with equals formatting (alternative style)
     * @param sectionName The name of the completed section
     */
    static void printSimpleSectionFooter(const std::string& sectionName) {
        getFormatter().printSimpleSectionFooter(sectionName + " Completed");
    }

    /**
     * Print a test result with consistent formatting
     * @param testName The name of the test
     * @param passed Whether the test passed
     */
    static void printTestResult(const std::string& testName, bool passed) {
        getFormatter().printTestResult(testName, passed);
    }

    /**
     * Print an informational message with consistent formatting
     * @param message The information message
     */
    static void printInfo(const std::string& message) {
        getFormatter().printInfo(message);
    }

    /**
     * Print a warning message with consistent formatting
     * @param message The warning message
     */
    static void printWarning(const std::string& message) {
        getFormatter().printWarning(message);
    }

    /**
     * Print an error message with consistent formatting
     * @param message The error message
     */
    static void printError(const std::string& message) {
        getFormatter().printError(message);
    }

    /**
     * Print exception information with consistent formatting
     * @param e The exception object
     * @param context Optional context information (e.g., test name, function name)
     */
    static void printException(const std::exception& e, const std::string& context = "") {
        std::string message = "Exception caught";
        if (!context.empty()) {
            message += " in " + context;
        }
        message += ": " + std::string(e.what());
        getFormatter().printError(message);
    }

    /**
     * Print unknown exception information with consistent formatting
     * @param context Optional context information (e.g., test name, function name)
     */
    static void printUnknownException(const std::string& context = "") {
        std::string message = "Unknown exception caught";
        if (!context.empty()) {
            message += " in " + context;
        }
        getFormatter().printError(message);
    }
    
    // New hierarchical interface
    /**
     * Print a level-specific header
     * @param level The test level
     * @param title The title text
     * @param description Optional description
     */
    static void printLevelHeader(TestLevel level, const std::string& title, 
                                const std::string& description = "") {
        getFormatter().printLevelHeader(level, title, description);
    }
    
    /**
     * Print a level-specific footer
     * @param level The test level
     * @param summary Optional summary text
     */
    static void printLevelFooter(TestLevel level, const std::string& summary = "") {
        getFormatter().printLevelFooter(level, summary);
    }
    
    // Configuration interface
    /**
     * Enable or disable color output
     * @param enabled Whether to enable colors
     */
    static void setColorEnabled(bool enabled) {
        getFormatter().setColorEnabled(enabled);
    }
    
    /**
     * Set the color theme
     * @param theme The theme name
     */
    static void setTheme(const std::string& theme) {
        getFormatter().setTheme(theme);
    }
    
    /**
     * Load configuration from file
     * @param filename The configuration file path
     */
    static void loadConfig(const std::string& filename) {
        getFormatter().getConfig().loadFromFile(filename);
    }
    
    /**
     * Get the formatter instance for advanced usage
     * @return Reference to the formatter
     */
    static TestFormatting::TestFormatter& getFormatterInstance() {
        return getFormatter();
    }
};

} // namespace Tests
} // namespace Lua

/**
 * Macro for individual test execution (individual level - INDIVIDUAL)
 * Usage: RUN_TEST(ClassName, methodName)
 * Example: RUN_TEST(BinaryExprTest, testAddition)
 * 
 * This macro is used for running individual test cases within a test group.
 * It provides exception handling and result reporting for single test methods.
 * Should be called from within test group functions.
 * Includes automatic memory leak detection.
 */
#define RUN_TEST(TestClass, TestMethod) \
    do { \
        MEMORY_LEAK_TEST_GUARD(#TestClass "::" #TestMethod); \
        try { \
            Lua::Tests::TestUtils::printInfo("Running " #TestClass "::" #TestMethod "..."); \
            Lua::Tests::TestClass::TestMethod(); \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, true); \
        } catch (const std::exception& e) { \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::Tests::TestUtils::printException(e, #TestClass "::" #TestMethod); \
            throw; \
        } catch (...) { \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::Tests::TestUtils::printUnknownException(#TestClass "::" #TestMethod); \
            throw; \
        } \
    } while(0)

/**
 * Macro for main test execution (top level - MAIN)
 * Usage: RUN_MAIN_TEST(TestName, TestFunction)
 * Example: RUN_MAIN_TEST("All Tests", runAllTests)
 * 
 * This is the highest level macro for running the entire test suite.
 * It should only be used in the main test entry point.
 */
#define RUN_MAIN_TEST(TestName, TestFunction) \
    do { \
        try { \
            Lua::Tests::TestUtils::printLevelHeader(Lua::Tests::TestUtils::TestLevel::MAIN, TestName, "Running complete test suite"); \
            TestFunction(); \
            Lua::Tests::TestUtils::printLevelFooter(Lua::Tests::TestUtils::TestLevel::MAIN, "All tests completed successfully"); \
        } catch (const std::exception& e) { \
            Lua::Tests::TestUtils::printException(e, "Main test"); \
            throw; \
        } catch (...) { \
            Lua::Tests::TestUtils::printUnknownException("Main test"); \
            throw; \
        } \
    } while(0)

/**
 * Macro for module test execution (module level - MODULE)
 * Usage: RUN_TEST_MODULE(ModuleName, ModuleTestClass)
 * Example: RUN_TEST_MODULE("Parser Module", ParserTestSuite)
 * 
 * This macro is used for running tests of a specific module (e.g., parser, lexer, vm).
 * It provides clear separation between different functional modules.
 */
#define RUN_TEST_MODULE(ModuleName, ModuleTestClass) \
    do { \
        try { \
            Lua::Tests::TestUtils::printLevelHeader(Lua::Tests::TestUtils::TestLevel::MODULE, ModuleName, "Running module tests"); \
            Lua::Tests::ModuleTestClass::runAllTests(); \
            Lua::Tests::TestUtils::printLevelFooter(Lua::Tests::TestUtils::TestLevel::MODULE, ModuleName " module tests completed successfully"); \
        } catch (const std::exception& e) { \
            Lua::Tests::TestUtils::printException(e, ModuleName " module"); \
            throw; \
        } catch (...) { \
            Lua::Tests::TestUtils::printUnknownException(ModuleName " module"); \
            throw; \
        } \
    } while(0)

/**
 * Macro for test suite execution (suite level - SUITE)
 * Usage: RUN_TEST_SUITE(TestSuiteName)
 * Example: RUN_TEST_SUITE(ExprTestSuite)
 * 
 * This macro is used for running a specific test suite within a module.
 * Test suites group related functionality tests (e.g., expression tests, statement tests).
 * Should be called from within module-level test classes.
 */
#define RUN_TEST_SUITE(TestSuite) \
    do { \
        try { \
            Lua::Tests::TestUtils::printLevelHeader(Lua::Tests::TestUtils::TestLevel::SUITE, #TestSuite " Test Suite"); \
            Lua::Tests::TestSuite::runAllTests(); \
            Lua::Tests::TestUtils::printLevelFooter(Lua::Tests::TestUtils::TestLevel::SUITE, #TestSuite " tests completed successfully"); \
        } catch (const std::exception& e) { \
            Lua::Tests::TestUtils::printException(e, #TestSuite " test suite"); \
            throw; \
        } catch (...) { \
            Lua::Tests::TestUtils::printUnknownException(#TestSuite " test suite"); \
            throw; \
        } \
    } while(0)

/**
 * Macro for test group execution (group level - GROUP)
 * Usage: RUN_TEST_GROUP(GroupName, GroupFunction)
 * Example: RUN_TEST_GROUP("Binary Expression Tests", testBinaryExpressions)
 * 
 * This macro is used for running a group of related tests within a test suite.
 * Test groups organize tests by specific functionality or feature area.
 * Should be called from within test suite classes.
 */
#define RUN_TEST_GROUP(GroupName, GroupFunction) \
    do { \
        try { \
            Lua::Tests::TestUtils::printLevelHeader(Lua::Tests::TestUtils::TestLevel::GROUP, GroupName); \
            GroupFunction(); \
            Lua::Tests::TestUtils::printLevelFooter(Lua::Tests::TestUtils::TestLevel::GROUP, GroupName " completed"); \
        } catch (const std::exception& e) { \
            Lua::Tests::TestUtils::printException(e, GroupName); \
            throw; \
        } catch (...) { \
            Lua::Tests::TestUtils::printUnknownException(GroupName); \
            throw; \
        } \
    } while(0)

/**
 * Macro for safe individual test execution (individual level - INDIVIDUAL)
 * Usage: SAFE_RUN_TEST(ClassName, methodName)
 * Example: SAFE_RUN_TEST(BinaryExprTest, testAddition)
 * 
 * This is the safe version of RUN_TEST that catches exceptions and continues execution
 * instead of re-throwing. Useful for running multiple tests where one failure
 * shouldn't stop the entire test run. Should be called from within test group functions.
 * Includes automatic memory leak detection.
 */
#define SAFE_RUN_TEST(TestClass, TestMethod) \
    do { \
        MEMORY_LEAK_TEST_GUARD(#TestClass "::" #TestMethod); \
        try { \
            Lua::Tests::TestUtils::printInfo("Running " #TestClass "::" #TestMethod "..."); \
            Lua::Tests::TestClass::TestMethod(); \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, true); \
        } catch (const std::exception& e) { \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::Tests::TestUtils::printException(e, #TestClass "::" #TestMethod); \
        } catch (...) { \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::Tests::TestUtils::printUnknownException(#TestClass "::" #TestMethod); \
        } \
    } while(0)

/**
 * Macro for comprehensive test execution with memory leak, timeout, and deadlock detection
 * Usage: RUN_COMPREHENSIVE_TEST(ClassName, methodName, timeoutMs)
 * Example: RUN_COMPREHENSIVE_TEST(BinaryExprTest, testAddition, 5000)
 * 
 * This macro provides the most comprehensive testing with all detection features enabled.
 * Includes memory leak detection, timeout detection, deadlock detection, and recursion detection.
 */
#define RUN_COMPREHENSIVE_TEST(TestClass, TestMethod, timeoutMs) \
    do { \
        COMPREHENSIVE_TEST_GUARD(#TestClass "::" #TestMethod, timeoutMs); \
        try { \
            Lua::Tests::TestUtils::printInfo("Running comprehensive test " #TestClass "::" #TestMethod "..."); \
            Lua::Tests::TestClass::TestMethod(); \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, true); \
        } catch (const std::exception& e) { \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::Tests::TestUtils::printException(e, #TestClass "::" #TestMethod); \
            throw; \
        } catch (...) { \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::Tests::TestUtils::printUnknownException(#TestClass "::" #TestMethod); \
            throw; \
        } \
    } while(0)

/**
 * Macro for comprehensive test execution with default timeout
 * Usage: RUN_COMPREHENSIVE_TEST_DEFAULT(ClassName, methodName)
 * Example: RUN_COMPREHENSIVE_TEST_DEFAULT(BinaryExprTest, testAddition)
 * 
 * Uses default 30-second timeout for comprehensive testing.
 */
#define RUN_COMPREHENSIVE_TEST_DEFAULT(TestClass, TestMethod) \
    RUN_COMPREHENSIVE_TEST(TestClass, TestMethod, 30000)

/**
 * Macro for safe comprehensive test execution
 * Usage: SAFE_RUN_COMPREHENSIVE_TEST(ClassName, methodName, timeoutMs)
 * Example: SAFE_RUN_COMPREHENSIVE_TEST(BinaryExprTest, testAddition, 5000)
 * 
 * Safe version that doesn't re-throw exceptions, allowing test suite to continue.
 */
#define SAFE_RUN_COMPREHENSIVE_TEST(TestClass, TestMethod, timeoutMs) \
    do { \
        COMPREHENSIVE_TEST_GUARD(#TestClass "::" #TestMethod, timeoutMs); \
        try { \
            Lua::Tests::TestUtils::printInfo("Running comprehensive test " #TestClass "::" #TestMethod "..."); \
            Lua::Tests::TestClass::TestMethod(); \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, true); \
        } catch (const std::exception& e) { \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::Tests::TestUtils::printException(e, #TestClass "::" #TestMethod); \
        } catch (...) { \
            Lua::Tests::TestUtils::printTestResult(#TestClass "::" #TestMethod, false); \
            Lua::Tests::TestUtils::printUnknownException(#TestClass "::" #TestMethod); \
        } \
    } while(0)

/**
 * Macro for test group execution with memory leak detection
 * Usage: RUN_TEST_GROUP_WITH_MEMORY_CHECK(GroupName, GroupFunction)
 * Example: RUN_TEST_GROUP_WITH_MEMORY_CHECK("Binary Expression Tests", testBinaryExpressions)
 * 
 * Enhanced version of RUN_TEST_GROUP that includes memory leak detection for the entire group.
 */
#define RUN_TEST_GROUP_WITH_MEMORY_CHECK(GroupName, GroupFunction) \
    do { \
        MEMORY_LEAK_TEST_GUARD(GroupName); \
        try { \
            Lua::Tests::TestUtils::printLevelHeader(Lua::Tests::TestUtils::TestLevel::GROUP, GroupName); \
            GroupFunction(); \
            Lua::Tests::TestUtils::printLevelFooter(Lua::Tests::TestUtils::TestLevel::GROUP, GroupName " completed"); \
        } catch (const std::exception& e) { \
            Lua::Tests::TestUtils::printException(e, GroupName); \
            throw; \
        } catch (...) { \
            Lua::Tests::TestUtils::printUnknownException(GroupName); \
            throw; \
        } \
    } while(0)

/**
 * Macro for test suite execution with memory leak detection
 * Usage: RUN_TEST_SUITE_WITH_MEMORY_CHECK(TestSuiteName)
 * Example: RUN_TEST_SUITE_WITH_MEMORY_CHECK(ExprTestSuite)
 * 
 * Enhanced version of RUN_TEST_SUITE that includes memory leak detection for the entire suite.
 */
#define RUN_TEST_SUITE_WITH_MEMORY_CHECK(TestSuite) \
    do { \
        MEMORY_LEAK_TEST_GUARD(#TestSuite " Test Suite"); \
        try { \
            Lua::Tests::TestUtils::printLevelHeader(Lua::Tests::TestUtils::TestLevel::SUITE, #TestSuite " Test Suite"); \
            Lua::Tests::TestSuite::runAllTests(); \
            Lua::Tests::TestUtils::printLevelFooter(Lua::Tests::TestUtils::TestLevel::SUITE, #TestSuite " tests completed successfully"); \
        } catch (const std::exception& e) { \
            Lua::Tests::TestUtils::printException(e, #TestSuite " test suite"); \
            throw; \
        } catch (...) { \
            Lua::Tests::TestUtils::printUnknownException(#TestSuite " test suite"); \
            throw; \
        } \
    } while(0)

#endif // TEST_UTILS_HPP