#include "lib_init.hpp"
#include "lib_manager.hpp"
#include "lib_utils.hpp"
#include "base_lib.hpp"
#include "string_lib.hpp"
#include "table_lib.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace Lua {
    namespace LibInit {
        
        // Global configuration variables
        namespace {
            std::vector<std::string> g_library_paths;
            int g_loading_timeout = 5000; // 5 seconds default
            bool g_logging_enabled = false;
            Error::InitError g_last_error = Error::InitError::NONE;
            std::string g_last_error_message;
            Error::ErrorHandler g_error_handler;
            
            // Log function
            void log(const std::string& message) {
                if (g_logging_enabled) {
                    std::cout << "[LibInit] " << message << std::endl;
                }
            }
            
            // Set error state
            void setError(Error::InitError error, const std::string& message = "") {
                g_last_error = error;
                g_last_error_message = message;
                
                if (g_error_handler) {
                    g_error_handler(error, message);
                }
            }
        }
        
        // Core initialization functions
        void initCoreLibraries(State* state) {
            log("Initializing core libraries...");
            
            InitOptions options = getDefaultOptions();
            options.load_io = false;
            options.load_os = false;
            options.load_debug = false;
            options.load_coroutine = false;
            options.load_package = false;
            
            initLibrariesWithOptions(state, options);
        }
        
        void initExtendedLibraries(State* state) {
            log("Initializing extended libraries...");
            
            LibManager& manager = LibManager::getInstance();
            
            // Load IO library
            if (manager.isRegistered("io")) {
                if (!manager.loadLibrary(state, "io")) {
                    setError(Error::InitError::INITIALIZATION_FAILED, "Failed to load IO library");
                }
            }
            
            // Load OS library
            if (manager.isRegistered("os")) {
                if (!manager.loadLibrary(state, "os")) {
                    setError(Error::InitError::INITIALIZATION_FAILED, "Failed to load OS library");
                }
            }
        }
        
        void initAdvancedLibraries(State* state) {
            log("Initializing advanced libraries...");
            
            LibManager& manager = LibManager::getInstance();
            
            // Load debug library
            if (manager.isRegistered("debug")) {
                if (!manager.loadLibrary(state, "debug")) {
                    setError(Error::InitError::INITIALIZATION_FAILED, "Failed to load debug library");
                }
            }
            
            // Load coroutine library
            if (manager.isRegistered("coroutine")) {
                if (!manager.loadLibrary(state, "coroutine")) {
                    setError(Error::InitError::INITIALIZATION_FAILED, "Failed to load coroutine library");
                }
            }
            
            // Load package library
            if (manager.isRegistered("package")) {
                if (!manager.loadLibrary(state, "package")) {
                    setError(Error::InitError::INITIALIZATION_FAILED, "Failed to load package library");
                }
            }
        }
        
        void initAllLibraries(State* state) {
            log("Initializing all libraries...");
            
            InitOptions options = getDefaultOptions();
            options.load_io = true;
            options.load_os = true;
            options.load_debug = true;
            options.load_coroutine = true;
            options.load_package = true;
            
            initLibrariesWithOptions(state, options);
        }
        
        void initMinimalLibraries(State* state) {
            log("Initializing minimal libraries...");
            
            // Only load base library
            LibManager& manager = LibManager::getInstance();
            if (!manager.loadLibrary(state, "base")) {
                setError(Error::InitError::INITIALIZATION_FAILED, "Failed to load base library");
            }
        }
        
        void registerAllLibraries() {
            log("Registering all library factories...");
            
            LibManager& manager = LibManager::getInstance();
            
            // Register base library (already implemented)
            manager.registerLibrary("base", []() -> std::unique_ptr<LibModule> {
                // For now, we'll create a wrapper for the existing base library
                class BaseLibWrapper : public LibModule {
                public:
                    const std::string& getName() const override {
                        static const std::string name = "base";
                        return name;
                    }
                    
                    void registerModule(State* state) override {
                        registerBaseLib(state); // Use existing function
                        setLoaded(true);
                    }
                };
                return std::make_unique<BaseLibWrapper>();
            });
            
            // Register other libraries
            manager.registerLibrary("string", []() { return std::make_unique<StringLib>(); });
            manager.registerLibrary("table", []() { return std::make_unique<TableLib>(); });
            // manager.registerLibrary("math", []() { return std::make_unique<MathLib>(); });
            // manager.registerLibrary("io", []() { return std::make_unique<IOLib>(); });
            // manager.registerLibrary("os", []() { return std::make_unique<OSLib>(); });
            
            log("Library registration completed.");
        }
        
        void initLibrariesWithOptions(State* state, const InitOptions& options) {
            log("Initializing libraries with custom options...");
            
            if (!Validation::validateOptions(options)) {
                setError(Error::InitError::INITIALIZATION_FAILED, "Invalid initialization options");
                return;
            }
            
            LibManager& manager = LibManager::getInstance();
            
            // Load libraries based on options
            if (options.load_base && manager.isRegistered("base")) {
                if (!manager.loadLibrary(state, "base")) {
                    setError(Error::InitError::INITIALIZATION_FAILED, "Failed to load base library");
                    return;
                }
            }
            
            if (options.load_string && manager.isRegistered("string")) {
                manager.loadLibrary(state, "string");
            }
            
            if (options.load_table && manager.isRegistered("table")) {
                manager.loadLibrary(state, "table");
            }
            
            if (options.load_math && manager.isRegistered("math")) {
                manager.loadLibrary(state, "math");
            }
            
            if (options.load_io && !options.safe_mode && manager.isRegistered("io")) {
                manager.loadLibrary(state, "io");
            }
            
            if (options.load_os && !options.safe_mode && manager.isRegistered("os")) {
                manager.loadLibrary(state, "os");
            }
            
            if (options.load_debug && !options.sandbox_mode && manager.isRegistered("debug")) {
                manager.loadLibrary(state, "debug");
            }
            
            if (options.load_coroutine && manager.isRegistered("coroutine")) {
                manager.loadLibrary(state, "coroutine");
            }
            
            if (options.load_package && !options.sandbox_mode && manager.isRegistered("package")) {
                manager.loadLibrary(state, "package");
            }
            
            log("Library initialization completed.");
        }
        
        InitOptions getDefaultOptions() {
            InitOptions options;
            options.load_base = true;
            options.load_string = true;
            options.load_table = true;
            options.load_math = true;
            options.load_io = false;
            options.load_os = false;
            options.load_debug = false;
            options.load_coroutine = false;
            options.load_package = false;
            options.safe_mode = false;
            options.sandbox_mode = false;
            return options;
        }
        
        InitOptions getSafeModeOptions() {
            InitOptions options = getDefaultOptions();
            options.safe_mode = true;
            options.load_io = false;
            options.load_os = false;
            options.load_debug = false;
            options.load_package = false;
            return options;
        }
        
        InitOptions getSandboxModeOptions() {
            InitOptions options;
            options.load_base = true;
            options.load_string = true;
            options.load_table = true;
            options.load_math = true;
            options.load_io = false;
            options.load_os = false;
            options.load_debug = false;
            options.load_coroutine = false;
            options.load_package = false;
            options.safe_mode = true;
            options.sandbox_mode = true;
            return options;
        }
        
        // Configuration implementation
        namespace Config {
            void setLibraryPaths(const std::vector<std::string>& paths) {
                g_library_paths = paths;
            }
            
            void addLibraryPath(const std::string& path) {
                g_library_paths.push_back(path);
            }
            
            std::vector<std::string> getLibraryPaths() {
                return g_library_paths;
            }
            
            void setLoadingTimeout(int timeout_ms) {
                g_loading_timeout = timeout_ms;
            }
            
            int getLoadingTimeout() {
                return g_loading_timeout;
            }
            
            void setLoggingEnabled(bool enabled) {
                g_logging_enabled = enabled;
            }
            
            bool isLoggingEnabled() {
                return g_logging_enabled;
            }
        }
        
        // Validation implementation
        namespace Validation {
            bool validateLibrary(const std::string& name) {
                return LibManager::getInstance().isRegistered(name);
            }
            
            bool checkDependencies(const std::string& name) {
                // TODO: Implement dependency checking
                return true;
            }
            
            bool validateOptions(const InitOptions& options) {
                // Basic validation
                if (options.sandbox_mode && (options.load_io || options.load_os || options.load_debug || options.load_package)) {
                    return false; // Sandbox mode shouldn't allow these libraries
                }
                
                if (options.safe_mode && (options.load_io || options.load_os)) {
                    return false; // Safe mode shouldn't allow IO/OS access
                }
                
                return true;
            }
            
            std::vector<std::string> getValidationErrors() {
                // TODO: Implement validation error collection
                return {};
            }
        }
        
        // Information implementation
        namespace Info {
            std::string getLibraryVersion(const std::string& name) {
                auto* module = LibManager::getInstance().getModule(name);
                return module ? module->getVersion() : "unknown";
            }
            
            std::string getLibraryDescription(const std::string& name) {
                // TODO: Implement library descriptions
                static const std::unordered_map<std::string, std::string> descriptions = {
                    {"base", "Basic Lua functions and utilities"},
                    {"string", "String manipulation functions"},
                    {"table", "Table manipulation functions"},
                    {"math", "Mathematical functions and constants"},
                    {"io", "Input/output functions"},
                    {"os", "Operating system interface"},
                    {"debug", "Debug facilities"},
                    {"coroutine", "Coroutine manipulation functions"},
                    {"package", "Module system"}
                };
                
                auto it = descriptions.find(name);
                return (it != descriptions.end()) ? it->second : "Unknown library";
            }
            
            std::string getLibraryAuthor(const std::string& name) {
                return "Lua Implementation Team";
            }
            
            std::string getLibraryLicense(const std::string& name) {
                return "MIT License";
            }
            
            std::vector<LibraryMetadata> getAllLibraryMetadata() {
                std::vector<LibraryMetadata> metadata;
                
                auto registered = LibManager::getInstance().getRegisteredLibraries();
                for (const std::string& name : registered) {
                    LibraryMetadata meta;
                    meta.name = name;
                    meta.version = getLibraryVersion(name);
                    meta.description = getLibraryDescription(name);
                    meta.author = getLibraryAuthor(name);
                    meta.license = getLibraryLicense(name);
                    meta.is_core = (name == "base" || name == "string" || name == "table" || name == "math");
                    meta.is_safe = (name != "io" && name != "os" && name != "debug" && name != "package");
                    
                    metadata.push_back(meta);
                }
                
                return metadata;
            }
        }
        
        // Error handling implementation
        namespace Error {
            InitError getLastError() {
                return g_last_error;
            }
            
            std::string getErrorMessage(InitError error) {
                switch (error) {
                    case InitError::NONE:
                        return "No error";
                    case InitError::LIBRARY_NOT_FOUND:
                        return "Library not found";
                    case InitError::DEPENDENCY_MISSING:
                        return "Missing dependency";
                    case InitError::INCOMPATIBLE_VERSION:
                        return "Incompatible version";
                    case InitError::INITIALIZATION_FAILED:
                        return "Initialization failed";
                    case InitError::PERMISSION_DENIED:
                        return "Permission denied";
                    case InitError::TIMEOUT:
                        return "Operation timeout";
                    case InitError::UNKNOWN_ERROR:
                    default:
                        return "Unknown error";
                }
            }
            
            void clearError() {
                g_last_error = InitError::NONE;
                g_last_error_message.clear();
            }
            
            void setErrorHandler(ErrorHandler handler) {
                g_error_handler = handler;
            }
        }
    }
}