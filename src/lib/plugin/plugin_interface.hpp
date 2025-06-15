#pragma once

#include "../lib_module.hpp"
#include "../../common/types.hpp"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

namespace Lua {
    // Forward declarations
    class PluginContext;
    class PluginManager;
    
    /**
     * 插件版本信息
     */
    struct PluginVersion {
        u32 major = 1;
        u32 minor = 0;
        u32 patch = 0;
        
        PluginVersion() = default;
        PluginVersion(u32 maj, u32 min, u32 pat) : major(maj), minor(min), patch(pat) {}
        
        // 版本比较
        bool operator==(const PluginVersion& other) const noexcept {
            return major == other.major && minor == other.minor && patch == other.patch;
        }
        
        bool operator!=(const PluginVersion& other) const noexcept;
        
        bool operator<(const PluginVersion& other) const noexcept {
            if (major != other.major) return major < other.major;
            if (minor != other.minor) return minor < other.minor;
            return patch < other.patch;
        }
        
        bool operator<=(const PluginVersion& other) const noexcept;
        bool operator>(const PluginVersion& other) const noexcept;
        bool operator>=(const PluginVersion& other) const noexcept;
        
        bool isCompatible(const PluginVersion& required) const noexcept {
            // 主版本号必须相同，次版本号必须大于等于要求
            return major == required.major && 
                   (minor > required.minor || (minor == required.minor && patch >= required.patch));
        }
        
        Str toString() const {
            return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
        }
        
        // 静态方法
        static PluginVersion fromString(const Str& versionStr);
        static PluginVersion fromNumeric(u32 numeric) noexcept;
        
        // 实例方法
        bool isNewerThan(const PluginVersion& other) const noexcept;
        bool isOlderThan(const PluginVersion& other) const noexcept;
        u32 toNumeric() const noexcept;
    };
    
    /**
     * 插件依赖信息
     */
    struct PluginDependency {
        Str name;                    // 依赖的插件名称
        PluginVersion minVersion;    // 最小版本要求
        bool optional = false;       // 是否为可选依赖
        
        PluginDependency(const Str& n, const PluginVersion& v, bool opt = false)
            : name(n), minVersion(v), optional(opt) {}
        
        // 方法声明
        bool isSatisfiedBy(const PluginVersion& version) const noexcept;
        bool operator==(const PluginDependency& other) const noexcept;
        Str toString() const;
    };
    
    /**
     * 插件元数据
     */
    struct PluginMetadata {
        Str name;                           // 插件名称
        Str displayName;                    // 显示名称
        Str description;                    // 插件描述
        Str author;                         // 作者
        Str license;                        // 许可证
        PluginVersion version;              // 插件版本
        PluginVersion apiVersion;           // 所需API版本
        Vec<PluginDependency> dependencies; // 依赖列表
        HashMap<Str, Str> properties;       // 自定义属性
        
        PluginMetadata() = default;
        PluginMetadata(const Str& n, const PluginVersion& v)
            : name(n), version(v), apiVersion(1, 0, 0) {}
        
        // 验证方法
        bool isValid() const noexcept;
        
        // 属性管理方法
        bool hasProperty(const Str& key) const noexcept;
        Str getProperty(const Str& key, const Str& defaultValue = "") const;
        void setProperty(const Str& key, const Str& value);
        void removeProperty(const Str& key);
        Vec<Str> getPropertyKeys() const;
        
        // 依赖管理方法
        bool hasDependency(const Str& pluginName) const noexcept;
        const PluginDependency* getDependency(const Str& pluginName) const noexcept;
        void addDependency(const PluginDependency& dependency);
        void removeDependency(const Str& pluginName);
        Vec<Str> getRequiredDependencies() const;
        Vec<Str> getOptionalDependencies() const;
        
        // 字符串表示
        Str toString() const;
    };
    
    /**
     * 插件状态枚举
     */
    enum class PluginState {
        Unloaded,    // 未加载
        Loading,     // 加载中
        Loaded,      // 已加载
        Initializing,// 初始化中
        Active,      // 活跃状态
        Stopping,    // 停止中
        Stopped,     // 已停止
        Error        // 错误状态
    };
    
