#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_ref.hpp"
#include "value.hpp"
#include "table.hpp"

namespace Lua {
    // Forward declarations
    class State;
    
    /**
     * @brief Enumeration of all metamethods in Lua 5.1
     * 
     * This enum represents all metamethods defined in the Lua 5.1 specification.
     * The order matches the official Lua implementation for consistency.
     */
    enum class MetaMethod : u8 {
        // Core metamethods - basic access control
        Index,      // __index - index access metamethod
        NewIndex,   // __newindex - index assignment metamethod
        Call,       // __call - function call metamethod
        ToString,   // __tostring - string conversion metamethod
        
        // Arithmetic metamethods - mathematical operations
        Add,        // __add - addition
        Sub,        // __sub - subtraction
        Mul,        // __mul - multiplication
        Div,        // __div - division
        Mod,        // __mod - modulo
        Pow,        // __pow - exponentiation
        Unm,        // __unm - unary minus
        Concat,     // __concat - string concatenation
        
        // Comparison metamethods - relational operations
        Eq,         // __eq - equality
        Lt,         // __lt - less than
        Le,         // __le - less than or equal
        
        // Special metamethods - advanced features
        Gc,         // __gc - garbage collection
        Mode,       // __mode - weak reference mode
        Metatable,  // __metatable - metatable protection
        
        // Count of metamethods
        Count
    };
    
    /**
     * @brief Metamethod manager for efficient metamethod lookup and invocation
     * 
     * This class provides a centralized interface for metamethod operations,
     * including lookup, caching, and invocation. It follows the Lua 5.1
     * specification for metamethod behavior.
     */
    class MetaMethodManager {
    private:
        // Metamethod name strings (must match Lua 5.1 specification)
        static const Str metaMethodNames_[static_cast<usize>(MetaMethod::Count)];
        
    public:
        // === Metamethod Lookup ===
        
        /**
         * @brief Get metamethod from an object
         * @param obj The object to get metamethod from
         * @param method The metamethod to look for
         * @return The metamethod handler or nil if not found
         */
        static Value getMetaMethod(const Value& obj, MetaMethod method);
        
        /**
         * @brief Get metamethod from a metatable directly
         * @param metatable The metatable to search
         * @param method The metamethod to look for
         * @return The metamethod handler or nil if not found
         */
        static Value getMetaMethod(GCRef<Table> metatable, MetaMethod method);
        
        // === Metamethod Invocation ===
        
        /**
         * @brief Call a metamethod with arguments
         * @param state The Lua state
         * @param method The metamethod type
         * @param obj The object that triggered the metamethod
         * @param args Arguments to pass to the metamethod
         * @return Result of the metamethod call
         */
        static Value callMetaMethod(State* state, MetaMethod method, 
                                   const Value& obj, const Vec<Value>& args);
        
        /**
         * @brief Call a binary metamethod (for arithmetic/comparison operations)
         * @param state The Lua state
         * @param method The metamethod type
         * @param lhs Left operand
         * @param rhs Right operand
         * @return Result of the metamethod call
         */
        static Value callBinaryMetaMethod(State* state, MetaMethod method,
                                         const Value& lhs, const Value& rhs);
        
        /**
         * @brief Call a unary metamethod
         * @param state The Lua state
         * @param method The metamethod type
         * @param operand The operand
         * @return Result of the metamethod call
         */
        static Value callUnaryMetaMethod(State* state, MetaMethod method,
                                        const Value& operand);
        
        // === Utility Functions ===
        
        /**
         * @brief Convert metamethod name to enum
         * @param name The metamethod name (e.g., "__index")
         * @return The corresponding MetaMethod enum value
         */
        static MetaMethod getMetaMethodFromName(const Str& name);
        
        /**
         * @brief Get metamethod name from enum
         * @param method The metamethod enum value
         * @return The metamethod name string
         */
        static const Str& getMetaMethodName(MetaMethod method);
        
        /**
         * @brief Check if an object has a specific metamethod
         * @param obj The object to check
         * @param method The metamethod to look for
         * @return True if the metamethod exists
         */
        static bool hasMetaMethod(const Value& obj, MetaMethod method);
        
        /**
         * @brief Check if a value is callable (function or has __call metamethod)
         * @param value The value to check
         * @return True if the value is callable
         */
        static bool isCallable(const Value& value);
        
    private:
        // === Internal Helper Functions ===
        
        /**
         * @brief Get metatable from a value
         * @param value The value to get metatable from
         * @return The metatable or nullptr if none
         */
        static GCRef<Table> getMetatable(const Value& value);
        
        /**
         * @brief Validate metamethod enum value
         * @param method The metamethod to validate
         * @return True if valid
         */
        static bool isValidMetaMethod(MetaMethod method);
    };
    
} // namespace Lua
