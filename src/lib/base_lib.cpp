#include "base_lib.hpp"
#include "lib_common.hpp"
#include "lib_utils.hpp"
#include "../vm/state.hpp"
#include "../vm/function.hpp"
#include "../vm/table.hpp"
#include <iostream>

namespace Lua {
    
    // BaseLib class implementation
    void BaseLib::registerModule(State* state) {
        // Register all base library functions
        registerFunction(state, "print", print);
        registerFunction(state, "tonumber", tonumber);
        registerFunction(state, "tostring", tostring);
        registerFunction(state, "type", type);
        registerFunction(state, "ipairs", ipairs);
        registerFunction(state, "pairs", pairs);
        registerFunction(state, "next", next);
        registerFunction(state, "getmetatable", getmetatable);
        registerFunction(state, "setmetatable", setmetatable);
        registerFunction(state, "rawget", rawget);
        registerFunction(state, "rawset", rawset);
        registerFunction(state, "rawlen", rawlen);
        registerFunction(state, "rawequal", rawequal);
        registerFunction(state, "pcall", pcall);
        registerFunction(state, "xpcall", xpcall);
        registerFunction(state, "error", error);
        registerFunction(state, "assert", lua_assert);
        registerFunction(state, "select", select);
        registerFunction(state, "unpack", unpack);
        
        // Mark as loaded
        setLoaded(true);
    }
    
    // Legacy function for backward compatibility
    void registerBaseLib(State* state) {
        BaseLib baseLib;
        baseLib.registerModule(state);
    }
    
    // Static member function implementations
    Value BaseLib::print(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        for (int i = 1; i <= nargs; i++) {
            if (i > 1) std::cout << "\t";
            
            Value val = state->get(i);
            std::cout << LibUtils::Convert::toString(val);
        }
        std::cout << std::endl;
        return Value(nullptr); // Return nil
    }
    
    Value BaseLib::tonumber(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(nullptr);
        }
        
        auto val = checker.getValue();
        if (!val) return Value(nullptr);
        
        if (val->isNumber()) {
            return *val;
        } else if (val->isString()) {
            try {
                double num = std::stod(val->asString());
                return Value(num);
            } catch (...) {
                // Conversion failed
            }
        }
        