    // === 插件状态相关全局函数 ===
    
    /**
     * 将插件状态转换为字符串
     */
    Str pluginStateToString(PluginState state);
    
    /**
     * 将字符串转换为插件状态
     */
    PluginState stringToPluginState(const Str& stateStr);
    
    /**
     * 检查状态转换是否有效
     */
    bool isValidStateTransition(PluginState from, PluginState to);
    
    // === 全局插件API版本管理函数 ===
    
    /**
     * 获取当前插件API版本（数值形式）
     */
    u32 getCurrentPluginApiVersion() noexcept;
    
    /**
     * 获取当前插件API版本（结构体形式）
     */
    PluginVersion getCurrentPluginApiVersionStruct() noexcept;
    
    /**
     * 检查插件API兼容性
     */
    bool isPluginApiCompatible(u32 pluginApiVersion) noexcept;
    bool isPluginApiCompatible(const PluginVersion& pluginApiVersion) noexcept;
    
    /**
     * 插件接口 - 继承自LibModule以保持兼容性
     */
    class IPlugin : public LibModule {
    public:
        virtual ~IPlugin() = default;
        
        // === 插件元数据接口 ===
        
        /**
         * 获取插件元数据
         */
        virtual const PluginMetadata& getMetadata() const noexcept = 0;
        
        /**
         * 获取插件版本
         */
        virtual PluginVersion getVersion() const noexcept {
            return getMetadata().version;
        }
        
        /**
         * 获取插件描述
         */
        virtual StrView getDescription() const noexcept {
            return getMetadata().description;
        }
        
        /**
         * 获取插件作者
         */
        virtual StrView getAuthor() const noexcept {
            return getMetadata().author;
        }
        
        /**
         * 获取插件名称
         */
        virtual StrView getName() const noexcept override;
        
        /**
         * 获取插件显示名称
         */
        virtual StrView getDisplayName() const noexcept;
        
        /**
         * 获取插件许可证
         */
        virtual StrView getLicense() const noexcept;
        
        /**
         * 获取插件依赖列表
         */
        virtual const Vec<PluginDependency>& getDependencies() const noexcept;
        
        /**
         * 获取插件API版本
         */
        virtual PluginVersion getApiVersion() const noexcept;
        
        /**
         * 检查是否有指定属性
         */
        virtual bool hasProperty(const Str& key) const noexcept;
        
        /**
         * 获取属性值
         */
        virtual Str getProperty(const Str& key, const Str& defaultValue = "") const;
        
        /**
         * 检查与指定API版本的兼容性
         */
        virtual bool isCompatibleWith(const PluginVersion& apiVersion) const noexcept;
        
        /**
         * 检查是否依赖指定插件
         */
        virtual bool dependsOn(const Str& pluginName) const noexcept;
        
        /**
         * 检查是否可以与其他插件共存
         */
        virtual bool canCoexistWith(const IPlugin* other) const noexcept;
        
        /**
         * 状态变化回调
         */
        virtual void onStateChanged(PluginState oldState, PluginState newState);
        
        /**
         * 依赖加载回调
         */
        virtual void onDependencyLoaded(const Str& dependencyName, IPlugin* dependency);
        
        /**
         * 依赖卸载回调
         */
        virtual void onDependencyUnloaded(const Str& dependencyName);
        
        /**
         * 获取状态信息
         */
        virtual Str getStatusInfo() const;
        
        // === LibModule接口重写 ===
        
        /**
         * 初始化插件
         */
        virtual void initialize(State* state) override;
        
        /**
         * 清理插件
         */
        virtual void cleanup(State* state) override;
        
        // === 插件生命周期接口 ===
        
        /**
         * 插件加载时调用
         * @param context 插件运行时上下文
         * @return 是否加载成功
         */
        virtual bool onLoad(PluginContext* context) { return true; }
        
        /**
         * 插件卸载时调用
         * @param context 插件运行时上下文
         */
        virtual void onUnload(PluginContext* context) {}
        
        /**
         * 插件启用时调用
         * @param context 插件运行时上下文
         * @return 是否启用成功
         */
        virtual bool onEnable(PluginContext* context) { return true; }
        
