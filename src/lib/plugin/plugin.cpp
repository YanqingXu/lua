#include "plugin.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

namespace Lua {

// PluginSystem Implementation
class PluginSystemImpl : public PluginSystem {
public:
    PluginSystemImpl(State* luaState, LibManager* libManager)
        : PluginSystem(luaState, libManager), luaState_(luaState), libManager_(libManager), state_(PluginSystemState::Uninitialized) {
        manager_ = std::make_unique<PluginManager>(luaState, libManager);
        loader_ = std::make_unique<PluginLoader>();
        registry_ = std::make_unique<PluginRegistry>();
        sandboxManager_ = std::make_unique<SandboxManager>();
    }

    ~PluginSystemImpl() {
        if (state_ != PluginSystemState::Shutdown) {
            shutdown();
        }
    }

    // System lifecycle
    bool initialize(const PluginSystemConfig& config) {
        if (state_ != PluginSystemState::Uninitialized) {
            lastError_ = "Plugin system already initialized";
            return false;
        }

        config_ = config;
        
        try {
            // Initialize components
            // Initialize components
            // All components are assumed to be ready for use

            // Initialize components
            // Registry and sandbox manager are assumed to be ready

            // Set up search paths
            setupSearchPaths();

            // Load configuration
            if (!config.configDirectory.empty()) {
                loadConfiguration(config.configDirectory + "/config.txt");
            }

            state_ = PluginSystemState::Running;

            // Auto-load plugins
            autoLoadPlugins(PluginLoadOptions{});

            return true;
        }
        catch (const std::exception& e) {
            lastError_ = "Exception during initialization: " + std::string(e.what());
            return false;
        }
    }

    void shutdown() {
        if (state_ == PluginSystemState::Shutdown) {
            return;
        }

        try {
            // Unload all plugins
            unloadAllPlugins();

            // Shutdown components
            if (manager_) {
                manager_->shutdown();
            }

            state_ = PluginSystemState::Shutdown;
        }
        catch (const std::exception& e) {
            // Log error but continue shutdown
            lastError_ = "Exception during shutdown: " + std::string(e.what());
        }
    }

    PluginSystemState getState() const {
        return state_;
    }

    // Plugin management
    bool loadPlugin(StrView name, const PluginLoadOptions& options) {
        if (state_ != PluginSystemState::Running) {
            lastError_ = "Plugin system not initialized";
            return false;
        }

        try {
            return manager_->loadPlugin(name, options);
        }
        catch (const std::exception& e) {
            lastError_ = "Exception loading plugin '" + std::string(name) + "': " + e.what();
            return false;
        }
    }

    bool unloadPlugin(StrView name) {
        if (state_ != PluginSystemState::Running) {
            lastError_ = "Plugin system not initialized";
            return false;
        }

        try {
            return manager_->unloadPlugin(name);
        }
        catch (const std::exception& e) {
            lastError_ = "Exception unloading plugin '" + std::string(name) + "': " + e.what();
            return false;
        }
    }

    bool reloadPlugin(StrView name) {
        if (!unloadPlugin(name)) {
            return false;
        }
        
        // Small delay to ensure cleanup
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        return loadPlugin(name, PluginLoadOptions{});
    }

    bool enablePlugin(StrView name) {
        return manager_->enablePlugin(name);
    }

    bool disablePlugin(StrView name) {
        return manager_->disablePlugin(name);
    }

    // Query methods
    IPlugin* getPlugin(StrView name) const {
        return manager_->getPlugin(name);
    }

    bool isPluginLoaded(StrView name) const {
        return manager_->isPluginLoaded(name);
    }

    bool isPluginEnabled(StrView name) const {
        return manager_->isPluginEnabled(name);
    }

    Vec<Str> getLoadedPlugins() const {
        return manager_->getLoadedPlugins();
    }

    Vec<PluginMetadata> getAvailablePlugins() const {
        Vec<Str> names = registry_->getRegisteredPluginNames();
        Vec<PluginMetadata> plugins;
        for (const auto& name : names) {
            auto metadata = registry_->getMetadata(name);
            if (metadata) {
                plugins.push_back(*metadata);
            }
        }
        return plugins;
    }

