#include "base_lib.hpp"
#include "../vm/state.hpp"
#include "../vm/function.hpp"
#include "../vm/table.hpp"
#include <iostream>

namespace Lua {
    void registerBaseLib(State* state) {
        // Register base library functions
        state->setGlobal("print", Function::createNative(print));
        state->setGlobal("tonumber", Function::createNative(tonumber));
        state->setGlobal("tostring", Function::createNative(tostring));
        state->setGlobal("type", Function::createNative(type));
        state->setGlobal("ipairs", Function::createNative(ipairs));
        state->setGlobal("pairs", Function::createNative(pairs));
    }
    
    Value print(State* state, int nargs) {
        int stackTop = state->getTop();

        for (int i = 1; i <= nargs; i++) {
            if (i > 1) std::cout << "\t";

            int index = stackTop - nargs + i;
            std::cout << state->toString(index);
        }
        std::cout << std::endl;
        return Value(nullptr); // Return nil
    }
    
    Value tonumber(State* state, int nargs) {
        if (nargs < 1) return Value(nullptr);
        
        Value val = state->get(1);
        if (val.isNumber()) {
            return val;
        } else if (val.isString()) {
            try {
                double num = std::stod(val.asString());
                return Value(num);
            } catch (...) {
                // Conversion failed
            }
        }
        
        return Value(nullptr); // Cannot convert, return nil
    }
    
    Value tostring(State* state, int nargs) {
        if (nargs < 1) return Value("");
        
        return Value(state->toString(1));
    }
    
    Value type(State* state, int nargs) {
        if (nargs < 1) return Value("no value");
        
        Value val = state->get(1);
        switch (val.type()) {
            case ValueType::Nil: return Value("nil");
            case ValueType::Boolean: return Value("boolean");
            case ValueType::Number: return Value("number");
            case ValueType::String: return Value("string");
            case ValueType::Table: return Value("table");
            case ValueType::Function: return Value("function");
            default: return Value("unknown");
        }
    }
    
    Value ipairs(State* state, int nargs) {
        if (nargs < 1) {
            // Return nil if no table provided
            state->push(Value(nullptr));
            return Value(nullptr);
        }
        
        Value table = state->get(1);
        if (!table.isTable()) {
            // Return nil if not a table
            state->push(Value(nullptr));
            return Value(nullptr);
        }
        
        // Return iterator function, table, and initial index (0)
        state->push(Function::createNative([](State* state, int nargs) -> Value {
            if (nargs < 2) return Value(nullptr);
            
            Value table = state->get(1);
            Value index = state->get(2);
            
            if (!table.isTable() || !index.isNumber()) {
                return Value(nullptr);
            }
            
            LuaNumber i = index.asNumber() + 1;
            Value key = Value(i);
            Value value = table.asTable()->get(key);
            
            if (value.isNil()) {
                return Value(nullptr); // End of iteration
            }
            
            // Return key, value
            state->push(key);
            state->push(value);
            return Value(2); // Return 2 values
        }));
        state->push(table);
        state->push(Value(0.0));
        
        return Value(3); // Return 3 values: iterator, table, initial index
    }
    
    Value pairs(State* state, int nargs) {
        if (nargs < 1) {
            // Return nil if no table provided
            state->push(Value(nullptr));
            return Value(nullptr);
        }
        
        Value table = state->get(1);
        if (!table.isTable()) {
            // Return nil if not a table
            state->push(Value(nullptr));
            return Value(nullptr);
        }
        
        // Return iterator function, table, and nil as initial key
        state->push(Function::createNative([](State* state, int nargs) -> Value {
            if (nargs < 2) return Value(nullptr);
            
            Value table = state->get(1);
            Value key = state->get(2);
            
            if (!table.isTable()) {
                return Value(nullptr);
            }
            
            // This is a simplified implementation
            // In a real implementation, we would need to iterate through
            // both the array part and the hash part of the table
            // For now, we'll just iterate through array indices
            
            LuaNumber nextIndex;
            if (key.isNil()) {
                nextIndex = 1; // Start from index 1
            } else if (key.isNumber()) {
                nextIndex = key.asNumber() + 1;
            } else {
                return Value(nullptr); // End iteration for non-numeric keys
            }
            
            Value nextKey = Value(nextIndex);
            Value value = table.asTable()->get(nextKey);
            
            if (value.isNil()) {
                return Value(nullptr); // End of iteration
            }
            
            // Return key, value
            state->push(nextKey);
            state->push(value);
            return Value(2); // Return 2 values
        }));
        state->push(table);
        state->push(Value(nullptr)); // nil as initial key
        
        return Value(3); // Return 3 values: iterator, table, initial key
    }
}
