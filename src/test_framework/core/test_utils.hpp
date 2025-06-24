#ifndef LUA_TEST_FRAMEWORK_TEST_UTILS_HPP
#define LUA_TEST_FRAMEWORK_TEST_UTILS_HPP

#include "../formatting/format_formatter.hpp"
#include "../formatting/format_config.hpp"
#include "../formatting/format_define.hpp"
#include "test_memory.hpp"
#include "../../common/types.hpp"
#include <iostream>
#include <exception>

namespace Lua {
namespace TestFramework {

/**
 * @brief Test utility class - provides unified test assistance functionality
 * 
 * This is a simplified facade interface that delegates to the formatting module for specific output formatting.
 * Provides backward compatible interfaces and new hierarchical interfaces.
 */
class TestUtils {
private:
    /**
     * @brief Get formatter instance
     * @return Formatter reference
     */
    static Lua::Tests::TestFormatting::TestFormatter& getFormatter() {
        return Lua::Tests::TestFormatting::TestFormatter::getInstance();
    }

public:
    
    // ========== Backward Compatible Interface ==========
    
    /**
     * @brief Print standardized section header
     * @param sectionName Section name
     */
    static void printSectionHeader(const Str& sectionName) {
        getFormatter().printSectionHeader(sectionName);
    }

    /**
     * @brief Print standardized section footer, indicating completion
     */
    static void printSectionFooter() {
        getFormatter().printSectionFooter("Section completed");
    }

    /**
     * @brief Print simple section header (alternative style with equals format)
     * @param sectionName Section name
     */
    static void printSimpleSectionHeader(const Str& sectionName) {
        getFormatter().printSimpleSectionHeader(sectionName);
    }

    /**
     * @brief Print simple section footer (alternative style with equals format)
     * @param sectionName Completed section name
     */
    static void printSimpleSectionFooter(const Str& sectionName) {
        getFormatter().printSimpleSectionFooter(sectionName + " Completed");
    }

    /**
     * @brief Print consistently formatted test results
     * @param testName Test name
     * @param passed Whether the test passed
     */
    static void printTestResult(const Str& testName, bool passed) {
        getFormatter().printTestResult(testName, passed);
    }

    /**
     * @brief Print consistently formatted info message
     * @param message Info message
     */
    static void printInfo(const Str& message) {
        getFormatter().printInfo(message);
    }

    /**
     * @brief Print consistently formatted warning message
     * @param message Warning message
     */
    static void printWarning(const Str& message) {
        getFormatter().printWarning(message);
    }

    /**
     * @brief Print consistently formatted error message
     * @param message Error message
     */
    static void printError(const Str& message) {
        getFormatter().printError(message);
    }

    /**
     * @brief Print consistently formatted exception information
     * @param e Exception object
     * @param context Optional context information (such as test name, function name)
     */
    static void printException(const std::exception& e, const Str& context = "") {
        Str message = "Exception caught";
        if (!context.empty()) {
            message += " in " + context;
        }
        message += ": " + Str(e.what());
        getFormatter().printError(message);
    }

    /**
     * @brief Print consistently formatted unknown exception information
     * @param context Optional context information (such as test name, function name)
     */
    static void printUnknownException(const Str& context = "") {
        Str message = "Unknown exception caught";
        if (!context.empty()) {
            message += " in " + context;
        }
        getFormatter().printError(message);
    }
    
    // ========== New Hierarchical Interface ==========
    
    /**
     * @brief Print header for specific level
     * @param level Test level
     * @param title Title text
     */
    static void printLevelHeader(Lua::Tests::TestFormatting::TestLevel level, const Str& title) {
        getFormatter().printLevelHeader(level, title);
    }
    
    /**
     * @brief Print header for specific level
     * @param level Test level
     * @param title Title text
     * @param description Optional description
     */
    static void printLevelHeader(Lua::Tests::TestFormatting::TestLevel level, const Str& title, 
                                const Str& description) {
        getFormatter().printLevelHeader(level, title, description);
    }
    
    /**
     * @brief Print footer for specific level
     * @param level Test level
     * @param summary Optional summary text
     */
    static void printLevelFooter(Lua::Tests::TestFormatting::TestLevel level, const Str& summary = "") {
        getFormatter().printLevelFooter(level, summary);
    }
    
    // ========== Configuration Interface ==========
    
    /**
     * @brief Enable or disable color output
     * @param enabled Whether to enable color
     */
    static void setColorEnabled(bool enabled) {
        getFormatter().setColorEnabled(enabled);
    }
    
    /**
     * @brief Set color theme
     * @param theme Theme name
     */
    static void setTheme(const Str& theme) {
        getFormatter().setTheme(theme);
    }
    
    /**
     * @brief Get configuration object
     * @return Configuration object reference
     */
    static Lua::Tests::TestFormatting::TestConfig& getConfig() {
        return getFormatter().getConfig();
    }
    
    // ========== Memory Test Tools ==========
    
    /**
     * @brief Start memory leak detection
     * @param testName Test name
     */
    static void startMemoryCheck(const Str& testName) {
        MemoryTestUtils::startMemoryCheck(testName);
    }
    
    /**
     * @brief End memory leak detection
     * @param testName Test name
     * @return Whether memory leak was detected
     */
    static bool endMemoryCheck(const Str& testName) {
        return MemoryTestUtils::endMemoryCheck(testName);
    }
    
    /**
     * @brief Check if memory detection is enabled
     * @return Whether memory detection is enabled
     */
    static bool isMemoryCheckEnabled() {
        return MemoryTestUtils::isEnabled();
    }
    
    /**
     * @brief Set memory detection switch
     * @param enabled Whether to enable memory detection
     */
    static void setMemoryCheckEnabled(bool enabled) {
        MemoryTestUtils::setEnabled(enabled);
    }
    
    // ========== Statistics and Reports ==========
    
    /**
     * @brief Test statistics structure
     */
    struct TestStatistics {
        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;
        int skippedTests = 0;
        
        double getPassRate() const {
            return totalTests > 0 ? (double)passedTests / totalTests * 100.0 : 0.0;
        }
        
        bool allPassed() const {
            return totalTests > 0 && failedTests == 0;
        }
    };
};

} // namespace TestFramework
} // namespace Lua

#endif // LUA_TEST_FRAMEWORK_TEST_UTILS_HPP