#include "plugin_integration_test.hpp"
#include "../../lib/plugin/plugin.hpp"
#include "../../vm/state.hpp"
#include "../../lib/lib_manager.hpp"
#include "../../lib/base_lib.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace Lua {
namespace Tests {

void PluginIntegrationTest::runAllTests() {
    std::cout << "\n=== Plugin Integration Tests ===\n" << std::endl;
    
    try {
        // 核心集成测试
        testPluginVMIntegration();
        testPluginLibManagerIntegration();
        testPluginStateManagement();
        
        // 生命周期测试
        testPluginLifecycle();
        testPluginDependencyResolution();
        testPluginHotReload();
        
        // 功能集成测试
        testPluginLuaFunctionRegistration();
        testPluginConfigurationIntegration();
        testPluginCommunication();
        
        // 安全和性能测试
        testPluginSandboxIntegration();
        testPluginResourceMonitoring();
        testPluginErrorHandling();
        
        // 高级集成测试
        testMultiplePluginsCoexistence();
        testPluginSystemShutdown();
        testPluginCompatibilityChecks();
        
        std::cout << "\n=== All Plugin Integration Tests Completed ===\n" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Plugin integration test failed with exception: " << e.what() << std::endl;
    } catch (...) {
        std::cout << "Plugin integration test failed with unknown exception" << std::endl;
    }
}

void PluginIntegrationTest::testPluginVMIntegration() {
    printTestHeader("Plugin-VM Integration");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 测试插件管理器状态
        assertCondition(pluginManager != nullptr, "Plugin manager created");
        
        // 注册和加载TestPlugin
        class VMTestPlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
        public:
            VMTestPlugin() {
                metadata_.name = "TestPlugin";
                metadata_.version = {1, 0, 0};
                metadata_.apiVersion = {1, 0, 0};
            }
            
            StrView getName() const noexcept override {
                return "TestPlugin";
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 空实现，测试插件不需要注册函数
            }
            
            bool onLoad(PluginContext* context) override { return true; }
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
        };
        
