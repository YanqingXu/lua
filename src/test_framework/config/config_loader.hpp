#ifndef LUA_TEST_FRAMEWORK_CONFIG_LOADER_HPP
#define LUA_TEST_FRAMEWORK_CONFIG_LOADER_HPP

/**
 * @file config_loader.hpp
 * @brief 测试框架配置加载器
 * 
 * 这个文件提供了从配置文件和环境变量加载测试框架配置的功能。
 * 支持读取test_format_config.txt文件以及相关的环境变量。
 * 
 * 配置优先级（从高到低）：
 * 1. 环境变量
 * 2. 配置文件
 * 3. 默认值
 */

#include "../formatting/format_define.hpp"
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>

namespace Lua {
namespace TestFramework {
namespace Config {

/**
 * @brief 安全的环境变量获取函数
 * @param name 环境变量名
 * @return 环境变量值，如果不存在则返回空字符串
 */
inline std::string safe_getenv(const char* name) {
#ifdef _WIN32
    char* buffer = nullptr;
    size_t size = 0;
    errno_t err = _dupenv_s(&buffer, &size, name);
    if (err == 0 && buffer != nullptr) {
        std::string result(buffer);
        free(buffer);
        return result;
    }
    return "";
#else
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string("");
#endif
}

/**
 * @brief 测试框架配置结构
 */
struct TestConfig {
    bool colorEnabled = true;           ///< 是否启用颜色输出
    ColorTheme theme = ColorTheme::MODERN;  ///< 颜色主题
    bool verbose = false;               ///< 是否启用详细输出
    bool memoryCheckEnabled = true;     ///< 是否启用内存检查
    int defaultTimeout = 5000;          ///< 默认超时时间（毫秒）
    std::string logLevel = "INFO";      ///< 日志级别
    
    /**
     * @brief 重置为默认配置
     */
    void reset() {
        colorEnabled = true;
        theme = ColorTheme::MODERN;
        verbose = false;
        memoryCheckEnabled = true;
        defaultTimeout = 5000;
        logLevel = "INFO";
    }
};

/**
 * @brief 配置加载器类
 */
class ConfigLoader {
public:
    /**
     * @brief 加载配置
     * @param configFilePath 配置文件路径（可选）
     * @return 加载的配置
     */
    static TestConfig loadConfig(const std::string& configFilePath = "") {
        TestConfig config;
        
        // 1. 首先加载默认配置
        config.reset();
        
        // 2. 尝试从配置文件加载
        std::string actualConfigPath = configFilePath;
        if (actualConfigPath.empty()) {
            actualConfigPath = findDefaultConfigFile();
        }
        
        if (!actualConfigPath.empty()) {
            loadFromFile(config, actualConfigPath);
        }
        
        // 3. 从环境变量覆盖配置
        loadFromEnvironment(config);
        
        return config;
    }
    
    /**
     * @brief 保存配置到文件
     * @param config 要保存的配置
     * @param configFilePath 配置文件路径
     * @return 是否保存成功
     */
    static bool saveConfig(const TestConfig& config, const std::string& configFilePath) {
        std::ofstream file(configFilePath);
        if (!file.is_open()) {
            return false;
        }
        
        file << "# Lua Test Framework Configuration\n";
        file << "# This file is auto-generated\n\n";
        
        file << "# Enable or disable color output\n";
        file << "colorEnabled=" << (config.colorEnabled ? "true" : "false") << "\n\n";
        
        file << "# Color theme (modern, classic, minimal)\n";
        file << "theme=" << themeToString(config.theme) << "\n\n";
        
        file << "# Enable verbose output\n";
        file << "verbose=" << (config.verbose ? "true" : "false") << "\n\n";
        
        file << "# Enable memory checking\n";
        file << "memoryCheckEnabled=" << (config.memoryCheckEnabled ? "true" : "false") << "\n\n";
        
        file << "# Default timeout in milliseconds\n";
        file << "defaultTimeout=" << config.defaultTimeout << "\n\n";
        
        file << "# Log level (DEBUG, INFO, WARNING, ERROR)\n";
        file << "logLevel=" << config.logLevel << "\n\n";
        
        file << "# Environment variables can also be used:\n";
        file << "# NO_COLOR=1          - Disable colors\n";
        file << "# FORCE_COLOR=1       - Force enable colors\n";
        file << "# TEST_THEME=<theme>  - Set theme\n";
        file << "# TEST_VERBOSE=1      - Enable verbose output\n";
        file << "# TEST_TIMEOUT=<ms>   - Set default timeout\n";
        
        return true;
    }
    
    /**
     * @brief 打印当前配置
     * @param config 要打印的配置
     */
    static void printConfig(const TestConfig& config) {
        std::cout << "=== Test Framework Configuration ===\n";
        std::cout << "Color Enabled: " << (config.colorEnabled ? "Yes" : "No") << "\n";
        std::cout << "Theme: " << themeToString(config.theme) << "\n";
        std::cout << "Verbose: " << (config.verbose ? "Yes" : "No") << "\n";
        std::cout << "Memory Check: " << (config.memoryCheckEnabled ? "Yes" : "No") << "\n";
        std::cout << "Default Timeout: " << config.defaultTimeout << "ms\n";
        std::cout << "Log Level: " << config.logLevel << "\n";
        std::cout << "=====================================\n";
    }
    
private:
    /**
     * @brief 查找默认配置文件
     * @return 配置文件路径，如果未找到则返回空字符串
     */
    static std::string findDefaultConfigFile() {
        // 可能的配置文件位置
        std::vector<std::string> possiblePaths = {
            "test_format_config.txt",
            "config/test_format_config.txt",
            "../test_framework/config/test_format_config.txt",
            "test_framework/config/test_format_config.txt"
        };
        
        for (const auto& path : possiblePaths) {
            std::ifstream file(path);
            if (file.good()) {
                return path;
            }
        }
        
        return "";
    }
    
