#include "plugin_interface.hpp"
#include "plugin_context.hpp"
#include "plugin_manager.hpp"
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace Lua {
    
    // ===== PluginVersion 实现 =====
    
    bool PluginVersion::operator!=(const PluginVersion& other) const noexcept {
        return !(*this == other);
    }
    
    bool PluginVersion::operator<=(const PluginVersion& other) const noexcept {
        return *this < other || *this == other;
    }
    
    bool PluginVersion::operator>(const PluginVersion& other) const noexcept {
        return !(*this <= other);
    }
    
    bool PluginVersion::operator>=(const PluginVersion& other) const noexcept {
        return !(*this < other);
    }
    
    PluginVersion PluginVersion::fromString(const Str& versionStr) {
        PluginVersion version;
        std::istringstream iss(versionStr);
        std::string token;
        
        // 解析主版本号
        if (std::getline(iss, token, '.')) {
            try {
                version.major = static_cast<u32>(std::stoul(token));
            } catch (const std::exception&) {
                version.major = 0;
            }
        }
        
        // 解析次版本号
        if (std::getline(iss, token, '.')) {
            try {
                version.minor = static_cast<u32>(std::stoul(token));
            } catch (const std::exception&) {
                version.minor = 0;
            }
        }
        
        // 解析补丁版本号
        if (std::getline(iss, token)) {
            try {
                version.patch = static_cast<u32>(std::stoul(token));
            } catch (const std::exception&) {
                version.patch = 0;
            }
        }
        
        return version;
    }
    
    bool PluginVersion::isNewerThan(const PluginVersion& other) const noexcept {
        return *this > other;
    }
    
    bool PluginVersion::isOlderThan(const PluginVersion& other) const noexcept {
        return *this < other;
    }
    
    u32 PluginVersion::toNumeric() const noexcept {
        return (major << 16) | (minor << 8) | patch;
    }
    
    PluginVersion PluginVersion::fromNumeric(u32 numeric) noexcept {
        return PluginVersion(
            (numeric >> 16) & 0xFF,
            (numeric >> 8) & 0xFF,
            numeric & 0xFF
        );
    }
    
    // ===== PluginDependency 实现 =====
    
    bool PluginDependency::isSatisfiedBy(const PluginVersion& version) const noexcept {
        return version.isCompatible(minVersion);
    }
    
    bool PluginDependency::operator==(const PluginDependency& other) const noexcept {
        return name == other.name && 
               minVersion == other.minVersion && 
               optional == other.optional;
    }
    
    Str PluginDependency::toString() const {
        std::ostringstream oss;
        oss << name << " >= " << minVersion.toString();
        if (optional) {
            oss << " (optional)";
        }
        return oss.str();
    }
    
    // ===== PluginMetadata 实现 =====
    
    bool PluginMetadata::isValid() const noexcept {
        return !name.empty() && 
               !displayName.empty() && 
               !author.empty();
    }
    
    bool PluginMetadata::hasProperty(const Str& key) const noexcept {
        return properties.find(key) != properties.end();
    }
    
    Str PluginMetadata::getProperty(const Str& key, const Str& defaultValue) const {
        auto it = properties.find(key);
        return (it != properties.end()) ? it->second : defaultValue;
    }
    
    void PluginMetadata::setProperty(const Str& key, const Str& value) {
        properties[key] = value;
    }
    
    void PluginMetadata::removeProperty(const Str& key) {
        properties.erase(key);
    }
    
    Vec<Str> PluginMetadata::getPropertyKeys() const {
        Vec<Str> keys;
        keys.reserve(properties.size());
        for (const auto& pair : properties) {
            keys.push_back(pair.first);
        }
        return keys;
    }
    
    bool PluginMetadata::hasDependency(const Str& pluginName) const noexcept {
        return std::any_of(dependencies.begin(), dependencies.end(),
            [&pluginName](const PluginDependency& dep) {
                return dep.name == pluginName;
            });
    }
    
    const PluginDependency* PluginMetadata::getDependency(const Str& pluginName) const noexcept {
        auto it = std::find_if(dependencies.begin(), dependencies.end(),
            [&pluginName](const PluginDependency& dep) {
                return dep.name == pluginName;
            });
        return (it != dependencies.end()) ? &(*it) : nullptr;
    }
    
    void PluginMetadata::addDependency(const PluginDependency& dependency) {
        // 检查是否已存在同名依赖
        auto it = std::find_if(dependencies.begin(), dependencies.end(),
            [&dependency](const PluginDependency& dep) {
                return dep.name == dependency.name;
            });
        
        if (it != dependencies.end()) {
            *it = dependency; // 更新现有依赖
        } else {
            dependencies.push_back(dependency); // 添加新依赖
        }
    }
    
    void PluginMetadata::removeDependency(const Str& pluginName) {
        dependencies.erase(
            std::remove_if(dependencies.begin(), dependencies.end(),
                [&pluginName](const PluginDependency& dep) {
                    return dep.name == pluginName;
                }),
            dependencies.end());
    }
    
    Vec<Str> PluginMetadata::getRequiredDependencies() const {
        Vec<Str> required;
        for (const auto& dep : dependencies) {
            if (!dep.optional) {
                required.push_back(dep.name);
            }
        }
        return required;
    }
    
    Vec<Str> PluginMetadata::getOptionalDependencies() const {
        Vec<Str> optional;
        for (const auto& dep : dependencies) {
            if (dep.optional) {
                optional.push_back(dep.name);
            }
        }
        return optional;
    }
    
    Str PluginMetadata::toString() const {
        std::ostringstream oss;
        oss << "Plugin: " << displayName << " (" << name << ")\n";
        oss << "Version: " << version.toString() << "\n";
        oss << "Author: " << author << "\n";
        oss << "Description: " << description << "\n";
        oss << "License: " << license << "\n";
        oss << "API Version: " << apiVersion.toString() << "\n";
        
        if (!dependencies.empty()) {
            oss << "Dependencies:\n";
            for (const auto& dep : dependencies) {
                oss << "  - " << dep.toString() << "\n";
            }
        }
        
        if (!properties.empty()) {
            oss << "Properties:\n";
            for (const auto& prop : properties) {
                oss << "  " << prop.first << ": " << prop.second << "\n";
            }
        }
        
        return oss.str();
    }
    
    // ===== 插件状态相关函数 =====
    
    Str pluginStateToString(PluginState state) {
        switch (state) {
            case PluginState::Unloaded:     return "Unloaded";
            case PluginState::Loading:      return "Loading";
            case PluginState::Loaded:       return "Loaded";
            case PluginState::Initializing: return "Initializing";
            case PluginState::Active:       return "Active";
            case PluginState::Stopping:     return "Stopping";
            case PluginState::Stopped:      return "Stopped";
            case PluginState::Error:        return "Error";
            default:                        return "Unknown";
        }
    }
    
    PluginState stringToPluginState(const Str& stateStr) {
        if (stateStr == "Unloaded")     return PluginState::Unloaded;
        if (stateStr == "Loading")      return PluginState::Loading;
        if (stateStr == "Loaded")       return PluginState::Loaded;
        if (stateStr == "Initializing") return PluginState::Initializing;
        if (stateStr == "Active")       return PluginState::Active;
        if (stateStr == "Stopping")     return PluginState::Stopping;
        if (stateStr == "Stopped")      return PluginState::Stopped;
        if (stateStr == "Error")        return PluginState::Error;
        return PluginState::Unloaded; // 默认状态
    }
    
    bool isValidStateTransition(PluginState from, PluginState to) {
        switch (from) {
            case PluginState::Unloaded:
                return to == PluginState::Loading || to == PluginState::Error;
                
            case PluginState::Loading:
                return to == PluginState::Loaded || to == PluginState::Error;
                
            case PluginState::Loaded:
                return to == PluginState::Initializing || 
                       to == PluginState::Stopping || 
                       to == PluginState::Error;
                
            case PluginState::Initializing:
                return to == PluginState::Active || to == PluginState::Error;
                
            case PluginState::Active:
                return to == PluginState::Stopping || to == PluginState::Error;
                
            case PluginState::Stopping:
                return to == PluginState::Stopped || to == PluginState::Error;
                
            case PluginState::Stopped:
                return to == PluginState::Unloaded || 
                       to == PluginState::Initializing || 
                       to == PluginState::Error;
                
            case PluginState::Error:
                return to == PluginState::Unloaded || to == PluginState::Stopping;
                
            default:
                return false;
        }
    }
    
    // ===== IPlugin 基础实现 =====
    
    void IPlugin::initialize(State* state) {
        // 默认实现：调用LibModule的初始化
        LibModule::initialize(state);
    }
    
    void IPlugin::cleanup(State* state) {
        // 默认实现：调用LibModule的清理
        LibModule::cleanup(state);
    }
    
    StrView IPlugin::getName() const noexcept {
        return getMetadata().name;
    }
    
    StrView IPlugin::getDisplayName() const noexcept {
        const auto& metadata = getMetadata();
        return metadata.displayName.empty() ? metadata.name : metadata.displayName;
    }
    
    StrView IPlugin::getLicense() const noexcept {
        return getMetadata().license;
    }
    
    const Vec<PluginDependency>& IPlugin::getDependencies() const noexcept {
        return getMetadata().dependencies;
    }
    
    PluginVersion IPlugin::getApiVersion() const noexcept {
        return getMetadata().apiVersion;
    }
    
    bool IPlugin::hasProperty(const Str& key) const noexcept {
        return getMetadata().hasProperty(key);
    }
    
    Str IPlugin::getProperty(const Str& key, const Str& defaultValue) const {
        return getMetadata().getProperty(key, defaultValue);
    }
    
    bool IPlugin::isCompatibleWith(const PluginVersion& apiVersion) const noexcept {
        return getApiVersion().isCompatible(apiVersion);
    }
    
    bool IPlugin::dependsOn(const Str& pluginName) const noexcept {
        return getMetadata().hasDependency(pluginName);
    }
    
    bool IPlugin::canCoexistWith(const IPlugin* other) const noexcept {
        if (!other) return true;
        
        // 检查是否有冲突的属性或资源
        // 默认实现：不同名称的插件可以共存
        return getName() != other->getName();
    }
    
    void IPlugin::onStateChanged(PluginState oldState, PluginState newState) {
        // 默认实现：空操作
        // 子类可以重写此方法来响应状态变化
    }
    
    void IPlugin::onDependencyLoaded(const Str& dependencyName, IPlugin* dependency) {
        // 默认实现：空操作
        // 子类可以重写此方法来响应依赖加载
    }
    
    void IPlugin::onDependencyUnloaded(const Str& dependencyName) {
        // 默认实现：空操作
        // 子类可以重写此方法来响应依赖卸载
    }
    
    Str IPlugin::getStatusInfo() const {
        std::ostringstream oss;
        oss << "Plugin: " << getDisplayName() << "\n";
        oss << "State: " << pluginStateToString(getState()) << "\n";
        oss << "Version: " << getVersion().toString() << "\n";
        oss << "Active: " << (isActive() ? "Yes" : "No") << "\n";
        oss << "Can Unload: " << (canUnload() ? "Yes" : "No") << "\n";
        return oss.str();
    }
    
    // ===== IPluginFactory 基础实现 =====
    
    bool IPluginFactory::validatePlugin(const UPtr<IPlugin>& plugin) const {
        if (!plugin) return false;
        
        const auto& metadata = plugin->getMetadata();
        
        // 验证基本元数据
        if (!metadata.isValid()) return false;
        
        // 验证版本兼容性
        if (!isApiCompatible(metadata.apiVersion)) return false;
        
        return true;
    }
    
    UPtr<IPlugin> IPluginFactory::createValidatedPlugin() {
        auto plugin = createPlugin();
        if (!validatePlugin(plugin)) {
            return nullptr;
        }
        return plugin;
    }
    
    Str IPluginFactory::getPluginName() const {
        return getPluginMetadata().name;
    }
    
    PluginVersion IPluginFactory::getPluginVersion() const {
        return getPluginMetadata().version;
    }
    
    bool IPluginFactory::supportsHotReload() const {
        // 默认实现：不支持热重载
        return false;
    }
    
    // ===== 全局插件API版本管理 =====
    
    namespace {
        constexpr u32 CURRENT_PLUGIN_API_VERSION = (1 << 16) | (0 << 8) | 0; // 1.0.0
    }
    
    u32 getCurrentPluginApiVersion() noexcept {
        return CURRENT_PLUGIN_API_VERSION;
    }
    
    PluginVersion getCurrentPluginApiVersionStruct() noexcept {
        return PluginVersion::fromNumeric(CURRENT_PLUGIN_API_VERSION);
    }
    
    bool isPluginApiCompatible(u32 pluginApiVersion) noexcept {
        PluginVersion current = getCurrentPluginApiVersionStruct();
        PluginVersion plugin = PluginVersion::fromNumeric(pluginApiVersion);
        return plugin.isCompatible(current);
    }
    
    bool isPluginApiCompatible(const PluginVersion& pluginApiVersion) noexcept {
        PluginVersion current = getCurrentPluginApiVersionStruct();
        return pluginApiVersion.isCompatible(current);
    }
    
} // namespace Lua