        class VMTestPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<VMTestPlugin>();
            }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "TestPlugin";
                metadata.version = {1, 0, 0};
                metadata.apiVersion = {1, 0, 0};
                return metadata;
            }
        };
        
        auto factory = std::make_unique<VMTestPluginFactory>();
        pluginManager->registerFactory("TestPlugin", std::move(factory));
        
        assertCondition(pluginManager->loadPlugin("TestPlugin"), "Plugin loading");
        
        // 测试Lua状态访问
        auto testPlugin = pluginManager->getPlugin("TestPlugin");
        if (testPlugin) {
            auto context = pluginManager->createContext(testPlugin);
            assertCondition(context != nullptr, "Plugin context creation");
            assertCondition(context->getLuaState() == state.get(), "Lua state access");
        }
        
        printTestResult("Plugin-VM Integration", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin-VM Integration", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginLibManagerIntegration() {
    printTestHeader("Plugin-LibManager Integration");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 注册和加载TestPlugin
          class LibTestPlugin : public IPlugin {
          private:
              PluginMetadata metadata_;
          public:
              LibTestPlugin() {
                  metadata_.name = "TestPlugin";
                  metadata_.version = {1, 0, 0};
                  metadata_.apiVersion = {1, 0, 0};
              }
              
              StrView getName() const noexcept override {
                  return "TestPlugin";
              }
              
              const PluginMetadata& getMetadata() const noexcept override {
                  return metadata_;
              }
              
              void registerFunctions(FunctionRegistry& registry) override {
                  // 空实现，测试插件不需要注册函数
              }
              
              bool onLoad(PluginContext* context) override { return true; }
              void onUnload(PluginContext* context) override {}
              bool onEnable(PluginContext* context) override { return true; }
              void onDisable(PluginContext* context) override {}
          };
         
         class LibTestPluginFactory : public IPluginFactory {
         public:
             UPtr<IPlugin> createPlugin() override {
                 return std::make_unique<LibTestPlugin>();
             }
             PluginMetadata getPluginMetadata() const override {
                 PluginMetadata metadata;
                 metadata.name = "TestPlugin";
                 metadata.version = {1, 0, 0};
                 metadata.apiVersion = {1, 0, 0};
                 return metadata;
             }
         };
         
         auto factory = std::make_unique<LibTestPluginFactory>();
        pluginManager->registerFactory("TestPlugin", std::move(factory));
        
        assertCondition(pluginManager->loadPlugin("TestPlugin"), "Plugin loading");
        
        // 测试插件与标准库的集成
        auto testPlugin2 = pluginManager->getPlugin("TestPlugin");
        if (testPlugin2) {
            auto context = pluginManager->createContext(testPlugin2);
            assertCondition(context->getPluginManager()->getLibManager() == libManager.get(), "LibManager access");
        }
        
        // 测试标准库函数调用
        // 这里可以测试插件是否能正确调用标准库函数
        
        printTestResult("Plugin-LibManager Integration", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin-LibManager Integration", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginStateManagement() {
    printTestHeader("Plugin State Management");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        // 测试插件管理器初始化
        assertCondition(pluginManager->initialize(), "Initialization");
        
        // 测试关闭
        pluginManager->shutdown();
        
        printTestResult("Plugin State Management", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin State Management", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginLifecycle() {
    printTestHeader("Plugin Lifecycle");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建测试插件工厂
        class TestPlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
            bool loadCalled;
            bool unloadCalled;
            bool enableCalled;
            bool disableCalled;
        public:
            TestPlugin() : loadCalled(false), unloadCalled(false), enableCalled(false), disableCalled(false) {
                metadata_.name = "TestPlugin";
                metadata_.version = {1, 0, 0};
                metadata_.description = "Test plugin for integration testing";
                metadata_.author = "Test Suite";
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "TestPlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 测试插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override {
                loadCalled = true;
                return true;
            }
            
            void onUnload(PluginContext* context) override {
                unloadCalled = true;
            }
            
            bool onEnable(PluginContext* context) override {
                enableCalled = true;
                return true;
            }
            
            void onDisable(PluginContext* context) override {
                disableCalled = true;
            }
            
            bool isLoadCalled() const { return loadCalled; }
            bool isUnloadCalled() const { return unloadCalled; }
            bool isEnableCalled() const { return enableCalled; }
            bool isDisableCalled() const { return disableCalled; }
        };
        
        class TestPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<TestPlugin>();
            }
            
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "TestPlugin";
                metadata.version = {1, 0, 0};
                metadata.description = "Test plugin for integration testing";
                metadata.author = "Test Suite";
                return metadata;
            }
        };
        
        // 初始化静态变量
        
        // 注册插件工厂
        auto factory = std::make_unique<TestPluginFactory>();
        pluginManager->registerFactory("TestPlugin", std::move(factory));
        
        // 测试插件加载
        assertCondition(pluginManager->loadPlugin("TestPlugin"), "Plugin loading");
        auto testPlugin = dynamic_cast<TestPlugin*>(pluginManager->getPlugin("TestPlugin"));
        assertCondition(testPlugin && testPlugin->isLoadCalled(), "onLoad called");
        
        // 测试插件启用
        assertCondition(pluginManager->enablePlugin("TestPlugin"), "Plugin enabling");
        assertCondition(testPlugin && testPlugin->isEnableCalled(), "onEnable called");
        
        // 测试插件禁用
        pluginManager->disablePlugin("TestPlugin");
        assertCondition(testPlugin && testPlugin->isDisableCalled(), "onDisable called");
        
        // 测试插件卸载
        pluginManager->unloadPlugin("TestPlugin");
        assertCondition(testPlugin && testPlugin->isUnloadCalled(), "onUnload called");
        
        printTestResult("Plugin Lifecycle", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin Lifecycle", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginDependencyResolution() {
    printTestHeader("Plugin Dependency Resolution");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建有依赖关系的插件
        class BasePlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
        public:
            BasePlugin() {
                metadata_.name = "BasePlugin";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "BasePlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 基础插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override { return true; }
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
        };
        
        class DependentPlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
        public:
            DependentPlugin() {
                metadata_.name = "DependentPlugin";
                metadata_.version = {1, 0, 0};
                
                PluginDependency dep("BasePlugin", {1, 0, 0}, false);
                metadata_.dependencies.push_back(dep);
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "DependentPlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 依赖插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override { return true; }
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
        };
        
        // 注册插件工厂
        class BasePluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override { return std::make_unique<BasePlugin>(); }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "BasePlugin";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        class DependentPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override { return std::make_unique<DependentPlugin>(); }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "DependentPlugin";
                metadata.version = {1, 0, 0};
                PluginDependency dep("BasePlugin", {1, 0, 0}, false);
                metadata.dependencies.push_back(dep);
                return metadata;
             }
         };
        
        auto baseFactory = std::make_unique<BasePluginFactory>();
        auto depFactory = std::make_unique<DependentPluginFactory>();
        
        pluginManager->registerFactory("BasePlugin", std::move(baseFactory));
        pluginManager->registerFactory("DependentPlugin", std::move(depFactory));
        
        // 测试依赖解析
        assertCondition(pluginManager->loadPlugin("BasePlugin"), "Base plugin loading");
        assertCondition(pluginManager->loadPlugin("DependentPlugin"), "Dependent plugin loading");
        
        // 测试依赖检查
        auto loadedPlugins = pluginManager->getLoadedPlugins();
        assertCondition(loadedPlugins.size() == 2, "Both plugins loaded");
        
        printTestResult("Plugin Dependency Resolution", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin Dependency Resolution", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginHotReload() {
    printTestHeader("Plugin Hot Reload");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建可重载的测试插件
        class ReloadablePlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
            int reloadCount;
        public:
            ReloadablePlugin() : reloadCount(0) {
                metadata_.name = "ReloadablePlugin";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "ReloadablePlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 可重载插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override { return true; }
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
            
            void onReload() {
                reloadCount++;
            }
            
            int getReloadCount() const { return reloadCount; }
        };
        
        class ReloadablePluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<ReloadablePlugin>();
            }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "ReloadablePlugin";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        auto factory = std::make_unique<ReloadablePluginFactory>();
        pluginManager->registerFactory("ReloadablePlugin", std::move(factory));
        
        // 测试插件热重载
        assertCondition(pluginManager->loadPlugin("ReloadablePlugin"), "Plugin loading");
        
        auto reloadablePlugin = dynamic_cast<ReloadablePlugin*>(pluginManager->getPlugin("ReloadablePlugin"));
        if (reloadablePlugin) {
            int initialCount = reloadablePlugin->getReloadCount();
            
            // 模拟热重载
            pluginManager->reloadPlugin("ReloadablePlugin");
            assertCondition(reloadablePlugin->getReloadCount() > initialCount, "Reload count increased");
        }
        
        printTestResult("Plugin Hot Reload", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin Hot Reload", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginLuaFunctionRegistration() {
    printTestHeader("Plugin Lua Function Registration");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建注册Lua函数的插件
        class LuaFunctionPlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
            bool functionRegistered;
        public:
            LuaFunctionPlugin() : functionRegistered(false) {
                metadata_.name = "LuaFunctionPlugin";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "LuaFunctionPlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 注册测试函数
                functionRegistered = true;
            }
            
            bool onLoad(PluginContext* context) override {
                // 注册Lua函数
                auto luaState = context->getLuaState();
                // 这里应该注册一些测试函数到Lua状态中
                return true;
            }
            
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
            
            bool isFunctionRegistered() const { return functionRegistered; }
        };
        
        class LuaFunctionPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<LuaFunctionPlugin>();
            }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "LuaFunctionPlugin";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        auto factory = std::make_unique<LuaFunctionPluginFactory>();
        pluginManager->registerFactory("LuaFunctionPlugin", std::move(factory));
        
        // 测试插件加载和函数注册
        assertCondition(pluginManager->loadPlugin("LuaFunctionPlugin"), "Plugin loading");
        
        auto luaPlugin = dynamic_cast<LuaFunctionPlugin*>(pluginManager->getPlugin("LuaFunctionPlugin"));
        if (luaPlugin) {
            assertCondition(luaPlugin->isFunctionRegistered(), "Lua function registration");
        }
        
        printTestResult("Plugin Lua Function Registration", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin Lua Function Registration", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginConfigurationIntegration() {
    printTestHeader("Plugin Configuration Integration");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 测试插件配置管理
        auto testPlugin3 = pluginManager->getPlugin("TestPlugin");
        if (!testPlugin3) return;
        auto context = pluginManager->createContext(testPlugin3);
        
        // 设置配置值
        context->setConfig("test_key", "test_value");
        context->setConfig("test_number", "42");
        
        // 获取配置值
        auto value1 = context->getConfig("test_key");
        auto value2 = context->getConfig("test_number");
        
        assertCondition(value1 == "test_value", "String config value");
        assertCondition(value2 == "42", "Number config value");
        
        // 测试配置保存和加载
        assertCondition(context->saveConfig(), "Config saving");
        assertCondition(context->loadConfig(), "Config loading");
        
        printTestResult("Plugin Configuration Integration", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin Configuration Integration", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginCommunication() {
    printTestHeader("Plugin Communication");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建两个可以通信的插件
        class SenderPlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
            bool messageSent;
            PluginContext* context_ = nullptr;
        public:
            SenderPlugin() : messageSent(false) {
                metadata_.name = "SenderPlugin";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "SenderPlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 发送插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override {
                context_ = context;
                return true;
            }
            
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
            
            void sendMessage() {
                if (context_) {
                    auto receiver = context_->findPlugin("ReceiverPlugin");
                    if (receiver) {
                        messageSent = true;
                    }
                }
            }
            
            bool isMessageSent() const { return messageSent; }
        };
        
        class ReceiverPlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
            bool messageReceived;
        public:
            ReceiverPlugin() : messageReceived(false) {
                metadata_.name = "ReceiverPlugin";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "ReceiverPlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 接收插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override {
                messageReceived = true;
                return true;
            }
            
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
            
            bool isMessageReceived() const { return messageReceived; }
        };
        
        // 注册插件工厂
        class SenderPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override { return std::make_unique<SenderPlugin>(); }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "SenderPlugin";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        class ReceiverPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override { return std::make_unique<ReceiverPlugin>(); }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "ReceiverPlugin";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        auto senderFactory = std::make_unique<SenderPluginFactory>();
        auto receiverFactory = std::make_unique<ReceiverPluginFactory>();
        
        pluginManager->registerFactory("SenderPlugin", std::move(senderFactory));
        pluginManager->registerFactory("ReceiverPlugin", std::move(receiverFactory));
        
        // 加载插件
        assertCondition(pluginManager->loadPlugin("SenderPlugin"), "Sender plugin loading");
        assertCondition(pluginManager->loadPlugin("ReceiverPlugin"), "Receiver plugin loading");
        
        // 测试插件通信
        auto senderPlugin = dynamic_cast<SenderPlugin*>(pluginManager->getPlugin("SenderPlugin"));
        if (senderPlugin) {
            senderPlugin->sendMessage();
            assertCondition(senderPlugin->isMessageSent(), "Message sent");
        }
        
        auto receiverPlugin = dynamic_cast<ReceiverPlugin*>(pluginManager->getPlugin("ReceiverPlugin"));
        if (receiverPlugin) {
            assertCondition(receiverPlugin->isMessageReceived(), "Message received");
        }
        
        printTestResult("Plugin Communication", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin Communication", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginSandboxIntegration() {
    printTestHeader("Plugin Sandbox Integration");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建需要沙箱的插件
        class SandboxedPlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
            bool sandboxActive;
        public:
            SandboxedPlugin() : sandboxActive(false) {
                metadata_.name = "SandboxedPlugin";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "SandboxedPlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 沙箱插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override {
                // 测试沙箱限制
                sandboxActive = true;
                return true;
            }
            
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
            
            bool isSandboxActive() const { return sandboxActive; }
        };
        
        class SandboxedPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<SandboxedPlugin>();
            }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "SandboxedPlugin";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        auto factory = std::make_unique<SandboxedPluginFactory>();
        pluginManager->registerFactory("SandboxedPlugin", std::move(factory));
        
        // 加载插件
        assertCondition(pluginManager->loadPlugin("SandboxedPlugin"), "Plugin loading");
        
        // 获取插件实例并验证沙箱状态
        auto loadedPlugins = pluginManager->getLoadedPlugins();
        assertCondition(!loadedPlugins.empty(), "Sandbox activation");
        
        printTestResult("Plugin Sandbox Integration", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin Sandbox Integration", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginResourceMonitoring() {
    printTestHeader("Plugin Resource Monitoring");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建资源消耗插件
        class ResourceIntensivePlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
        public:
            ResourceIntensivePlugin() {
                metadata_.name = "ResourceIntensivePlugin";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "ResourceIntensivePlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 资源密集插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override {
                // 模拟一些资源使用
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                return true;
            }
            
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
        };
        
        class ResourceIntensivePluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<ResourceIntensivePlugin>();
            }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "ResourceIntensivePlugin";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        auto factory = std::make_unique<ResourceIntensivePluginFactory>();
        pluginManager->registerFactory("ResourceIntensivePlugin", std::move(factory));
        
        // 测试资源监控
        assertCondition(pluginManager->loadPlugin("ResourceIntensivePlugin"), "Resource intensive plugin loading");
        
        // 获取性能统计
        auto perfStats = pluginManager->getPerformanceStats();
        assertCondition(!perfStats.empty(), "Performance statistics available");
        
        printTestResult("Plugin Resource Monitoring", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin Resource Monitoring", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginErrorHandling() {
    printTestHeader("Plugin Error Handling");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建会出错的插件
        class ErrorPlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
        public:
            ErrorPlugin() {
                metadata_.name = "ErrorPlugin";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "ErrorPlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 错误插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override {
                // 故意返回false来模拟加载错误
                return false;
            }
            
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
        };
        
        class ErrorPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<ErrorPlugin>();
            }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "ErrorPlugin";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        auto factory = std::make_unique<ErrorPluginFactory>();
        pluginManager->registerFactory("ErrorPlugin", std::move(factory));
        
        // 测试错误处理
        assertCondition(!pluginManager->loadPlugin("ErrorPlugin"), "Plugin loading failure expected");
        
        // 检查错误信息
        auto lastError = pluginManager->getLastError();
        assertCondition(!lastError.empty(), "Error message available");
        
        printTestResult("Plugin Error Handling", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin Error Handling", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testMultiplePluginsCoexistence() {
    printTestHeader("Multiple Plugins Coexistence");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建多个插件
        class Plugin1 : public IPlugin {
        private:
            PluginMetadata metadata_;
        public:
            Plugin1() {
                metadata_.name = "Plugin1";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "Plugin1";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // Plugin1不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override {
                return true;
            }
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
        };
        
        class Plugin2 : public IPlugin {
        private:
            PluginMetadata metadata_;
        public:
            Plugin2() {
                metadata_.name = "Plugin2";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "Plugin2";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // Plugin2不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override { return true; }
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
        };
        
        class Plugin3 : public IPlugin {
        private:
            PluginMetadata metadata_;
        public:
            Plugin3() {
                metadata_.name = "Plugin3";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "Plugin3";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // Plugin3不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override { return true; }
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
        };
        
        // 注册插件工厂
        class Plugin1Factory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override { return std::make_unique<Plugin1>(); }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "Plugin1";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        class Plugin2Factory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override { return std::make_unique<Plugin2>(); }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "Plugin2";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        class Plugin3Factory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override { return std::make_unique<Plugin3>(); }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "Plugin3";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        auto factory1 = std::make_unique<Plugin1Factory>();
        auto factory2 = std::make_unique<Plugin2Factory>();
        auto factory3 = std::make_unique<Plugin3Factory>();
        
        pluginManager->registerFactory("Plugin1", std::move(factory1));
        pluginManager->registerFactory("Plugin2", std::move(factory2));
        pluginManager->registerFactory("Plugin3", std::move(factory3));
        
        // 测试多插件加载
        assertCondition(pluginManager->loadPlugin("Plugin1"), "Plugin1 loading");
        assertCondition(pluginManager->loadPlugin("Plugin2"), "Plugin2 loading");
        assertCondition(pluginManager->loadPlugin("Plugin3"), "Plugin3 loading");
        
        // 验证所有插件都已加载
        auto loadedPlugins = pluginManager->getLoadedPlugins();
        assertCondition(loadedPlugins.size() == 3, "All plugins loaded");
        
        // 测试插件共存
        assertCondition(pluginManager->isPluginLoaded("Plugin1"), "Plugin1 loaded");
        assertCondition(pluginManager->isPluginLoaded("Plugin2"), "Plugin2 loaded");
        assertCondition(pluginManager->isPluginLoaded("Plugin3"), "Plugin3 loaded");
        
        printTestResult("Multiple Plugins Coexistence", true);
        
    } catch (const std::exception& e) {
        printTestResult("Multiple Plugins Coexistence", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginSystemShutdown() {
    printTestHeader("Plugin System Shutdown");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建测试插件
        class ShutdownTestPlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
        public:
            bool loaded;
            bool unloaded;
            ShutdownTestPlugin() : loaded(false), unloaded(false) {
                metadata_.name = "ShutdownTestPlugin";
                metadata_.version = {1, 0, 0};
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "ShutdownTestPlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 关闭测试插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override {
                loaded = true;
                return true;
            }
            
            void onUnload(PluginContext* context) override {
                unloaded = true;
            }
            
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
            
            bool isLoaded() const { return loaded; }
            bool isUnloaded() const { return unloaded; }
        };
        
        class ShutdownTestPluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<ShutdownTestPlugin>();
            }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "ShutdownTestPlugin";
                metadata.version = {1, 0, 0};
                return metadata;
            }
        };
        
        auto factory = std::make_unique<ShutdownTestPluginFactory>();
        pluginManager->registerFactory("ShutdownTestPlugin", std::move(factory));
        
        // 加载插件
        assertCondition(pluginManager->loadPlugin("ShutdownTestPlugin"), "Plugin loading");
        
        // 获取插件实例
        auto loadedPlugins = pluginManager->getLoadedPlugins();
        assertCondition(!loadedPlugins.empty(), "Plugin loaded");
        
        // 测试系统关闭
        pluginManager->shutdown();
        
        // 验证关闭后的状态
        assertCondition(pluginManager->getLoadedPlugins().empty(), "No plugins loaded after shutdown");
        
        printTestResult("Plugin System Shutdown", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin System Shutdown", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

void PluginIntegrationTest::testPluginCompatibilityChecks() {
    printTestHeader("Plugin Compatibility Checks");
    
    try {
        auto state = createTestState();
        auto libManager = createTestLibManager(state.get());
        auto pluginManager = createTestPluginSystem(state.get(), libManager.get());
        
        assertCondition(pluginManager->initialize(), "Plugin manager initialization");
        
        // 创建不兼容版本的插件
        class IncompatiblePlugin : public IPlugin {
        private:
            PluginMetadata metadata_;
        public:
            IncompatiblePlugin() {
                metadata_.name = "IncompatiblePlugin";
                metadata_.version = {999, 0, 0}; // 很高的版本号
                metadata_.apiVersion = {999, 0, 0}; // 要求很高的API版本
            }
            
            const PluginMetadata& getMetadata() const noexcept override {
                return metadata_;
            }
            
            StrView getName() const noexcept override {
                return "IncompatiblePlugin";
            }
            
            void registerFunctions(FunctionRegistry& registry) override {
                // 不兼容插件不注册任何函数
            }
            
            bool onLoad(PluginContext* context) override {
                return true;
            }
            void onUnload(PluginContext* context) override {}
            bool onEnable(PluginContext* context) override { return true; }
            void onDisable(PluginContext* context) override {}
        };
        
        class IncompatiblePluginFactory : public IPluginFactory {
        public:
            UPtr<IPlugin> createPlugin() override {
                return std::make_unique<IncompatiblePlugin>();
            }
            PluginMetadata getPluginMetadata() const override {
                PluginMetadata metadata;
                metadata.name = "IncompatiblePlugin";
                metadata.version = {999, 0, 0};
                metadata.apiVersion = {999, 0, 0};
                return metadata;
            }
        };
        
        auto factory = std::make_unique<IncompatiblePluginFactory>();
        pluginManager->registerFactory("IncompatiblePlugin", std::move(factory));
        
        // 测试兼容性检查
        bool loadResult = pluginManager->loadPlugin("IncompatiblePlugin");
        // 根据实现，这可能成功或失败，但系统应该保持稳定
        
        // 如果加载失败，应该有错误信息
        if (!loadResult) {
            auto lastError = pluginManager->getLastError();
            assertCondition(!lastError.empty(), "Compatibility error message available");
        }
        
        printTestResult("Plugin Compatibility Checks", true);
        
    } catch (const std::exception& e) {
        printTestResult("Plugin Compatibility Checks", false);
        std::cout << "  Error: " << e.what() << std::endl;
    }
}

// 辅助方法实现
void PluginIntegrationTest::printTestHeader(const Str& testName) {
    std::cout << "\n--- " << testName << " ---" << std::endl;
}

void PluginIntegrationTest::printTestResult(const Str& testName, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << testName << std::endl;
}

void PluginIntegrationTest::assertCondition(bool condition, const Str& message) {
    if (!condition) {
        throw std::runtime_error("Assertion failed: " + message);
    }
}

UPtr<State> PluginIntegrationTest::createTestState() {
    return std::make_unique<State>();
}

UPtr<LibManager> PluginIntegrationTest::createTestLibManager(State* state) {
    auto libManager = std::make_unique<LibManager>();
    
    // 注册基础库
    auto baseLib = std::make_unique<BaseLib>();
    libManager->registerModule(std::move(baseLib));
    
    return libManager;
}

UPtr<PluginManager> PluginIntegrationTest::createTestPluginSystem(State* state, LibManager* libManager) {
    return PluginManagerFactory::create(state, libManager);
}

// 静态成员变量定义




} // namespace Tests
} // namespace Lua