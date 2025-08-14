#pragma once

#include "../common/types.hpp"
#include "value.hpp"
#include "metamethod_manager.hpp"
#include "call_result.hpp"

namespace Lua {
    // Forward declarations
    class State;
    
    /**
     * @brief Core metamethods implementation
     * 
     * This class implements the core metamethods that are fundamental to Lua's
     * object model: __index, __newindex, __call, and __tostring.
     * These metamethods provide the foundation for object-oriented programming
     * and custom behavior in Lua.
     */
    class CoreMetaMethods {
    public:
        // === Core Metamethod Handlers ===
        
        /**
         * @brief Handle __index metamethod
         * 
         * The __index metamethod is called when accessing a table field that doesn't exist.
         * It can be either a function or a table:
         * - If function: called with (table, key) and returns the value
         * - If table: the key is looked up in that table
         * 
         * @param state The Lua state
         * @param table The table being indexed
         * @param key The key being accessed
         * @return The value returned by the metamethod or nil
         */
        static Value handleIndex(LuaState* state, const Value& table, const Value& key);
        
        /**
         * @brief Handle __newindex metamethod
         * 
         * The __newindex metamethod is called when assigning to a table field that doesn't exist.
         * It can be either a function or a table:
         * - If function: called with (table, key, value)
         * - If table: the assignment is performed on that table
         * 
         * @param state The Lua state
         * @param table The table being assigned to
         * @param key The key being assigned
         * @param value The value being assigned
         */
        static void handleNewIndex(LuaState* state, const Value& table,
                                  const Value& key, const Value& value);
        
        /**
         * @brief Handle __call metamethod
         *
         * The __call metamethod allows a table to be called like a function.
         * The metamethod function is called with the table as the first argument,
         * followed by the arguments passed to the call.
         *
         * @param state The Lua state
         * @param func The table/object being called
         * @param args Arguments passed to the call
         * @return The result of the call
         */
        static Value handleCall(LuaState* state, const Value& func, const Vec<Value>& args);

        /**
         * @brief Handle __call metamethod with multiple return values
         *
         * Similar to handleCall but supports multiple return values.
         *
         * @param state The Lua state
         * @param func The table/object being called
         * @param args Arguments passed to the call
         * @return CallResult containing all return values
         */
        static CallResult handleCallMultiple(LuaState* state, const Value& func, const Vec<Value>& args);
        
        /**
         * @brief Handle __tostring metamethod
         * 
         * The __tostring metamethod is called when converting an object to a string.
         * It should return a string representation of the object.
         * 
         * @param state The Lua state
         * @param obj The object being converted to string
         * @return String representation of the object
         */
        static Value handleToString(LuaState* state, const Value& obj);
        
        // === Utility Functions ===
        
        /**
         * @brief Perform raw table index (without metamethods)
         * 
         * This function performs a direct table lookup without triggering
         * any metamethods. It's used internally and for rawget functionality.
         * 
         * @param table The table to index
         * @param key The key to look up
         * @return The value at the key or nil if not found
         */
        static Value rawIndex(const Value& table, const Value& key);
        
        /**
         * @brief Perform raw table assignment (without metamethods)
         * 
         * This function performs a direct table assignment without triggering
         * any metamethods. It's used internally and for rawset functionality.
         * 
         * @param table The table to assign to
         * @param key The key to assign
         * @param value The value to assign
         */
        static void rawNewIndex(const Value& table, const Value& key, const Value& value);
        
        /**
         * @brief Check if a value is callable
         *
         * A value is callable if it's a function or has a __call metamethod.
         *
         * @param obj The value to check
         * @return True if the value is callable
         */
        static bool isCallable(const Value& obj);

        /**
         * @brief Validate call arguments
         *
         * Validates that the arguments for a function call are within acceptable limits
         * and properly formed according to Lua 5.1 specifications.
         *
         * @param args The arguments to validate
         * @return True if arguments are valid
         */
        static bool validateCallArguments(const Vec<Value>& args);

        /**
         * @brief Get detailed error message for call failures
         *
         * Generates a descriptive error message for failed function calls,
         * including information about the function type and argument count.
         *
         * @param func The function that failed to be called
         * @param args The arguments that were passed
         * @return Detailed error message string
         */
        static std::string getCallErrorMessage(const Value& func, const Vec<Value>& args);
        
        /**
         * @brief Get default string representation of a value
         *
         * This function provides the default string representation for values
         * that don't have a __tostring metamethod.
         *
         * @param obj The value to convert to string
         * @return Default string representation
         */
        static Str getDefaultString(const Value& obj);

        /**
         * @brief Handle metamethod that can be either function or table
         *
         * This function is used internally by metamethod implementations and
         * by MetaMethodManager for consistent metamethod calling.
         *
         * @param state The Lua state
         * @param handler The metamethod handler (function or table)
         * @param args Arguments for the metamethod
         * @return Result of the metamethod call or nil
         */
        static Value handleMetaMethodCall(LuaState* state, const Value& handler, const Vec<Value>& args);

        /**
         * @brief Call a function directly without metamethod recursion
         *
         * This function provides direct function calling to avoid infinite
         * recursion when handling __call metamethods.
         *
         * @param state The Lua state
         * @param func The function to call
         * @param args Arguments for the function
         * @return Result of the function call
         */
        static Value callFunctionDirect(LuaState* state, const Value& func, const Vec<Value>& args);

        /**
         * @brief Direct function call with multiple return values support
         *
         * Similar to callFunctionDirect but supports multiple return values.
         *
         * @param state The Lua state
         * @param func The function to call
         * @param args Arguments for the function
         * @return CallResult containing all return values
         */
        static CallResult callFunctionDirectMultiple(LuaState* state, const Value& func, const Vec<Value>& args);

    private:
        // === Internal Helper Functions ===

        /**
         * @brief Validate table value for metamethod operations
         * @param table The value to validate
         * @return True if the value is a valid table
         */
        static bool isValidTable(const Value& table);
        
        /**
         * @brief Perform table lookup in metamethod handler table
         * @param handlerTable The table to look up in
         * @param key The key to look up
         * @return The value found or nil
         */
        static Value lookupInHandlerTable(const Value& handlerTable, const Value& key);
        
        /**
         * @brief Perform table assignment in metamethod handler table
         * @param handlerTable The table to assign to
         * @param key The key to assign
         * @param value The value to assign
         */
        static void assignToHandlerTable(const Value& handlerTable, const Value& key, const Value& value);
    };
    
} // namespace Lua
