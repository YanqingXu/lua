#ifndef LUA_TEST_FRAMEWORK_TEST_UTILS_HPP
#define LUA_TEST_FRAMEWORK_TEST_UTILS_HPP

#include "../formatting/format_formatter.hpp"
#include "../formatting/format_config.hpp"
#include "test_memory.hpp"
#include <iostream>
#include <string>
#include <exception>

namespace Lua {
namespace TestFramework {

/**
 * @brief 测试工具类 - 提供统一的测试辅助功能
 * 
 * 这是一个简化的门面接口，委托给格式化模块处理具体的输出格式。
 * 提供向后兼容的接口和新的层次化接口。
 */
class TestUtils {
private:
    /**
     * @brief 获取格式化器实例
     * @return 格式化器引用
     */
    static Lua::Tests::TestFormatting::TestFormatter& getFormatter() {
        return Lua::Tests::TestFormatting::TestFormatter::getInstance();
    }

public:
    
    // ========== 向后兼容接口 ==========
    
    /**
     * @brief 打印标准化的节标题
     * @param sectionName 节名称
     */
    static void printSectionHeader(const std::string& sectionName) {
        getFormatter().printSectionHeader(sectionName);
    }

    /**
     * @brief 打印标准化的节尾部，表示完成
     */
    static void printSectionFooter() {
        getFormatter().printSectionFooter("Section completed");
    }

    /**
     * @brief 打印简单的节标题（等号格式的替代样式）
     * @param sectionName 节名称
     */
    static void printSimpleSectionHeader(const std::string& sectionName) {
        getFormatter().printSimpleSectionHeader(sectionName);
    }

    /**
     * @brief 打印简单的节尾部（等号格式的替代样式）
     * @param sectionName 完成的节名称
     */
    static void printSimpleSectionFooter(const std::string& sectionName) {
        getFormatter().printSimpleSectionFooter(sectionName + " Completed");
    }

    /**
     * @brief 打印一致格式的测试结果
     * @param testName 测试名称
     * @param passed 测试是否通过
     */
    static void printTestResult(const std::string& testName, bool passed) {
        getFormatter().printTestResult(testName, passed);
    }

    /**
     * @brief 打印一致格式的信息消息
     * @param message 信息消息
     */
    static void printInfo(const std::string& message) {
        getFormatter().printInfo(message);
    }

    /**
     * @brief 打印一致格式的警告消息
     * @param message 警告消息
     */
    static void printWarning(const std::string& message) {
        getFormatter().printWarning(message);
    }

    /**
     * @brief 打印一致格式的错误消息
     * @param message 错误消息
     */
    static void printError(const std::string& message) {
        getFormatter().printError(message);
    }

    /**
     * @brief 打印一致格式的异常信息
     * @param e 异常对象
     * @param context 可选的上下文信息（如测试名称、函数名称）
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
     * @brief 打印一致格式的未知异常信息
     * @param context 可选的上下文信息（如测试名称、函数名称）
     */
    static void printUnknownException(const std::string& context = "") {
        std::string message = "Unknown exception caught";
        if (!context.empty()) {
            message += " in " + context;
        }
        getFormatter().printError(message);
    }
    
    // ========== 新的层次化接口 ==========
    
    /**
     * @brief 打印特定级别的头部
     * @param level 测试级别
     * @param title 标题文本
     * @param description 可选的描述
     */
    static void printLevelHeader(Lua::Tests::TestFormatting::TestLevel level, const std::string& title, 
                                const std::string& description = "") {
        getFormatter().printLevelHeader(level, title, description);
    }
    
    /**
     * @brief 打印特定级别的尾部
     * @param level 测试级别
     * @param summary 可选的总结文本
     */
    static void printLevelFooter(Lua::Tests::TestFormatting::TestLevel level, const std::string& summary = "") {
        getFormatter().printLevelFooter(level, summary);
    }
    
    // ========== 配置接口 ==========
    
    /**
     * @brief 启用或禁用颜色输出
     * @param enabled 是否启用颜色
     */
    static void setColorEnabled(bool enabled) {
        getFormatter().setColorEnabled(enabled);
    }
    
    /**
     * @brief 设置颜色主题
     * @param theme 主题名称
     */
    static void setTheme(const std::string& theme) {
        getFormatter().setTheme(theme);
    }
    
    /**
     * @brief 获取配置对象
     * @return 配置对象引用
     */
    static Lua::Tests::TestFormatting::TestConfig& getConfig() {
        return getFormatter().getConfig();
    }
    
    // ========== 内存测试工具 ==========
    
    /**
     * @brief 开始内存泄漏检测
     * @param testName 测试名称
     */
    static void startMemoryCheck(const std::string& testName) {
        MemoryTestUtils::startMemoryCheck(testName);
    }
    
    /**
     * @brief 结束内存泄漏检测
     * @param testName 测试名称
     * @return 是否检测到内存泄漏
     */
    static bool endMemoryCheck(const std::string& testName) {
        return MemoryTestUtils::endMemoryCheck(testName);
    }
    
    /**
     * @brief 检查是否启用了内存检测
     * @return 是否启用内存检测
     */
    static bool isMemoryCheckEnabled() {
        return MemoryTestUtils::isEnabled();
    }
    
    /**
     * @brief 设置内存检测开关
     * @param enabled 是否启用内存检测
     */
    static void setMemoryCheckEnabled(bool enabled) {
        MemoryTestUtils::setEnabled(enabled);
    }
    
    // ========== 统计和报告 ==========
    
    /**
     * @brief 测试统计信息结构
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
    
    /**
     * @brief 获取当前测试统计信息
     * @return 统计信息
     */
    static TestStatistics getStatistics();
    
    /**
     * @brief 重置测试统计信息
     */
    static void resetStatistics();
    
    /**
     * @brief 打印测试统计报告
     */
    static void printStatisticsReport();
    
private:
    static TestStatistics statistics_;
};

} // namespace TestFramework
} // namespace Lua

#endif // LUA_TEST_FRAMEWORK_TEST_UTILS_HPP