    Vec<PluginMetadata> scanPlugins() {
        Vec<PluginMetadata> plugins;
        
        for (const auto& path : config_.searchPaths.systemPaths) {
            auto found = loader_->scanDirectory(path);
            // 从PluginFileInfo转换为PluginMetadata
            for (const auto& fileInfo : found) {
                auto metadata = loader_->preloadMetadata(fileInfo.filePath);
                if (metadata.has_value()) {
                    plugins.push_back(metadata.value());
                }
            }
        }
        
        for (const auto& path : config_.searchPaths.userPaths) {
            auto found = loader_->scanDirectory(path);
            // 从PluginFileInfo转换为PluginMetadata
            for (const auto& fileInfo : found) {
                auto metadata = loader_->preloadMetadata(fileInfo.filePath);
                if (metadata.has_value()) {
                    plugins.push_back(metadata.value());
                }
            }
        }
        
        // Update registry
        for (const auto& plugin : plugins) {
            registry_->registerPlugin(plugin);
        }
        
        return plugins;
    }

    // Batch operations
    Vec<Str> loadPlugins(const Vec<Str>& names, const PluginLoadOptions& options) {
        Vec<Str> loaded;
        
        for (const auto& name : names) {
            if (loadPlugin(name, options)) {
                loaded.push_back(name);
            }
        }
        
        return loaded;
    }

    Vec<Str> autoLoadPlugins(const PluginLoadOptions& options) {
        Vec<Str> loaded;
        
        // Scan for available plugins
        auto available = scanPlugins();
        
        // Extract plugin names for dependency sorting
        Vec<Str> pluginNames;
        for (const auto& metadata : available) {
            pluginNames.push_back(metadata.name);
        }
        auto sorted = manager_->getLoadOrder(pluginNames);
        
        for (const auto& pluginName : sorted) {
            // Find the metadata for this plugin
            auto it = std::find_if(available.begin(), available.end(),
                [&pluginName](const PluginMetadata& meta) {
                    return meta.name == pluginName;
                });
            if (it != available.end() && loadPlugin(pluginName, options)) {
                loaded.push_back(pluginName);
            }
        }
        
        return loaded;
    }

    void unloadAllPlugins() {
        auto loaded = getLoadedPlugins();
        
        // Unload in reverse dependency order
        std::reverse(loaded.begin(), loaded.end());
        
        for (const auto& name : loaded) {
            unloadPlugin(name);
        }
    }

    // Configuration
    bool loadConfiguration(StrView configFile) {
        try {
            std::ifstream file;
            file.open(std::string(configFile));
            if (!file.is_open()) {
                lastError_ = "Cannot open config file: " + std::string(configFile);
                return false;
            }
            
            // Simple key=value parser
            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;
                
                auto pos = line.find('=');
                if (pos != std::string::npos) {
                    auto key = line.substr(0, pos);
                    auto value = line.substr(pos + 1);
                    configuration_[key] = value;
                }
            }
            
            return true;
        }
        catch (const std::exception& e) {
            lastError_ = "Exception loading configuration: " + std::string(e.what());
            return false;
        }
    }

    bool saveConfiguration(StrView configFile) {
        try {
            std::ofstream file;
            file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
            file.open(std::string(configFile));
            
            // Save configuration in JSON format
            file << "{\n";
            for (const auto& [key, value] : configuration_) {
                file << "  \"" << key << "\": \"" << value << "\",\n";
            }
            file << "}\n";
            
            return true;
        } catch (const std::exception&) {
            return false;
        }
    }

    Str getConfigValue(StrView key, StrView defaultValue) const {
        auto it = configuration_.find(std::string(key));
        return it != configuration_.end() ? it->second : std::string(defaultValue);
    }

    void setConfigValue(StrView key, StrView value) {
        configuration_[std::string(key)] = std::string(value);
    }

    // Statistics and monitoring
    PluginStatistics getPluginStatistics() const {
        // 从插件注册表获取统计信息
        PluginStatistics stats;
        auto loaded = manager_->getLoadedPlugins();
        stats.totalPlugins = loaded.size();
        stats.loadedPlugins = loaded.size();
        stats.enabledPlugins = 0;
        for (const auto& name : loaded) {
            auto plugin = manager_->getPlugin(name);
            if (plugin && plugin->getState() == PluginState::Active) {
                stats.enabledPlugins++;
            }
        }
        return stats;
    }

    HashMap<Str, HashMap<Str, double>> getPerformanceStatistics() const {
        return manager_->getPerformanceStats();
    }

    // Event system
    void addEventListener(PluginEventType eventType, PluginEventListener listener) {
        manager_->addEventListener(eventType, listener);
    }

