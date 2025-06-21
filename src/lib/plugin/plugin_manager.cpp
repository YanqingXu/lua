#include "plugin_manager.hpp"
#include "plugin_loader.hpp"
#include "plugin_sandbox.hpp"
#include "plugin_registry.hpp"
#include "plugin_interface.hpp"
#include "plugin_context.hpp"
#include "../../common/defines.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <queue>
#include <set>

namespace Lua {

// === PluginManager Implementation ===

PluginManager::PluginManager(State* state, LibManager* libManager)
    : state_(state), libManager_(libManager) {
    
    if (!state_) {
        throw std::invalid_argument("PluginManager: State cannot be null");
    }
    
    // 初始化组件
    loader_ = std::make_unique<PluginLoader>();
    sandbox_ = std::make_unique<PluginSandbox>("PluginManager");
    registry_ = std::make_unique<PluginRegistry>();
    
    logInfo("PluginManager created");
}

PluginManager::~PluginManager() {
    shutdown();
    logInfo("PluginManager destroyed");
}

// === 初始化和配置 ===

bool PluginManager::initialize() {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    
    if (initialized_) {
        logWarning("PluginManager already initialized");
        return true;
    }
    
    try {
        logInfo("Starting PluginManager initialization");
        
        // 组件在构造函数中已完成初始化
        logInfo("Components initialized");
        
        // 创建插件目录 - 跳过无效路径
        logInfo("Creating plugin directories");
        
        for (const auto& path : searchPaths_.systemPaths) {
            if (path.find('~') != Str::npos) {
                logWarning("Skipping path with tilde: " + path);
                continue;
            }
            try {
                logInfo("Creating system path: " + path);
                std::filesystem::create_directories(path);
            } catch (const std::exception& e) {
                logWarning("Failed to create system path " + path + ": " + e.what());
            }
        }
        
        for (const auto& path : searchPaths_.userPaths) {
            if (path.find('~') != Str::npos) {
                logWarning("Skipping path with tilde: " + path);
                continue;
            }
            try {
                logInfo("Creating user path: " + path);
                std::filesystem::create_directories(path);
            } catch (const std::exception& e) {
                logWarning("Failed to create user path " + path + ": " + e.what());
            }
        }
        
        for (const auto& path : searchPaths_.projectPaths) {
            if (path.find('~') != Str::npos) {
                logWarning("Skipping path with tilde: " + path);
                continue;
            }
            try {
                logInfo("Creating project path: " + path);
                std::filesystem::create_directories(path);
            } catch (const std::exception& e) {
                logWarning("Failed to create project path " + path + ": " + e.what());
            }
        }
        
        logInfo("Plugin directories created");
        
        // 加载配置
        logInfo("Loading configurations");
        loadAllConfigs();
        logInfo("Configurations loaded");
        
        initialized_ = true;
        logInfo("PluginManager initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        setError("Exception during initialization: " + Str(e.what()));
        logError("PluginManager initialization failed: " + Str(e.what()));
        return false;
    }
}

void PluginManager::shutdown() {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    
    if (!initialized_) {
        return;
    }
    
    try {
        // 停止文件监控
        stopFileWatcher();
        
        // 保存配置
        saveAllConfigs();
        
        // 卸载所有插件
        auto pluginNames = getLoadedPlugins();
        for (const auto& name : pluginNames) {
            unloadPluginInternal(name);
        }
        
        // 组件在析构时自动清理，无需显式shutdown调用
        
        // 清理数据
        loadedPlugins_.clear();
        pluginMetadata_.clear();
        pluginStates_.clear();
        pluginContexts_.clear();
        pluginConfigs_.clear();
        dependencyGraph_.clear();
        reverseDependencyGraph_.clear();
        pluginPermissions_.clear();
        eventListeners_.clear();
        pluginErrors_.clear();
        performanceStats_.clear();
        
        initialized_ = false;
        logInfo("PluginManager shutdown completed");
        
    } catch (const std::exception& e) {
        logError("Exception during shutdown: " + Str(e.what()));
    }
}

void PluginManager::setSearchPaths(const PluginSearchPaths& paths) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    searchPaths_ = paths;
    logInfo("Search paths updated");
}

void PluginManager::addSearchPath(StrView path, bool isSystemPath) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    
    if (isSystemPath) {
        searchPaths_.systemPaths.push_back(Str(path));
    } else {
        searchPaths_.userPaths.push_back(Str(path));
    }
    