        return Value(nullptr); // Cannot convert, return nil
    }
    
    Value BaseLib::tostring(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value("");
        }
        
        auto val = checker.getValue();
        if (!val) return Value("");
        
        return Value(LibUtils::Convert::toString(*val));
    }
    
    Value BaseLib::type(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value("no value");
        }
        
        auto val = checker.getValue();
        if (!val) return Value("no value");
        
        switch (val->type()) {
            case ValueType::Nil: return Value("nil");
            case ValueType::Boolean: return Value("boolean");
            case ValueType::Number: return Value("number");
            case ValueType::String: return Value("string");
            case ValueType::Table: return Value("table");
            case ValueType::Function: return Value("function");
            default: return Value("unknown");
        }
    }
    
    Value BaseLib::ipairs(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(nullptr);
        }
        
        auto val = checker.getValue();
        if (!val || !val->isTable()) {
            LibUtils::Error::throwError(state, "bad argument #1 to 'ipairs' (table expected)");
            return Value(nullptr);
        }
        
        // Return iterator function, table, and initial index
        // This is a simplified implementation
        // In a real implementation, you'd need to create proper iterator functions
        return Value(nullptr);
    }
    
    Value BaseLib::pairs(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(nullptr);
        }
        
        auto val = checker.getValue();
        if (!val || !val->isTable()) {
            LibUtils::Error::throwError(state, "bad argument #1 to 'pairs' (table expected)");
            return Value(nullptr);
        }
        
        // Return iterator function, table, and initial key
        // This is a simplified implementation
        // In a real implementation, you'd need to create proper iterator functions
        return Value(nullptr);
    }
    
    Value BaseLib::next(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(nullptr);
        }
        
        auto table = checker.getValue();
        if (!table || !table->isTable()) {
            LibUtils::Error::throwError(state, "bad argument #1 to 'next' (table expected)");
            return Value(nullptr);
        }
        
        // Get the key (can be nil for first iteration)
        Value key = (nargs >= 2) ? state->get(2) : Value(nullptr);
        
        // Find next key-value pair
        // This is a simplified implementation
        return Value(nullptr);
    }
    
    Value BaseLib::getmetatable(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(nullptr);
        }
        
        auto val = checker.getValue();
        if (!val) return Value(nullptr);
        
        // Get metatable for the value
        // This is a simplified implementation
        return Value(nullptr);
    }
    
    Value BaseLib::setmetatable(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(2)) {
            return Value(nullptr);
        }
        
        auto table = checker.getValue();
        if (!table || !table->isTable()) {
            LibUtils::Error::throwError(state, "bad argument #1 to 'setmetatable' (table expected)");
            return Value(nullptr);
        }
        
        Value metatable = state->get(2);
        if (!metatable.isNil() && !metatable.isTable()) {
            LibUtils::Error::throwError(state, "bad argument #2 to 'setmetatable' (nil or table expected)");
            return Value(nullptr);
        }
        
        // Set metatable for the table
        // This is a simplified implementation
        return *table;
    }
    
    Value BaseLib::rawget(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(2)) {
            return Value(nullptr);
        }
        
        auto table = checker.getValue();
        if (!table || !table->isTable()) {
            LibUtils::Error::throwError(state, "bad argument #1 to 'rawget' (table expected)");
            return Value(nullptr);
        }
        
        Value key = state->get(2);
        
        // Get value from table without invoking metamethods
        // This is a simplified implementation
        return Value(nullptr);
    }
    
    Value BaseLib::rawset(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(3)) {
            return Value(nullptr);
        }
        
        auto table = checker.getValue();
        if (!table || !table->isTable()) {
            LibUtils::Error::throwError(state, "bad argument #1 to 'rawset' (table expected)");
            return Value(nullptr);
        }
        
        Value key = state->get(2);
        Value value = state->get(3);
        
        // Set value in table without invoking metamethods
        // This is a simplified implementation
        return *table;
    }
    
    Value BaseLib::rawlen(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(nullptr);
        }
        
        auto val = checker.getValue();
        if (!val) return Value(nullptr);
        
        if (val->isString()) {
            return Value(static_cast<double>(val->asString().length()));
        } else if (val->isTable()) {
            // Get raw length of table without invoking metamethods
            // This is a simplified implementation
            return Value(0.0);
        } else {
            LibUtils::Error::throwError(state, "bad argument to 'rawlen' (string or table expected)");
            return Value(nullptr);
        }
    }
    
    Value BaseLib::rawequal(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(2)) {
            return Value(false);
        }
        
        auto val1 = checker.getValue();
        Value val2 = state->get(2);
        
        if (!val1) return Value(false);
        
        // Compare values without invoking metamethods
        return Value(LibUtils::Convert::rawEqual(*val1, val2));
    }
    
    Value BaseLib::pcall(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(false);
        }
        
        auto func = checker.getValue();
        if (!func || !func->isFunction()) {
            LibUtils::Error::throwError(state, "bad argument #1 to 'pcall' (function expected)");
            return Value(false);
        }
        
        // Protected call implementation
        // This is a simplified implementation
        return Value(true);
    }
    
    Value BaseLib::xpcall(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(2)) {
            return Value(false);
        }
        
        auto func = checker.getValue();
        Value errorHandler = state->get(2);
        
        if (!func || !func->isFunction()) {
            LibUtils::Error::throwError(state, "bad argument #1 to 'xpcall' (function expected)");
            return Value(false);
        }
        
        if (!errorHandler.isFunction()) {
            LibUtils::Error::throwError(state, "bad argument #2 to 'xpcall' (function expected)");
            return Value(false);
        }
        
        // Extended protected call implementation
        // This is a simplified implementation
        return Value(true);
    }
    
    Value BaseLib::error(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        std::string message = "";
        int level = 1;
        
        if (nargs >= 1) {
            auto val = checker.getValue();
            if (val) {
                message = LibUtils::Convert::toString(*val);
            }
        }
        
        if (nargs >= 2) {
            Value levelVal = state->get(2);
            if (levelVal.isNumber()) {
                level = static_cast<int>(levelVal.asNumber());
            }
        }
        
        // Throw error with message and level
        LibUtils::Error::throwError(state, message, level);
        return Value(nullptr);
    }
    
    Value BaseLib::lua_assert(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            LibUtils::Error::throwError(state, "assertion failed!");
            return Value(nullptr);
        }
        
        auto val = checker.getValue();
        if (!val || (val->isBoolean() && !val->asBoolean()) || val->isNil()) {
            std::string message = "assertion failed!";
            if (nargs >= 2) {
                Value msgVal = state->get(2);
                if (!msgVal.isNil()) {
                    message = LibUtils::Convert::toString(msgVal);
                }
            }
            LibUtils::Error::throwError(state, message);
            return Value(nullptr);
        }
        
        return *val;
    }
    
    Value BaseLib::select(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(nullptr);
        }
        
        auto selector = checker.getValue();
        if (!selector) return Value(nullptr);
        
        if (selector->isString() && selector->asString() == "#") {
            // Return number of arguments after the selector
            return Value(static_cast<double>(nargs - 1));
        } else if (selector->isNumber()) {
            int index = static_cast<int>(selector->asNumber());
            if (index < 0) {
                index = nargs + index;
            }
            
            if (index >= 1 && index <= nargs - 1) {
                return state->get(index + 1);
            }
        }
        
        return Value(nullptr);
    }
    
    Value BaseLib::unpack(State* state, int nargs) {
        LibUtils::ArgChecker checker(state, nargs);
        
        if (!checker.checkMinArgs(1)) {
            return Value(nullptr);
        }
        
        auto table = checker.getValue();
        if (!table || !table->isTable()) {
            LibUtils::Error::throwError(state, "bad argument #1 to 'unpack' (table expected)");
            return Value(nullptr);
        }
        
        int start = 1;
        int end = -1; // Will be determined from table length
        
        if (nargs >= 2) {
            Value startVal = state->get(2);
            if (startVal.isNumber()) {
                start = static_cast<int>(startVal.asNumber());
            }
        }
        
        if (nargs >= 3) {
            Value endVal = state->get(3);
            if (endVal.isNumber()) {
                end = static_cast<int>(endVal.asNumber());
            }
        }
        
        // Unpack table elements
        // This is a simplified implementation
        return Value(nullptr);
    }
}
