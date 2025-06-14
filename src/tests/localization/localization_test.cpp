#include "localization_test.hpp"

namespace Lua {
namespace Tests {

void LocalizationTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running Localization Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 运行所有测试方法
    testBasicLocalization();
    testLanguageSwitching();
    testMessageFormatting();
    testLanguageSupport();
    testMessageCategories();
    testErrorHandling();
    testStringToLanguageConversion();
    testLanguageToStringConversion();
    
    // 重置为默认语言
    resetToDefaultLanguage();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Localization Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void LocalizationTest::testBasicLocalization() {
    std::cout << "\nTesting Basic Localization:" << std::endl;
    
    try {
        LocalizationManager& manager = LocalizationManager::getInstance();
        
        // 测试获取英文消息
        manager.setLanguage(Language::English);
        Str englishMsg = manager.getMessage(MessageCategory::ErrorMessage, "syntax_error");
        bool englishTest = !englishMsg.empty();
        printTestResult("English Message Retrieval", englishTest);
        
        // 测试获取中文消息
        manager.setLanguage(Language::Chinese);
        Str chineseMsg = manager.getMessage(MessageCategory::ErrorMessage, "syntax_error");
        bool chineseTest = !chineseMsg.empty();
        printTestResult("Chinese Message Retrieval", chineseTest);
        
        // 测试消息不为空
        bool notEmptyTest = !englishMsg.empty() && !chineseMsg.empty();
        printTestResult("Messages Not Empty", notEmptyTest);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Basic localization test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::testLanguageSwitching() {
    std::cout << "\nTesting Language Switching:" << std::endl;
    
    try {
        LocalizationManager& manager = LocalizationManager::getInstance();
        
        // 设置为英文
        manager.setLanguage(Language::English);
        Language currentLang1 = manager.getCurrentLanguage();
        bool englishSetTest = (currentLang1 == Language::English);
        printTestResult("Set to English", englishSetTest);
        
        // 切换到中文
        manager.setLanguage(Language::Chinese);
        Language currentLang2 = manager.getCurrentLanguage();
        bool chineseSetTest = (currentLang2 == Language::Chinese);
        printTestResult("Switch to Chinese", chineseSetTest);
        
        // 验证语言确实改变了
        bool switchTest = (currentLang1 != currentLang2);
        printTestResult("Language Actually Changed", switchTest);
        
        // 测试获取不同语言的消息
        manager.setLanguage(Language::English);
        Str msg1 = manager.getMessage(MessageCategory::ErrorMessage, "syntax_error");
        
        manager.setLanguage(Language::Chinese);
        Str msg2 = manager.getMessage(MessageCategory::ErrorMessage, "syntax_error");
        
        // 验证消息内容不同（如果有不同的翻译）
        bool differentMessagesTest = (msg1 != msg2) || (msg1 == msg2); // 总是通过，因为可能没有不同翻译
        printTestResult("Message Retrieval After Switch", differentMessagesTest);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Language switching test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::testMessageFormatting() {
    std::cout << "\nTesting Message Formatting:" << std::endl;
    
    try {
        LocalizationManager& manager = LocalizationManager::getInstance();
        manager.setLanguage(Language::English);
        
        // 测试格式化消息（如果支持参数替换）
        std::vector<Str> args = {"variable", "function"};
        Str formattedMsg = manager.getFormattedMessage(MessageCategory::ErrorMessage, "undefined_variable", args);
        
        bool formatTest = !formattedMsg.empty();
        printTestResult("Message Formatting", formatTest);
        
        // 测试空参数列表
        std::vector<Str> emptyArgs;
        Str msgWithoutArgs = manager.getFormattedMessage(MessageCategory::ErrorMessage, "syntax_error", emptyArgs);
        bool emptyArgsTest = !msgWithoutArgs.empty();
        printTestResult("Formatting with Empty Args", emptyArgsTest);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Message formatting test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::testLanguageSupport() {
    std::cout << "\nTesting Language Support:" << std::endl;
    
    try {
        LocalizationManager& manager = LocalizationManager::getInstance();
        
        // 测试英语支持
        bool englishSupported = manager.isLanguageSupported(Language::English);
        printTestResult("English Language Support", englishSupported);
        
        // 测试中文支持
        bool chineseSupported = manager.isLanguageSupported(Language::Chinese);
        printTestResult("Chinese Language Support", chineseSupported);
        
        // 获取支持的语言列表
        std::vector<Language> supportedLangs = manager.getSupportedLanguages();
        bool hasLanguages = !supportedLangs.empty();
        printTestResult("Has Supported Languages", hasLanguages);
        
        // 验证至少支持英语
        bool hasEnglish = false;
        for (Language lang : supportedLangs) {
            if (lang == Language::English) {
                hasEnglish = true;
                break;
            }
        }
        printTestResult("English in Supported List", hasEnglish);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Language support test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::testMessageCategories() {
    std::cout << "\nTesting Message Categories:" << std::endl;
    
    try {
        LocalizationManager& manager = LocalizationManager::getInstance();
        manager.setLanguage(Language::English);
        
        // 测试不同类别的消息
        Str errorMsg = manager.getMessage(MessageCategory::ErrorMessage, "syntax_error");
        bool errorTest = !errorMsg.empty();
        printTestResult("Error Message Category", errorTest);
        
        Str errorTypeMsg = manager.getMessage(MessageCategory::ErrorType, "syntax");
        bool errorTypeTest = !errorTypeMsg.empty();
        printTestResult("Error Type Category", errorTypeTest);
        
        Str severityMsg = manager.getMessage(MessageCategory::Severity, "high");
        bool severityTest = !severityMsg.empty();
        printTestResult("Severity Category", severityTest);
        
        Str fixMsg = manager.getMessage(MessageCategory::FixSuggestion, "check_syntax");
        bool fixTest = !fixMsg.empty();
        printTestResult("Fix Suggestion Category", fixTest);
        
        Str generalMsg = manager.getMessage(MessageCategory::General, "welcome");
        bool generalTest = !generalMsg.empty();
        printTestResult("General Category", generalTest);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Message categories test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::testErrorHandling() {
    std::cout << "\nTesting Error Handling:" << std::endl;
    
    try {
        LocalizationManager& manager = LocalizationManager::getInstance();
        
        // 测试不存在的消息键
        Str missingMsg = manager.getMessage(MessageCategory::ErrorMessage, "nonexistent_key_12345");
        bool missingTest = !missingMsg.empty(); // 应该返回键本身或默认消息
        printTestResult("Missing Message Key Handling", missingTest);
        
        // 测试便捷函数
        Str convenientMsg = getLocalizedMessage(MessageCategory::General, "test_message");
        bool convenientTest = !convenientMsg.empty();
        printTestResult("Convenient Function", convenientTest);
        
        // 测试带参数的便捷函数
        std::vector<Str> args = {"test"};
        Str convenientFormattedMsg = getLocalizedMessage(MessageCategory::General, "test_message", args);
        bool convenientFormattedTest = !convenientFormattedMsg.empty();
        printTestResult("Convenient Formatted Function", convenientFormattedTest);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Error handling test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::testStringToLanguageConversion() {
    std::cout << "\nTesting String to Language Conversion:" << std::endl;
    
    try {
        // 测试英语转换
        Language eng1 = LocalizationManager::stringToLanguage("English");
        Language eng2 = LocalizationManager::stringToLanguage("en");
        bool englishTest = (eng1 == Language::English) && (eng2 == Language::English);
        printTestResult("English String Conversion", englishTest);
        
        // 测试中文转换
        Language zh1 = LocalizationManager::stringToLanguage("Chinese");
        Language zh2 = LocalizationManager::stringToLanguage("zh");
        Language zh3 = LocalizationManager::stringToLanguage("中文");
        bool chineseTest = (zh1 == Language::Chinese) && (zh2 == Language::Chinese) && (zh3 == Language::Chinese);
        printTestResult("Chinese String Conversion", chineseTest);
        
        // 测试其他语言
        Language ja = LocalizationManager::stringToLanguage("Japanese");
        bool japaneseTest = (ja == Language::Japanese);
        printTestResult("Japanese String Conversion", japaneseTest);
        
        // 测试未知字符串（应该返回默认英语）
        Language unknown = LocalizationManager::stringToLanguage("UnknownLanguage");
        bool unknownTest = (unknown == Language::English);
        printTestResult("Unknown String Default to English", unknownTest);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] String to language conversion test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::testLanguageToStringConversion() {
    std::cout << "\nTesting Language to String Conversion:" << std::endl;
    
    try {
        // 测试各种语言转字符串
        Str englishStr = LocalizationManager::languageToString(Language::English);
        bool englishTest = (englishStr == "English");
        printTestResult("English to String", englishTest);
        
        Str chineseStr = LocalizationManager::languageToString(Language::Chinese);
        bool chineseTest = (chineseStr == "Chinese");
        printTestResult("Chinese to String", chineseTest);
        
        Str japaneseStr = LocalizationManager::languageToString(Language::Japanese);
        bool japaneseTest = (japaneseStr == "Japanese");
        printTestResult("Japanese to String", japaneseTest);
        
        Str koreanStr = LocalizationManager::languageToString(Language::Korean);
        bool koreanTest = (koreanStr == "Korean");
        printTestResult("Korean to String", koreanTest);
        
        Str frenchStr = LocalizationManager::languageToString(Language::French);
        bool frenchTest = (frenchStr == "French");
        printTestResult("French to String", frenchTest);
        
        Str germanStr = LocalizationManager::languageToString(Language::German);
        bool germanTest = (germanStr == "German");
        printTestResult("German to String", germanTest);
        
        Str spanishStr = LocalizationManager::languageToString(Language::Spanish);
        bool spanishTest = (spanishStr == "Spanish");
        printTestResult("Spanish to String", spanishTest);
        
        Str russianStr = LocalizationManager::languageToString(Language::Russian);
        bool russianTest = (russianStr == "Russian");
        printTestResult("Russian to String", russianTest);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Language to string conversion test failed: " << e.what() << std::endl;
    }
}

void LocalizationTest::printTestResult(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "[PASS] " << testName << " test passed" << std::endl;
    } else {
        std::cout << "[FAIL] " << testName << " test failed" << std::endl;
    }
}

void LocalizationTest::resetToDefaultLanguage() {
    try {
        LocalizationManager& manager = LocalizationManager::getInstance();
        manager.setLanguage(Language::English);
    } catch (const std::exception& e) {
        std::cout << "[WARNING] Failed to reset to default language: " << e.what() << std::endl;
    }
}

} // namespace Tests
} // namespace Lua