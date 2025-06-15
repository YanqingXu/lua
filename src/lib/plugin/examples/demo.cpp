#include "example_plugin.hpp"
#include "../plugin.hpp"
#include <iostream>
#include <memory>
#include <string>

// Mock implementations for demonstration
// In a real application, these would be provided by the Lua interpreter

namespace Lua {

// Mock State class
class MockState {
public:
    lua_State* getLuaState() { return nullptr; } // Would return actual lua_State*
};

// Mock LibManager class
class MockLibManager {
public:
    void registerModule(const std::string& name, std::unique_ptr<LibModule> module) {
        std::cout << "Registered module: " << name << std::endl;
    }
};

// Mock FunctionRegistry class
class MockFunctionRegistry : public FunctionRegistry {
public:
    void registerFunction(const std::string& name, lua_CFunction func) override {
        std::cout << "Registered function: " << name << std::endl;
        functions_[name] = func;
    }
    
    void unregisterFunction(const std::string& name) override {
        std::cout << "Unregistered function: " << name << std::endl;
        functions_.erase(name);
    }
    
    bool hasFunction(const std::string& name) const override {
        return functions_.find(name) != functions_.end();
    }
    
    lua_CFunction getFunction(const std::string& name) const override {
        auto it = functions_.find(name);
        return it != functions_.end() ? it->second : nullptr;
    }
    
private:
    std::unordered_map<std::string, lua_CFunction> functions_;
};

} // namespace Lua

int main() {
    std::cout << "=== Lua Plugin System Demo ===\n\n";
    
    try {
        // Create mock dependencies
        auto mockState = std::make_unique<Lua::MockState>();
        auto mockLibManager = std::make_unique<Lua::MockLibManager>();
        
        // Create plugin system
        std::cout << "Creating plugin system...\n";
        auto pluginSystem = Lua::PluginSystemFactory::create(
            reinterpret_cast<Lua::State*>(mockState.get()),
            reinterpret_cast<Lua::LibManager*>(mockLibManager.get())
        );
        
        if (!pluginSystem) {
            std::cerr << "Failed to create plugin system\n";
            return 1;
        }
        
        // Initialize with development configuration
        std::cout << "Initializing plugin system...\n";
        auto config = Lua::PluginSystemFactory::createDevelopmentConfig();
        
        if (!pluginSystem->initialize(config)) {
            std::cerr << "Failed to initialize plugin system: " 
                      << pluginSystem->getLastError() << "\n";
            return 1;
        }
        
        std::cout << "Plugin system initialized successfully!\n\n";
        
        // Register static plugins (since we don't have dynamic loading in this demo)
        std::cout << "Registering static plugins...\n";
        auto manager = pluginSystem->getPluginManager();
        
        // Register example plugins
        manager->registerFactory("ExamplePlugin", std::make_unique<Lua::ExamplePluginFactory>());
        manager->registerFactory("MathPlugin", std::make_unique<Lua::MathPluginFactory>());
        manager->registerFactory("StringPlugin", std::make_unique<Lua::StringPluginFactory>());
        
        std::cout << "Static plugins registered.\n\n";
        
        // Run demonstrations
        Lua::demonstratePluginUsage(pluginSystem.get());
        Lua::demonstratePluginCommunication(pluginSystem.get());
        Lua::demonstrateConfigurationManagement(pluginSystem.get());
        Lua::demonstrateLifecycleManagement(pluginSystem.get());
        Lua::demonstrateErrorHandling(pluginSystem.get());
        Lua::demonstratePerformanceMonitoring(pluginSystem.get());
        
        // Test Lua function calls (mock)
        std::cout << "\n=== Mock Lua Function Calls ===\n";
        
        // Create a mock function registry to test function registration
        Lua::MockFunctionRegistry registry;
        
        // Test ExamplePlugin functions
        auto examplePlugin = pluginSystem->getPlugin("ExamplePlugin");
        if (examplePlugin) {
            std::cout << "Testing ExamplePlugin function registration:\n";
            examplePlugin->registerFunctions(registry);
            
            // Test if functions are registered
            std::cout << "  example_hello registered: " << registry.hasFunction("example_hello") << "\n";
            std::cout << "  example_add registered: " << registry.hasFunction("example_add") << "\n";
            std::cout << "  example_info registered: " << registry.hasFunction("example_info") << "\n";
        }
        
        // Test MathPlugin functions
        auto mathPlugin = pluginSystem->getPlugin("MathPlugin");
        if (mathPlugin) {
            std::cout << "\nTesting MathPlugin function registration:\n";
            mathPlugin->registerFunctions(registry);
            
            std::cout << "  factorial registered: " << registry.hasFunction("factorial") << "\n";
            std::cout << "  fibonacci registered: " << registry.hasFunction("fibonacci") << "\n";
            std::cout << "  is_prime registered: " << registry.hasFunction("is_prime") << "\n";
            std::cout << "  gcd registered: " << registry.hasFunction("gcd") << "\n";
            std::cout << "  lcm registered: " << registry.hasFunction("lcm") << "\n";
            std::cout << "  power registered: " << registry.hasFunction("power") << "\n";
        }
        
        // Test StringPlugin functions
        auto stringPlugin = pluginSystem->getPlugin("StringPlugin");
        if (stringPlugin) {
            std::cout << "\nTesting StringPlugin function registration:\n";
            stringPlugin->registerFunctions(registry);
            
            std::cout << "  str_reverse registered: " << registry.hasFunction("str_reverse") << "\n";
            std::cout << "  str_capitalize registered: " << registry.hasFunction("str_capitalize") << "\n";
            std::cout << "  str_count registered: " << registry.hasFunction("str_count") << "\n";
            std::cout << "  str_split registered: " << registry.hasFunction("str_split") << "\n";
            std::cout << "  str_join registered: " << registry.hasFunction("str_join") << "\n";
            std::cout << "  str_trim registered: " << registry.hasFunction("str_trim") << "\n";
        }
        
        // Test plugin configuration
        std::cout << "\n=== Plugin Configuration Test ===\n";
        if (examplePlugin) {
            Lua::HashMap<Lua::Str, Lua::Str> config = {
                {"debug", "true"},
                {"log_level", "info"},
                {"feature_x", "enabled"}
            };
            
            if (examplePlugin->configure(config)) {
                std::cout << "ExamplePlugin configured successfully\n";
                
                auto currentConfig = examplePlugin->getConfiguration();
                std::cout << "Current configuration (" << currentConfig.size() << " items):\n";
                for (const auto& [key, value] : currentConfig) {
                    std::cout << "  " << key << " = " << value << "\n";
                }
            }
        }
        
        // Test plugin metadata
        std::cout << "\n=== Plugin Metadata ===\n";
        auto loadedPlugins = pluginSystem->getLoadedPlugins();
        for (const auto& pluginName : loadedPlugins) {
            auto plugin = pluginSystem->getPlugin(pluginName);
            if (plugin) {
                const auto& metadata = plugin->getMetadata();
                std::cout << "Plugin: " << metadata.name << "\n";
                std::cout << "  Version: " << metadata.version.toString() << "\n";
                std::cout << "  Author: " << metadata.author << "\n";
                std::cout << "  Description: " << metadata.description << "\n";
                std::cout << "  Category: " << metadata.category << "\n";
                std::cout << "  Tags: ";
                for (size_t i = 0; i < metadata.tags.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << metadata.tags[i];
                }
                std::cout << "\n";
                std::cout << "  Auto-load: " << (metadata.autoLoad ? "yes" : "no") << "\n\n";
            }
        }
        
        // Test batch operations
        std::cout << "=== Batch Operations Test ===\n";
        
        // Unload all plugins
        std::cout << "Unloading all plugins...\n";
        pluginSystem->unloadAllPlugins();
        
        auto remainingPlugins = pluginSystem->getLoadedPlugins();
        std::cout << "Remaining loaded plugins: " << remainingPlugins.size() << "\n";
        
        // Reload plugins
        std::cout << "\nReloading plugins...\n";
        Lua::Vec<Lua::Str> pluginsToLoad = {"ExamplePlugin", "MathPlugin", "StringPlugin"};
        auto loadedCount = pluginSystem->loadPlugins(pluginsToLoad);
        std::cout << "Loaded " << loadedCount.size() << " out of " << pluginsToLoad.size() << " plugins\n";
        
        // Final state dump
        std::cout << "\n=== Final State Dump ===\n";
        std::string dumpFile = "plugin_state_dump.txt";
        pluginSystem->dumpState(dumpFile);
        std::cout << "State dumped to: " << dumpFile << "\n";
        
        // Shutdown
        std::cout << "\nShutting down plugin system...\n";
        pluginSystem->shutdown();
        
        std::cout << "Demo completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Exception during demo: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception during demo\n";
        return 1;
    }
    
    return 0;
}

