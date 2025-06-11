#pragma once

#include "../common/types.hpp"
#include "lib_common.hpp"

namespace Lua {
    
    // Library manager for registering and managing standard library modules
    class LibManager {
    public:
        // Singleton instance
        static LibManager& getInstance() {
            static LibManager instance;
            return instance;
        }
        
        // Register a library module
        void registerLibrary(const Str& name, std::function<UPtr<LibModule>()> factory) {
            libraries_[name] = factory;
        }
        
        // Load a specific library into state
        bool loadLibrary(State* state, const Str& name) {
            auto it = libraries_.find(name);
            if (it == libraries_.end()) {
                return false;
            }
            
            // Check if already loaded
            auto loaded_it = loaded_modules_.find(name);
            if (loaded_it != loaded_modules_.end()) {
                return true; // Already loaded
            }
            
            // Create and register module
            auto module = it->second();
            if (module) {
                module->registerModule(state);
                loaded_modules_[name] = std::move(module);
                return true;
            }
            
            return false;
        }
        
        // Load all registered libraries
        void loadAllLibraries(State* state) {
            for (const auto& [name, factory] : libraries_) {
                loadLibrary(state, name);
            }
        }
        
        // Load core libraries (base, string, table, math)
        void loadCoreLibraries(State* state) {
            const Vec<Str> core_libs = {
                "base", "string", "table", "math"
            };
            
            for (const Str& lib : core_libs) {
                loadLibrary(state, lib);
            }
        }
        
        // Load extended libraries (io, os)
        void loadExtendedLibraries(State* state) {
            const Vec<Str> extended_libs = {
                "io", "os"
            };
            
            for (const Str& lib : extended_libs) {
                loadLibrary(state, lib);
            }
        }
        
        // Check if library is registered
        bool isRegistered(const Str& name) const {
            return libraries_.find(name) != libraries_.end();
        }
        
        // Check if library is loaded
        bool isLoaded(const Str& name) const {
            auto it = loaded_modules_.find(name);
            return it != loaded_modules_.end() && it->second->isLoaded();
        }
        
        // Get list of registered libraries
        Vec<Str> getRegisteredLibraries() const {
            Vec<Str> result;
            result.reserve(libraries_.size());
            
            for (const auto& [name, factory] : libraries_) {
                result.push_back(name);
            }
            
            return result;
        }
        
        // Get list of loaded libraries
        Vec<Str> getLoadedLibraries() const {
            Vec<Str> result;
            result.reserve(loaded_modules_.size());
            
            for (const auto& [name, module] : loaded_modules_) {
                if (module->isLoaded()) {
                    result.push_back(name);
                }
            }
            
            return result;
        }
        
        // Get module by name
        LibModule* getModule(const Str& name) {
            auto it = loaded_modules_.find(name);
            return (it != loaded_modules_.end()) ? it->second.get() : nullptr;
        }
        
        // Unload a library
        void unloadLibrary(const Str& name) {
            loaded_modules_.erase(name);
        }
        
        // Unload all libraries
        void unloadAllLibraries() {
            loaded_modules_.clear();
        }
        
        // Get library information
        struct LibraryInfo {
            Str name;
            Str version;
            bool loaded;
        };
        
        Vec<LibraryInfo> getLibraryInfo() const {
            Vec<LibraryInfo> result;
            
            for (const auto& [name, factory] : libraries_) {
                LibraryInfo info;
                info.name = name;
                info.loaded = isLoaded(name);
                
                // Try to get version from loaded module
                auto loaded_it = loaded_modules_.find(name);
                if (loaded_it != loaded_modules_.end()) {
                    info.version = loaded_it->second->getVersion();
                } else {
                    info.version = "unknown";
                }
                
                result.push_back(info);
            }
            
            return result;
        }
        
    private:
        LibManager() = default;
        ~LibManager() = default;
        
        // Non-copyable
        LibManager(const LibManager&) = delete;
        LibManager& operator=(const LibManager&) = delete;
        
        // Registered library factories
        HashMap<Str, std::function<UPtr<LibModule>()>> libraries_;
        
        // Loaded modules
        HashMap<Str, UPtr<LibModule>> loaded_modules_;
    };
    
    // Helper macros for library registration
    #define REGISTER_LIB(name, class_name) \
        LibManager::getInstance().registerLibrary(name, []() -> UPtr<LibModule> { \
            return std::make_unique<class_name>(); \
        })
    
    // Convenience functions
    namespace Lib {
        
        // Initialize standard libraries
        inline void initStandardLibraries(State* state) {
            LibManager::getInstance().loadCoreLibraries(state);
        }
        
        // Initialize all libraries
        inline void initAllLibraries(State* state) {
            LibManager::getInstance().loadAllLibraries(state);
        }
        
        // Load specific library
        inline bool load(State* state, const Str& name) {
            return LibManager::getInstance().loadLibrary(state, name);
        }
        
        // Check if library is available
        inline bool isAvailable(const Str& name) {
            return LibManager::getInstance().isRegistered(name);
        }
        
        // Check if library is loaded
        inline bool isLoaded(const Str& name) {
            return LibManager::getInstance().isLoaded(name);
        }
    }
}