#pragma once

/**
 * 示例插件 - 展示如何开发Lua插件
 * 
 * 这个示例插件演示了：
 * 1. 如何实现IPlugin接口
 * 2. 如何注册Lua函数
 * 3. 如何处理插件生命周期
 * 4. 如何使用插件上下文
 * 5. 如何处理配置和错误
 */

#include "../plugin_interface.hpp"
#include "../plugin_context.hpp"
#include "../../lib_module.hpp"

namespace Lua {
    namespace Examples {
        
        /**
         * 示例插件类
         */
        class ExamplePlugin : public IPlugin {
        public:
            ExamplePlugin();
            virtual ~ExamplePlugin() = default;
            
            // === IPlugin接口实现 ===
            
            StrView getName() const override {
                return "ExamplePlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override;
            
            bool onLoad(PluginContext* context) override;
            bool onUnload(PluginContext* context) override;
            bool onEnable(PluginContext* context) override;
            bool onDisable(PluginContext* context) override;
            
            PluginState getState() const override { return state_; }
            void setState(PluginState state) override { state_ = state; }
            
            const PluginMetadata& getMetadata() const override { return metadata_; }
            
            bool configure(const HashMap<Str, Str>& config) override;
            HashMap<Str, Str> getConfiguration() const override;
            
            bool handleMessage(StrView message, const HashMap<Str, Str>& data) override;
            
        private:
            PluginState state_ = PluginState::Unloaded;
            PluginMetadata metadata_;
            PluginContext* context_ = nullptr;
            HashMap<Str, Str> config_;
            
            // 示例数据
            int counter_ = 0;
            Vec<Str> messageHistory_;
            
            // 私有方法
            void initializeMetadata();
            void setupDefaultConfig();
            
            // Lua函数实现
            static int luaHello(lua_State* L);
            static int luaGetCounter(lua_State* L);
            static int luaIncrement(lua_State* L);
            static int luaGetConfig(lua_State* L);
            static int luaSetConfig(lua_State* L);
            static int luaGetHistory(lua_State* L);
            static int luaClearHistory(lua_State* L);
            static int luaLogMessage(lua_State* L);
        };
        
        /**
         * 示例插件工厂
         */
        class ExamplePluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<ExamplePlugin>();
            }
            
            StrView getPluginName() const override {
                return "ExamplePlugin";
            }
            
            PluginVersion getPluginVersion() const override {
                return {1, 0, 0};
            }
        };
        
        /**
         * 数学运算插件示例
         */
        class MathPlugin : public IPlugin {
        public:
            MathPlugin();
            virtual ~MathPlugin() = default;
            
            StrView getName() const override { return "MathPlugin"; }
            void registerFunctions(FunctionRegistry& registry) override;
            
            bool onLoad(PluginContext* context) override;
            bool onUnload(PluginContext* context) override;
            bool onEnable(PluginContext* context) override;
            bool onDisable(PluginContext* context) override;
            
            PluginState getState() const override { return state_; }
            void setState(PluginState state) override { state_ = state; }
            
            const PluginMetadata& getMetadata() const override { return metadata_; }
            
            bool configure(const HashMap<Str, Str>& config) override { return true; }
            HashMap<Str, Str> getConfiguration() const override { return {}; }
            
            bool handleMessage(StrView message, const HashMap<Str, Str>& data) override { return true; }
            
        private:
            PluginState state_ = PluginState::Unloaded;
            PluginMetadata metadata_;
            PluginContext* context_ = nullptr;
            
            void initializeMetadata();
            
            // 数学函数实现
            static int luaFactorial(lua_State* L);
            static int luaFibonacci(lua_State* L);
            static int luaIsPrime(lua_State* L);
            static int luaGcd(lua_State* L);
            static int luaLcm(lua_State* L);
            static int luaPower(lua_State* L);
        };
        
        /**
         * 数学插件工厂
         */
        class MathPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<MathPlugin>();
            }
            
            StrView getPluginName() const override {
                return "MathPlugin";
            }
            
            PluginVersion getPluginVersion() const override {
                return {1, 0, 0};
            }
        };
        
        /**
         * 字符串处理插件示例
         */
        class StringPlugin : public IPlugin {
        public:
            StringPlugin();
            virtual ~StringPlugin() = default;
            
            StrView getName() const override { return "StringPlugin"; }
            void registerFunctions(FunctionRegistry& registry) override;
            
            bool onLoad(PluginContext* context) override;
            bool onUnload(PluginContext* context) override;
            bool onEnable(PluginContext* context) override;
            bool onDisable(PluginContext* context) override;
            
            PluginState getState() const override { return state_; }
            void setState(PluginState state) override { state_ = state; }
            
            const PluginMetadata& getMetadata() const override { return metadata_; }
            
            bool configure(const HashMap<Str, Str>& config) override;
            HashMap<Str, Str> getConfiguration() const override { return config_; }
            
            bool handleMessage(StrView message, const HashMap<Str, Str>& data) override { return true; }
            
        private:
            PluginState state_ = PluginState::Unloaded;
            PluginMetadata metadata_;
            PluginContext* context_ = nullptr;
            HashMap<Str, Str> config_;
            
            void initializeMetadata();
            
            // 字符串函数实现
            static int luaReverse(lua_State* L);
            static int luaCapitalize(lua_State* L);
            static int luaWordCount(lua_State* L);
            static int luaRemoveSpaces(lua_State* L);
            static int luaIsEmail(lua_State* L);
            static int luaIsUrl(lua_State* L);
            static int luaBase64Encode(lua_State* L);
            static int luaBase64Decode(lua_State* L);
        };
        
        /**
         * 字符串插件工厂
         */
        class StringPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<StringPlugin>();
            }
            
            StrView getPluginName() const override {
                return "StringPlugin";
            }
            
            PluginVersion getPluginVersion() const override {
                return {1, 0, 0};
            }
        };
        
        /**
         * 插件使用示例函数
         */
        namespace Usage {
            
            /**
             * 演示如何创建和使用插件
             */
            void demonstratePluginUsage();
            
            /**
             * 演示插件间通信
             */
            void demonstratePluginCommunication();
            
            /**
             * 演示插件配置
             */
            void demonstratePluginConfiguration();
            
            /**
             * 演示插件生命周期
             */
            void demonstratePluginLifecycle();
            
            /**
             * 演示错误处理
             */
            void demonstrateErrorHandling();
            
            /**
             * 演示性能监控
             */
            void demonstratePerformanceMonitoring();
        }
    }
}

// 静态插件注册（用于演示）
LUA_REGISTER_STATIC_PLUGIN(ExamplePlugin, new Lua::Examples::ExamplePluginFactory());
LUA_REGISTER_STATIC_PLUGIN(MathPlugin, new Lua::Examples::MathPluginFactory());
LUA_REGISTER_STATIC_PLUGIN(StringPlugin, new Lua::Examples::StringPluginFactory());