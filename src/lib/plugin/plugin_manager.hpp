#pragma once

#include "plugin_interface.hpp"
#include "plugin_context.hpp"
#include "../lib_manager.hpp"
#include "../../common/types.hpp"
#include "../../vm/state.hpp"
#include <shared_mutex>
#include <memory>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace Lua {
    // Forward declarations
    class PluginLoader;
    class PluginSandbox;
    class PluginRegistry;
    
    /**
     * 插件加载选项
     */
    struct PluginLoadOptions {
        bool enableSandbox = true;          // 启用沙箱
        bool checkDependencies = true;      // 检查依赖
        bool autoLoadDependencies = true;   // 自动加载依赖
        bool enableHotReload = false;       // 启用热重载
        Vec<Str> permissions;               // 权限列表
        HashMap<Str, Str> config;           // 初始配置
        
        PluginLoadOptions() = default;
    };
    
    /**
     * 插件搜索路径配置
     */
    struct PluginSearchPaths {
        Vec<Str> systemPaths;     // 系统插件路径
        Vec<Str> userPaths;       // 用户插件路径
        Vec<Str> projectPaths;    // 项目插件路径
        
        PluginSearchPaths() {
            // 默认搜索路径
            systemPaths.push_back("./plugins");
            systemPaths.push_back("./lib/plugins");
            userPaths.push_back("~/.lua/plugins");
        }
    };
    
    /**
     * 插件管理器 - 负责插件的完整生命周期管理
     */
    class PluginManager {
    public:
        explicit PluginManager(State* state, LibManager* libManager = nullptr);
        ~PluginManager();
        
        // 禁用拷贝和移动
        PluginManager(const PluginManager&) = delete;
        PluginManager& operator=(const PluginManager&) = delete;
        PluginManager(PluginManager&&) = delete;
        PluginManager& operator=(PluginManager&&) = delete;
        
        // === 初始化和配置 ===
        
        /**
         * 初始化插件管理器
         */
        bool initialize();
        
        /**
         * 关闭插件管理器
         */
        void shutdown();
        
        /**
         * 设置搜索路径
         */
        void setSearchPaths(const PluginSearchPaths& paths);
        
        /**
         * 添加搜索路径
         */
        void addSearchPath(StrView path, bool isSystemPath = false);
        
        /**
         * 获取搜索路径
         */
        const PluginSearchPaths& getSearchPaths() const { return searchPaths_; }
        
        // === 插件发现和加载 ===
        
        /**
         * 扫描插件
         */
        Vec<PluginMetadata> scanPlugins();
        
        /**
         * 加载插件
         */
        bool loadPlugin(StrView name, const PluginLoadOptions& options = {});
        
        /**
         * 从文件加载插件
         */
        bool loadPluginFromFile(StrView filePath, const PluginLoadOptions& options = {});
        
        /**
         * 卸载插件
         */
        bool unloadPlugin(StrView name);
        
        /**
         * 重新加载插件
         */
        bool reloadPlugin(StrView name);
        
        /**
         * 批量加载插件
         */
        Vec<Str> loadPlugins(const Vec<Str>& names, const PluginLoadOptions& options = {});
        
        /**
         * 自动加载所有可用插件
         */
        Vec<Str> autoLoadPlugins(const PluginLoadOptions& options = {});
        
        // === 插件查询和状态 ===
        
        /**
         * 检查插件是否已加载
         */
        bool isPluginLoaded(StrView name) const;
        
        /**
         * 获取插件实例
         */
        IPlugin* getPlugin(StrView name) const;
        
        /**
         * 获取插件元数据
         */
        Opt<PluginMetadata> getPluginMetadata(StrView name) const;
        
        /**
         * 获取插件状态
         */
        PluginState getPluginState(StrView name) const;
        
        /**
         * 获取所有已加载的插件
         */
        Vec<Str> getLoadedPlugins() const;
        
        /**
         * 获取所有可用的插件
         */
        Vec<PluginMetadata> getAvailablePlugins() const;
        
        // === 依赖管理 ===
        
        /**
         * 解析插件依赖
         */
        Vec<Str> resolveDependencies(StrView pluginName) const;
        
        /**
         * 检查依赖是否满足
         */
        bool checkDependencies(StrView pluginName) const;
        
        /**
         * 获取依赖图
         */
        HashMap<Str, Vec<Str>> getDependencyGraph() const;
        
        /**
         * 获取加载顺序
         */
        Vec<Str> getLoadOrder(const Vec<Str>& pluginNames) const;
        
        // === 插件启用/禁用 ===
        
        /**
         * 启用插件
         */
        bool enablePlugin(StrView name);
        
        /**
         * 禁用插件
         */
        bool disablePlugin(StrView name);
        
        /**
         * 检查插件是否启用
         */
        bool isPluginEnabled(StrView name) const;
        
        // === 热重载支持 ===
        
        /**
         * 启用热重载
         */
        void enableHotReload(bool enable = true);
        
        /**
         * 检查是否启用热重载
         */
        bool isHotReloadEnabled() const { return hotReloadEnabled_; }
        
        /**
         * 开始监控文件变化
         */
        void startFileWatcher();
        
        /**
         * 停止监控文件变化
         */
        void stopFileWatcher();
        
        // === 事件系统 ===
        
        /**
         * 注册插件事件监听器
         */
        void addEventListener(PluginEventType type, PluginEventListener listener);
        
        /**
         * 移除事件监听器
         */
        void removeEventListener(PluginEventType type);
        
        /**
         * 触发插件事件
         */
        void fireEvent(const PluginEvent& event);
        
        // === 插件间通信 ===
        
        /**
         * 发送消息给插件
         */
        bool sendMessage(StrView targetPlugin, StrView sourcePlugin, StrView message, const HashMap<Str, Str>& data = {});
        
        /**
         * 广播消息给所有插件
         */
        void broadcastMessage(StrView sourcePlugin, StrView message, const HashMap<Str, Str>& data = {});
        
        // === 配置管理 ===
        
        /**
         * 获取插件配置
         */
        HashMap<Str, Str> getPluginConfig(StrView name) const;
        
        /**
         * 设置插件配置
         */
        void setPluginConfig(StrView name, const HashMap<Str, Str>& config);
        
        /**
         * 保存所有插件配置
         */
        bool saveAllConfigs();
        
        /**
         * 加载所有插件配置
         */
        bool loadAllConfigs();
        
        // === 安全和权限 ===
        
        /**
         * 检查插件权限
         */
        bool checkPermission(StrView pluginName, StrView permission) const;
        
        /**
         * 授予插件权限
         */
        void grantPermission(StrView pluginName, StrView permission);
        
        /**
         * 撤销插件权限
         */
        void revokePermission(StrView pluginName, StrView permission);
        
        /**
         * 获取插件权限列表
         */
        Vec<Str> getPluginPermissions(StrView pluginName) const;
        
        // === 性能监控 ===
        
        /**
         * 获取插件性能统计
         */
        HashMap<Str, HashMap<Str, double>> getPerformanceStats() const;
        
        /**
         * 重置性能统计
         */
        void resetPerformanceStats();
        
        // === 错误处理 ===
        
        /**
         * 获取最后的错误信息
         */
        StrView getLastError() const { return lastError_; }
        
        /**
         * 清除错误信息
         */
        void clearError() { lastError_.clear(); }
        
        /**
         * 获取插件错误历史
         */
        Vec<Str> getPluginErrors(StrView pluginName) const;
        
        // === 调试和诊断 ===
        
        /**
         * 启用调试模式
         */
        void setDebugMode(bool enable) { debugMode_ = enable; }
        
        /**
         * 检查是否为调试模式
         */
        bool isDebugMode() const { return debugMode_; }
        
        /**
         * 获取插件诊断信息
         */
        HashMap<Str, Str> getPluginDiagnostics(StrView pluginName) const;
        
        /**
         * 导出插件状态
         */
        Str exportPluginState() const;
        
        // === 内部接口（供PluginContext使用）===
        
        /**
         * 获取Lua状态机
         */
        State* getLuaState() const { return state_; }
        
        /**
         * 获取LibManager
         */
        LibManager* getLibManager() const { return libManager_; }
        
        /**
         * 创建插件上下文
         */
        UPtr<PluginContext> createContext(IPlugin* plugin);
        
        /**
         * 注册静态插件工厂
         */
        void registerFactory(StrView name, IPluginFactory* factory);
        
    private:
        // 核心组件
        State* state_;                                    // Lua状态机
        LibManager* libManager_;                          // 库管理器
        UPtr<PluginLoader> loader_;                       // 插件加载器
        UPtr<PluginSandbox> sandbox_;                     // 安全沙箱
        UPtr<PluginRegistry> registry_;                   // 插件注册表
        
        // 插件存储
        HashMap<Str, UPtr<IPlugin>> loadedPlugins_;       // 已加载的插件
        HashMap<Str, PluginMetadata> pluginMetadata_;     // 插件元数据
        HashMap<Str, PluginState> pluginStates_;          // 插件状态
        HashMap<Str, UPtr<PluginContext>> pluginContexts_; // 插件上下文
        
        // 配置和路径
        PluginSearchPaths searchPaths_;                   // 搜索路径
        HashMap<Str, HashMap<Str, Str>> pluginConfigs_;   // 插件配置
        
        // 依赖管理
        HashMap<Str, Vec<Str>> dependencyGraph_;          // 依赖图
        HashMap<Str, Vec<Str>> reverseDependencyGraph_;   // 反向依赖图
        
        // 权限管理
        HashMap<Str, Vec<Str>> pluginPermissions_;        // 插件权限
        
        // 事件系统
        HashMap<PluginEventType, Vec<PluginEventListener>> eventListeners_;
        
        // 热重载
        bool hotReloadEnabled_ = false;
        std::unique_ptr<std::thread> fileWatcherThread_;
        std::atomic<bool> fileWatcherRunning_{false};
        
        // 线程安全
        mutable std::shared_mutex pluginsMutex_;          // 插件访问锁
        mutable std::mutex eventMutex_;                   // 事件系统锁
        
        // 状态和错误
        bool initialized_ = false;
        bool debugMode_ = false;
        Str lastError_;
        HashMap<Str, Vec<Str>> pluginErrors_;             // 插件错误历史
        
        // 性能统计
        HashMap<Str, HashMap<Str, double>> performanceStats_;
        
        // 私有方法


        
        bool loadPluginInternal(StrView name, const PluginLoadOptions& options);
        bool unloadPluginInternal(StrView name);
        
        void buildDependencyGraph();
        Vec<Str> topologicalSort(const Vec<Str>& plugins) const;
        bool hasCyclicDependency(const Vec<Str>& plugins) const;
        
        void setError(StrView error);
        void addPluginError(StrView pluginName, StrView error);
        
        void fileWatcherLoop();
        void handleFileChange(StrView filePath);
        
        void logDebug(StrView message) const;
        void logInfo(StrView message) const;
        void logWarning(StrView message) const;
        void logError(StrView message) const;
    };
    
    /**
     * 插件管理器工厂
     */
    class PluginManagerFactory {
    public:
        static UPtr<PluginManager> create(State* state, LibManager* libManager = nullptr) {
            return std::make_unique<PluginManager>(state, libManager);
        }
    };
}