    /**
     * @brief 从文件加载配置
     * @param config 要更新的配置对象
     * @param filePath 配置文件路径
     */
    static void loadFromFile(TestConfig& config, const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // 跳过注释和空行
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // 解析键值对
            size_t equalPos = line.find('=');
            if (equalPos == std::string::npos) {
                continue;
            }
            
            std::string key = trim(line.substr(0, equalPos));
            std::string value = trim(line.substr(equalPos + 1));
            
            // 应用配置
            applyConfigValue(config, key, value);
        }
    }
    
    /**
     * @brief 从环境变量加载配置
     * @param config 要更新的配置对象
     */
    static void loadFromEnvironment(TestConfig& config) {
        // NO_COLOR 环境变量
        if (!safe_getenv("NO_COLOR").empty()) {
            config.colorEnabled = false;
        }
        
        // FORCE_COLOR 环境变量
        if (!safe_getenv("FORCE_COLOR").empty()) {
            config.colorEnabled = true;
        }
        
        // TEST_THEME 环境变量
        std::string themeEnv = safe_getenv("TEST_THEME");
        if (!themeEnv.empty()) {
            config.theme = stringToTheme(themeEnv.c_str());
        }
        
        // TEST_VERBOSE 环境变量
        if (!safe_getenv("TEST_VERBOSE").empty()) {
            config.verbose = true;
        }
        
        // TEST_TIMEOUT 环境变量
        std::string timeoutEnv = safe_getenv("TEST_TIMEOUT");
        if (!timeoutEnv.empty()) {
            try {
                config.defaultTimeout = std::stoi(timeoutEnv);
            } catch (...) {
                // 忽略无效的超时值
            }
        }
        
        // TEST_LOG_LEVEL 环境变量
        std::string logLevelEnv = safe_getenv("TEST_LOG_LEVEL");
        if (!logLevelEnv.empty()) {
            config.logLevel = logLevelEnv;
        }
    }
    
    /**
     * @brief 应用配置值
     * @param config 要更新的配置对象
     * @param key 配置键
     * @param value 配置值
     */
    static void applyConfigValue(TestConfig& config, const std::string& key, const std::string& value) {
        if (key == "colorEnabled") {
            config.colorEnabled = stringToBool(value);
        } else if (key == "theme") {
            config.theme = stringToTheme(value);
        } else if (key == "verbose") {
            config.verbose = stringToBool(value);
        } else if (key == "memoryCheckEnabled") {
            config.memoryCheckEnabled = stringToBool(value);
        } else if (key == "defaultTimeout") {
            try {
                config.defaultTimeout = std::stoi(value);
            } catch (...) {
                // 忽略无效值
            }
        } else if (key == "logLevel") {
            config.logLevel = value;
        }
    }
    
    /**
     * @brief 字符串转布尔值
     * @param str 字符串
     * @return 布尔值
     */
    static bool stringToBool(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower == "true" || lower == "1" || lower == "yes" || lower == "on";
    }
    
    /**
     * @brief 字符串转主题
     * @param str 字符串
     * @return 主题枚举
     */
    static ColorTheme stringToTheme(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (lower == "modern") {
            return ColorTheme::MODERN;
        } else if (lower == "classic" || lower == "default") {
            return ColorTheme::CLASSIC;
        } else if (lower == "minimal" || lower == "light") {
            return ColorTheme::MINIMAL;
        } else if (lower == "dark") {
            return ColorTheme::MODERN; // 将dark映射到modern主题
        }
        
        return ColorTheme::MODERN; // 默认主题
    }
    
    /**
     * @brief 主题转字符串
     * @param theme 主题枚举
     * @return 字符串
     */
    static std::string themeToString(ColorTheme theme) {
        switch (theme) {
            case ColorTheme::MODERN:
                return "modern";
            case ColorTheme::CLASSIC:
                return "classic";
            case ColorTheme::MINIMAL:
                return "minimal";
            default:
                return "modern";
        }
    }
    
    /**
     * @brief 去除字符串首尾空白
     * @param str 要处理的字符串
     * @return 处理后的字符串
     */
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return "";
        }
        
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }
};

/**
 * @brief 全局配置管理器
 */
class GlobalConfig {
private:
    static TestConfig instance;
    static bool initialized;
    
public:
    /**
     * @brief 获取全局配置实例
     * @return 配置引用
     */
    static TestConfig& getInstance() {
        if (!initialized) {
            instance = ConfigLoader::loadConfig();
            initialized = true;
        }
        return instance;
    }
    
    /**
     * @brief 重新加载配置
     * @param configFilePath 配置文件路径（可选）
     */
    static void reload(const std::string& configFilePath = "") {
        instance = ConfigLoader::loadConfig(configFilePath);
        initialized = true;
    }
    
    /**
     * @brief 应用配置到测试框架
     */
    static void applyToFramework() {
        auto& config = getInstance();
        
        // 这里需要调用TestUtils的相应方法
        // TestUtils::setColorEnabled(config.colorEnabled);
        // TestUtils::setTheme(config.theme);
        // TestUtils::setVerbose(config.verbose);
        // MemoryTestUtils::setMemoryCheckEnabled(config.memoryCheckEnabled);
        // MemoryTestUtils::setDefaultTimeout(config.defaultTimeout);
    }
};

// 静态成员定义
TestConfig GlobalConfig::instance;
bool GlobalConfig::initialized = false;

} // namespace Config
} // namespace TestFramework
} // namespace Lua

#endif // LUA_TEST_FRAMEWORK_CONFIG_LOADER_HPP