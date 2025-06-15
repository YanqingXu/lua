#pragma once

#include "plugin_interface.hpp"
#include "../../common/types.hpp"
#include "../../vm/state.hpp"
#include <memory>
#include <functional>

namespace Lua {
    // Forward declarations
    class PluginManager;
    class IPlugin;
    
    /**
     * 插件日志级别
     */
    enum class PluginLogLevel {
        Debug,
        Info,
        Warning,
        Error
    };
    
    /**
     * 插件事件类型
     */
    enum class PluginEventType {
        PluginLoaded,
        PluginUnloaded,
        PluginEnabled,
        PluginDisabled,
        ConfigChanged,
        StateChanged
    };
    
    /**
     * 插件事件数据
     */
    struct PluginEvent {
        PluginEventType type;
        Str pluginName;
        HashMap<Str, Str> data;
        
        PluginEvent(PluginEventType t, const Str& name) : type(t), pluginName(name) {}
    };
    
    /**
     * 插件事件监听器
     */
    using PluginEventListener = std::function<void(const PluginEvent&)>;
    
    /**
     * 插件上下文 - 为插件提供运行时环境和服务
     */
    class PluginContext {
    public:
        PluginContext(PluginManager* manager, IPlugin* plugin, State* state);
        ~PluginContext() = default;
        
        // 禁用拷贝和移动
        PluginContext(const PluginContext&) = delete;
        PluginContext& operator=(const PluginContext&) = delete;
        PluginContext(PluginContext&&) = delete;
        PluginContext& operator=(PluginContext&&) = delete;
        
        // === 基础服务接口 ===
        
        /**
         * 获取Lua状态机
         */
        State* getLuaState() const noexcept { return state_; }
        
        /**
         * 获取插件管理器
         */
        PluginManager* getPluginManager() const noexcept { return manager_; }
        
        /**
         * 获取当前插件实例
         */
        IPlugin* getPlugin() const noexcept { return plugin_; }
        
        /**
         * 获取插件名称
         */
        StrView getPluginName() const noexcept;
        
        // === 日志服务 ===
        
        /**
         * 记录日志
         */
        void log(PluginLogLevel level, StrView message);
        
        /**
         * 便捷日志方法
         */
        void logDebug(StrView message) { log(PluginLogLevel::Debug, message); }
        void logInfo(StrView message) { log(PluginLogLevel::Info, message); }
        void logWarning(StrView message) { log(PluginLogLevel::Warning, message); }
        void logError(StrView message) { log(PluginLogLevel::Error, message); }
        
        /**
         * 格式化日志
         */
        template<typename... Args>
        void logf(PluginLogLevel level, StrView format, Args&&... args) {
            // 简单的格式化实现，实际项目中可以使用fmt库
            log(level, format); // 暂时简化实现
        }
        
        // === 配置服务 ===
        
        /**
         * 获取配置值
         */
        Str getConfig(StrView key, StrView defaultValue = "") const;
        
        /**
         * 设置配置值
         */
        void setConfig(StrView key, StrView value);
        
        /**
         * 获取所有配置
         */
        HashMap<Str, Str> getAllConfig() const;
        
        /**
         * 保存配置到文件
         */
        bool saveConfig();
        
        /**
         * 从文件加载配置
         */
        bool loadConfig();
        
        // === 插件间通信 ===
        
        /**
         * 查找其他插件
         */
        IPlugin* findPlugin(StrView name) const;
        
        /**
         * 检查插件是否存在
         */
        bool hasPlugin(StrView name) const;
        
        /**
         * 获取所有已加载的插件名称
         */
        Vec<Str> getLoadedPlugins() const;
        
        /**
         * 发送消息给其他插件
         */
        bool sendMessage(StrView targetPlugin, StrView message, const HashMap<Str, Str>& data = {});
        
        // === 事件系统 ===
        
        /**
         * 注册事件监听器
         */
        void addEventListener(PluginEventType type, PluginEventListener listener);
        
        /**
         * 移除事件监听器
         */
        void removeEventListener(PluginEventType type);
        
        /**
         * 触发事件
         */
        void fireEvent(const PluginEvent& event);
        
        /**
         * 触发自定义事件
         */
        void fireCustomEvent(StrView eventName, const HashMap<Str, Str>& data = {});
        
        // === 资源管理 ===
        
        /**
         * 获取插件数据目录
         */
        Str getDataDirectory() const;
        
        /**
         * 获取插件配置目录
         */
        Str getConfigDirectory() const;
        
        /**
         * 获取插件临时目录
         */
        Str getTempDirectory() const;
        
        /**
         * 创建目录
         */
        bool createDirectory(StrView path) const;
        
        /**
         * 检查文件是否存在
         */
        bool fileExists(StrView path) const;
        
        /**
         * 读取文件内容
         */
        Str readFile(StrView path) const;
        
        /**
         * 写入文件内容
         */
        bool writeFile(StrView path, StrView content) const;
        
        // === 函数注册服务 ===
        
        /**
         * 注册Lua函数
         */
        template<typename F>
        void registerFunction(StrView name, F&& func) {
            // 添加插件名称前缀避免冲突
            Str fullName = Str(getPluginName()) + "." + Str(name);
            registry_->registerFunction(fullName, std::forward<F>(func));
        }
        
        /**
         * 注册全局函数（不带插件前缀）
         */
        template<typename F>
        void registerGlobalFunction(StrView name, F&& func) {
            registry_->registerFunction(name, std::forward<F>(func));
        }
        
        /**
         * 取消注册函数
         */
        void unregisterFunction(StrView name);
        
        // === 安全和权限 ===
        
        /**
         * 检查权限
         */
        bool hasPermission(StrView permission) const;
        
        /**
         * 请求权限
         */
        bool requestPermission(StrView permission);
        
        /**
         * 获取所有权限
         */
        Vec<Str> getPermissions() const;
        
        // === 性能监控 ===
        
        /**
         * 开始性能计时
         */
        void startTimer(StrView name);
        
        /**
         * 结束性能计时
         */
        void endTimer(StrView name);
        
        /**
         * 获取性能统计
         */
        HashMap<Str, double> getPerformanceStats() const;
        
    private:
        PluginManager* manager_;           // 插件管理器
        IPlugin* plugin_;                  // 当前插件实例
        State* state_;                     // Lua状态机
        FunctionRegistry* registry_;       // 函数注册表
        
        // 配置数据
        HashMap<Str, Str> config_;
        
        // 事件监听器
        HashMap<PluginEventType, Vec<PluginEventListener>> eventListeners_;
        
        // 权限列表
        Vec<Str> permissions_;
        
        // 性能计时器
        HashMap<Str, std::chrono::high_resolution_clock::time_point> timers_;
        HashMap<Str, double> performanceStats_;
        
        // 私有辅助方法
        void initializeDirectories();
        Str getPluginDirectory(StrView subdir) const;
        void logWithPrefix(PluginLogLevel level, StrView message);
    };
    
    /**
     * 插件上下文工厂
     */
    class PluginContextFactory {
    public:
        static UPtr<PluginContext> create(PluginManager* manager, IPlugin* plugin, State* state) {
            return std::make_unique<PluginContext>(manager, plugin, state);
        }
    };
}