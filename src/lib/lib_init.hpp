#pragma once

#include "../common/types.hpp"
#include "lib_common.hpp"
#include "lib_manager.hpp"
#include "../vm/state.hpp"

namespace Lua {
    
    // Forward declarations for all library modules
    class BaseLib;
    class StringLib;
    class TableLib;
    class MathLib;
    class IOLib;
    class OSLib;
    class DebugLib;
    class CoroutineLib;
    class PackageLib;
    
    // Library initialization functions
    namespace LibInit {
        
        // Initialize core libraries (base, string, table, math)
        void initCoreLibraries(State* state);
        
        // Initialize extended libraries (io, os)
        void initExtendedLibraries(State* state);
        
        // Initialize advanced libraries (debug, coroutine, package)
        void initAdvancedLibraries(State* state);
        
        // Initialize all libraries
        void initAllLibraries(State* state);
        
        // Initialize minimal libraries (base only)
        void initMinimalLibraries(State* state);
        
        // Register all library factories
        void registerAllLibraries();
        
        // Library initialization options
        struct InitOptions {
            bool load_base = true;          // Always load base library
            bool load_string = true;        // String manipulation
            bool load_table = true;         // Table operations
            bool load_math = true;          // Mathematical functions
            bool load_io = false;           // File I/O (optional)
            bool load_os = false;           // OS operations (optional)
            bool load_debug = false;        // Debug facilities (optional)
            bool load_coroutine = false;    // Coroutines (optional)
            bool load_package = false;      // Module system (optional)
            
            // Safety options
            bool safe_mode = false;         // Disable potentially unsafe libraries
            bool sandbox_mode = false;      // Enable sandboxing
        };
        
        // Initialize libraries with custom options
        void initLibrariesWithOptions(State* state, const InitOptions& options);
        
        // Get default initialization options
        InitOptions getDefaultOptions();
        
        // Get safe mode options (no IO, OS access)
        InitOptions getSafeModeOptions();
        
        // Get sandbox mode options (very restricted)
        InitOptions getSandboxModeOptions();
        
        // Library configuration
        namespace Config {
            
            // Set library search paths
            void setLibraryPaths(const Vec<Str>& paths);
            
            // Add library search path
            void addLibraryPath(const Str& path);
            
            // Get library search paths
            Vec<Str> getLibraryPaths();
            
            // Set library loading timeout
            void setLoadingTimeout(int timeout_ms);
            
            // Get library loading timeout
            int getLoadingTimeout();
            
            // Enable/disable library loading logs
            void setLoggingEnabled(bool enabled);
            
            // Check if logging is enabled
            bool isLoggingEnabled();
        }
        
        // Library validation
        namespace Validation {
            
            // Validate library compatibility
            bool validateLibrary(const Str& name);
            
            // Check library dependencies
            bool checkDependencies(const Str& name);
            
            // Validate initialization options
            bool validateOptions(const InitOptions& options);
            
            // Get validation errors
            Vec<Str> getValidationErrors();
        }
        
        // Library information
        namespace Info {
            
            // Get library version
            Str getLibraryVersion(const Str& name);
            
            // Get library description
            Str getLibraryDescription(const Str& name);
            
            // Get library author
            Str getLibraryAuthor(const Str& name);
            
            // Get library license
            Str getLibraryLicense(const Str& name);
            
            // Get all library information
            struct LibraryMetadata {
                Str name;
                Str version;
                Str description;
                Str author;
                Str license;
                Vec<Str> dependencies;
                bool is_core;
                bool is_safe;
            };
            
            Vec<LibraryMetadata> getAllLibraryMetadata();
        }
        
        // Error handling
        namespace Error {
            
            // Library initialization error types
            enum class InitError {
                NONE,
                LIBRARY_NOT_FOUND,
                DEPENDENCY_MISSING,
                INCOMPATIBLE_VERSION,
                INITIALIZATION_FAILED,
                PERMISSION_DENIED,
                TIMEOUT,
                UNKNOWN_ERROR
            };
            
            // Get last initialization error
            InitError getLastError();
            
            // Get error message
            Str getErrorMessage(InitError error);
            
            // Clear error state
            void clearError();
            
            // Set error handler
            using ErrorHandler = std::function<void(InitError, const Str&)>;
            void setErrorHandler(ErrorHandler handler);
        }
    }
    
    // Convenience macros
    #define LUA_INIT_CORE(state) Lua::LibInit::initCoreLibraries(state)
    #define LUA_INIT_ALL(state) Lua::LibInit::initAllLibraries(state)
    #define LUA_INIT_SAFE(state) Lua::LibInit::initLibrariesWithOptions(state, Lua::LibInit::getSafeModeOptions())
    #define LUA_INIT_MINIMAL(state) Lua::LibInit::initMinimalLibraries(state)
}