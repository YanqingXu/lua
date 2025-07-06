#pragma once

#include "../../common/types.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"

namespace Lua {
    namespace Lib {

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
         * Library module interface forward declaration
         */
        class LibModule;

        /**
         * Library context forward declaration
         */
        class LibContext;

        /**
         * Library function registry forward declaration
         */
        class LibFuncRegistry;

        /**
         * Convenient macros for function registration
         */
        #define LUA_FUNCTION(name) \
            static ::Lua::Value name(::Lua::State* state, ::Lua::i32 nargs)

        #define LUA_REGISTER_FUNCTION(registry, name, func) \
            (registry).registerFunction(#name, [](::Lua::State* s, ::Lua::i32 n) -> ::Lua::Value { return func(s, n); })

        #define LUA_REGISTER_FUNCTION_WITH_META(registry, meta, func) \
            (registry).registerFunction(meta, [](::Lua::State* s, ::Lua::i32 n) -> ::Lua::Value { return func(s, n); })

        #define REGISTER_NAMESPACED_FUNCTION(registry, namespace_name, func_name, func) \
            (registry).registerFunction(namespace_name "." func_name, [](::Lua::State* s, ::Lua::i32 n) -> ::Lua::Value { return func(s, n); })

        /**
         * Common library constants
         */
        namespace Constants {
            constexpr i32 DEFAULT_STACK_SIZE = 256;
            constexpr i32 MAX_FUNCTION_ARGS = 255;
            constexpr StrView DEFAULT_MODULE_VERSION = "1.0";
            constexpr StrView LIB_NAMESPACE_SEPARATOR = ".";
        }

        /**
         * Library error codes
         */
        enum class ErrorCode {
            SUCCESS = 0,
            FUNCTION_NOT_FOUND,
            INVALID_ARGUMENT_COUNT,
            INVALID_ARGUMENT_TYPE,
            MODULE_NOT_FOUND,
            DEPENDENCY_MISSING,
            INITIALIZATION_FAILED,
            RUNTIME_ERROR
        };

        /**
         * Library configuration flags
         */
        enum class ConfigFlag {
            STRICT_TYPE_CHECKING = 1 << 0,
            ENABLE_DEBUG_INFO = 1 << 1,
            ENABLE_PROFILING = 1 << 2,
            ENABLE_MEMORY_TRACKING = 1 << 3,
            ENABLE_SECURITY_CHECKS = 1 << 4
        };

        /**
         * Utility type traits for library functions
         */
        template<typename T>
        struct is_lib_function : std::false_type {};
        
        template<>
        struct is_lib_function<LibFunction> : std::true_type {};
        
        template<typename T>
        constexpr bool is_lib_function_v = is_lib_function<T>::value;

    } // namespace Lib
} // namespace Lua