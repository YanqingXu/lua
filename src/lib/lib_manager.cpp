#include "lib_manager.hpp"
#include "base_lib.hpp"
#include "../vm/state.hpp"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace Lua {

    // LibManager implementation
    LibManager::LibManager(std::shared_ptr<LibraryContext> context)
        : context_(context ? context : std::make_shared<LibraryContext>())
        , globalRegistry_(std::make_unique<FunctionRegistry>()) {
    }

    void LibManager::registerModule(std::unique_ptr<LibModule> module, LoadStrategy strategy) {
        if (!module) {
            throw std::invalid_argument("Cannot register null module");
        }

        Str name(module->getName());
        
        if (modules_.find(name) != modules_.end()) {
            std::ostringstream oss;
            oss << "Module '" << name << "' is already registered";
            throw std::runtime_error(oss.str());
        }

        // Create module info
        ModuleInfo info(name, module->getVersion());
        info.strategy = strategy;
        auto deps = module->getDependencies();
        for (auto dep : deps) {
            info.dependencies.emplace_back(dep);
        }

        // Store module and info
        modules_[name] = std::move(module);
        moduleInfo_[name] = std::move(info);
        moduleRegistries_[name] = std::make_unique<FunctionRegistry>();
    }

    void LibManager::registerModuleFactory(StrView name, 
                                         std::function<std::unique_ptr<LibModule>()> factory,
                                         LoadStrategy strategy) {
        Str moduleName(name);
        
        if (factories_.find(moduleName) != factories_.end()) {
            std::ostringstream oss;
            oss << "Module factory '" << moduleName << "' is already registered";
            throw std::runtime_error(oss.str());
        }

        factories_[moduleName] = std::move(factory);
        
        // Create placeholder module info
        ModuleInfo info(moduleName);
        info.strategy = strategy;
        info.status = ModuleStatus::Registered;
        moduleInfo_[moduleName] = std::move(info);
    }

    bool LibManager::loadModule(StrView name, State* state) {
        if (!state) {
            return false;
        }

        Str moduleName(name);
        return loadModuleInternal(moduleName, state);
    }

    void LibManager::loadAllModules(State* state) {
        if (!state) {
            return;
        }

        // First pass: load modules without dependencies
        for (auto& [name, info] : moduleInfo_) {
            if (info.status == ModuleStatus::Registered && info.dependencies.empty()) {
                loadModuleInternal(name, state);
            }
        }

        // Second pass: load modules with dependencies
        bool progressMade = true;
        while (progressMade) {
            progressMade = false;
            
            for (auto& [name, info] : moduleInfo_) {
                if (info.status == ModuleStatus::Registered) {
                    if (resolveDependencies(name, state)) {
                        loadModuleInternal(name, state);
                        progressMade = true;
                    }
                }
            }
        }

        // Check for unresolved dependencies
        for (const auto& [name, info] : moduleInfo_) {
            if (info.status == ModuleStatus::Registered) {
                std::ostringstream oss;
                oss << "Module '" << name << "' has unresolved dependencies";
                moduleInfo_[name].status = ModuleStatus::Failed;
                moduleInfo_[name].errorMessage = oss.str();
            }
        }

        updateGlobalRegistry();
    }

    bool LibManager::unloadModule(StrView name, State* state) {
        Str moduleName(name);
        
        auto moduleIt = modules_.find(moduleName);
        if (moduleIt == modules_.end()) {
            return false;
        }

        auto& info = moduleInfo_[moduleName];
        if (info.status != ModuleStatus::Loaded) {
            return false;
        }

        try {
            // Call cleanup
            moduleIt->second->cleanup(state, *context_);
            
            // Clear registry
            if (auto regIt = moduleRegistries_.find(moduleName); regIt != moduleRegistries_.end()) {
                regIt->second->clear();
            }

            info.status = ModuleStatus::Unloaded;
            info.functionCount = 0;

            updateGlobalRegistry();
            return true;

        } catch (const std::exception& e) {
            std::ostringstream oss;
            oss << "Failed to unload module '" << moduleName << "': " << e.what();
            info.status = ModuleStatus::Failed;
            info.errorMessage = oss.str();
            return false;
        }
    }

    bool LibManager::isModuleLoaded(StrView name) const noexcept {
        auto it = moduleInfo_.find(Str(name));
        return it != moduleInfo_.end() && it->second.status == ModuleStatus::Loaded;
    }

    LibManager::ModuleStatus LibManager::getModuleStatus(StrView name) const {
        auto it = moduleInfo_.find(Str(name));
        return it != moduleInfo_.end() ? it->second.status : ModuleStatus::Registered;
    }

    std::optional<LibManager::ModuleInfo> LibManager::getModuleInfo(StrView name) const {
        auto it = moduleInfo_.find(Str(name));
        return it != moduleInfo_.end() ? std::make_optional(it->second) : std::nullopt;
    }

    std::vector<Str> LibManager::getModuleNames() const {
        std::vector<Str> names;
        names.reserve(moduleInfo_.size());
        
        for (const auto& [name, _] : moduleInfo_) {
            names.push_back(name);
        }
        
        std::sort(names.begin(), names.end());
        return names;
    }

    std::vector<Str> LibManager::getLoadedModules() const {
        std::vector<Str> names;
        
        for (const auto& [name, info] : moduleInfo_) {
            if (info.status == ModuleStatus::Loaded) {
                names.push_back(name);
            }
        }
        
        std::sort(names.begin(), names.end());
        return names;
    }

    Value LibManager::callFunction(StrView name, State* state, i32 nargs) const {
        return globalRegistry_->callFunction(name, state, nargs);
    }

    bool LibManager::hasFunction(StrView name) const noexcept {
        return globalRegistry_->hasFunction(name);
    }

    const FunctionMetadata* LibManager::getFunctionMetadata(StrView name) const {
        return globalRegistry_->getFunctionMetadata(name);
    }

    std::vector<Str> LibManager::getAllFunctionNames() const {
        return globalRegistry_->getFunctionNames();
    }

    FunctionRegistry& LibManager::getRegistry() {
        return *globalRegistry_;
    }

    const FunctionRegistry& LibManager::getRegistry() const {
        return *globalRegistry_;
    }

    LibraryContext& LibManager::getContext() {
        return *context_;
    }

    const LibraryContext& LibManager::getContext() const {
        return *context_;
    }

    void LibManager::clear(State* state) {
        // Unload all modules
        if (state) {
            for (const auto& [name, info] : moduleInfo_) {
                if (info.status == ModuleStatus::Loaded) {
                    unloadModule(name, state);
                }
            }
        }

        // Clear all data structures
        modules_.clear();
        factories_.clear();
        moduleInfo_.clear();
        moduleRegistries_.clear();
        globalRegistry_->clear();
        currentlyLoading_.clear();
    }

    LibManager::Statistics LibManager::getStatistics() const {
        Statistics stats;
        stats.totalModules = moduleInfo_.size();
        stats.totalFunctions = globalRegistry_->size();

        for (const auto& [name, info] : moduleInfo_) {
            switch (info.status) {
                case ModuleStatus::Loaded:
                    stats.loadedModules++;
                    break;
                case ModuleStatus::Failed:
                    stats.failedModules++;
                    stats.failedModuleNames.push_back(name);
                    break;
                default:
                    break;
            }
        }

        return stats;
    }

    // Private helper methods
    bool LibManager::loadModuleInternal(const Str& name, State* state) {
        // Check for circular dependencies
        if (currentlyLoading_.find(name) != currentlyLoading_.end()) {
            std::ostringstream oss;
            oss << "Circular dependency detected for module '" << name << "'";
            moduleInfo_[name].status = ModuleStatus::Failed;
            moduleInfo_[name].errorMessage = oss.str();
            return false;
        }

        auto& info = moduleInfo_[name];
        if (info.status == ModuleStatus::Loaded) {
            return true; // Already loaded
        }

        if (info.status == ModuleStatus::Failed) {
            return false; // Previously failed
        }

        currentlyLoading_.insert(name);
        info.status = ModuleStatus::Loading;

        try {
            // Get or create module
            std::unique_ptr<LibModule> module;
            
            auto moduleIt = modules_.find(name);
            if (moduleIt != modules_.end()) {
                module = std::move(moduleIt->second);
                modules_.erase(moduleIt);
            } else {
                auto factoryIt = factories_.find(name);
                if (factoryIt != factories_.end()) {
                    module = factoryIt->second();
                    if (!module) {
                        throw std::runtime_error("Module factory returned null");
                    }
                } else {
                    throw std::runtime_error("Module not found");
                }
            }

            // Configure module
            module->configure(*context_);

            // Get module registry
            auto& registry = moduleRegistries_[name];
            if (!registry) {
                registry = std::make_unique<FunctionRegistry>();
            }

            // Register functions
            module->registerFunctions(*registry, *context_);
            info.functionCount = registry->size();

            // Initialize module
            module->initialize(state, *context_);

            // Store module back
            modules_[name] = std::move(module);

            info.status = ModuleStatus::Loaded;
            currentlyLoading_.erase(name);

            return true;

        } catch (const std::exception& e) {
            std::ostringstream oss;
            oss << "Failed to load module '" << name << "': " << e.what();
            
            info.status = ModuleStatus::Failed;
            info.errorMessage = oss.str();
            currentlyLoading_.erase(name);
            return false;
        }
    }

    bool LibManager::resolveDependencies(const Str& name, State* state) {
        const auto& info = moduleInfo_[name];
        
        for (const auto& dep : info.dependencies) {
            if (!isModuleLoaded(dep)) {
                if (moduleInfo_.find(dep) == moduleInfo_.end()) {
                    return false;
                }
                
                if (!loadModuleInternal(dep, state)) {
                    return false;
                }
            }
        }
        
        return true;
    }

    void LibManager::updateGlobalRegistry() {
        globalRegistry_->clear();
        
        for (const auto& [name, registry] : moduleRegistries_) {
            if (moduleInfo_[name].status == ModuleStatus::Loaded) {
                // Copy functions from module registry to global registry
                auto functionNames = registry->getFunctionNames();
                for (const auto& funcName : functionNames) {
                    if (auto meta = registry->getFunctionMetadata(funcName)) {
                        // Note: This is a simplified approach
                        // In a real implementation, we'd need to properly copy the function
                    }
                }
            }
        }
    }

} // namespace Lua