    // 创建目录
    std::filesystem::create_directories(path);
    
    logInfo("Added search path: " + Str(path));
}

// === 插件发现和加载 ===

Vec<PluginMetadata> PluginManager::scanPlugins() {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    Vec<PluginMetadata> plugins;
    
    // 扫描所有搜索路径
    auto allPaths = searchPaths_.systemPaths;
    allPaths.insert(allPaths.end(), searchPaths_.userPaths.begin(), searchPaths_.userPaths.end());
    allPaths.insert(allPaths.end(), searchPaths_.projectPaths.begin(), searchPaths_.projectPaths.end());
    
    for (const auto& searchPath : allPaths) {
        try {
            if (!std::filesystem::exists(searchPath)) {
                continue;
            }
            
            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(searchPath, ec)) {
                if (ec) {
                    logWarning("Error iterating directory " + searchPath + ": " + ec.message());
                    break;
                }
                if (entry.is_regular_file()) {
                    auto ext = entry.path().extension().string();
                    if (ext == ".dll" || ext == ".so" || ext == ".dylib") {
                        // 尝试获取插件元数据
                        auto metadata = loader_->preloadMetadata(entry.path().string());
                        if (metadata.has_value()) {
                            plugins.push_back(metadata.value());
                        }
                    }
                }
            }
        } catch (const std::exception& e) {
            logWarning("Error scanning path " + searchPath + ": " + e.what());
        }
    }
    
    logInfo("Scanned " + std::to_string(plugins.size()) + " plugins");
    return plugins;
}

bool PluginManager::loadPlugin(StrView name, const PluginLoadOptions& options) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    return loadPluginInternal(name, options);
}

bool PluginManager::loadPluginFromFile(StrView filePath, const PluginLoadOptions& options) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    
    try {
        if (!std::filesystem::exists(filePath)) {
            setError("Plugin file not found: " + Str(filePath));
            return false;
        }
        
        // 获取插件元数据
        auto metadata = loader_->preloadMetadata(Str(filePath));
        if (!metadata.has_value()) {
            setError("Failed to get plugin metadata from: " + Str(filePath));
            return false;
        }
        
        // 检查是否已加载
        if (isPluginLoaded(metadata->name)) {
            logWarning("Plugin already loaded: " + metadata->name);
            return true;
        }
        
        // 加载插件
        auto result = loader_->loadFromFile(Str(filePath));
        if (!result.success) {
            setError("Failed to load plugin from: " + Str(filePath) + ". Error: " + result.errorMessage);
            return false;
        }
        auto plugin = std::move(result.plugin);
        if (!plugin) {
            setError("Plugin loaded but factory returned null plugin from: " + Str(filePath));
            return false;
        }
        
        // 存储插件信息
        pluginMetadata_[metadata->name] = *metadata;
        loadedPlugins_[metadata->name] = std::move(plugin);
        pluginStates_[metadata->name] = PluginState::Loaded;
        
        // 创建插件上下文
        auto context = createContext(loadedPlugins_[metadata->name].get());
        pluginContexts_[metadata->name] = std::move(context);
        
        // 设置配置
        if (!options.config.empty()) {
            pluginConfigs_[metadata->name] = options.config;
        }
        
        // 设置权限
        if (!options.permissions.empty()) {
            pluginPermissions_[metadata->name] = options.permissions;
        }
        
        // 初始化插件
        if (!loadedPlugins_[metadata->name]->onLoad(pluginContexts_[metadata->name].get())) {
            setError("Plugin initialization failed: " + metadata->name);
            unloadPluginInternal(metadata->name);
            return false;
        }
        
        pluginStates_[metadata->name] = PluginState::Active;
        
        // 触发事件
        PluginEvent event(PluginEventType::PluginLoaded, metadata->name);
        fireEvent(event);
        
        logInfo("Plugin loaded successfully: " + metadata->name);
        return true;
        
    } catch (const std::exception& e) {
        setError("Exception while loading plugin: " + Str(e.what()));
        return false;
    }
}

bool PluginManager::unloadPlugin(StrView name) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    return unloadPluginInternal(name);
}