        /**
         * 插件禁用时调用
         * @param context 插件运行时上下文
         */
        virtual void onDisable(PluginContext* context) {}
        
        /**
         * 配置更新时调用
         * @param config 新的配置参数
         */
        virtual void onConfigUpdate(const HashMap<Str, Str>& config) {}
        
        // === 插件状态管理 ===
        
        /**
         * 获取当前插件状态
         */
        virtual PluginState getState() const noexcept { return state_; }
        
        /**
         * 检查插件是否处于活跃状态
         */
        virtual bool isActive() const noexcept { return state_ == PluginState::Active; }
        
        /**
         * 检查插件是否可以卸载
         */
        virtual bool canUnload() const noexcept { return true; }
        
        // === 插件配置接口 ===
        
        /**
         * 获取配置参数
         */
        virtual HashMap<Str, Str> getDefaultConfig() const { return {}; }
        
        /**
         * 验证配置参数
         */
        virtual bool validateConfig(const HashMap<Str, Str>& config) const { return true; }
        
    protected:
        /**
         * 设置插件状态（仅供插件管理器使用）
         */
        virtual void setState(PluginState state) noexcept { state_ = state; }
        
        friend class PluginManager;
        
    private:
        PluginState state_ = PluginState::Unloaded;
    };
    
    /**
     * 插件工厂接口 - 用于动态创建插件实例
     */
    class IPluginFactory {
    public:
        virtual ~IPluginFactory() = default;
        
        /**
         * 创建插件实例
         */
        virtual UPtr<IPlugin> createPlugin() = 0;
        
        /**
         * 获取插件元数据
         */
        virtual PluginMetadata getPluginMetadata() const = 0;
        
        /**
         * 检查API兼容性
         */
        virtual bool isApiCompatible(const PluginVersion& apiVersion) const {
            return getPluginMetadata().apiVersion.isCompatible(apiVersion);
        }
        
        /**
         * 验证插件实例
         */
        virtual bool validatePlugin(const UPtr<IPlugin>& plugin) const;
        
        /**
         * 创建并验证插件实例
         */
        virtual UPtr<IPlugin> createValidatedPlugin();
        
        /**
         * 获取插件名称
         */
        virtual Str getPluginName() const;
        
        /**
         * 获取插件版本
         */
        virtual PluginVersion getPluginVersion() const;
        
        /**
         * 检查是否支持热重载
         */
        virtual bool supportsHotReload() const;
    };
    
    /**
     * 模板化插件工厂 - 简化插件注册
     */
    template<typename PluginType>
    class TypedPluginFactory : public IPluginFactory {
    public:
        UPtr<IPlugin> createPlugin() override {
            return std::make_unique<PluginType>();
        }
        
        PluginMetadata getPluginMetadata() const override {
            // 创建临时实例获取元数据
            auto plugin = std::make_unique<PluginType>();
            return plugin->getMetadata();
        }
    };
    
    // === 插件导出宏 ===
    
    /**
     * 插件导出函数类型定义
     */
    using CreatePluginFactoryFn = IPluginFactory*(*)();
    using DestroyPluginFactoryFn = void(*)(IPluginFactory*);
    using GetPluginApiFn = u32(*)();
    
    /**
     * 插件导出宏 - 用于动态库中导出插件
     */
    #define EXPORT_PLUGIN(PluginClass) \
        extern "C" { \
            __declspec(dllexport) Lua::IPluginFactory* createPluginFactory() { \
                return new Lua::TypedPluginFactory<PluginClass>(); \
            } \
            __declspec(dllexport) void destroyPluginFactory(Lua::IPluginFactory* factory) { \
                delete factory; \
            } \
            __declspec(dllexport) u32 getPluginApiVersion() { \
                return (1 << 16) | (0 << 8) | 0; /* 1.0.0 */ \
            } \
        }
    
    /**
     * 静态插件注册宏 - 用于静态链接的插件
     */
    #define REGISTER_STATIC_PLUGIN(PluginClass) \
        namespace { \
            struct PluginClass##Registrar { \
                PluginClass##Registrar() { \
                    /* 注册到静态插件列表 */ \
                } \
            }; \
            static PluginClass##Registrar g_##PluginClass##_registrar; \
        }
}