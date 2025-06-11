#pragma once

#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include "../vm/function.hpp"
#include "../vm/table.hpp"
#include <string>
#include <vector>
#include <functional>

namespace Lua {
    // Forward declarations
    class State;
    class Value;
    class Function;
    
    // Library function signature
    using LibFunction = std::function<Value(State*, int)>;
    
    // Library module interface
    class LibModule {
    public:
        virtual ~LibModule() = default;
        
        // Get module name
        virtual const Str& getName() const = 0;
        
        // Register module functions to state
        virtual void registerModule(State* state) = 0;
        
        // Get module version
        virtual const Str& getVersion() const {
            static const Str version = "1.0.0";
            return version;
        }
        
        // Check if module is loaded
        virtual bool isLoaded() const { return loaded_; }
        
    protected:
        // Helper function to register a function
        void registerFunction(State* state, const Str& name, LibFunction func) {
            state->setGlobal(name, Function::createNative([func](State* s, int n) -> Value {
                return func(s, n);
            }));
        }
        
        // Helper function to register a function in a table
        void registerFunction(State* state, Value table, const Str& name, LibFunction func) {
            if (table.isTable()) {
                NativeFn nativeFn = [func](State* s, int n) -> Value {
                    return func(s, n);
                };
                table.asTable()->set(Value(name), Function::createNative(nativeFn));
            }
        }
        
        // Mark module as loaded
        void setLoaded(bool loaded) { loaded_ = loaded; }
        
    private:
        bool loaded_ = false;
    };
    
    // Library registration info
    struct LibInfo {
        Str name;
        Str version;
        std::function<std::unique_ptr<LibModule>()> factory;
    };
    
    // Common error messages
    namespace LibErrors {
        constexpr const char* INVALID_ARGUMENT = "invalid argument";
        constexpr const char* WRONG_TYPE = "wrong argument type";
        constexpr const char* TOO_FEW_ARGS = "too few arguments";
        constexpr const char* TOO_MANY_ARGS = "too many arguments";
        constexpr const char* OUT_OF_RANGE = "argument out of range";
        constexpr const char* INVALID_OPERATION = "invalid operation";
    }
    
    // Common macros for argument checking
    #define LUA_CHECK_ARGS(state, expected) \
        if (nargs < expected) { \
            state->error(LibErrors::TOO_FEW_ARGS); \
            return Value(nullptr); \
        }
    
    #define LUA_CHECK_TYPE(state, index, type_check, type_name) \
        if (!state->get(index).type_check()) { \
            state->error(Str(LibErrors::WRONG_TYPE) + ": expected " + type_name); \
            return Value(nullptr); \
        }
    
    #define LUA_CHECK_NUMBER(state, index) \
        LUA_CHECK_TYPE(state, index, isNumber, "number")
    
    #define LUA_CHECK_STRING(state, index) \
        LUA_CHECK_TYPE(state, index, isString, "string")
    
    #define LUA_CHECK_TABLE(state, index) \
        LUA_CHECK_TYPE(state, index, isTable, "table")
    
    #define LUA_CHECK_FUNCTION(state, index) \
        LUA_CHECK_TYPE(state, index, isFunction, "function")
}