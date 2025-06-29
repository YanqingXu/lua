#include "base_lib.hpp"
#include "lib_manager.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include "../vm/table.hpp"
#include "../vm/function.hpp"
#include "../gc/core/gc_ref.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>

namespace Lua {

    // BaseLib implementation
    StrView BaseLib::getName() const noexcept {
        return "base";
    }

    StrView BaseLib::getVersion() const noexcept {
        return "1.0.0";
    }

    void BaseLib::registerFunctions(FunctionRegistry& registry, const LibraryContext& context) {
        // Essential functions
        LUA_REGISTER_FUNCTION(registry, print, print);
        LUA_REGISTER_FUNCTION(registry, type, type);
        LUA_REGISTER_FUNCTION(registry, tostring, tostring);
        LUA_REGISTER_FUNCTION(registry, tonumber, tonumber);
        
        // Table functions
        LUA_REGISTER_FUNCTION(registry, pairs, pairs);
        LUA_REGISTER_FUNCTION(registry, ipairs, ipairs);
        LUA_REGISTER_FUNCTION(registry, next, next);
        
        // Metatable functions
        LUA_REGISTER_FUNCTION(registry, getmetatable, getmetatable);
        LUA_REGISTER_FUNCTION(registry, setmetatable, setmetatable);
        
        // Raw access functions
        LUA_REGISTER_FUNCTION(registry, rawget, rawget);
        LUA_REGISTER_FUNCTION(registry, rawset, rawset);
        LUA_REGISTER_FUNCTION(registry, rawlen, rawlen);
        LUA_REGISTER_FUNCTION(registry, rawequal, rawequal);
        
        // Error handling
        LUA_REGISTER_FUNCTION(registry, pcall, pcall);
        LUA_REGISTER_FUNCTION(registry, xpcall, xpcall);
        LUA_REGISTER_FUNCTION(registry, error, error);
        LUA_REGISTER_FUNCTION(registry, assert, assert_func);
        
        // Utility functions
        LUA_REGISTER_FUNCTION(registry, select, select);
        LUA_REGISTER_FUNCTION(registry, unpack, unpack);
        
        // Loading functions (if enabled in context)
        if (auto allowLoad = context.getConfig<bool>("allow_load"); allowLoad && *allowLoad) {
            LUA_REGISTER_FUNCTION(registry, load, load);
            LUA_REGISTER_FUNCTION(registry, loadstring, loadstring);
            LUA_REGISTER_FUNCTION(registry, dofile, dofile);
            LUA_REGISTER_FUNCTION(registry, loadfile, loadfile);
        }
    }

    void BaseLib::initialize(State* state, const LibraryContext& context) {
        // Set global variables
        // _VERSION is typically set by the Lua state itself
        
        // Set global functions that need special handling
        // These would typically be set directly in the global table
        
        // Initialize any module-specific state
        if (auto debug = context.getConfig<bool>("debug_mode"); debug && *debug) {
            std::cout << "[BaseLib] Initialized in debug mode" << std::endl;
        }
    }

    void BaseLib::cleanup(State* state, const LibraryContext& context) {
        // Cleanup any resources if needed
        if (auto debug = context.getConfig<bool>("debug_mode"); debug && *debug) {
            std::cout << "[BaseLib] Cleanup completed" << std::endl;
        }
    }

    // Function implementations
    Value BaseLib::print(State* state, i32 nargs) {
        // Arguments are pushed to stack top by VM, access from stack top backwards
        int stackTop = state->getTop();
        for (i32 i = 0; i < nargs; i++) {
            if (i > 0) std::cout << "\t";
            // Access arguments from stack: stackTop - nargs + i
            Value val = state->get(stackTop - nargs + i);
            std::cout << valueToString(val);
        }
        std::cout << std::endl;
        return Value(); // nil
    }

