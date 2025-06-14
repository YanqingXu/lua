#include "localization_test.hpp"

namespace Lua {
namespace Tests {

void LocalizationTest::runAllTests() {
    std::cout << "\n=== Localization Manager Tests ===\n" << std::endl;
    
    // Run all tests
    testBasicLocalization();
    testLanguageSwitching();
    testMessageFormatting();
    testLanguageSupport();
    testMessageCategories();
    testErrorHandling();
    testStringToLanguageConversion();
    testLanguageToStringConversion();
    
    std::cout << "\n=== Localization Manager Tests Completed ===\n" << std::endl;
}

void LocalizationTest::testBasicLocalization() {
    resetToDefaultLanguage();
    
    // Test getting English messages
    auto& manager = LocalizationManager::getInstance();
    std::string errorMsg = manager.getMessage(MessageCategory::ErrorMessage, "syntax_error");
    
    bool passed = !errorMsg.empty() && errorMsg != "syntax_error";
    printTestResult("Basic Localization Test", passed);
}

void LocalizationTest::testLanguageSwitching() {
    resetToDefaultLanguage();
    
    auto& manager = LocalizationManager::getInstance();
    
    // Switch to Chinese
    manager.setLanguage(Language::Chinese);
    std::string chineseMsg = manager.getMessage(MessageCategory::ErrorMessage, "syntax_error");
    
    // Switch back to English
    manager.setLanguage(Language::English);
    std::string englishMsg = manager.getMessage(MessageCategory::ErrorMessage, "syntax_error");
    
    // Verify messages are different (assuming Chinese translation exists)
    bool passed = chineseMsg != englishMsg;
    printTestResult("Language Switching Test", passed);
}

void LocalizationTest::testMessageFormatting() {
    resetToDefaultLanguage();
    
    auto& manager = LocalizationManager::getInstance();
    
    // Test formatted messages
    std::vector<std::string> args = {"variable_name", "10"};
    std::string formattedMsg = manager.getFormattedMessage(
        MessageCategory::ErrorMessage, "undefined_variable", args);
    
    // Check if arguments are included
    bool passed = formattedMsg.find("variable_name") != std::string::npos;
    printTestResult("Message Formatting Test", passed);
}

void LocalizationTest::testLanguageSupport() {
    resetToDefaultLanguage();
    
    auto& manager = LocalizationManager::getInstance();
    
    // Test supported languages
    bool englishSupported = manager.isLanguageSupported(Language::English);
    bool chineseSupported = manager.isLanguageSupported(Language::Chinese);
    
    // Get supported language list
    auto supportedLanguages = manager.getSupportedLanguages();
    
    bool passed = englishSupported && chineseSupported && !supportedLanguages.empty();
    printTestResult("Language Support Test", passed);
}

void LocalizationTest::testMessageCategories() {
    resetToDefaultLanguage();
    
    auto& manager = LocalizationManager::getInstance();
    
    // Test different message categories
    std::string errorTypeMsg = manager.getMessage(MessageCategory::ErrorType, "syntax");
    std::string errorMsg = manager.getMessage(MessageCategory::ErrorMessage, "syntax_error");
    std::string severityMsg = manager.getMessage(MessageCategory::Severity, "error");
    std::string fixMsg = manager.getMessage(MessageCategory::FixSuggestion, "check_syntax");
    std::string generalMsg = manager.getMessage(MessageCategory::General, "welcome");
    
    bool passed = !errorTypeMsg.empty() && !errorMsg.empty() && 
                  !severityMsg.empty() && !fixMsg.empty() && !generalMsg.empty();
    printTestResult("Message Categories Test", passed);
}

void LocalizationTest::testErrorHandling() {
    resetToDefaultLanguage();
    
    auto& manager = LocalizationManager::getInstance();
    
    // Test non-existent message key
    std::string nonExistentMsg = manager.getMessage(MessageCategory::ErrorMessage, "non_existent_key");
    bool keyFallback = (nonExistentMsg == "non_existent_key"); // Should return the key itself
    
    // Test empty key
    std::string emptyKeyMsg = manager.getMessage(MessageCategory::ErrorMessage, "");
    bool emptyKeyHandled = !emptyKeyMsg.empty(); // Should return empty string or key
    
    bool passed = keyFallback && emptyKeyHandled;
    printTestResult("Error Handling Test", passed);
}

void LocalizationTest::testStringToLanguageConversion() {
    // Test various string to language conversions
    Language english1 = LocalizationManager::stringToLanguage("English");
    Language english2 = LocalizationManager::stringToLanguage("en");
    Language chinese1 = LocalizationManager::stringToLanguage("Chinese");
    Language chinese2 = LocalizationManager::stringToLanguage("zh");
    Language chinese3 = LocalizationManager::stringToLanguage("中文");
    
    bool englishTest = (english1 == Language::English) && (english2 == Language::English);
    bool chineseTest = (chinese1 == Language::Chinese) && (chinese2 == Language::Chinese) && (chinese3 == Language::Chinese);
    
    // Test invalid string (should return default language)
    Language defaultLang = LocalizationManager::stringToLanguage("invalid_language");
    bool defaultTest = (defaultLang == Language::English);
    
    bool passed = englishTest && chineseTest && defaultTest;
    printTestResult("String to Language Conversion Test", passed);
}

void LocalizationTest::testLanguageToStringConversion() {
    // Test language to string conversions
    std::string englishStr = LocalizationManager::languageToString(Language::English);
    std::string chineseStr = LocalizationManager::languageToString(Language::Chinese);
    std::string japaneseStr = LocalizationManager::languageToString(Language::Japanese);
    
    bool englishTest = (englishStr == "English");
    bool chineseTest = (chineseStr == "Chinese");
    bool japaneseTest = (japaneseStr == "Japanese");
    
    // Test round trip conversion
    Language roundTripLang = LocalizationManager::stringToLanguage(englishStr);
    bool roundTripTest = (roundTripLang == Language::English);
    
    bool passed = englishTest && chineseTest && japaneseTest && roundTripTest;
    printTestResult("Language to String Conversion Test", passed);
}

void LocalizationTest::printTestResult(const std::string& testName, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
}

void LocalizationTest::resetToDefaultLanguage() {
    // Reset to default language (English)
    LocalizationManager::getInstance().setLanguage(Language::English);
}

} // namespace Tests
} // namespace Lua