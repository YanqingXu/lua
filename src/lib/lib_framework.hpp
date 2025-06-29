#pragma once

#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include <functional>
#include <unordered_map>
#include <memory>
#include <string_view>
#include <vector>
#include <optional>
#include <any>

namespace Lua {

    /**
     * Modern library function signature compatible with existing codebase
     * Returns Value instead of int for better integration
     */
    using LibFunction = std::function<Value(State*, i32)>;

    /**
     * Function metadata for better introspection and debugging
     */
    struct FunctionMetadata {
        Str name;
        Str description;
        i32 minArgs = 0;
        i32 maxArgs = -1; // -1 means unlimited
        std::vector<Str> argTypes;
        std::vector<Str> returnTypes;
        bool isVariadic = false;
        
        FunctionMetadata() = default;
        FunctionMetadata(StrView n) : name(n) {}
        
        FunctionMetadata& withDescription(StrView desc) {
            description = Str(desc);
            return *this;
        }
        
        FunctionMetadata& withArgs(i32 min, i32 max = -1) {
            minArgs = min;
            maxArgs = max;
            return *this;
        }
        
        FunctionMetadata& withVariadic() {
            isVariadic = true;
            return *this;
        }
    };

    /**
     * Enhanced function registry with metadata support
     */
    class FunctionRegistry {
    public:
        /**
         * Register function with metadata
         */
        void registerFunction(const FunctionMetadata& meta, LibFunction func);
        
        /**
         * Register simple function (convenience method)
         */
        void registerFunction(StrView name, LibFunction func);
        
        /**
         * Call registered function
         */
        Value callFunction(StrView name, State* state, i32 nargs) const;
        
        /**
         * Check if function exists
         */
        bool hasFunction(StrView name) const noexcept;
        
        /**
         * Get function metadata
         */
        const FunctionMetadata* getFunctionMetadata(StrView name) const;
        
        /**
         * Get all registered function names
         */
        std::vector<Str> getFunctionNames() const;
        
        /**
         * Clear all functions
         */
        void clear();
        
        /**
         * Get function count
         */
        size_t size() const noexcept;

    private:
        std::unordered_map<Str, LibFunction> functions_;
        std::unordered_map<Str, FunctionMetadata> metadata_;
    };

    /**
     * Library context for dependency injection and configuration
     */
    class LibraryContext {
    public:
        LibraryContext() = default;
        
        /**
         * Set configuration value
         */
        template<typename T>
        void setConfig(StrView key, T&& value) {
            config_[Str(key)] = std::forward<T>(value);
        }
        
        /**
         * Get configuration value
         */
        template<typename T>
        std::optional<T> getConfig(StrView key) const {
            auto it = config_.find(Str(key));
            if (it != config_.end()) {
                try {
                    return std::any_cast<T>(it->second);
                } catch (const std::bad_any_cast&) {
                    return std::nullopt;
                }
            }
            return std::nullopt;
        }
        
        /**
         * Add dependency
         */
        template<typename T>
        void addDependency(std::shared_ptr<T> dependency) {
            dependencies_[typeid(T).name()] = dependency;
        }
        
        /**
         * Get dependency
         */
        template<typename T>
        std::shared_ptr<T> getDependency() const {
            auto it = dependencies_.find(typeid(T).name());
            if (it != dependencies_.end()) {
                return std::static_pointer_cast<T>(it->second);
            }
            return nullptr;
        }

    private:
        std::unordered_map<Str, std::any> config_;
        std::unordered_map<Str, std::shared_ptr<void>> dependencies_;
    };

    /**
     * Modern library module interface compatible with existing codebase
     */
    class LibModule {
    public:
        virtual ~LibModule() = default;
        
        /**
         * Get module name
         */
        virtual StrView getName() const noexcept = 0;
        
        /**
         * Get module version
         */
        virtual StrView getVersion() const noexcept { return "1.0"; }
        
        /**
         * Register module functions
         */
        virtual void registerFunctions(FunctionRegistry& registry, const LibraryContext& context) = 0;
        
        /**
         * Initialize module (called after registration)
         */
        virtual void initialize(State* state, const LibraryContext& context) {}
        
        /**
         * Cleanup module resources
         */
        virtual void cleanup(State* state, const LibraryContext& context) {}
        
        /**
         * Check if module has dependencies
         */
        virtual std::vector<StrView> getDependencies() const { return {}; }
        
        /**
         * Module configuration
         */
        virtual void configure(LibraryContext& context) {}
    };

    /**
     * Utility functions for argument checking (compatible with existing patterns)
     */
    namespace ArgUtils {
        /**
         * Check argument count
         */
        void checkArgCount(State* state, i32 expected, StrView funcName);
        void checkArgCount(State* state, i32 min, i32 max, StrView funcName);
        
        /**
         * Check argument types
         */
        Value checkNumber(State* state, i32 index, StrView funcName);
        Value checkString(State* state, i32 index, StrView funcName);
        Value checkTable(State* state, i32 index, StrView funcName);
        Value checkFunction(State* state, i32 index, StrView funcName);
        
        /**
         * Optional argument checking
         */
        std::optional<Value> optNumber(State* state, i32 index, f64 defaultValue = 0.0);
        std::optional<Value> optString(State* state, i32 index, StrView defaultValue = "");
        
        /**
         * Type name utilities
         */
        StrView getTypeName(const Value& value);
        void typeError(State* state, i32 index, StrView expected, StrView funcName);
    }

    /**
     * Error handling utilities
     */
    namespace ErrorUtils {
        /**
         * Throw Lua error with formatted message
         */
        [[noreturn]] void error(State* state, StrView message);
        
        /**
         * Argument error
         */
        [[noreturn]] void argError(State* state, i32 index, StrView message);
        
        /**
         * Type error
         */
        [[noreturn]] void typeError(State* state, i32 index, StrView expected);
        
        /**
         * Protected call wrapper
         */
        template<typename F>
        Value protectedCall(State* state, F&& func) {
            try {
                return func();
            } catch (const std::exception& e) {
                // Return nil value for error (since createError may not exist)
                return Value();
            }
        }
    }

    /**
     * Convenient macros for function registration
     */
    #define LUA_FUNCTION(name) \
        static Value name(::Lua::State* state, ::Lua::i32 nargs)

    #define LUA_REGISTER_FUNCTION(registry, name, func) \
        (registry).registerFunction(#name, [](::Lua::State* s, ::Lua::i32 n) -> ::Lua::Value { return func(s, n); })

    #define LUA_REGISTER_FUNCTION_WITH_META(registry, meta, func) \
        (registry).registerFunction(meta, [](::Lua::State* s, ::Lua::i32 n) -> ::Lua::Value { return func(s, n); })

    #define REGISTER_NAMESPACED_FUNCTION(registry, namespace_name, func_name, func) \
        (registry).registerFunction(namespace_name "." func_name, [](::Lua::State* s, ::Lua::i32 n) -> ::Lua::Value { return func(s, n); })

    /**
     * Module registration helper
     */
    template<typename ModuleType>
    std::unique_ptr<LibModule> createModule() {
        static_assert(std::is_base_of_v<LibModule, ModuleType>, 
                     "ModuleType must inherit from LibModule");
        return std::make_unique<ModuleType>();
    }

} // namespace Lua
