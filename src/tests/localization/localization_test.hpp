#ifndef LOCALIZATION_TEST_HPP
#define LOCALIZATION_TEST_HPP

#include <iostream>
#include "../../localization/localization_manager.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief 本地化管理器测试类
 * 
 * 测试本地化管理器的各种功能，包括：
 * - 基本消息获取
 * - 语言切换
 * - 消息格式化
 * - 错误处理
 * - 语言支持检查
 */
class LocalizationTest {
public:
    /**
     * @brief 运行所有测试
     * 
     * 执行这个测试类中的所有测试用例
     */
    static void runAllTests();
    
private:
    // 私有测试方法
    static void testBasicLocalization();
    static void testLanguageSwitching();
    static void testMessageFormatting();
    static void testLanguageSupport();
    static void testMessageCategories();
    static void testErrorHandling();
    static void testStringToLanguageConversion();
    static void testLanguageToStringConversion();
    
    // 辅助方法
    static void printTestResult(const std::string& testName, bool passed);
    static void resetToDefaultLanguage();
};

} // namespace Tests
} // namespace Lua

#endif // LOCALIZATION_TEST_HPP