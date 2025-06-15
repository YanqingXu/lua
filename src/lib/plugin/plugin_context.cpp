#include "plugin_context.hpp"
#include "plugin_manager.hpp"
#include "plugin_interface.hpp"
#include "../../common/defines.hpp"
#include "../lib_manager.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iostream>

namespace Lua {

// === PluginContext Implementation ===

PluginContext::PluginContext(PluginManager* manager, IPlugin* plugin, State* state)
    : manager_(manager), plugin_(plugin), state_(state), registry_(nullptr) {
    
    if (!manager_ || !plugin_ || !state_) {
        throw std::invalid_argument("PluginContext: Invalid parameters");
    }
    
    // 获取函数注册表（假设从状态机或管理器获取）
    // registry_ = state_->getFunctionRegistry(); // 需要根据实际API调整
    
    // 初始化目录
    initializeDirectories();
    
    // 加载插件配置
    loadConfig();
    
    // 设置默认权限
    permissions_.push_back("basic");
    permissions_.push_back("file_read");
    permissions_.push_back("log_write");
}

// === 基础服务接口 ===

StrView PluginContext::getPluginName() const noexcept {
    return plugin_ ? plugin_->getMetadata().name : "";
}

// === 日志服务 ===

void PluginContext::log(PluginLogLevel level, StrView message) {
    logWithPrefix(level, message);
}

// === 配置服务 ===

Str PluginContext::getConfig(StrView key, StrView defaultValue) const {
    auto it = config_.find(Str(key));
    return (it != config_.end()) ? it->second : Str(defaultValue);
}

void PluginContext::setConfig(StrView key, StrView value) {
    config_[Str(key)] = Str(value);
}

HashMap<Str, Str> PluginContext::getAllConfig() const {
    return config_;
}

bool PluginContext::saveConfig() {
    try {
        Str configPath = getConfigDirectory() + "/config.ini";
        std::ofstream file(configPath);
        
        if (!file.is_open()) {
            logError("Failed to open config file for writing: " + configPath);
            return false;
        }
        
        file << "# Plugin configuration for " << getPluginName() << "\n";
        file << "# Generated automatically\n\n";
        
        for (const auto& [key, value] : config_) {
            file << key << "=" << value << "\n";
        }
        
        file.close();
        logInfo("Configuration saved to: " + configPath);
        return true;
        
    } catch (const std::exception& e) {
        logError("Exception while saving config: " + Str(e.what()));
        return false;
    }
}

bool PluginContext::loadConfig() {
    try {
        Str configPath = getConfigDirectory() + "/config.ini";
        
        if (!fileExists(configPath)) {
            // 使用默认配置
            config_ = plugin_->getDefaultConfig();
            logInfo("Using default configuration");
            return true;
        }
        
        std::ifstream file(configPath);
        if (!file.is_open()) {
            logError("Failed to open config file: " + configPath);
            return false;
        }
        
        config_.clear();
        Str line;
        
        while (std::getline(file, line)) {
            // 跳过注释和空行
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            size_t pos = line.find('=');
            if (pos != Str::npos) {
                Str key = line.substr(0, pos);
                Str value = line.substr(pos + 1);
                
                // 去除前后空格
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                config_[key] = value;
            }
        }
        
        file.close();
        logInfo("Configuration loaded from: " + configPath);
        return true;
        
    } catch (const std::exception& e) {
        logError("Exception while loading config: " + Str(e.what()));
        return false;
    }
}

// === 插件间通信 ===

IPlugin* PluginContext::findPlugin(StrView name) const {
    if (!manager_) return nullptr;
    
    // 通过插件管理器查找插件
    // return manager_->getPlugin(name); // 需要根据实际API调整
    return nullptr; // 临时返回
}

bool PluginContext::hasPlugin(StrView name) const {
    return findPlugin(name) != nullptr;
}

Vec<Str> PluginContext::getLoadedPlugins() const {
    if (!manager_) return {};
    
    // 通过插件管理器获取已加载插件列表
    // return manager_->getLoadedPluginNames(); // 需要根据实际API调整
    return {}; // 临时返回
}

bool PluginContext::sendMessage(StrView targetPlugin, StrView message, const HashMap<Str, Str>& data) {
    IPlugin* target = findPlugin(targetPlugin);
    if (!target) {
        logWarning("Target plugin not found: " + Str(targetPlugin));
        return false;
    }
    
    // 创建消息事件
    PluginEvent event(PluginEventType::StateChanged, Str(targetPlugin));
    event.data = data;
    event.data["message"] = Str(message);
    event.data["sender"] = Str(getPluginName());
    
    // 触发事件
    fireEvent(event);
    
    logDebug("Message sent to plugin: " + Str(targetPlugin));
    return true;
}

// === 事件系统 ===

void PluginContext::addEventListener(PluginEventType type, PluginEventListener listener) {
    eventListeners_[type].push_back(std::move(listener));
    logDebug("Event listener added for type: " + std::to_string(static_cast<int>(type)));
}

void PluginContext::removeEventListener(PluginEventType type) {
    auto it = eventListeners_.find(type);
    if (it != eventListeners_.end()) {
        it->second.clear();
        logDebug("Event listeners removed for type: " + std::to_string(static_cast<int>(type)));
    }
}

void PluginContext::fireEvent(const PluginEvent& event) {
    auto it = eventListeners_.find(event.type);
    if (it != eventListeners_.end()) {
        for (const auto& listener : it->second) {
            try {
                listener(event);
            } catch (const std::exception& e) {
                logError("Exception in event listener: " + Str(e.what()));
            }
        }
    }
    
    // 也通知插件管理器
    if (manager_) {
        // manager_->notifyEvent(event); // 需要根据实际API调整
    }
}

void PluginContext::fireCustomEvent(StrView eventName, const HashMap<Str, Str>& data) {
    PluginEvent event(PluginEventType::StateChanged, Str(getPluginName()));
    event.data = data;
    event.data["custom_event"] = Str(eventName);
    
    fireEvent(event);
}

// === 资源管理 ===

Str PluginContext::getDataDirectory() const {
    return getPluginDirectory("data");
}

Str PluginContext::getConfigDirectory() const {
    return getPluginDirectory("config");
}

Str PluginContext::getTempDirectory() const {
    return getPluginDirectory("temp");
}

bool PluginContext::createDirectory(StrView path) const {
    try {
        std::filesystem::create_directories(path);
        return true;
    } catch (const std::exception& e) {
        //logError("Failed to create directory: " + Str(path) + " - " + e.what());
		std::cerr << "Failed to create directory: " << path << " - " << e.what() << std::endl;
        return false;
    }
}

bool PluginContext::fileExists(StrView path) const {
    return std::filesystem::exists(path);
}

Str PluginContext::readFile(StrView path) const {
    if (!hasPermission("file_read")) {
        //logError("Permission denied: file_read");
		std::cerr << "Permission denied: file_read" << std::endl;
        return "";
    }
    
    try {
        std::ifstream file(path.data());
        if (!file.is_open()) {
            //logError("Failed to open file: " + Str(path));
			std::cerr << "Failed to open file: " << path << std::endl;
            return "";
        }
        
        std::ostringstream content;
        content << file.rdbuf();
        file.close();
        
        return content.str();
        
    } catch (const std::exception& e) {
        //logError("Exception while reading file: " + Str(e.what()));
		std::cerr << "Exception while reading file: " << e.what() << std::endl;
        return "";
    }
}

bool PluginContext::writeFile(StrView path, StrView content) const {
    if (!hasPermission("file_write")) {
        //logError("Permission denied: file_write");
		std::cerr << "Permission denied: file_write" << std::endl;
        return false;
    }
    
    try {
        // 确保目录存在
        std::filesystem::path filePath(path);
        std::filesystem::create_directories(filePath.parent_path());
        
        std::ofstream file(path.data());
        if (!file.is_open()) {
            //logError("Failed to open file for writing: " + Str(path));
			std::cerr << "Failed to open file for writing: " << path << std::endl;
            return false;
        }
        
        file << content;
        file.close();
        
        return true;
        
    } catch (const std::exception& e) {
        //logError("Exception while writing file: " + Str(e.what()));
		std::cerr << "Exception while writing file: " << e.what() << std::endl;
        return false;
    }
}

// === 函数注册服务 ===

void PluginContext::unregisterFunction(StrView name) {
    if (!registry_) {
        logError("Function registry not available");
        return;
    }
    
    // 尝试移除带插件前缀的函数
    Str fullName = Str(getPluginName()) + "." + Str(name);
    // registry_->unregisterFunction(fullName); // 需要根据实际API调整
    
    logDebug("Function unregistered: " + fullName);
}

// === 安全和权限 ===

bool PluginContext::hasPermission(StrView permission) const {
    auto it = std::find(permissions_.begin(), permissions_.end(), Str(permission));
    return it != permissions_.end();
}

bool PluginContext::requestPermission(StrView permission) {
    if (hasPermission(permission)) {
        return true;
    }
    
    // 简单实现：自动授予基本权限
    Vec<Str> autoGrantPermissions = {"basic", "log_write", "file_read", "config_read"};
    
    for (const auto& autoGrant : autoGrantPermissions) {
        if (autoGrant == permission) {
            permissions_.push_back(Str(permission));
            logInfo("Permission granted: " + Str(permission));
            return true;
        }
    }
    
    logWarning("Permission denied: " + Str(permission));
    return false;
}

Vec<Str> PluginContext::getPermissions() const {
    return permissions_;
}

// === 性能监控 ===

void PluginContext::startTimer(StrView name) {
    timers_[Str(name)] = std::chrono::high_resolution_clock::now();
}

void PluginContext::endTimer(StrView name) {
    auto it = timers_.find(Str(name));
    if (it != timers_.end()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double, std::milli>(endTime - it->second);
        
        performanceStats_[Str(name)] = duration.count();
        timers_.erase(it);
        
        logDebug("Timer " + Str(name) + ": " + std::to_string(duration.count()) + "ms");
    } else {
        logWarning("Timer not found: " + Str(name));
    }
}

HashMap<Str, double> PluginContext::getPerformanceStats() const {
    return performanceStats_;
}

// === 私有辅助方法 ===

void PluginContext::initializeDirectories() {
    // 创建插件相关目录
    createDirectory(getDataDirectory());
    createDirectory(getConfigDirectory());
    createDirectory(getTempDirectory());
}

Str PluginContext::getPluginDirectory(StrView subdir) const {
    // 构建插件目录路径
    Str basePath = "plugins/" + Str(getPluginName());
    
    if (!subdir.empty()) {
        basePath += "/" + Str(subdir);
    }
    
    return basePath;
}

void PluginContext::logWithPrefix(PluginLogLevel level, StrView message) {
    Str levelStr;
    switch (level) {
        case PluginLogLevel::Debug:   levelStr = "DEBUG"; break;
        case PluginLogLevel::Info:    levelStr = "INFO"; break;
        case PluginLogLevel::Warning: levelStr = "WARNING"; break;
        case PluginLogLevel::Error:   levelStr = "ERROR"; break;
    }
    
    Str fullMessage = "[" + Str(getPluginName()) + "][" + levelStr + "] " + Str(message);
    
    // 输出到控制台（实际项目中应该使用日志系统）
    switch (level) {
        case PluginLogLevel::Debug:
        case PluginLogLevel::Info:
            std::cout << fullMessage << std::endl;
            break;
        case PluginLogLevel::Warning:
        case PluginLogLevel::Error:
            std::cerr << fullMessage << std::endl;
            break;
    }
    
    // 如果有权限，也写入日志文件
    if (hasPermission("log_write")) {
        try {
            Str logPath = getDataDirectory() + "/plugin.log";
            std::ofstream logFile(logPath, std::ios::app);
            if (logFile.is_open()) {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                
                struct tm timeinfo;
                if (localtime_s(&timeinfo, &time_t) == 0) {
                    logFile << "[" << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S") 
                            << "] " << fullMessage << std::endl;
                }
                logFile.close();
            }
        } catch (...) {
            // 忽略日志文件写入错误
        }
    }
}

} // namespace Lua