    Value BaseLib::type(State* state, i32 nargs) {
        if (nargs != 1) {
            ErrorUtils::error(state, "type: expected 1 argument, got " + std::to_string(nargs));
        }
        // Access first argument from stack top (same as print function)
        int stackTop = state->getTop();
        Value val = state->get(stackTop - nargs + 0);  // First argument
        return Value(Str(ArgUtils::getTypeName(val)));
    }

    Value BaseLib::tostring(State* state, i32 nargs) {
        if (nargs != 1) {
            ErrorUtils::error(state, "tostring: expected 1 argument, got " + std::to_string(nargs));
        }
        // Access first argument from stack top (same as print function)
        int stackTop = state->getTop();
        Value val = state->get(stackTop - nargs + 0);  // First argument
        return Value(valueToString(val));
    }

    Value BaseLib::tonumber(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, 2, "tonumber");
        Value val = state->get(1);
        
        if (val.isNumber()) {
            return val;
        }
        
        if (val.isString()) {
            Str str = val.asString();
            if (isValidNumber(str)) {
                return Value(stringToNumber(str));
            }
        }
        
        return Value(); // nil
    }

    Value BaseLib::pairs(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, "pairs");
        Value table = ArgUtils::checkTable(state, 1, "pairs");
        
        // Return iterator function, table, nil
        // This is a simplified implementation
        return Value(); // TODO: Implement proper iterator
    }

    Value BaseLib::ipairs(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, "ipairs");
        Value table = ArgUtils::checkTable(state, 1, "ipairs");
        
        // Return iterator function, table, 0
        // This is a simplified implementation
        return Value(); // TODO: Implement proper iterator
    }

    Value BaseLib::next(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, 2, "next");
        Value table = ArgUtils::checkTable(state, 1, "next");
        
        // TODO: Implement table iteration
        return Value(); // nil
    }

    Value BaseLib::getmetatable(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, "getmetatable");
        Value val = state->get(1);
        
        // TODO: Implement metatable access
        return Value(); // nil for now
    }

    Value BaseLib::setmetatable(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 2, "setmetatable");
        Value table = ArgUtils::checkTable(state, 1, "setmetatable");
        Value metatable = state->get(2);
        
        // TODO: Implement metatable setting
        return table;
    }

    Value BaseLib::rawget(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 2, "rawget");
        Value table = ArgUtils::checkTable(state, 1, "rawget");
        Value key = state->get(2);
        
        // TODO: Implement raw table access
        return Value(); // nil
    }

    Value BaseLib::rawset(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 3, "rawset");
        Value table = ArgUtils::checkTable(state, 1, "rawset");
        Value key = state->get(2);
        Value value = state->get(3);
        
        // TODO: Implement raw table setting
        return table;
    }

    Value BaseLib::rawlen(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, "rawlen");
        Value val = state->get(1);
        
        if (val.isString()) {
            return Value(static_cast<f64>(val.asString().length()));
        } else if (val.isTable()) {
            // TODO: Implement table length
            return Value(0.0);
        }
        
        ErrorUtils::typeError(state, 1, "string or table");
    }

    Value BaseLib::rawequal(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 2, "rawequal");
        Value a = state->get(1);
        Value b = state->get(2);
        
        // TODO: Implement raw equality check
        return Value(false);
    }

    Value BaseLib::pcall(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, -1, "pcall");
        Value func = ArgUtils::checkFunction(state, 1, "pcall");
        
        // TODO: Implement protected call
        return Value(true); // success
    }

    Value BaseLib::xpcall(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 2, -1, "xpcall");
        Value func = ArgUtils::checkFunction(state, 1, "xpcall");
        Value errHandler = ArgUtils::checkFunction(state, 2, "xpcall");
        
        // TODO: Implement extended protected call
        return Value(true); // success
    }

    Value BaseLib::error(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, 2, "error");
        Value message = state->get(1);
        
        Str errorMsg = valueToString(message);
        ErrorUtils::error(state, errorMsg);
    }

    Value BaseLib::assert_func(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, -1, "assert");
        Value condition = state->get(1);
        
        if (!BaseLibUtils::isTruthy(condition)) {
            Str message = "assertion failed!";
            if (nargs >= 2) {
                message = valueToString(state->get(2));
            }
            ErrorUtils::error(state, message);
        }
        
        return condition;
    }

    Value BaseLib::select(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, -1, "select");
        Value index = state->get(1);
        
        if (index.isString() && index.asString() == "#") {
            return Value(static_cast<f64>(nargs - 1));
        }
        
        if (!index.isNumber()) {
            ErrorUtils::typeError(state, 1, "number");
        }
        
        i32 idx = static_cast<i32>(index.asNumber());
        if (idx < 1 || idx >= nargs) {
            return Value(); // nil
        }
        
        return state->get(idx + 1);
    }

    Value BaseLib::unpack(State* state, i32 nargs) {
        ArgUtils::checkArgCount(state, 1, 3, "unpack");
        Value table = ArgUtils::checkTable(state, 1, "unpack");
        
        // TODO: Implement table unpacking
        return Value(); // nil
    }

    Value BaseLib::load(State* state, i32 nargs) {
        // TODO: Implement load function
        return Value(); // nil
    }

    Value BaseLib::loadstring(State* state, i32 nargs) {
        // TODO: Implement loadstring function
        return Value(); // nil
    }

    Value BaseLib::dofile(State* state, i32 nargs) {
        // TODO: Implement dofile function
        return Value(); // nil
    }

    Value BaseLib::loadfile(State* state, i32 nargs) {
        // TODO: Implement loadfile function
        return Value(); // nil
    }

    // Helper functions
    Str BaseLib::valueToString(const Value& value) {
        return BaseLibUtils::toString(value);
    }

    bool BaseLib::isValidNumber(StrView str) {
        return BaseLibUtils::toNumber(str).has_value();
    }

    f64 BaseLib::stringToNumber(StrView str) {
        auto result = BaseLibUtils::toNumber(str);
        return result ? *result : 0.0;
    }

    // MinimalBaseLib implementation
    StrView MinimalBaseLib::getName() const noexcept {
        return "minimal_base";
    }

    StrView MinimalBaseLib::getVersion() const noexcept {
        return "1.0.0";
    }

    void MinimalBaseLib::registerFunctions(FunctionRegistry& registry, const LibraryContext& context) {
        LUA_REGISTER_FUNCTION(registry, print, print);
        LUA_REGISTER_FUNCTION(registry, type, type);
        LUA_REGISTER_FUNCTION(registry, tostring, tostring);
        LUA_REGISTER_FUNCTION(registry, error, error);
    }

    void MinimalBaseLib::initialize(State* state, const LibraryContext& context) {
        if (auto debug = context.getConfig<bool>("debug_mode"); debug && *debug) {
            std::cout << "[MinimalBaseLib] Initialized in debug mode" << std::endl;
        }
    }

    Value MinimalBaseLib::print(State* state, i32 nargs) {
        return BaseLib::print(state, nargs);
    }

    Value MinimalBaseLib::type(State* state, i32 nargs) {
        return BaseLib::type(state, nargs);
    }

    Value MinimalBaseLib::tostring(State* state, i32 nargs) {
        return BaseLib::tostring(state, nargs);
    }

    Value MinimalBaseLib::error(State* state, i32 nargs) {
        return BaseLib::error(state, nargs);
    }

} // namespace Lua

