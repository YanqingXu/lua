#pragma once

/**
 * Lua插件系统 - 主头文件
 * 
 * 这个文件提供了完整的插件系统接口，包括：
 * - 插件接口定义
 * - 插件管理器
 * - 插件加载器
 * - 插件注册表
 * - 插件上下文
 * - 插件沙箱
 * 
 * 使用方式：
 * 1. 包含此头文件即可使用完整的插件系统
 * 2. 创建插件管理器并初始化
 * 3. 加载和管理插件
 * 
 * 示例：
 * ```cpp
 * #include "plugin.hpp"
 * 
 * // 创建插件管理器
 * auto manager = Lua::PluginManagerFactory::create(luaState);
 * manager->initialize();
 * 
 * // 加载插件
 * manager->loadPlugin("my_plugin");
 * 
 * // 使用插件
 * auto plugin = manager->getPlugin("my_plugin");
 * ```
 */

// 核心插件接口
#include "plugin_interface.hpp"

// 插件管理组件
#include "plugin_manager.hpp"
#include "plugin_loader.hpp"
#include "plugin_registry.hpp"
#include "plugin_context.hpp"
#include "plugin_sandbox.hpp"

// 标准库集成
#include "../lib_manager.hpp"
#include "../lib_module.hpp"
#include "../../vm/state.hpp"

namespace Lua {
    /**
     * 插件系统版本信息
     */
    namespace PluginSystemVersion {
        constexpr u32 MAJOR_VERSION = 1;
        constexpr u32 MINOR_VERSION = 0;
        constexpr u32 PATCH_VERSION = 0;
        constexpr const char* VERSION_STRING = "1.0.0";
        constexpr const char* BUILD_DATE = __DATE__;
        constexpr const char* BUILD_TIME = __TIME__;
    }
    
    /**
     * 插件系统初始化配置
     */
    struct PluginSystemConfig {
        bool enableSandbox = true;              // 启用沙箱
        bool enableHotReload = false;           // 启用热重载
        bool enableAuditLog = true;             // 启用审计日志
        bool enablePerformanceMonitoring = true; // 启用性能监控
        bool strictMode = false;                // 严格模式
        bool debugMode = false;                 // 调试模式
        
        PluginSearchPaths searchPaths;          // 搜索路径
        ResourceLimits defaultLimits;           // 默认资源限制
        PermissionConfig defaultPermissions;    // 默认权限配置
        
        Str configDirectory = "./config/plugins"; // 配置目录
        Str logDirectory = "./logs/plugins";      // 日志目录
        Str cacheDirectory = "./cache/plugins";   // 缓存目录
        
        PluginSystemConfig() = default;
    };
    
    /**
     * 插件系统状态
     */
    enum class PluginSystemState {
        Uninitialized,  // 未初始化
        Initializing,   // 初始化中
        Running,        // 运行中
        Suspended,      // 暂停
        Shutting_Down,  // 关闭中
        Shutdown        // 已关闭
    };
    
    /**
     * 插件系统 - 统一的插件管理接口
     * 
     * 这是插件系统的主要入口点，提供了简化的API来管理插件的完整生命周期。
     * 它封装了插件管理器、加载器、注册表等组件，提供统一的接口。
     */
    class PluginSystem {
    public:
        explicit PluginSystem(State* state, LibManager* libManager = nullptr);
        ~PluginSystem();
        
        // 禁用拷贝和移动
        PluginSystem(const PluginSystem&) = delete;
        PluginSystem& operator=(const PluginSystem&) = delete;
        PluginSystem(PluginSystem&&) = delete;
        PluginSystem& operator=(PluginSystem&&) = delete;
        
        // === 系统生命周期 ===
        
        /**
         * 初始化插件系统
         */
        bool initialize(const PluginSystemConfig& config = {});
        
        /**
         * 关闭插件系统
         */
        void shutdown();
        
        /**
         * 获取系统状态
         */
        PluginSystemState getState() const { return state_; }
        