bool PluginManager::reloadPlugin(StrView name) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    
    if (!isPluginLoaded(name)) {
        setError("Plugin not loaded: " + Str(name));
        return false;
    }
    
    // 保存配置
    auto config = pluginConfigs_[Str(name)];
    auto permissions = pluginPermissions_[Str(name)];
    
    // 卸载插件
    if (!unloadPluginInternal(name)) {
        return false;
    }
    
    // 重新加载
    PluginLoadOptions options;
    options.config = config;
    options.permissions = permissions;
    
    return loadPluginInternal(name, options);
}

Vec<Str> PluginManager::loadPlugins(const Vec<Str>& names, const PluginLoadOptions& options) {
    Vec<Str> loaded;
    
    for (const auto& name : names) {
        if (loadPlugin(name, options)) {
            loaded.push_back(name);
        }
    }
    
    return loaded;
}

Vec<Str> PluginManager::autoLoadPlugins(const PluginLoadOptions& options) {
    auto availablePlugins = scanPlugins();
    Vec<Str> pluginNames;
    
    for (const auto& metadata : availablePlugins) {
        pluginNames.push_back(metadata.name);
    }
    
    return loadPlugins(pluginNames, options);
}

// === 插件查询和状态 ===

bool PluginManager::isPluginLoaded(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    return loadedPlugins_.find(Str(name)) != loadedPlugins_.end();
}

IPlugin* PluginManager::getPlugin(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto it = loadedPlugins_.find(Str(name));
    return (it != loadedPlugins_.end()) ? it->second.get() : nullptr;
}

Opt<PluginMetadata> PluginManager::getPluginMetadata(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto it = pluginMetadata_.find(Str(name));
    if (it != pluginMetadata_.end()) {
        return it->second;
    }
    return std::nullopt;
}

PluginState PluginManager::getPluginState(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto it = pluginStates_.find(Str(name));
    return (it != pluginStates_.end()) ? it->second : PluginState::Unloaded;
}

Vec<Str> PluginManager::getLoadedPlugins() const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    Vec<Str> names;
    for (const auto& [name, plugin] : loadedPlugins_) {
        names.push_back(name);
    }
    return names;
}

Vec<PluginMetadata> PluginManager::getAvailablePlugins() const {
    return const_cast<PluginManager*>(this)->scanPlugins();
}

// === 依赖管理 ===

Vec<Str> PluginManager::resolveDependencies(StrView pluginName) const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    Vec<Str> dependencies;
    std::set<Str> visited;
    std::queue<Str> queue;
    
    queue.push(Str(pluginName));
    
    while (!queue.empty()) {
        Str current = queue.front();
        queue.pop();
        
        if (visited.find(current) != visited.end()) {
            continue;
        }
        visited.insert(current);
        
        auto it = dependencyGraph_.find(current);
        if (it != dependencyGraph_.end()) {
            for (const auto& dep : it->second) {
                if (visited.find(dep) == visited.end()) {
                    dependencies.push_back(dep);
                    queue.push(dep);
                }
            }
        }
    }
    
    return dependencies;
}

bool PluginManager::checkDependencies(StrView pluginName) const {
    auto dependencies = resolveDependencies(pluginName);
    
    for (const auto& dep : dependencies) {
        if (!isPluginLoaded(dep)) {
            return false;
        }
    }
    
    return true;
}

HashMap<Str, Vec<Str>> PluginManager::getDependencyGraph() const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    return dependencyGraph_;
}

Vec<Str> PluginManager::getLoadOrder(const Vec<Str>& pluginNames) const {
    return topologicalSort(pluginNames);
}

// === 插件启用/禁用 ===

bool PluginManager::enablePlugin(StrView name) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto plugin = getPlugin(name);
    if (!plugin) {
        setError("Plugin not found: " + Str(name));
        return false;
    }
    
    if (pluginStates_[Str(name)] == PluginState::Active) {
        return true;
    }
    
    try {
        auto contextIt = pluginContexts_.find(Str(name));
        if (contextIt == pluginContexts_.end()) {
            setError("Plugin context not found: " + Str(name));
            return false;
        }
        
        if (plugin->onEnable(contextIt->second.get())) {
            pluginStates_[Str(name)] = PluginState::Active;
            
            PluginEvent event(PluginEventType::PluginEnabled, Str(name));
            fireEvent(event);
            
            logInfo("Plugin enabled: " + Str(name));
            return true;
        } else {
            setError("Plugin enable failed: " + Str(name));
            return false;
        }
    } catch (const std::exception& e) {
        setError("Exception while enabling plugin: " + Str(e.what()));
        return false;
    }
}