    void removeEventListener(PluginEventType eventType) {
        manager_->removeEventListener(eventType);
    }

    void triggerEvent(const PluginEvent& event) {
        manager_->fireEvent(event);
    }

    // Error handling
    Str getLastError() const {
        return lastError_;
    }

    Vec<Str> getErrorHistory() const {
        Vec<Str> errors;
        // 收集所有插件的错误
        auto loaded = manager_->getLoadedPlugins();
        for (const auto& name : loaded) {
            auto pluginErrors = manager_->getPluginErrors(name);
            errors.insert(errors.end(), pluginErrors.begin(), pluginErrors.end());
        }
        return errors;
    }

    void clearErrors() {
        lastError_.clear();
        manager_->clearError();
    }

    // Debug and diagnostics
    HashMap<Str, Str> getDiagnostics() const {
        HashMap<Str, Str> diagnostics;
        
        diagnostics["state"] = stateToString(state_);
        diagnostics["loaded_plugins"] = std::to_string(getLoadedPlugins().size());
        diagnostics["available_plugins"] = std::to_string(getAvailablePlugins().size());
        diagnostics["search_paths"] = std::to_string(config_.searchPaths.systemPaths.size() + config_.searchPaths.userPaths.size());
        diagnostics["sandbox_enabled"] = config_.enableSandbox ? "true" : "false";
        diagnostics["hot_reload_enabled"] = config_.enableHotReload ? "true" : "false";
        
        return diagnostics;
    }

    void dumpState(StrView outputFile) const {
        try {
            std::ofstream file;
            file.open(std::string(outputFile));
            if (!file.is_open()) {
                return;
            }
            
            file << "=== Plugin System State Dump ===\n";
            file << "State: " << stateToString(state_) << "\n";
            file << "\n";
            
            auto diagnostics = getDiagnostics();
            file << "=== Diagnostics ===\n";
            for (const auto& [key, value] : diagnostics) {
                file << key << ": " << value << "\n";
            }
            file << "\n";
            
            auto loaded = getLoadedPlugins();
            file << "=== Loaded Plugins (" << loaded.size() << ") ===\n";
            for (const auto& name : loaded) {
                auto plugin = getPlugin(name);
                if (plugin) {
                    file << "- " << name << " (" << stateToString(plugin->getState()) << ")\n";
                }
            }
            file << "\n";
            
            auto available = getAvailablePlugins();
            file << "=== Available Plugins (" << available.size() << ") ===\n";
            for (const auto& metadata : available) {
                file << "- " << metadata.name << " v" << metadata.version.toString() << "\n";
            }
        }
        catch (const std::exception&) {
            // Ignore errors in dump
        }
    }

    // Component access
    PluginManager* getPluginManager() const {
        return manager_.get();
    }

    PluginLoader* getPluginLoader() const {
        return loader_.get();
    }

    PluginRegistry* getPluginRegistry() const {
        return registry_.get();
    }

    SandboxManager* getSandboxManager() const {
        return sandboxManager_.get();
    }

private:
    State* luaState_;
    LibManager* libManager_;
    PluginSystemState state_;
    PluginSystemConfig config_;
    
    UPtr<PluginManager> manager_;
    UPtr<PluginLoader> loader_;
    UPtr<PluginRegistry> registry_;
    UPtr<SandboxManager> sandboxManager_;
    
    HashMap<Str, Str> configuration_;
    mutable Str lastError_;
    
    void setupSearchPaths() {
        // Add default system paths if not specified
        if (config_.searchPaths.systemPaths.empty()) {
            config_.searchPaths.systemPaths.push_back("./plugins");
            config_.searchPaths.systemPaths.push_back("./lib/plugins");
        }
        
        // Add default user paths if not specified
        if (config_.searchPaths.userPaths.empty()) {
            // Try to get user home directory
            char* home = nullptr;
            size_t homeLen = 0;
            
            // Try HOME first (Unix-style)
            if (_dupenv_s(&home, &homeLen, "HOME") == 0 && home != nullptr) {
                config_.searchPaths.userPaths.push_back(std::string(home) + "/.lua/plugins");
                free(home);
            } else {
                // Try USERPROFILE (Windows-style)
                if (_dupenv_s(&home, &homeLen, "USERPROFILE") == 0 && home != nullptr) {
                    config_.searchPaths.userPaths.push_back(std::string(home) + "/.lua/plugins");
                    free(home);
                }
            }
        }
    }
    