// Example of how to use the plugin system in a real Lua application
void exampleLuaIntegration() {
    std::cout << "\n=== Example Lua Integration ===\n";
    std::cout << "This is how you would integrate the plugin system with a real Lua interpreter:\n\n";
    
    std::cout << "```cpp\n";
    std::cout << "// In your Lua application:\n";
    std::cout << "#include \"plugin/plugin.hpp\"\n";
    std::cout << "\n";
    std::cout << "// Create and initialize plugin system\n";
    std::cout << "auto pluginSystem = Lua::PluginSystemFactory::create(luaState, libManager);\n";
    std::cout << "auto config = Lua::PluginSystemFactory::createProductionConfig();\n";
    std::cout << "pluginSystem->initialize(config);\n";
    std::cout << "\n";
    std::cout << "// Scan and load plugins\n";
    std::cout << "auto availablePlugins = pluginSystem->scanPlugins();\n";
    std::cout << "pluginSystem->autoLoadPlugins();\n";
    std::cout << "\n";
    std::cout << "// Use plugins in Lua scripts\n";
    std::cout << "luaL_dostring(L, \"\n";
    std::cout << "    print(factorial(5))     -- From MathPlugin\n";
    std::cout << "    print(str_reverse('hello')) -- From StringPlugin\n";
    std::cout << "    print(example_hello('World')) -- From ExamplePlugin\n";
    std::cout << "\");\n";
    std::cout << "\n";
    std::cout << "// Cleanup\n";
    std::cout << "pluginSystem->shutdown();\n";
    std::cout << "```\n";
}