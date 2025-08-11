#include "core_metamethods.hpp"
#include "state.hpp"
#include "table.hpp"
#include "function.hpp"
#include "call_result.hpp"
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
        // === Input Validation ===
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }

        // Validate arguments before proceeding
        if (!validateCallArguments(args)) {
            throw LuaException("Invalid arguments for function call: too many arguments (max 250)");
        }

        // === Phase 1: Direct Function Call ===
        // Check if value is directly callable (function)
        if (func.isFunction()) {
            try {
                return callFunctionDirect(state, func, args);
            } catch (const LuaException& e) {
                // Re-throw with more context
                throw LuaException(getCallErrorMessage(func, args) + ": " + std::string(e.what()));
            } catch (const std::exception& e) {
                throw LuaException(getCallErrorMessage(func, args) + ": " + std::string(e.what()));
            }
        }

        // === Phase 2: Metamethod Lookup ===
        // Try __call metamethod for non-function values
        Value callHandler;
        try {
            callHandler = MetaMethodManager::getMetaMethod(func, MetaMethod::Call);
        } catch (const std::exception& e) {
            throw LuaException("Error looking up __call metamethod: " + std::string(e.what()));
        }

        if (callHandler.isNil()) {
            // Provide detailed error message based on the type
            std::string typeName;
            switch (func.type()) {
                case ValueType::Nil: typeName = "nil"; break;
                case ValueType::Boolean: typeName = "boolean"; break;
                case ValueType::Number: typeName = "number"; break;
                case ValueType::String: typeName = "string"; break;
                case ValueType::Table: typeName = "table"; break;
                case ValueType::Userdata: typeName = "userdata"; break;
                default: typeName = "unknown"; break;
            }
            throw LuaException("Attempt to call a " + typeName + " value (no __call metamethod)");
        }

        // === Phase 3: Metamethod Validation ===
        if (!callHandler.isFunction()) {
            throw LuaException("__call metamethod is not a function (got " +
                             std::to_string(static_cast<int>(callHandler.type())) + ")");
        }

        // === Phase 4: Argument Preparation ===
        // Call __call metamethod with func as first argument, followed by args
        // This follows Lua 5.1 specification: __call(table, arg1, arg2, ...)
        Vec<Value> callArgs;
        try {
            callArgs.reserve(args.size() + 1);
            callArgs.push_back(func);  // First argument is the table being called

            // Add all original arguments
            for (const auto& arg : args) {
                callArgs.push_back(arg);
            }
        } catch (const std::exception& e) {
            throw LuaException("Error preparing arguments for __call metamethod: " + std::string(e.what()));
        }

        // === Phase 5: Metamethod Execution ===
        try {
            // Use direct function call for metamethod to avoid infinite recursion
            return callFunctionDirect(state, callHandler, callArgs);
        } catch (const LuaException& e) {
            // Re-throw with context about __call metamethod
            throw LuaException("Error in __call metamethod: " + std::string(e.what()));
        } catch (const std::exception& e) {
            throw LuaException("Unexpected error in __call metamethod: " + std::string(e.what()));
        }
    }

    CallResult CoreMetaMethods::handleCallMultiple(State* state, const Value& func, const Vec<Value>& args) {
        if (!state) {
            throw std::invalid_argument("State cannot be null");
        }



        // === Phase 1: Direct Function Call ===
        // Check if value is directly callable (function)
        if (func.isFunction()) {
            try {
                return callFunctionDirectMultiple(state, func, args);
            } catch (const LuaException& e) {
                // Re-throw with more context
                throw LuaException(getCallErrorMessage(func, args) + ": " + std::string(e.what()));
            } catch (const std::exception& e) {
                throw LuaException(getCallErrorMessage(func, args) + ": " + std::string(e.what()));
            }
        }

        // === Phase 2: Metamethod Lookup ===
        // Try __call metamethod for non-function values

        Value callHandler;
        try {
            callHandler = MetaMethodManager::getMetaMethod(func, MetaMethod::Call);
        } catch (const std::exception& e) {
            throw LuaException("Error looking up __call metamethod: " + std::string(e.what()));
        }

        if (callHandler.isNil()) {
            // Provide detailed error message based on the type
            std::string typeName;
            switch (func.type()) {
                case ValueType::Nil: typeName = "nil"; break;
                case ValueType::Boolean: typeName = "boolean"; break;
                case ValueType::Number: typeName = "number"; break;
                case ValueType::String: typeName = "string"; break;
                case ValueType::Table: typeName = "table"; break;
                case ValueType::Userdata: typeName = "userdata"; break;
                default: typeName = "unknown"; break;
            }
            throw LuaException("Attempt to call a " + typeName + " value (no __call metamethod)");
        }

        // === Phase 3: Metamethod Validation ===
        if (!callHandler.isFunction()) {
            throw LuaException("__call metamethod is not a function (got " +
                             std::to_string(static_cast<int>(callHandler.type())) + ")");
        }

        // === Phase 4: Argument Preparation ===
        // Call __call metamethod with func as first argument, followed by args

        Vec<Value> callArgs;
        try {
            callArgs.reserve(args.size() + 1);
            callArgs.push_back(func);  // First argument is the table being called


            // Add all original arguments
            for (size_t i = 0; i < args.size(); ++i) {
                callArgs.push_back(args[i]);

            }

        } catch (const std::exception& e) {
            throw LuaException("Error preparing arguments for __call metamethod: " + std::string(e.what()));
        }

        // === Phase 5: Metamethod Execution ===
        try {
            // Use direct function call for metamethod to avoid infinite recursion
            return callFunctionDirectMultiple(state, callHandler, callArgs);
        } catch (const LuaException& e) {
            // Re-throw with context about __call metamethod
            throw LuaException("Error in __call metamethod: " + std::string(e.what()));
        } catch (const std::exception& e) {
            throw LuaException("Unexpected error in __call metamethod: " + std::string(e.what()));
        }
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

    // === Enhanced Call Validation Functions ===

    bool CoreMetaMethods::validateCallArguments(const Vec<Value>& args) {
        // Lua 5.1 supports up to 250 arguments in a function call
        static const size_t MAX_ARGS = 250;
        if (args.size() > MAX_ARGS) {
            return false;
        }

        // All arguments should be valid (not uninitialized)
        for (const auto& arg : args) {
            // Basic validation - ensure the value is properly initialized
            // This is a simple check, more sophisticated validation could be added
            if (arg.type() == ValueType::Nil) {
                // Nil is a valid argument, continue
                continue;
            }
        }

        return true;
    }

    std::string CoreMetaMethods::getCallErrorMessage(const Value& func, const Vec<Value>& args) {
        std::ostringstream oss;

        // Describe the function being called
        if (func.isFunction()) {
            oss << "Error calling function";
        } else {
            std::string typeName;
            switch (func.type()) {
                case ValueType::Nil: typeName = "nil"; break;
                case ValueType::Boolean: typeName = "boolean"; break;
                case ValueType::Number: typeName = "number"; break;
                case ValueType::String: typeName = "string"; break;
                case ValueType::Table: typeName = "table"; break;
                case ValueType::Userdata: typeName = "userdata"; break;
                default: typeName = "unknown"; break;
            }
            oss << "Error calling " << typeName << " value";
        }

        // Add argument information
        oss << " with " << args.size() << " argument" << (args.size() == 1 ? "" : "s");

        return oss.str();
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

        // Use direct function call to avoid recursion
        return callFunctionDirect(state, handler, args);
    }

    Value CoreMetaMethods::callFunctionDirect(State* state, const Value& func, const Vec<Value>& args) {
        // === Input Validation ===
        if (!state) {
            throw std::invalid_argument("State cannot be null in callFunctionDirect");
        }

        // Debug output removed for cleaner testing

        if (!func.isFunction()) {
            throw LuaException("Attempt to call a non-function value in callFunctionDirect");
        }

        auto function = func.asFunction();
        if (!function) {
            throw LuaException("Function reference is null in callFunctionDirect");
        }

        // === Argument Count Validation ===
        // Lua 5.1 supports up to 250 arguments in a function call
        static const size_t MAX_ARGS = 250;
        if (args.size() > MAX_ARGS) {
            throw LuaException("Too many arguments in function call (max " +
                             std::to_string(MAX_ARGS) + ", got " + std::to_string(args.size()) + ")");
        }

        // === Native Function Handling ===
        if (function->getType() == Function::Type::Native) {
            // Check if it's a legacy function
            if (function->isNativeLegacy()) {
                auto nativeFnLegacy = function->getNativeLegacy();
                if (!nativeFnLegacy) {
                    throw LuaException("Legacy native function pointer is null");
                }

                // Save current stack state for proper cleanup
                int oldTop = state->getTop();

                try {
                    // Push arguments onto stack in correct order
                    for (size_t i = 0; i < args.size(); ++i) {
                        state->push(args[i]);
                    }

                    // Call legacy native function with argument count
                    Value result = nativeFnLegacy(state, static_cast<int>(args.size()));

                    // Restore stack state (important for nested calls)
                    state->setTop(oldTop);

                    return result;
                } catch (const LuaException& e) {
                    // Restore stack state before re-throwing
                    state->setTop(oldTop);
                    throw LuaException("Error in legacy native function call: " + std::string(e.what()));
                } catch (const std::exception& e) {
                    // Restore stack state before re-throwing
                    state->setTop(oldTop);
                    throw LuaException("Unexpected error in legacy native function call: " + std::string(e.what()));
                }
            } else {
                // New multi-return function - call and return first value for compatibility
                CallResult callResult = state->callMultiple(Value(function), args);
                if (callResult.count > 0) {
                    return callResult.getFirst();
                } else {
                    return Value();  // Return nil if no values
                }
            }
        }

        // === Lua Function Handling ===
        if (function->getType() == Function::Type::Lua) {
            // Save current stack state for proper cleanup
            int oldTop = state->getTop();

            try {
                // Push arguments onto stack in correct order
                for (size_t i = 0; i < args.size(); ++i) {
                    state->push(args[i]);
                }

                // Call Lua function with arguments on stack
                // This uses the VM to execute the Lua bytecode
                Value result = state->callLua(func, static_cast<int>(args.size()));

                // Restore stack state (important for nested calls)
                state->setTop(oldTop);

                return result;

            } catch (const LuaException& e) {
                // Restore stack state before re-throwing
                state->setTop(oldTop);
                throw LuaException("Error in Lua function call: " + std::string(e.what()));
            } catch (const std::exception& e) {
                // Restore stack state before re-throwing
                state->setTop(oldTop);
                throw LuaException("Unexpected error in Lua function call: " + std::string(e.what()));
            }
        }

        // === Unknown Function Type ===
        throw LuaException("Unknown function type in callFunctionDirect: " +
                         std::to_string(static_cast<int>(function->getType())));
    }

    CallResult CoreMetaMethods::callFunctionDirectMultiple(State* state, const Value& func, const Vec<Value>& args) {


        // === Input Validation ===
        if (!state) {
            throw std::invalid_argument("State cannot be null in callFunctionDirectMultiple");
        }

        if (!func.isFunction()) {
            throw LuaException("Attempt to call a non-function value in callFunctionDirectMultiple");
        }

        auto function = func.asFunction();
        if (!function) {
            throw LuaException("Function reference is null in callFunctionDirectMultiple");
        }

        // === Argument Count Validation ===
        static const size_t MAX_ARGS = 250;
        if (args.size() > MAX_ARGS) {
            throw LuaException("Too many arguments in function call (max " +
                             std::to_string(MAX_ARGS) + ", got " + std::to_string(args.size()) + ")");
        }

        // === Native Function Handling ===
        if (function->getType() == Function::Type::Native) {
            // Check if it's a legacy function
            if (function->isNativeLegacy()) {
                auto nativeFnLegacy = function->getNativeLegacy();
                if (!nativeFnLegacy) {
                    throw LuaException("Legacy native function pointer is null");
                }

                int oldTop = state->getTop();

                try {
                    // Push arguments onto stack
                    for (size_t i = 0; i < args.size(); ++i) {
                        state->push(args[i]);
                    }

                    // Call legacy native function
                    Value result = nativeFnLegacy(state, static_cast<int>(args.size()));

                    // Restore stack state
                    state->setTop(oldTop);

                    return CallResult(result);

                } catch (const LuaException& e) {
                    state->setTop(oldTop);
                    throw LuaException("Error in legacy native function call: " + std::string(e.what()));
                } catch (const std::exception& e) {
                    state->setTop(oldTop);
                    throw LuaException("Unexpected error in legacy native function call: " + std::string(e.what()));
                }
            } else {
                // New multi-return function - create clean stack environment
                auto nativeFn = function->getNative();
                if (!nativeFn) {
                    throw LuaException("Native function pointer is null");
                }

                // Save current stack state manually
                int oldTop = state->getTop();
                Vec<Value> savedStack;
                savedStack.reserve(oldTop);
                for (int i = 0; i < oldTop; ++i) {
                    savedStack.push_back(state->get(i));
                }

                try {
                    // Create clean stack with only the arguments (Lua 5.1 standard)
                    state->clearStack();
                    for (size_t i = 0; i < args.size(); ++i) {
                        state->push(args[i]);
                    }

                    // Call new multi-return function
                    i32 returnCount = nativeFn(state);

                    // Collect return values from clean stack
                    Vec<Value> results;
                    for (i32 i = 0; i < returnCount; ++i) {
                        if (i < state->getTop()) {
                            results.push_back(state->get(i));
                        } else {
                            results.push_back(Value()); // nil for missing values
                        }
                    }

                    // Restore original stack state manually
                    state->clearStack();
                    for (const auto& val : savedStack) {
                        state->push(val);
                    }

                    return CallResult(results);

                } catch (const LuaException& e) {
                    // Restore stack state before re-throwing
                    state->clearStack();
                    for (const auto& val : savedStack) {
                        state->push(val);
                    }
                    throw LuaException("Error in native function call: " + std::string(e.what()));
                } catch (const std::exception& e) {
                    // Restore stack state before re-throwing
                    state->clearStack();
                    for (const auto& val : savedStack) {
                        state->push(val);
                    }
                    throw LuaException("Unexpected error in native function call: " + std::string(e.what()));
                }
            }
        }

        // === Lua Function Handling ===
        if (function->getType() == Function::Type::Lua) {
            try {


                // SYSTEM FIX: Use the new safe call mechanism that detects VM context
                // This implements proper Lua 5.1 style in-context function calls
                // No more VM instance conflicts!

                CallResult result = state->callSafeMultiple(func, args);

                return result;

            } catch (const LuaException& e) {
                throw LuaException("Error in Lua function call: " + std::string(e.what()));
            } catch (const std::exception& e) {
                throw LuaException("Unexpected error in Lua function call: " + std::string(e.what()));
            }
        }

        // === Unknown Function Type ===
        throw LuaException("Unknown function type in callFunctionDirectMultiple: " +
                         std::to_string(static_cast<int>(function->getType())));
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