bool PluginManager::disablePlugin(StrView name) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto plugin = getPlugin(name);
    if (!plugin) {
        setError("Plugin not found: " + Str(name));
        return false;
    }
    
    if (pluginStates_[Str(name)] == PluginState::Stopped) {
        return true;
    }
    
    try {
        auto contextIt = pluginContexts_.find(Str(name));
        if (contextIt == pluginContexts_.end()) {
            setError("Plugin context not found: " + Str(name));
            return false;
        }
        
        plugin->onDisable(contextIt->second.get());
        pluginStates_[Str(name)] = PluginState::Stopped;
        
        PluginEvent event(PluginEventType::PluginDisabled, Str(name));
        fireEvent(event);
        
        logInfo("Plugin disabled: " + Str(name));
        return true;
        
    } catch (const std::exception& e) {
        setError("Exception while disabling plugin: " + Str(e.what()));
        return false;
    }
}

bool PluginManager::isPluginEnabled(StrView name) const {
    return getPluginState(name) == PluginState::Active;
}

// === 热重载支持 ===

void PluginManager::enableHotReload(bool enable) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    
    if (hotReloadEnabled_ == enable) {
        return;
    }
    
    hotReloadEnabled_ = enable;
    
    if (enable) {
        startFileWatcher();
    } else {
        stopFileWatcher();
    }
    
    logInfo("Hot reload " + Str(enable ? "enabled" : "disabled"));
}

void PluginManager::startFileWatcher() {
    if (fileWatcherRunning_) {
        return;
    }
    
    fileWatcherRunning_ = true;
    fileWatcherThread_ = std::make_unique<std::thread>(&PluginManager::fileWatcherLoop, this);
    
    logInfo("File watcher started");
}

void PluginManager::stopFileWatcher() {
    if (!fileWatcherRunning_) {
        return;
    }
    
    fileWatcherRunning_ = false;
    
    if (fileWatcherThread_ && fileWatcherThread_->joinable()) {
        fileWatcherThread_->join();
    }
    
    fileWatcherThread_.reset();
    
    logInfo("File watcher stopped");
}

// === 事件系统 ===

void PluginManager::addEventListener(PluginEventType type, PluginEventListener listener) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    eventListeners_[type].push_back(std::move(listener));
}

void PluginManager::removeEventListener(PluginEventType type) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    eventListeners_[type].clear();
}

void PluginManager::fireEvent(const PluginEvent& event) {
    std::lock_guard<std::mutex> lock(eventMutex_);
    
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
}

// === 插件间通信 ===

bool PluginManager::sendMessage(StrView targetPlugin, StrView sourcePlugin, StrView message, const HashMap<Str, Str>& data) {
    auto target = getPlugin(targetPlugin);
    if (!target) {
        setError("Target plugin not found: " + Str(targetPlugin));
        return false;
    }
    
    PluginEvent event(PluginEventType::StateChanged, Str(targetPlugin));
    event.data = data;
    event.data["message"] = Str(message);
    event.data["source"] = Str(sourcePlugin);
    
    fireEvent(event);
    
    logDebug("Message sent from " + Str(sourcePlugin) + " to " + Str(targetPlugin));
    return true;
}

void PluginManager::broadcastMessage(StrView sourcePlugin, StrView message, const HashMap<Str, Str>& data) {
    auto plugins = getLoadedPlugins();
    
    for (const auto& pluginName : plugins) {
        if (pluginName != sourcePlugin) {
            sendMessage(pluginName, sourcePlugin, message, data);
        }
    }
}

// === 配置管理 ===

HashMap<Str, Str> PluginManager::getPluginConfig(StrView name) const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto it = pluginConfigs_.find(Str(name));
    return (it != pluginConfigs_.end()) ? it->second : HashMap<Str, Str>{};
}

void PluginManager::setPluginConfig(StrView name, const HashMap<Str, Str>& config) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    pluginConfigs_[Str(name)] = config;
}

