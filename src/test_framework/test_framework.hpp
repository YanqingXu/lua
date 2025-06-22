#ifndef LUA_TEST_FRAMEWORK_HPP
#define LUA_TEST_FRAMEWORK_HPP

/**
 * @file test_framework.hpp
 * @brief Lua测试框架主头文件
 * 
 * 这个文件提供了Lua测试框架的统一入口点，包含了所有核心组件。
 * 用户只需要包含这个文件就可以使用完整的测试框架功能。
 * 
 * @version 2.0
 * @date 2024
 * 
 * 使用示例:
 * ```cpp
 * #include "test_framework/test_framework.hpp"
 * 
 * class MyTestSuite {
 * public:
 *     static void runAllTests() {
 *         RUN_TEST_GROUP("Basic Tests", runBasicTests);
 *     }
 * private:
 *     static void runBasicTests() {
 *         RUN_TEST(MyTestClass, testFunction);
 *     }
 * };
 * ```
 */

// 核心组件
#include "core/test_runner.hpp"
#include "core/test_macros.hpp"
#include "core/test_utils.hpp"
#include "core/test_memory.hpp"

// 格式化组件
#include "formatting/format_formatter.hpp"
#include "formatting/format_config.hpp"
#include "formatting/format_define.hpp"

// ColorTheme枚举定义
namespace Lua {
namespace TestFramework {
enum class ColorTheme {
    MODERN,
    CLASSIC,
    MINIMAL
};
}
}

// 配置组件
//#include "config/config_loader.hpp"

// 便捷命名空间别名
namespace Lua {
namespace Test = TestFramework;
}



/**
 * @brief 测试框架版本信息
 */
namespace Lua {
namespace TestFramework {

/**
 * @brief 版本信息类
 */
class Version {
public:
    static constexpr int MAJOR = 2;
    static constexpr int MINOR = 0;
    static constexpr int PATCH = 0;
    
    /**
     * @brief 获取版本字符串
     * @return 版本字符串，格式为 "major.minor.patch"
     */
    static std::string getString() {
        return std::to_string(MAJOR) + "." + 
               std::to_string(MINOR) + "." + 
               std::to_string(PATCH);
    }
    
    /**
     * @brief 打印版本信息
     */
    static void printVersion() {
        TestUtils::printInfo("Lua Test Framework v" + getString());
    }
};

/**
 * @brief 测试框架初始化器
 */
class Initializer {
public:
    /**
     * @brief 初始化测试框架
     * @param enableColor 是否启用颜色输出
     * @param theme 颜色主题
     * @param showVersion 是否显示版本信息
     */
    static void initialize(bool enableColor = true, 
                          const std::string& theme = "modern",
                          bool showVersion = true) {
        if (showVersion) {
            Version::printVersion();
        }
        
        TestUtils::setColorEnabled(enableColor);
        TestUtils::setTheme(theme);
        
        TestUtils::printInfo("Test framework initialized successfully");
    }
    
    /**
     * @brief 快速初始化（使用默认设置）
     */
    static void quickInit() {
        initialize(true, "modern", false);
    }
};

} // namespace TestFramework
} // namespace Lua



#endif // LUA_TEST_FRAMEWORK_HPP