// Manager factory implementations
namespace Lua::ManagerFactory {
    std::unique_ptr<LibManager> createStandardManager() {
        auto context = std::make_shared<LibraryContext>();
        context->setConfig("standard_mode", true);

        auto manager = std::make_unique<LibManager>(context);
        manager->registerModule(BaseLibFactory::createStandard());

        return manager;
    }

    std::unique_ptr<LibManager> createMinimalManager() {
        auto context = std::make_shared<LibraryContext>();
        context->setConfig("minimal_mode", true);

        auto manager = std::make_unique<LibManager>(context);
        manager->registerModule(BaseLibFactory::createMinimal());

        return manager;
    }

    std::unique_ptr<LibManager> createCustomManager(const std::vector<Str>& moduleNames) {
        auto context = std::make_shared<LibraryContext>();
        context->setConfig("custom_mode", true);

        auto manager = std::make_unique<LibManager>(context);

        for (const auto& moduleName : moduleNames) {
            if (moduleName == "base") {
                manager->registerModule(BaseLibFactory::createStandard());
            }
            // Add other modules as they become available
        }

        return manager;
    }
}

// Quick setup implementations
namespace Lua::QuickSetup {
    void openStandardLibraries(State* state) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }

        auto manager = ManagerFactory::createStandardManager();
        manager->loadAllModules(state);
    }

    void openLibrary(State* state, StrView libraryName) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }

        auto manager = ManagerFactory::createCustomManager({Str(libraryName)});
        manager->loadModule(libraryName, state);
    }

    void openLibraries(State* state, const std::vector<Str>& libraryNames) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }

        auto manager = ManagerFactory::createCustomManager(libraryNames);
        manager->loadAllModules(state);
    }
}
