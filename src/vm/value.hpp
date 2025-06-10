#pragma once

#include "../common/types.hpp"
#include <variant>
#include <iostream>
#include <functional>   // For std::less
#include <cmath>        // For floating point operations

namespace Lua {
    // Forward declarations
    class Table;
    class Function;
    
    // Lua value types
    enum class ValueType {
        Nil,
        Boolean,
        Number,
        String,
        Table,
        Function
    };
    
    // Lua value
    class Value {
    private:
        // Use std::variant to store different types of values
        using ValueVariant = std::variant<
            std::monostate,      // Nil
            LuaBoolean,          // Boolean
            LuaNumber,           // Number
            Ptr<Str>,      // String
            Ptr<Table>,          // Table
            Ptr<Function>        // Function
        >;
        
        ValueVariant data;
        
    public:
        // Constructors
        Value() : data(std::monostate{}) {}
        Value(std::nullptr_t) : data(std::monostate{}) {}
        Value(LuaBoolean val) : data(val) {}
        Value(LuaNumber val) : data(val) {}
        Value(i32 val) : data(static_cast<LuaNumber>(val)) {}  // Accept 32-bit integer
        Value(i64 val) : data(static_cast<LuaNumber>(val)) {}  // Accept 64-bit integer
        Value(const Str& val) : data(make_ptr<Str>(val)) {}
        Value(const char* val) : data(make_ptr<Str>(val)) {}
        Value(Ptr<Table> val) : data(val) {}
        Value(Ptr<Function> val) : data(val) {}
        
        // Copy constructor
        Value(const Value& other) = default;
        
        // Move constructor
        Value(Value&& other) noexcept = default;
        
        // Copy assignment operator
        Value& operator=(const Value& other) = default;
        
        // Move assignment operator
        Value& operator=(Value&& other) noexcept = default;
        
        // Type checking
        ValueType type() const;
        bool isNil() const { return type() == ValueType::Nil; }
        bool isBoolean() const { return type() == ValueType::Boolean; }
        bool isNumber() const { return type() == ValueType::Number; }
        bool isString() const { return type() == ValueType::String; }
        bool isTable() const { return type() == ValueType::Table; }
        bool isFunction() const { return type() == ValueType::Function; }
        
        // Get values
        LuaBoolean asBoolean() const;
        LuaNumber asNumber() const;
        const Str& asString() const;
        Ptr<Table> asTable() const;
        Ptr<Function> asFunction() const;
        
        // Convert to string for printing
        Str toString() const;
        
        // Equality comparison
        bool operator==(const Value& other) const;
        bool operator!=(const Value& other) const { return !(*this == other); }
        
        // Less than comparison, for map sorting
        bool operator<(const Value& other) const {
            // First sort by type
            if (type() != other.type()) {
                return static_cast<int>(type()) < static_cast<int>(other.type());
            }
            
            // Same type comparison
            switch (type()) {
                case ValueType::Nil:
                    return false; // nil is not less than nil
                case ValueType::Boolean:
                    return asBoolean() < other.asBoolean();
                case ValueType::Number:
                    return asNumber() < other.asNumber();
                case ValueType::String:
                    return asString() < other.asString();
                case ValueType::Table:
                case ValueType::Function:
                    // Compare pointer addresses
                    // Use std::less to ensure pointer comparison safety
                    return std::less<void*>()(asTable().get(), other.asTable().get());
                default:
                    return false;
            }
        }
    };
}
