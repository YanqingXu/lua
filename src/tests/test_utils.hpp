#ifndef TEST_UTILS_HPP
#define TEST_UTILS_HPP

#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

/**
 * Test utility functions for consistent formatting across all test suites
 */
class TestUtils {
public:
    /**
     * Print a standardized section header with the given section name
     * @param sectionName The name of the test section
     */
    static void printSectionHeader(const std::string& sectionName) {
        std::cout << "\n" << std::string(50, '-') << std::endl;
        std::cout << "  " << sectionName << std::endl;
        std::cout << std::string(50, '-') << std::endl;
    }

    /**
     * Print a standardized section footer indicating completion
     */
    static void printSectionFooter() {
        std::cout << std::string(50, '-') << std::endl;
        std::cout << "  [OK] Section completed" << std::endl;
    }

    /**
     * Print a simple section header with equals formatting (alternative style)
     * @param sectionName The name of the test section
     */
    static void printSimpleSectionHeader(const std::string& sectionName) {
        std::cout << "\n=== " << sectionName << " ===" << std::endl;
    }

    /**
     * Print a simple section footer with equals formatting (alternative style)
     * @param sectionName The name of the completed section
     */
    static void printSimpleSectionFooter(const std::string& sectionName) {
        std::cout << "\n=== " << sectionName << " Completed ===\n" << std::endl;
    }

    /**
     * Print a test result with consistent formatting
     * @param testName The name of the test
     * @param passed Whether the test passed
     */
    static void printTestResult(const std::string& testName, bool passed) {
        std::cout << "    [" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
    }

    /**
     * Print an informational message with consistent formatting
     * @param message The information message
     */
    static void printInfo(const std::string& message) {
        std::cout << "    [INFO] " << message << std::endl;
    }

    /**
     * Print a warning message with consistent formatting
     * @param message The warning message
     */
    static void printWarning(const std::string& message) {
        std::cout << "    [WARN] " << message << std::endl;
    }

    /**
     * Print an error message with consistent formatting
     * @param message The error message
     */
    static void printError(const std::string& message) {
        std::cout << "    [ERROR] " << message << std::endl;
    }
};

} // namespace Tests
} // namespace Lua

#endif // TEST_UTILS_HPP