bool PluginManager::saveAllConfigs() {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    try {
        Str configPath = "plugins/config/manager.ini";
        std::filesystem::create_directories(std::filesystem::path(configPath).parent_path());
        
        std::ofstream file(configPath);
        if (!file.is_open()) {
            setError("Failed to open config file for writing");
            return false;
        }
        
        file << "# Plugin Manager Configuration\n";
        file << "# Generated automatically\n\n";
        
        for (const auto& [pluginName, config] : pluginConfigs_) {
            file << "[" << pluginName << "]\n";
            for (const auto& [key, value] : config) {
                file << key << "=" << value << "\n";
            }
            file << "\n";
        }
        
        file.close();
        logInfo("All configurations saved");
        return true;
        
    } catch (const std::exception& e) {
        setError("Exception while saving configs: " + Str(e.what()));
        return false;
    }
}

bool PluginManager::loadAllConfigs() {
    try {
        Str configPath = "plugins/config/manager.ini";
        
        if (!std::filesystem::exists(configPath)) {
            logInfo("No configuration file found, using defaults");
            return true;
        }
        
        std::ifstream file(configPath);
        if (!file.is_open()) {
            setError("Failed to open config file for reading");
            return false;
        }
        
        Str line;
        Str currentPlugin;
        
        while (std::getline(file, line)) {
            // 跳过注释和空行
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // 检查是否为节标题
            if (line[0] == '[' && line.back() == ']') {
                currentPlugin = line.substr(1, line.length() - 2);
                continue;
            }
            
            // 解析键值对
            if (!currentPlugin.empty()) {
                size_t pos = line.find('=');
                if (pos != Str::npos) {
                    Str key = line.substr(0, pos);
                    Str value = line.substr(pos + 1);
                    
                    // 去除前后空格
                    key.erase(0, key.find_first_not_of(" \t"));
                    key.erase(key.find_last_not_of(" \t") + 1);
                    value.erase(0, value.find_first_not_of(" \t"));
                    value.erase(value.find_last_not_of(" \t") + 1);
                    
                    pluginConfigs_[currentPlugin][key] = value;
                }
            }
        }
        
        file.close();
        logInfo("All configurations loaded");
        return true;
        
    } catch (const std::exception& e) {
        setError("Exception while loading configs: " + Str(e.what()));
        return false;
    }
}

// === 安全和权限 ===

bool PluginManager::checkPermission(StrView pluginName, StrView permission) const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto it = pluginPermissions_.find(Str(pluginName));
    if (it == pluginPermissions_.end()) {
        return false;
    }
    
    const auto& permissions = it->second;
    return std::find(permissions.begin(), permissions.end(), Str(permission)) != permissions.end();
}

void PluginManager::grantPermission(StrView pluginName, StrView permission) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto& permissions = pluginPermissions_[Str(pluginName)];
    if (std::find(permissions.begin(), permissions.end(), Str(permission)) == permissions.end()) {
        permissions.push_back(Str(permission));
        logInfo("Permission granted to " + Str(pluginName) + ": " + Str(permission));
    }
}

void PluginManager::revokePermission(StrView pluginName, StrView permission) {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto it = pluginPermissions_.find(Str(pluginName));
    if (it != pluginPermissions_.end()) {
        auto& permissions = it->second;
        permissions.erase(
            std::remove(permissions.begin(), permissions.end(), Str(permission)),
            permissions.end()
        );
        logInfo("Permission revoked from " + Str(pluginName) + ": " + Str(permission));
    }
}

Vec<Str> PluginManager::getPluginPermissions(StrView pluginName) const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto it = pluginPermissions_.find(Str(pluginName));
    return (it != pluginPermissions_.end()) ? it->second : Vec<Str>{};
}

// === 性能监控 ===

HashMap<Str, HashMap<Str, double>> PluginManager::getPerformanceStats() const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    return performanceStats_;
}

void PluginManager::resetPerformanceStats() {
    std::unique_lock<std::shared_mutex> lock(pluginsMutex_);
    performanceStats_.clear();
    logInfo("Performance statistics reset");
}

// === 错误处理 ===

Vec<Str> PluginManager::getPluginErrors(StrView pluginName) const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    auto it = pluginErrors_.find(Str(pluginName));
    return (it != pluginErrors_.end()) ? it->second : Vec<Str>{};
}

// === 调试和诊断 ===

