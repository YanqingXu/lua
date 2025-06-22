#ifndef LUA_TEST_FRAMEWORK_TEST_RUNNER_HPP
#define LUA_TEST_FRAMEWORK_TEST_RUNNER_HPP

#include "test_macros.hpp"
#include "../formatting/format_formatter.hpp"
#include <string>
#include <functional>
#include <vector>

namespace Lua {
namespace TestFramework {

/**
 * @brief 测试运行器 - 框架的核心组件
 * 
 * 负责协调和执行所有测试，提供统一的测试入口点。
 * 支持模块化测试执行和结果统计。
 */
class TestRunner {
public:
    /**
     * @brief 构造函数
     */
    TestRunner();
    
    /**
     * @brief 析构函数
     */
    ~TestRunner();
    
    /**
     * @brief 运行所有注册的测试模块
     * @return 是否所有测试都通过
     */
    bool runAllTests();
    
    /**
     * @brief 注册测试模块
     * @param moduleName 模块名称
     * @param testFunction 测试函数
     */
    void registerModule(const std::string& moduleName, std::function<void()> testFunction);
    
    /**
     * @brief 运行指定模块的测试
     * @param moduleName 模块名称
     * @return 是否测试通过
     */
    bool runModule(const std::string& moduleName);
    
    /**
     * @brief 获取测试统计信息
     * @return 测试统计结果
     */
    struct TestStats {
        int totalModules = 0;
        int passedModules = 0;
        int failedModules = 0;
        int totalTests = 0;
        int passedTests = 0;
        int failedTests = 0;
    };
    
    TestStats getStats() const;
    
    /**
     * @brief 设置详细输出模式
     * @param verbose 是否启用详细输出
     */
    void setVerbose(bool verbose);
    
    /**
     * @brief 设置颜色输出
     * @param enabled 是否启用颜色
     */
    void setColorEnabled(bool enabled);
    
    /**
     * @brief 获取格式化器实例
     * @return 格式化器引用
     */
    Lua::Tests::TestFormatting::TestFormatter& getFormatter();
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // 禁用拷贝和赋值
    TestRunner(const TestRunner&) = delete;
    TestRunner& operator=(const TestRunner&) = delete;
};

/**
 * @brief 获取全局测试运行器实例
 * @return 测试运行器引用
 */
TestRunner& getGlobalTestRunner();

/**
 * @brief 便捷函数：运行所有测试
 * @return 是否所有测试都通过
 */
bool runAllTests();

/**
 * @brief 便捷函数：注册测试模块
 * @param moduleName 模块名称
 * @param testFunction 测试函数
 */
void registerTestModule(const std::string& moduleName, std::function<void()> testFunction);

} // namespace TestFramework
} // namespace Lua

#endif // LUA_TEST_FRAMEWORK_TEST_RUNNER_HPP