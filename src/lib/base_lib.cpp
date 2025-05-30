#include "base_lib.hpp"
#include "../vm/function.hpp"
#include <iostream>

namespace Lua {
    void registerBaseLib(State* state) {
        // Register base library functions
        state->setGlobal("print", Function::createNative(print));
        state->setGlobal("tonumber", Function::createNative(tonumber));
        state->setGlobal("tostring", Function::createNative(tostring));
        state->setGlobal("type", Function::createNative(type));
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
}