// Factory implementations
namespace Lua::BaseLibFactory {
    std::unique_ptr<LibModule> createStandard() {
        return std::make_unique<BaseLib>();
    }

    std::unique_ptr<LibModule> createMinimal() {
        return std::make_unique<MinimalBaseLib>();
    }

    std::unique_ptr<LibModule> createExtended() {
        // For now, return standard base lib
        // TODO: Implement ExtendedBaseLib
        return std::make_unique<BaseLib>();
    }

    std::unique_ptr<LibModule> createDebug() {
        // For now, return standard base lib
        // TODO: Implement DebugBaseLib
        return std::make_unique<BaseLib>();
    }

    std::unique_ptr<LibModule> createFromConfig(const LibraryContext& context) {
        auto mode = context.getConfig<Str>("base_lib_mode");
        if (mode) {
            if (*mode == "minimal") return createMinimal();
            if (*mode == "extended") return createExtended();
            if (*mode == "debug") return createDebug();
        }
        return createStandard();
    }
} // namespace Lua::BaseLibFactory

// Base library native function implementations
namespace Lua::BaseLibImpl {

        Value luaPrint(State* state, int nargs) {
            // Arguments are pushed to stack top by VM, access from stack top backwards
            int stackTop = state->getTop();
            for (int i = 0; i < nargs; ++i) {
                if (i > 0) std::cout << "\t";

                // Access arguments from stack: stackTop - nargs + i
                Value val = state->get(stackTop - nargs + i);

                if (val.isNil()) {
                    std::cout << "nil";
                } else if (val.isBoolean()) {
                    std::cout << (val.asBoolean() ? "true" : "false");
                } else if (val.isNumber()) {
                    std::cout << val.asNumber();
                } else if (val.isString()) {
                    std::cout << val.asString();
                } else if (val.isTable()) {
                    std::cout << "table";
                } else if (val.isFunction()) {
                    std::cout << "function";
                } else {
                    std::cout << "unknown";
                }
            }
            std::cout << std::endl;
            return Value(); // return nil
        }

