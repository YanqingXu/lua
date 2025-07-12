#include "metamethod_manager.hpp"
#include "core_metamethods.hpp"
#include "state.hpp"
#include "userdata.hpp"
#include "common/types.hpp"
#include <stdexcept>

namespace Lua {
    
    // Metamethod name strings (must match Lua 5.1 specification exactly)
    const Str MetaMethodManager::metaMethodNames_[static_cast<usize>(MetaMethod::Count)] = {
        // Core metamethods
        "__index",
        "__newindex", 
        "__call",
        "__tostring",
        
        // Arithmetic metamethods
        "__add",
        "__sub",
        "__mul",
        "__div",
        "__mod",
        "__pow",
        "__unm",
        "__concat",
        
        // Comparison metamethods
        "__eq",
        "__lt",
        "__le",
        
        // Special metamethods
        "__gc",
        "__mode",
        "__metatable"
    };
    
    // === Metamethod Lookup Implementation ===
    
    Value MetaMethodManager::getMetaMethod(const Value& obj, MetaMethod method) {
        if (!isValidMetaMethod(method)) {
            return Value(); // Return nil for invalid metamethod
        }
        
        GCRef<Table> metatable = getMetatable(obj);
        if (!metatable) {
            return Value(); // No metatable, return nil
        }
        
        return getMetaMethod(metatable, method);
    }
    
    Value MetaMethodManager::getMetaMethod(GCRef<Table> metatable, MetaMethod method) {
        if (!metatable || !isValidMetaMethod(method)) {
            return Value(); // Return nil for invalid input
        }
        
        // Look up the metamethod in the metatable
        const Str& methodName = getMetaMethodName(method);
        Value methodValue = metatable->get(Value(methodName));
        
        return methodValue;
    }
    
    // === Metamethod Invocation Implementation ===
    
    Value MetaMethodManager::callMetaMethod(State* state, MetaMethod method, 
                                           const Value& obj, const Vec<Value>& args) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }
        
        Value handler = getMetaMethod(obj, method);
        if (handler.isNil()) {
            throw LuaException("No metamethod found: " + getMetaMethodName(method));
        }
        
        // Prepare arguments: object + additional args
        Vec<Value> callArgs;
        callArgs.reserve(args.size() + 1);
        callArgs.push_back(obj);
        for (const auto& arg : args) {
            callArgs.push_back(arg);
        }
        
        // Use CoreMetaMethods to handle the actual call
        return CoreMetaMethods::handleMetaMethodCall(state, handler, callArgs);
    }
    
    Value MetaMethodManager::callBinaryMetaMethod(State* state, MetaMethod method,
                                                 const Value& lhs, const Value& rhs) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }
        
        // Try left operand's metamethod first
        Value handler = getMetaMethod(lhs, method);
        if (!handler.isNil()) {
            Vec<Value> args = {rhs};
            return callMetaMethod(state, method, lhs, args);
        }
        
        // If left operand doesn't have the metamethod and operands are different types,
        // try right operand's metamethod
        if (lhs.type() != rhs.type()) {
            handler = getMetaMethod(rhs, method);
            if (!handler.isNil()) {
                Vec<Value> args = {lhs};
                return callMetaMethod(state, method, rhs, args);
            }
        }
        
        throw LuaException("No metamethod found for binary operation: " + getMetaMethodName(method));
    }
    
    Value MetaMethodManager::callUnaryMetaMethod(State* state, MetaMethod method,
                                                const Value& operand) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }
        
        Value handler = getMetaMethod(operand, method);
        if (handler.isNil()) {
            throw LuaException("No metamethod found for unary operation: " + getMetaMethodName(method));
        }
        
        Vec<Value> args; // No additional arguments for unary operations
        return callMetaMethod(state, method, operand, args);
    }
    
    // === Utility Functions Implementation ===
    
    MetaMethod MetaMethodManager::getMetaMethodFromName(const Str& name) {
        for (usize i = 0; i < static_cast<usize>(MetaMethod::Count); ++i) {
            if (metaMethodNames_[i] == name) {
                return static_cast<MetaMethod>(i);
            }
        }
        
        // Return an invalid value if not found
        return MetaMethod::Count;
    }
    
    const Str& MetaMethodManager::getMetaMethodName(MetaMethod method) {
        if (!isValidMetaMethod(method)) {
            static const Str invalidName = "";
            return invalidName;
        }
        
        return metaMethodNames_[static_cast<usize>(method)];
    }
    
    bool MetaMethodManager::hasMetaMethod(const Value& obj, MetaMethod method) {
        Value handler = getMetaMethod(obj, method);
        return !handler.isNil();
    }
    
    bool MetaMethodManager::isCallable(const Value& value) {
        // Functions are always callable
        if (value.isFunction()) {
            return true;
        }
        
        // Check for __call metamethod
        return hasMetaMethod(value, MetaMethod::Call);
    }
    
    // === Internal Helper Functions Implementation ===
    
    GCRef<Table> MetaMethodManager::getMetatable(const Value& value) {
        switch (value.type()) {
            case ValueType::Table: {
                auto table = value.asTable();
                return table->getMetatable();
            }
            
            case ValueType::Userdata: {
                auto userdata = value.asUserdata();
                return userdata->getMetatable();
            }
            
            // Other types don't have metatables in this implementation
            case ValueType::Nil:
            case ValueType::Boolean:
            case ValueType::Number:
            case ValueType::String:
            case ValueType::Function:
            default:
                return GCRef<Table>(nullptr);
        }
    }
    
    bool MetaMethodManager::isValidMetaMethod(MetaMethod method) {
        return method < MetaMethod::Count;
    }
    
} // namespace Lua
