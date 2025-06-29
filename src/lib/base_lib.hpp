#pragma once

#include "lib_framework.hpp"
#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"

namespace Lua {

    /**
     * Base library implementation using the new framework
     * Provides essential Lua functions like print, type, etc.
     */
    class BaseLib : public LibModule {
    public:
        /**
         * Get module name
         */
        StrView getName() const noexcept override;

        /**
         * Get module version
         */
        StrView getVersion() const noexcept override;

        /**
         * Register functions to registry
         */
        void registerFunctions(FunctionRegistry& registry, const LibraryContext& context) override;

        /**
         * Initialize module
         */
        void initialize(State* state, const LibraryContext& context) override;

        /**
         * Cleanup module
         */
        void cleanup(State* state, const LibraryContext& context) override;

    public:
        // Base library function implementations
        LUA_FUNCTION(print);
        LUA_FUNCTION(tonumber);
        LUA_FUNCTION(tostring);
        LUA_FUNCTION(type);
        LUA_FUNCTION(ipairs);
        LUA_FUNCTION(pairs);
        LUA_FUNCTION(next);
        LUA_FUNCTION(getmetatable);
        LUA_FUNCTION(setmetatable);
        LUA_FUNCTION(rawget);
        LUA_FUNCTION(rawset);
        LUA_FUNCTION(rawlen);
        LUA_FUNCTION(rawequal);
        LUA_FUNCTION(pcall);
        LUA_FUNCTION(xpcall);
        LUA_FUNCTION(error);
        LUA_FUNCTION(assert_func);
        LUA_FUNCTION(select);
        LUA_FUNCTION(unpack);
        LUA_FUNCTION(load);
        LUA_FUNCTION(loadstring);
        LUA_FUNCTION(dofile);
        LUA_FUNCTION(loadfile);

    private:
        // Helper functions
        static Str valueToString(const Value& value);
        static bool isValidNumber(StrView str);
        static f64 stringToNumber(StrView str);
    };

    /**
     * Minimal base library with only essential functions
     */
    class MinimalBaseLib : public LibModule {
    public:
        StrView getName() const noexcept override;
        StrView getVersion() const noexcept override;
        void registerFunctions(FunctionRegistry& registry, const LibraryContext& context) override;
        void initialize(State* state, const LibraryContext& context) override;

    public:
        LUA_FUNCTION(print);
        LUA_FUNCTION(type);
        LUA_FUNCTION(tostring);
        LUA_FUNCTION(error);
    };

    /**
     * Extended base library with additional utility functions
     */
    class ExtendedBaseLib : public BaseLib {
    public:
        StrView getName() const noexcept override;
        void registerFunctions(FunctionRegistry& registry, const LibraryContext& context) override;

    public:
        // Additional utility functions
        LUA_FUNCTION(typeof);        // Enhanced type checking
        LUA_FUNCTION(instanceof);    // Instance checking
        LUA_FUNCTION(deepcopy);      // Deep copy tables
        LUA_FUNCTION(merge);         // Merge tables
        LUA_FUNCTION(keys);          // Get table keys
        LUA_FUNCTION(values);        // Get table values
        LUA_FUNCTION(len);           // Enhanced length function
        LUA_FUNCTION(empty);         // Check if table/string is empty
        LUA_FUNCTION(defaultValue);  // Provide default value
    };

    /**
     * Debug-enabled base library with additional debugging functions
     */
    class DebugBaseLib : public BaseLib {
    public:
        StrView getName() const noexcept override;
        void registerFunctions(FunctionRegistry& registry, const LibraryContext& context) override;
        void initialize(State* state, const LibraryContext& context) override;

    public:
        // Debug functions
        LUA_FUNCTION(trace);         // Print stack trace
        LUA_FUNCTION(inspect);       // Detailed value inspection
        LUA_FUNCTION(dump);          // Dump object structure
        LUA_FUNCTION(benchmark);     // Simple benchmarking
        LUA_FUNCTION(meminfo);       // Memory information

    private:
        bool debugEnabled_ = false;
    };

    /**
     * Factory functions for different base library configurations
     */
    namespace BaseLibFactory {
        /**
         * Create standard base library
         */
        std::unique_ptr<LibModule> createStandard();

        /**
         * Create minimal base library
         */
        std::unique_ptr<LibModule> createMinimal();

        /**
         * Create extended base library
         */
        std::unique_ptr<LibModule> createExtended();

        /**
         * Create debug-enabled base library
         */
        std::unique_ptr<LibModule> createDebug();

        /**
         * Create base library based on configuration
         */
        std::unique_ptr<LibModule> createFromConfig(const LibraryContext& context);
    }

    /**
     * Utility functions for base library operations
     */
    namespace BaseLibUtils {
        /**
         * Convert value to string representation
         */
        Str toString(const Value& value);

        /**
         * Convert string to number if possible
         */
        std::optional<f64> toNumber(StrView str);

        /**
         * Get type name of value
         */
        StrView getTypeName(const Value& value);

        /**
         * Check if two values are equal (including tables)
         */
        bool deepEqual(const Value& a, const Value& b);

        /**
         * Get length of value (string, table, etc.)
         */
        i32 getLength(const Value& value);

        /**
         * Check if value is "truthy" in Lua sense
         */
        bool isTruthy(const Value& value);
    }

    /**
     * Convenience function to register base library to a state
     * This is a simple wrapper around the new framework for backward compatibility
     */
    void registerBaseLib(State* state);

} // namespace Lua