        Value luaType(State* state, int nargs) {
            if (nargs < 1) return Value("nil");
            // Access first argument from stack top (same as print function)
            int stackTop = state->getTop();
            Value val = state->get(stackTop - nargs + 0);
            return Value(Str(BaseLibUtils::getTypeName(val)));
        }

        Value luaTostring(State* state, int nargs) {
            if (nargs < 1) return Value("nil");
            // Access first argument from stack top (same as print function)
            int stackTop = state->getTop();
            Value val = state->get(stackTop - nargs + 0);
            return Value(BaseLibUtils::toString(val));
        }

        Value luaTonumber(State* state, int nargs) {
            if (nargs < 1) return Value(); // nil

            Value val = state->get(1);
            if (val.isNumber()) {
                return val;
            } else if (val.isString()) {
                auto result = BaseLibUtils::toNumber(val.asString());
                return result ? Value(*result) : Value();
            }
            return Value(); // nil
        }

        Value luaError(State* state, int nargs) {
            Str message = "error";
            if (nargs >= 1) {
                message = BaseLibUtils::toString(state->get(1));
            }
            throw std::runtime_error(message);
        }

        Value luaAssert(State* state, int nargs) {
            if (nargs < 1) {
                throw std::runtime_error("assertion failed!");
            }

            Value val = state->get(1);
            if (!BaseLibUtils::isTruthy(val)) {
                Str message = "assertion failed!";
                if (nargs >= 2) {
                    message = BaseLibUtils::toString(state->get(2));
                }
                throw std::runtime_error(message);
            }

            return val; // return the first argument
        }

    // Helper function to register a native function
    void registerNativeFunction(State* state, const char* name, NativeFn fn) {
        auto func = Function::createNative(fn);
        state->setGlobal(name, Value(func));
    }

} // namespace Lua::BaseLibImpl

// Main Lua namespace - registerBaseLib implementation
namespace Lua {

    // Clean and optimized registerBaseLib implementation
    void registerBaseLib(State* state) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }

        try {
            using namespace BaseLibImpl;

            // Register core base library functions
            registerNativeFunction(state, "print", luaPrint);
            registerNativeFunction(state, "type", luaType);
            registerNativeFunction(state, "tostring", luaTostring);
            registerNativeFunction(state, "tonumber", luaTonumber);
            registerNativeFunction(state, "error", luaError);
            registerNativeFunction(state, "assert", luaAssert);

            // Set global constants
            state->setGlobal("_VERSION", Value("Lua 5.1"));

        } catch (const std::exception& e) {
            std::cerr << "Error registering base library: " << e.what() << std::endl;
            throw;
        }
    }

} // namespace Lua
