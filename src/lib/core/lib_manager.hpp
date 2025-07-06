#pragma once

#include "lib_define.hpp"
#include "lib_module.hpp"
#include "lib_context.hpp"
#include "lib_func_registry.hpp"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>

namespace Lua {

    /**
     * Modern library manager with dependency injection and lifecycle management
     */
    class LibManager {
    public:
        enum class LoadStrategy {
            Immediate,  // Load immediately when registered
            Lazy,       // Load when first accessed
            Manual      // Load only when explicitly requested
        };

        enum class ModuleStatus {
            Registered, // Module is registered but not loaded
            Loading,    // Module is currently being loaded
            Loaded,     // Module is loaded and ready
            Failed,     // Module failed to load
            Unloaded    // Module was unloaded
        };

        /**
         * Module information
         */
        struct ModuleInfo {
            Str name;
            Str version;
            ModuleStatus status = ModuleStatus::Registered;
            LoadStrategy strategy = LoadStrategy::Immediate;
            std::vector<Str> dependencies;
            Str errorMessage; // If status is Failed
            size_t functionCount = 0;
            
            ModuleInfo() = default;
            ModuleInfo(StrView n, StrView v = "1.0") 
                : name(n), version(v) {}
        };

        /**
         * Statistics
         */
        struct Statistics {
            size_t totalModules = 0;
            size_t loadedModules = 0;
            size_t failedModules = 0;
            size_t totalFunctions = 0;
            std::vector<Str> failedModuleNames;
        };

        /**
         * Constructor with optional context
         */
        explicit LibManager(std::shared_ptr<Lib::LibContext> context = nullptr);

        /**
         * Register module
         */
        void registerModule(std::unique_ptr<Lib::LibModule> module, LoadStrategy strategy = LoadStrategy::Immediate);

        /**
         * Register module factory
         */
        void registerModuleFactory(StrView name,
                                         std::function<std::unique_ptr<Lib::LibModule>()> factory,
                                 LoadStrategy strategy = LoadStrategy::Lazy);

        /**
         * Load single module
         */
        bool loadModule(StrView name, State* state);

        /**
         * Load all registered modules
         */
        void loadAllModules(State* state);

        /**
         * Unload module
         */
        bool unloadModule(StrView name, State* state = nullptr);

        /**
         * Check if module is loaded
         */
        bool isModuleLoaded(StrView name) const noexcept;

        /**
         * Get module status
         */
        ModuleStatus getModuleStatus(StrView name) const;

        /**
         * Get module information
         */
        std::optional<ModuleInfo> getModuleInfo(StrView name) const;

        /**
         * Get all module names
         */
        std::vector<Str> getModuleNames() const;

        /**
         * Get loaded module names
         */
        std::vector<Str> getLoadedModules() const;

        /**
         * Call function
         */
        Value callFunction(StrView name, State* state, i32 nargs) const;

        /**
         * Check if function exists
         */
        bool hasFunction(StrView name) const noexcept;

        /**
         * Get function metadata
         */
        const Lib::FunctionMetadata* getFunctionMetadata(StrView name) const;

        /**
         * Get all function names
         */
        std::vector<Str> getAllFunctionNames() const;

        /**
         * Get function registry (for advanced operations)
         */
        Lib::LibFuncRegistry& getRegistry();
        const Lib::LibFuncRegistry& getRegistry() const;

        /**
         * Get library context
         */
        Lib::LibContext& getContext();
        const Lib::LibContext& getContext() const;

        /**
         * Clear all modules
         */
        void clear(State* state = nullptr);

        /**
         * Get statistics
         */
        Statistics getStatistics() const;

    private:
        // Internal helper methods
        bool loadModuleInternal(const Str& name, State* state);
        bool resolveDependencies(const Str& name, State* state);
        void updateGlobalRegistry();

        // Data members
        std::unordered_map<Str, std::unique_ptr<Lib::LibModule>> modules_;
        std::unordered_map<Str, std::function<std::unique_ptr<Lib::LibModule>()>> factories_;
        std::unordered_map<Str, ModuleInfo> moduleInfo_;
        std::unordered_map<Str, std::unique_ptr<Lib::LibFuncRegistry>> moduleRegistries_;
        std::unique_ptr<Lib::LibFuncRegistry> globalRegistry_;
        std::shared_ptr<Lib::LibContext> context_;
        std::unordered_set<Str> currentlyLoading_; // For circular dependency detection
    };

    /**
     * Factory functions for common configurations
     */
    namespace ManagerFactory {
        /**
         * Create standard library manager with all standard modules
         */
        std::unique_ptr<LibManager> createStandardManager();

        /**
         * Create minimal library manager with only essential modules
         */
        std::unique_ptr<LibManager> createMinimalManager();

        /**
         * Create custom library manager with specified modules
         */
        std::unique_ptr<LibManager> createCustomManager(const std::vector<Str>& moduleNames);
    }

    /**
     * Quick setup functions for common use cases
     */
    namespace QuickSetup {
        /**
         * Open standard libraries in state
         */
        void openStandardLibraries(State* state);

        /**
         * Open specific library
         */
        void openLibrary(State* state, StrView libraryName);

        /**
         * Open multiple libraries
         */
        void openLibraries(State* state, const std::vector<Str>& libraryNames);
    }

} // namespace Lua