        /**
         * 检查系统是否已初始化
         */
        bool isInitialized() const { return state_ != PluginSystemState::Uninitialized; }
        
        /**
         * 检查系统是否正在运行
         */
        bool isRunning() const { return state_ == PluginSystemState::Running; }
        
        // === 插件管理 ===
        
        /**
         * 扫描可用插件
         */
        Vec<PluginMetadata> scanPlugins();
        
        /**
         * 加载插件
         */
        bool loadPlugin(StrView name, const PluginLoadOptions& options = {});
        
        /**
         * 卸载插件
         */
        bool unloadPlugin(StrView name);
        
        /**
         * 重新加载插件
         */
        bool reloadPlugin(StrView name);
        
        /**
         * 启用插件
         */
        bool enablePlugin(StrView name);
        
        /**
         * 禁用插件
         */
        bool disablePlugin(StrView name);
        
        /**
         * 获取插件实例
         */
        IPlugin* getPlugin(StrView name) const;
        
        /**
         * 检查插件是否已加载
         */
        bool isPluginLoaded(StrView name) const;
        
        /**
         * 检查插件是否已启用
         */
        bool isPluginEnabled(StrView name) const;
        
        /**
         * 获取所有已加载的插件
         */
        Vec<Str> getLoadedPlugins() const;
        
        /**
         * 获取所有可用的插件
         */
        Vec<PluginMetadata> getAvailablePlugins() const;
        
        // === 批量操作 ===
        
        /**
         * 批量加载插件
         */
        Vec<Str> loadPlugins(const Vec<Str>& names, const PluginLoadOptions& options = {});
        
        /**
         * 自动加载所有可用插件
         */
        Vec<Str> autoLoadPlugins(const PluginLoadOptions& options = {});
        
        /**
         * 卸载所有插件
         */
        void unloadAllPlugins();
        
        /**
         * 重新加载所有插件
         */
        Vec<Str> reloadAllPlugins();
        
        // === 配置管理 ===
        
        /**
         * 获取系统配置
         */
        const PluginSystemConfig& getConfig() const { return config_; }
        
        /**
         * 更新系统配置
         */
        bool updateConfig(const PluginSystemConfig& config);
        
        /**
         * 保存配置到文件
         */
        bool saveConfig(StrView filePath = "") const;
        
        /**
         * 从文件加载配置
         */
        bool loadConfig(StrView filePath = "");
        
        // === 统计和监控 ===
        
        /**
         * 获取系统统计信息
         */
        HashMap<Str, Str> getSystemStatistics() const;
        
        /**
         * 获取插件统计信息
         */
        PluginStatistics getPluginStatistics() const;
        
        /**
         * 获取性能统计
         */
        HashMap<Str, HashMap<Str, double>> getPerformanceStatistics() const;
        
        /**
         * 获取资源使用统计
         */
        HashMap<Str, ResourceUsage> getResourceUsage() const;
        
        /**
         * 重置统计信息
         */
        void resetStatistics();
        
        // === 事件系统 ===
        
        /**
         * 注册系统事件监听器
         */
        void addEventListener(PluginEventType type, PluginEventListener listener);
        
        /**
         * 移除事件监听器
         */
        void removeEventListener(PluginEventType type);
        
        /**
         * 触发系统事件
         */
        void fireEvent(const PluginEvent& event);
        
        // === 错误处理 ===
        
        /**
         * 获取最后的错误信息
         */
        StrView getLastError() const;
        
        /**
         * 清除错误信息
         */
        void clearError();
        
        /**
         * 获取错误历史
         */
        Vec<Str> getErrorHistory() const;
        
        // === 调试和诊断 ===
        
        /**
         * 启用调试模式
         */
        void setDebugMode(bool enable);
        
        /**
         * 检查是否为调试模式
         */
        bool isDebugMode() const;
        
        /**
         * 获取系统诊断信息
         */
        HashMap<Str, Str> getDiagnostics() const;
        
