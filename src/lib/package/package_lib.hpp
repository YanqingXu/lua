#pragma once

#include "../core/lib_module.hpp"
#include "../../common/types.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include <memory>
#include <set>

namespace Lua {

/**
 * @brief Package library implementation for Lua 5.1 module system
 *
 * Provides essential module loading functions:
 * - require: Load and cache modules
 * - loadfile: Load Lua file without execution
 * - dofile: Load and execute Lua file
 * - package.searchpath: Search for module files
 * - package.path: Module search path patterns
 * - package.loaded: Module cache table
 * - package.preload: Preloaded module table
 * - package.loaders: Module searcher functions (Lua 5.1 name)
 *
 * This implementation follows Lua 5.1 design patterns:
 * - Single VM instance with CallInfo stack for nested calls
 * - VM context-aware calling mechanisms
 * - Proper module caching and circular dependency detection
 * - Standard Lua 5.1 package library API compatibility
 */
class PackageLib : public LibModule {
public:
    /**
     * @brief Get module name
     * @return Module name as string literal
     */
    const char* getName() const override { return "package"; }

    /**
     * @brief Register package functions to State
     * @param state Lua state to register functions to
     * @throws std::invalid_argument if state is null
     */
    void registerFunctions(State* state) override;

    /**
     * @brief Initialize package library with default values
     * @param state Lua state to initialize
     * @throws std::invalid_argument if state is null
     */
    void initialize(State* state) override;

private:
    // ===================================================================
    // Core Package Functions (Global)
    // ===================================================================

    /**
     * @brief require(modname) - Load and cache module
     * @param state Lua state
     * @param nargs Number of arguments (should be 1)
     * @return Loaded module value
     * @throws LuaException if module not found or circular dependency
     */
    static Value require(State* state, i32 nargs);

    /**
     * @brief loadfile(filename) - Load Lua file without execution
     * @param state Lua state
     * @param nargs Number of arguments (should be 1)
     * @return Compiled function or nil on error
     */
    static Value loadfile(State* state, i32 nargs);

    /**
     * @brief dofile(filename) - Load and execute Lua file
     * @param state Lua state
     * @param nargs Number of arguments (should be 1)
     * @return Result of file execution
     */
    static Value dofile(State* state, i32 nargs);

    // ===================================================================
    // Package Table Functions
    // ===================================================================

    /**
     * @brief package.searchpath(name, path [, sep [, rep]]) - Search for module file
     * @param state Lua state
     * @param nargs Number of arguments (1-4)
     * @return File path if found, nil plus error message otherwise
     */
    static Value searchpath(State* state, i32 nargs);

    // ===================================================================
    // Internal Helper Functions
    // ===================================================================

    /**
     * @brief Find module using package.loaders searchers
     * @param state Lua state
     * @param modname Module name
     * @return Loaded module value
     * @throws LuaException if module not found
     */
    static Value findModule(State* state, const Str& modname);

    /**
     * @brief Load Lua module from file
     * @param state Lua state
     * @param filename File path
     * @param modname Module name (for error reporting)
     * @return Loaded module value
     * @throws LuaException if compilation or execution fails
     */
    static Value loadLuaModule(State* state, const Str& filename, const Str& modname);

    /**
     * @brief Check for circular dependency
     * @param state Lua state
     * @param modname Module name
     * @return true if circular dependency detected
     */
    static bool checkCircularDependency(State* state, const Str& modname);

    /**
     * @brief Mark module as loading (for circular dependency detection)
     * @param state Lua state
     * @param modname Module name
     */
    static void markModuleLoading(State* state, const Str& modname);

    /**
     * @brief Unmark module as loading
     * @param state Lua state
     * @param modname Module name
     */
    static void unmarkModuleLoading(State* state, const Str& modname);

    // ===================================================================
    // Package Searcher Functions (Lua 5.1 package.loaders)
    // ===================================================================

    /**
     * @brief Searcher for preloaded modules (package.preload)
     * @param state Lua state
     * @param nargs Number of arguments (should be 1)
     * @return Loader function or nil
     */
    static Value searcherPreload(State* state, i32 nargs);

    /**
     * @brief Searcher for Lua modules (package.path)
     * @param state Lua state
     * @param nargs Number of arguments (should be 1)
     * @return Loader function or nil
     */
    static Value searcherLua(State* state, i32 nargs);

    // ===================================================================
    // Utility Functions
    // ===================================================================

    /**
     * @brief Get package table from global environment
     * @param state Lua state
     * @return Package table value
     * @throws LuaException if package table not found
     */
    static Value getPackageTable(State* state);

    /**
     * @brief Get package.loaded table
     * @param state Lua state
     * @return package.loaded table value
     * @throws LuaException if table not found
     */
    static Value getLoadedTable(State* state);

    /**
     * @brief Get package.preload table
     * @param state Lua state
     * @return package.preload table value
     * @throws LuaException if table not found
     */
    static Value getPreloadTable(State* state);

    /**
     * @brief Get package.loaders array
     * @param state Lua state
     * @return package.loaders array value
     * @throws LuaException if array not found
     */
    static Value getLoadersArray(State* state);

    /**
     * @brief Get package.path string
     * @param state Lua state
     * @return package.path string value
     * @throws LuaException if path not found
     */
    static Value getPackagePath(State* state);

    /**
     * @brief Set up package.loaded entries for existing standard libraries
     * @param state Lua state
     * @param loadedTable package.loaded table
     */
    static void setupStandardLibraryEntries(State* state, GCRef<Table> loadedTable);

    // ===================================================================
    // Constants
    // ===================================================================

    // Default package.path value (Lua 5.1 standard)
    static constexpr const char* DEFAULT_PACKAGE_PATH = "./?.lua;./?/init.lua;./lua/?.lua;./lua/?/init.lua";
    
    // Loading marker prefix for circular dependency detection
    static constexpr const char* LOADING_MARKER_PREFIX = "__LOADING__";
};

/**
 * @brief Initialize package library (convenience function)
 * @param state Lua state to initialize
 * @throws std::invalid_argument if state is null
 */
void initializePackageLib(State* state);

} // namespace Lua
