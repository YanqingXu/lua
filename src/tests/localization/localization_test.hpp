#ifndef LOCALIZATION_TEST_HPP
#define LOCALIZATION_TEST_HPP

#include <iostream>
#include "../../localization/localization_manager.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief Localization manager test class
 * 
 * Tests various functionalities of the localization manager, including:
 * - Basic message retrieval
 * - Language switching
 * - Message formatting
 * - Error handling
 * - Language support checking
 */
class LocalizationTest {
public:
    /**
     * @brief Run all tests
     * 
     * Execute all test cases in this test class
     */
    static void runAllTests();
    
private:
    // Private test methods
    static void testBasicLocalization();
    static void testLanguageSwitching();
    static void testMessageFormatting();
    static void testLanguageSupport();
    static void testMessageCategories();
    static void testErrorHandling();
    static void testStringToLanguageConversion();
    static void testLanguageToStringConversion();
    
    // Helper methods
    static void printTestResult(const std::string& testName, bool passed);
    static void resetToDefaultLanguage();
};

} // namespace Tests
} // namespace Lua

#endif // LOCALIZATION_TEST_HPP