HashMap<Str, Str> PluginManager::getPluginDiagnostics(StrView pluginName) const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    HashMap<Str, Str> diagnostics;
    
    auto plugin = getPlugin(pluginName);
    if (!plugin) {
        diagnostics["error"] = "Plugin not found";
        return diagnostics;
    }
    
    auto metadata = getPluginMetadata(pluginName);
    if (metadata.has_value()) {
        diagnostics["name"] = metadata->name;
        diagnostics["version"] = metadata->version.toString();
        diagnostics["description"] = metadata->description;
        diagnostics["author"] = metadata->author;
    }
    
    diagnostics["state"] = std::to_string(static_cast<int>(getPluginState(pluginName)));
    diagnostics["enabled"] = isPluginEnabled(pluginName) ? "true" : "false";
    
    auto permissions = getPluginPermissions(pluginName);
    diagnostics["permissions"] = std::to_string(permissions.size());
    
    auto errors = getPluginErrors(pluginName);
    diagnostics["error_count"] = std::to_string(errors.size());
    
    return diagnostics;
}

Str PluginManager::exportPluginState() const {
    std::shared_lock<std::shared_mutex> lock(pluginsMutex_);
    
    std::ostringstream oss;
    oss << "Plugin Manager State Export\n";
    oss << "==========================\n\n";
    
    oss << "Loaded Plugins: " << loadedPlugins_.size() << "\n";
    for (const auto& [name, plugin] : loadedPlugins_) {
        oss << "  - " << name << " (" << static_cast<int>(getPluginState(name)) << ")\n";
    }
    
    oss << "\nSearch Paths:\n";
    for (const auto& path : searchPaths_.systemPaths) {
        oss << "  System: " << path << "\n";
    }
    for (const auto& path : searchPaths_.userPaths) {
        oss << "  User: " << path << "\n";
    }
    for (const auto& path : searchPaths_.projectPaths) {
        oss << "  Project: " << path << "\n";
    }
    
    oss << "\nHot Reload: " << (hotReloadEnabled_ ? "Enabled" : "Disabled") << "\n";
    oss << "Debug Mode: " << (debugMode_ ? "Enabled" : "Disabled") << "\n";
    
    return oss.str();
}

// === 内部接口 ===

UPtr<PluginContext> PluginManager::createContext(IPlugin* plugin) {
    return std::make_unique<PluginContext>(this, plugin, state_);
}

// === 私有方法 ===

bool PluginManager::loadPluginInternal(StrView name, const PluginLoadOptions& options) {
    try {
        // 检查是否已加载
        if (isPluginLoaded(name)) {
            logWarning("Plugin already loaded: " + Str(name));
            return true;
        }
        
        // 查找插件文件
        Str pluginPath;
        auto allPaths = searchPaths_.systemPaths;
        allPaths.insert(allPaths.end(), searchPaths_.userPaths.begin(), searchPaths_.userPaths.end());
        allPaths.insert(allPaths.end(), searchPaths_.projectPaths.begin(), searchPaths_.projectPaths.end());
        
        for (const auto& searchPath : allPaths) {
            Vec<Str> extensions = {".dll", ".so", ".dylib"};
            for (const auto& ext : extensions) {
                Str candidatePath = searchPath + "/" + Str(name) + ext;
                if (std::filesystem::exists(candidatePath)) {
                    pluginPath = candidatePath;
                    break;
                }
            }
            if (!pluginPath.empty()) break;
        }
        
        if (pluginPath.empty()) {
            setError("Plugin file not found: " + Str(name));
            return false;
        }
        
        // 使用文件路径加载
        return loadPluginFromFile(pluginPath, options);
        
    } catch (const std::exception& e) {
        setError("Exception while loading plugin: " + Str(e.what()));
        return false;
    }
}

bool PluginManager::unloadPluginInternal(StrView name) {
    try {
        auto it = loadedPlugins_.find(Str(name));
        if (it == loadedPlugins_.end()) {
            setError("Plugin not loaded: " + Str(name));
            return false;
        }
        
        // 禁用插件
        if (isPluginEnabled(name)) {
            disablePlugin(name);
        }
        
        // 调用卸载回调
        auto contextIt = pluginContexts_.find(Str(name));
        if (contextIt != pluginContexts_.end()) {
            it->second->onUnload(contextIt->second.get());
        }
        
        // 清理数据
        loadedPlugins_.erase(it);
        pluginStates_.erase(Str(name));
        pluginContexts_.erase(Str(name));
        
        // 触发事件
        PluginEvent event(PluginEventType::PluginUnloaded, Str(name));
        fireEvent(event);
        
        logInfo("Plugin unloaded: " + Str(name));
        return true;
        
    } catch (const std::exception& e) {
        setError("Exception while unloading plugin: " + Str(e.what()));
        return false;
    }
}