    static Str stateToString(PluginSystemState state) {
        switch (state) {
            case PluginSystemState::Uninitialized: return "Uninitialized";
            case PluginSystemState::Initializing: return "Initializing";
            case PluginSystemState::Running: return "Running";
            case PluginSystemState::Suspended: return "Suspended";
            case PluginSystemState::Shutting_Down: return "Shutting_Down";
            case PluginSystemState::Shutdown: return "Shutdown";
            default: return "Unknown";
        }
    }
    
    static Str stateToString(PluginState state) {
        switch (state) {
            case PluginState::Unloaded: return "Unloaded";
            case PluginState::Loading: return "Loading";
            case PluginState::Loaded: return "Loaded";
            case PluginState::Initializing: return "Initializing";
            case PluginState::Active: return "Active";
            case PluginState::Stopping: return "Stopping";
            case PluginState::Stopped: return "Stopped";
            case PluginState::Error: return "Error";
            default: return "Unknown";
        }
    }
};

// PluginSystemFactory Implementation
UPtr<PluginSystem> PluginSystemFactory::create(State* luaState, LibManager* libManager) {
    return std::make_unique<PluginSystemImpl>(luaState, libManager);
}

PluginSystemConfig PluginSystemFactory::createDefaultConfig() {
    PluginSystemConfig config;
    
    // Basic settings
    config.enableSandbox = false;
    config.enableHotReload = false;
    config.strictMode = false;
    
    // Default resource limits
    config.defaultLimits.maxMemoryUsage = 64 * 1024 * 1024; // 64MB
    config.defaultLimits.maxOpenFiles = 100;
    config.defaultLimits.maxExecutionTime = 10000; // 10 seconds
    config.defaultLimits.maxNetworkConnections = 10;
    
    // Default permissions (restrictive)
    config.defaultPermissions.permissions[PermissionType::FileRead] = false;
    config.defaultPermissions.permissions[PermissionType::FileWrite] = false;
    config.defaultPermissions.permissions[PermissionType::NetworkAccess] = false;
    config.defaultPermissions.permissions[PermissionType::SystemCall] = false;
    config.defaultPermissions.permissions[PermissionType::ProcessCreate] = false;
    config.defaultPermissions.permissions[PermissionType::EnvironmentAccess] = false;
    
    return config;
}

PluginSystemConfig PluginSystemFactory::createDevelopmentConfig() {
    auto config = createDefaultConfig();
    
    // Development-friendly settings
    config.enableHotReload = true;
    config.strictMode = false;
    
    // More permissive for development
    config.defaultPermissions.permissions[PermissionType::FileRead] = true;
    config.defaultPermissions.permissions[PermissionType::FileWrite] = true;
    
    return config;
}

PluginSystemConfig PluginSystemFactory::createProductionConfig() {
    auto config = createDefaultConfig();
    
    // Production settings
    config.enableSandbox = true;
    config.strictMode = true;
    config.enableHotReload = false;
    
    // Tighter resource limits
    config.defaultLimits.maxMemoryUsage = 32 * 1024 * 1024; // 32MB
    config.defaultLimits.maxOpenFiles = 50;
    config.defaultLimits.maxExecutionTime = 5000; // 5 seconds
    
    return config;
}

// Global plugin system instance (optional)
static UPtr<PluginSystem> g_pluginSystem;

PluginSystem* GetGlobalPluginSystem() {
    return g_pluginSystem.get();
}

void SetGlobalPluginSystem(UPtr<PluginSystem> system) {
    g_pluginSystem = std::move(system);
}

void ShutdownGlobalPluginSystem() {
    if (g_pluginSystem) {
        g_pluginSystem->shutdown();
        g_pluginSystem.reset();
    }
}

// PluginSystem constructor implementation
PluginSystem::PluginSystem(State* state, LibManager* libManager) 
    : luaState_(state), libManager_(libManager), state_(PluginSystemState::Uninitialized) {
    manager_ = std::make_unique<PluginManager>(state, libManager);
    sandboxManager_ = std::make_unique<SandboxManager>();
}

// PluginSystem destructor implementation
PluginSystem::~PluginSystem() {
    // Base destructor - derived classes should handle cleanup
}

// PluginSystem shutdown implementation
void PluginSystem::shutdown() {
    // Base shutdown - derived classes should override this
    state_ = PluginSystemState::Shutdown;
}

} // namespace Lua