        /**
         * 导出系统状态
         */
        Str exportSystemState() const;
        
        /**
         * 验证系统完整性
         */
        bool validateSystem() const;
        
        // === 组件访问 ===
        
        /**
         * 获取插件管理器
         */
        PluginManager* getPluginManager() const { return manager_.get(); }
        
        /**
         * 获取插件加载器
         */
        PluginLoader* getPluginLoader() const;
        
        /**
         * 获取插件注册表
         */
        PluginRegistry* getPluginRegistry() const;
        
        /**
         * 获取沙箱管理器
         */
        SandboxManager* getSandboxManager() const;
        
        /**
         * 获取Lua状态机
         */
        State* getLuaState() const { return luaState_; }
        
        /**
         * 获取库管理器
         */
        LibManager* getLibManager() const { return libManager_; }
        
    private:
        // 核心组件
        State* luaState_;                           // Lua状态机
        LibManager* libManager_;                    // 库管理器
        UPtr<PluginManager> manager_;               // 插件管理器
        UPtr<SandboxManager> sandboxManager_;       // 沙箱管理器
        
        // 系统状态
        PluginSystemState state_ = PluginSystemState::Uninitialized;
        PluginSystemConfig config_;                 // 系统配置
        
        // 错误处理
        Str lastError_;
        Vec<Str> errorHistory_;
        
        // 统计信息
        HashMap<Str, Str> systemStats_;
        
        // 线程安全
        mutable std::mutex systemMutex_;
        
        // 私有方法
        bool initializeComponents();
        bool createDirectories();
        void setError(StrView error);
        void updateSystemStats();
        void logSystemEvent(StrView event, StrView details = "");
    };
    
    /**
     * 插件系统工厂
     */
    class PluginSystemFactory {
    public:
        /**
         * 创建插件系统实例
         */
        static UPtr<PluginSystem> create(State* state, LibManager* libManager = nullptr);
        
        /**
         * 创建默认配置
         */
        static PluginSystemConfig createDefaultConfig();
        
        /**
         * 创建开发配置（启用调试和详细日志）
         */
        static PluginSystemConfig createDevelopmentConfig();
        
        /**
         * 创建生产配置（优化性能和安全性）
         */
        static PluginSystemConfig createProductionConfig();
    };
    
    /**
     * 全局插件系统实例（可选）
     * 
     * 提供全局访问点，方便在不同模块间共享插件系统实例。
     * 注意：使用全局实例时需要注意线程安全。
     */
    namespace GlobalPluginSystem {
        /**
         * 设置全局插件系统实例
         */
        void setInstance(PluginSystem* system);
        
        /**
         * 获取全局插件系统实例
         */
        PluginSystem* getInstance();
        
        /**
         * 检查是否有全局实例
         */
        bool hasInstance();
        
        /**
         * 清除全局实例
         */
        void clearInstance();
    }
}

// 便捷宏定义

/**
 * 快速创建插件系统
 */
#define LUA_CREATE_PLUGIN_SYSTEM(state, libManager) \
    ::Lua::PluginSystemFactory::create(state, libManager)

/**
 * 快速获取全局插件系统
 */
#define LUA_GLOBAL_PLUGIN_SYSTEM() \
    ::Lua::GlobalPluginSystem::getInstance()

/**
 * 插件系统版本检查
 */
#define LUA_PLUGIN_SYSTEM_VERSION_CHECK(major, minor, patch) \
    (::Lua::PluginSystemVersion::MAJOR_VERSION > (major) || \
     (::Lua::PluginSystemVersion::MAJOR_VERSION == (major) && ::Lua::PluginSystemVersion::MINOR_VERSION > (minor)) || \
     (::Lua::PluginSystemVersion::MAJOR_VERSION == (major) && ::Lua::PluginSystemVersion::MINOR_VERSION == (minor) && ::Lua::PluginSystemVersion::PATCH_VERSION >= (patch)))
