#include "lib_manager.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"

namespace Lua {
    
    // LibManager 类的实现
    
    LibManager::LibManager(UPtr<FunctionRegistry> registry)
        : registry_(registry ? std::move(registry) : std::make_unique<FunctionRegistry>()) {}
    
    void LibManager::registerModule(UPtr<LibModule> module) {
        if (!module) {
            throw LuaException("Module cannot be null");
        }
        
        auto name = Str(module->getName());
        modules_[name] = std::move(module);
    }
    
    void LibManager::registerModuleFactory(UPtr<ModuleFactory> factory) {
        if (!factory) {
            throw LuaException("Factory cannot be null");
        }
        
        auto name = Str(factory->getModuleName());
        factories_[name] = std::move(factory);
    }
    
    bool LibManager::loadModule(StrView name, State* state) {
        auto nameStr = Str(name);
        
        // 检查是否已加载
        if (loadedModules_.find(nameStr) != loadedModules_.end()) {
            return true;
        }
        
        // 尝试从已注册模块加载
        auto moduleIt = modules_.find(nameStr);
        if (moduleIt != modules_.end()) {
            moduleIt->second->registerFunctions(*registry_);
            moduleIt->second->initialize(state);
            loadedModules_.insert(nameStr);
            return true;
        }
        
        // 尝试从工厂创建并加载
        auto factoryIt = factories_.find(nameStr);
        if (factoryIt != factories_.end()) {
            auto module = factoryIt->second->createModule();
            module->registerFunctions(*registry_);
            module->initialize(state);
            
            // 保存模块实例
            modules_[nameStr] = std::move(module);
            loadedModules_.insert(nameStr);
            return true;
        }
        
        return false; // 模块未找到
    }
    
    void LibManager::loadAllModules(State* state) {
        // 加载直接注册的模块
        for (const auto& [name, module] : modules_) {
            loadModule(name, state);
        }
        
        // 加载工厂模块
        for (const auto& [name, factory] : factories_) {
            loadModule(name, state);
        }
    }
    
    bool LibManager::unloadModule(StrView name, State* state) {
        auto nameStr = Str(name);
        
        auto it = loadedModules_.find(nameStr);
        if (it == loadedModules_.end()) {
            return false; // 模块未加载
        }
        
        // 调用清理钩子
        auto moduleIt = modules_.find(nameStr);
        if (moduleIt != modules_.end()) {
            moduleIt->second->cleanup(state);
        }
        
        // 从已加载列表中移除
        loadedModules_.erase(it);
        
        // TODO: 从函数注册表中移除该模块的函数
        // 这需要FunctionRegistry支持按模块移除函数
        
        return true;
    }
    
    bool LibManager::isModuleLoaded(StrView name) const noexcept {
        return loadedModules_.find(Str(name)) != loadedModules_.end();
    }
    
    Vec<Str> LibManager::getLoadedModules() const {
        Vec<Str> result;
        result.reserve(loadedModules_.size());
        for (const auto& name : loadedModules_) {
            result.push_back(name);
        }
        return result;
    }
    
    Value LibManager::callFunction(StrView name, State* state, i32 nargs) const {
        return registry_->callFunction(name, state, nargs);
    }
    
    bool LibManager::hasFunction(StrView name) const noexcept {
        return registry_->hasFunction(name);
    }
    
    FunctionRegistry& LibManager::getRegistry() {
        return *registry_;
    }
    
    const FunctionRegistry& LibManager::getRegistry() const {
        return *registry_;
    }
    
    void LibManager::clear(State* state) {
        if (state) {
            for (const auto& [name, module] : modules_) {
                module->cleanup(state);
            }
        }
        modules_.clear();
        factories_.clear();
        loadedModules_.clear();
        registry_->clear();
    }
    
    LibManager::LibManagerStats LibManager::getStats() const {
        LibManagerStats stats;
        stats.totalModules = modules_.size() + factories_.size();
        stats.loadedModules = loadedModules_.size();
        stats.totalFunctions = registry_->size();
        
        // 收集模块名称
        for (const auto& [name, _] : modules_) {
            stats.moduleNames.push_back(name);
        }
        for (const auto& [name, _] : factories_) {
            stats.moduleNames.push_back(name);
        }
        
        return stats;
    }
    
    // 便利函数的实现
    
    UPtr<LibManager> createStandardLibManager() {
        auto manager = make_unique<LibManager>();
        
        // 这里可以注册标准库模块
        // manager->registerModule(make_unique<BaseLib>());
        // manager->registerModule(make_unique<StringLib>());
        // manager->registerModule(make_unique<TableLib>());
        
        return manager;
    }
    
    void initStandardLibraries(State* state, LibManager& manager) {
        manager.loadAllModules(state);
    }
}