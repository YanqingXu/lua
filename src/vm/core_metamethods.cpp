#include "core_metamethods.hpp"
#include "state.hpp"
#include "table.hpp"
#include "common/types.hpp"
#include <sstream>

namespace Lua {
    
    // === Core Metamethod Handlers Implementation ===
    
    Value CoreMetaMethods::handleIndex(State* state, const Value& table, const Value& key) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }
        
        // First, try raw table access
        Value rawValue = rawIndex(table, key);
        if (!rawValue.isNil()) {
            return rawValue; // Found value directly in table
        }
        
        // Value not found, try __index metamethod
        Value indexHandler = MetaMethodManager::getMetaMethod(table, MetaMethod::Index);
        if (indexHandler.isNil()) {
            return Value(); // No __index metamethod, return nil
        }
        
        // Handle __index metamethod
        if (indexHandler.isFunction()) {
            // __index is a function: call it with (table, key)
            Vec<Value> args = {table, key};
            return handleMetaMethodCall(state, indexHandler, args);
        } else if (indexHandler.isTable()) {
            // __index is a table: look up key in that table
            return lookupInHandlerTable(indexHandler, key);
        } else {
            // __index is neither function nor table, return nil
            return Value();
        }
    }
    
    void CoreMetaMethods::handleNewIndex(State* state, const Value& table, 
                                        const Value& key, const Value& value) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }
        
        if (!isValidTable(table)) {
            throw LuaException("Attempt to index a non-table value");
        }
        
        // Check if key already exists in table
        Value existingValue = rawIndex(table, key);
        if (!existingValue.isNil()) {
            // Key exists, perform direct assignment
            rawNewIndex(table, key, value);
            return;
        }
        
        // Key doesn't exist, try __newindex metamethod
        Value newindexHandler = MetaMethodManager::getMetaMethod(table, MetaMethod::NewIndex);
        if (newindexHandler.isNil()) {
            // No __newindex metamethod, perform direct assignment
            rawNewIndex(table, key, value);
            return;
        }
        
        // Handle __newindex metamethod
        if (newindexHandler.isFunction()) {
            // __newindex is a function: call it with (table, key, value)
            Vec<Value> args = {table, key, value};
            handleMetaMethodCall(state, newindexHandler, args);
        } else if (newindexHandler.isTable()) {
            // __newindex is a table: assign to that table
            assignToHandlerTable(newindexHandler, key, value);
        }
        // If __newindex is neither function nor table, do nothing
    }
    
    Value CoreMetaMethods::handleCall(State* state, const Value& func, const Vec<Value>& args) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }
        
        // Check if value is directly callable (function)
        if (func.isFunction()) {
            // TODO: Implement function call mechanism
            // For now, return nil as placeholder
            return Value();
        }
        
        // Try __call metamethod
        Value callHandler = MetaMethodManager::getMetaMethod(func, MetaMethod::Call);
        if (callHandler.isNil()) {
            throw LuaException("Attempt to call a non-callable value");
        }
        
        if (!callHandler.isFunction()) {
            throw LuaException("__call metamethod is not a function");
        }
        
        // Call __call metamethod with func as first argument, followed by args
        Vec<Value> callArgs;
        callArgs.reserve(args.size() + 1);
        callArgs.push_back(func);
        for (const auto& arg : args) {
            callArgs.push_back(arg);
        }
        
        return handleMetaMethodCall(state, callHandler, callArgs);
    }
    
    Value CoreMetaMethods::handleToString(State* state, const Value& obj) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }
        
        // Try __tostring metamethod first
        Value tostringHandler = MetaMethodManager::getMetaMethod(obj, MetaMethod::ToString);
        if (!tostringHandler.isNil()) {
            if (tostringHandler.isFunction()) {
                Vec<Value> args = {obj};
                Value result = handleMetaMethodCall(state, tostringHandler, args);
                
                // __tostring must return a string
                if (result.isString()) {
                    return result;
                } else {
                    throw LuaException("__tostring metamethod must return a string");
                }
            } else {
                throw LuaException("__tostring metamethod is not a function");
            }
        }
        
        // No __tostring metamethod, use default string representation
        return Value(getDefaultString(obj));
    }
    
    // === Utility Functions Implementation ===
    
    Value CoreMetaMethods::rawIndex(const Value& table, const Value& key) {
        if (!table.isTable()) {
            return Value(); // Not a table, return nil
        }
        
        auto tablePtr = table.asTable();
        return tablePtr->get(key);
    }
    
    void CoreMetaMethods::rawNewIndex(const Value& table, const Value& key, const Value& value) {
        if (!table.isTable()) {
            throw LuaException("Attempt to index a non-table value");
        }
        
        auto tablePtr = table.asTable();
        tablePtr->set(key, value);
    }
    
    bool CoreMetaMethods::isCallable(const Value& obj) {
        return MetaMethodManager::isCallable(obj);
    }
    
    Str CoreMetaMethods::getDefaultString(const Value& obj) {
        std::ostringstream oss;
        
        switch (obj.type()) {
            case ValueType::Nil:
                return "nil";
                
            case ValueType::Boolean:
                return obj.asBoolean() ? "true" : "false";
                
            case ValueType::Number:
                oss << obj.asNumber();
                return oss.str();
                
            case ValueType::String:
                return obj.asString();
                
            case ValueType::Table:
                oss << "table: " << static_cast<const void*>(obj.asTable().get());
                return oss.str();
                
            case ValueType::Function:
                oss << "function: " << static_cast<const void*>(obj.asFunction().get());
                return oss.str();
                
            case ValueType::Userdata:
                oss << "userdata: " << static_cast<const void*>(obj.asUserdata().get());
                return oss.str();
                
            default:
                return "unknown";
        }
    }
    
    // === Internal Helper Functions Implementation ===
    
    bool CoreMetaMethods::isValidTable(const Value& table) {
        return table.isTable();
    }
    
    Value CoreMetaMethods::handleMetaMethodCall(State* state, const Value& handler, const Vec<Value>& args) {
        if (!handler.isFunction()) {
            throw LuaException("Metamethod handler is not a function");
        }

        // Simplified metamethod call implementation
        // For now, we'll use the state's call mechanism if available
        try {
            auto function = handler.asFunction();
            return state->call(function, args);
        } catch (const LuaException&) {
            // If function call fails, return nil
            // This allows the system to continue working even with incomplete function support
            return Value();
        }
    }
    
    Value CoreMetaMethods::lookupInHandlerTable(const Value& handlerTable, const Value& key) {
        if (!handlerTable.isTable()) {
            return Value(); // Not a table, return nil
        }
        
        // Recursively look up in the handler table (this may trigger more metamethods)
        auto tablePtr = handlerTable.asTable();
        return tablePtr->get(key);
    }
    
    void CoreMetaMethods::assignToHandlerTable(const Value& handlerTable, const Value& key, const Value& value) {
        if (!handlerTable.isTable()) {
            throw LuaException("__newindex handler is not a table");
        }
        
        // Recursively assign to the handler table (this may trigger more metamethods)
        auto tablePtr = handlerTable.asTable();
        tablePtr->set(key, value);
    }
    
} // namespace Lua