void PluginManager::buildDependencyGraph() {
    dependencyGraph_.clear();
    reverseDependencyGraph_.clear();
    
    for (const auto& [name, metadata] : pluginMetadata_) {
        Vec<Str> deps;
        for (const auto& dep : metadata.dependencies) {
            deps.push_back(dep.name);
            reverseDependencyGraph_[dep.name].push_back(name);
        }
        dependencyGraph_[name] = deps;
    }
}

Vec<Str> PluginManager::topologicalSort(const Vec<Str>& plugins) const {
    HashMap<Str, int> inDegree;
    HashMap<Str, Vec<Str>> adjList;
    
    // 初始化
    for (const auto& plugin : plugins) {
        inDegree[plugin] = 0;
        adjList[plugin] = {};
    }
    
    // 构建图
    for (const auto& plugin : plugins) {
        auto it = dependencyGraph_.find(plugin);
        if (it != dependencyGraph_.end()) {
            for (const auto& dep : it->second) {
                if (std::find(plugins.begin(), plugins.end(), dep) != plugins.end()) {
                    adjList[dep].push_back(plugin);
                    inDegree[plugin]++;
                }
            }
        }
    }
    
    // 拓扑排序
    std::queue<Str> queue;
    for (const auto& [plugin, degree] : inDegree) {
        if (degree == 0) {
            queue.push(plugin);
        }
    }
    
    Vec<Str> result;
    while (!queue.empty()) {
        Str current = queue.front();
        queue.pop();
        result.push_back(current);
        
        for (const auto& neighbor : adjList[current]) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                queue.push(neighbor);
            }
        }
    }
    
    return result;
}

bool PluginManager::hasCyclicDependency(const Vec<Str>& plugins) const {
    auto sorted = topologicalSort(plugins);
    return sorted.size() != plugins.size();
}

void PluginManager::setError(StrView error) {
    lastError_ = Str(error);
    logError(Str(error));
}

void PluginManager::addPluginError(StrView pluginName, StrView error) {
    pluginErrors_[Str(pluginName)].push_back(Str(error));
    
    // 限制错误历史数量
    auto& errors = pluginErrors_[Str(pluginName)];
    if (errors.size() > 100) {
        errors.erase(errors.begin(), errors.begin() + 50);
    }
}

void PluginManager::fileWatcherLoop() {
    // 简单的文件监控实现
    HashMap<Str, std::filesystem::file_time_type> lastWriteTimes;
    
    while (fileWatcherRunning_) {
        try {
            // 检查所有已加载插件的文件
            for (const auto& [name, plugin] : loadedPlugins_) {
                // 这里需要获取插件文件路径，简化实现
                // 实际项目中应该保存插件文件路径
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        } catch (const std::exception& e) {
            logError("Exception in file watcher: " + Str(e.what()));
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void PluginManager::handleFileChange(StrView filePath) {
    logInfo("File changed: " + Str(filePath));
    
    // 查找对应的插件并重新加载
    for (const auto& [name, plugin] : loadedPlugins_) {
        // 简化实现：假设文件名包含插件名
        if (Str(filePath).find(name) != Str::npos) {
            logInfo("Reloading plugin due to file change: " + name);
            reloadPlugin(name);
            break;
        }
    }
}

// === 日志方法 ===

void PluginManager::logDebug(StrView message) const {
    if (debugMode_) {
        std::cout << "[PluginManager][DEBUG] " << message << std::endl;
    }
}

void PluginManager::logInfo(StrView message) const {
    std::cout << "[PluginManager][INFO] " << message << std::endl;
}

void PluginManager::logWarning(StrView message) const {
    std::cerr << "[PluginManager][WARNING] " << message << std::endl;
}

void PluginManager::logError(StrView message) const {
    std::cerr << "[PluginManager][ERROR] " << message << std::endl;
}

// === 静态插件注册 ===

//void PluginManager::registerFactory(StrView name, UPtr<IPluginFactory> factory) {
//    StaticPluginRegistry::registerFactory(name, factory.release());
//    logInfo("Registered static plugin factory: " + Str(name));
//}

} // namespace Lua