#include "lib_manager.hpp"
#include "lib_utils.hpp"
#include <iostream>
#include <algorithm>

namespace Lua {
    
    // Implementation is mostly in header due to template nature
    // This file can contain additional utility functions if needed
    
    namespace LibManagerUtils {
        
        // Validate library name
        bool isValidLibraryName(const std::string& name) {
            if (name.empty()) return false;
            
            // Check for valid characters (alphanumeric and underscore)
            return std::all_of(name.begin(), name.end(), [](char c) {
                return std::isalnum(c) || c == '_';
            });
        }
        
        // Get library priority for loading order
        int getLibraryPriority(const std::string& name) {
            // Define loading order priorities
            static const std::unordered_map<std::string, int> priorities = {
                {"base", 1},     // Load base library first
                {"string", 2},  // String library
                {"table", 3},   // Table library
                {"math", 4},    // Math library
                {"io", 5},      // IO library
                {"os", 6},      // OS library
                {"debug", 7},   // Debug library
                {"coroutine", 8}, // Coroutine library
                {"package", 9}  // Package library
            };
            
            auto it = priorities.find(name);
            return (it != priorities.end()) ? it->second : 100; // Unknown libraries get low priority
        }
        
        // Sort libraries by priority
        std::vector<std::string> sortLibrariesByPriority(const std::vector<std::string>& libraries) {
            std::vector<std::string> sorted = libraries;
            
            std::sort(sorted.begin(), sorted.end(), [](const std::string& a, const std::string& b) {
                return getLibraryPriority(a) < getLibraryPriority(b);
            });
            
            return sorted;
        }
    }
    
    // Extended LibManager functionality
    namespace LibManagerExtensions {
        
        // Load libraries in dependency order
        bool loadLibrariesInOrder(State* state, const std::vector<std::string>& libraries) {
            auto sorted_libs = LibManagerUtils::sortLibrariesByPriority(libraries);
            
            for (const std::string& lib : sorted_libs) {
                if (!LibManager::getInstance().loadLibrary(state, lib)) {
                    std::cerr << "Failed to load library: " << lib << std::endl;
                    return false;
                }
            }
            
            return true;
        }
        
        // Get library dependencies (placeholder for future implementation)
        std::vector<std::string> getLibraryDependencies(const std::string& library) {
            // For now, return empty dependencies
            // In the future, this could be extended to handle complex dependencies
            static const std::unordered_map<std::string, std::vector<std::string>> dependencies = {
                {"io", {"base"}},
                {"os", {"base"}},
                {"debug", {"base"}},
                {"coroutine", {"base"}},
                {"package", {"base", "string"}}
            };
            
            auto it = dependencies.find(library);
            return (it != dependencies.end()) ? it->second : std::vector<std::string>{};
        }
        
        // Load library with dependencies
        bool loadLibraryWithDependencies(State* state, const std::string& library) {
            // Load dependencies first
            auto deps = getLibraryDependencies(library);
            for (const std::string& dep : deps) {
                if (!LibManager::getInstance().loadLibrary(state, dep)) {
                    std::cerr << "Failed to load dependency: " << dep << " for library: " << library << std::endl;
                    return false;
                }
            }
            
            // Load the library itself
            return LibManager::getInstance().loadLibrary(state, library);
        }
        
        // Validate library compatibility
        bool validateLibraryCompatibility(const std::string& library) {
            // Placeholder for compatibility checks
            // Could check version compatibility, platform support, etc.
            return LibManagerUtils::isValidLibraryName(library);
        }
        
        // Get library statistics
        struct LibraryStats {
            size_t total_registered;
            size_t total_loaded;
            std::vector<std::string> failed_loads;
        };
        
        LibraryStats getLibraryStats() {
            LibraryStats stats;
            
            auto& manager = LibManager::getInstance();
            auto registered = manager.getRegisteredLibraries();
            auto loaded = manager.getLoadedLibraries();
            
            stats.total_registered = registered.size();
            stats.total_loaded = loaded.size();
            
            // Find failed loads (registered but not loaded)
            for (const std::string& lib : registered) {
                if (std::find(loaded.begin(), loaded.end(), lib) == loaded.end()) {
                    stats.failed_loads.push_back(lib);
                }
            }
            
            return stats;
        }
        
        // Print library status
        void printLibraryStatus() {
            auto stats = getLibraryStats();
            auto info = LibManager::getInstance().getLibraryInfo();
            
            std::cout << "=== Library Status ===" << std::endl;
            std::cout << "Total Registered: " << stats.total_registered << std::endl;
            std::cout << "Total Loaded: " << stats.total_loaded << std::endl;
            
            if (!stats.failed_loads.empty()) {
                std::cout << "Failed Loads: ";
                for (size_t i = 0; i < stats.failed_loads.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << stats.failed_loads[i];
                }
                std::cout << std::endl;
            }
            
            std::cout << "\n=== Library Details ===" << std::endl;
            for (const auto& lib : info) {
                std::cout << lib.name << " (v" << lib.version << ") - " 
                         << (lib.loaded ? "LOADED" : "NOT LOADED") << std::endl;
            }
        }
    }
}