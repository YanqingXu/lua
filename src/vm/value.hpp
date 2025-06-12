#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include "../gc/core/gc_string.hpp"
#include "../gc/core/gc_ref.hpp"
#include <variant>
#include <memory>
#include <functional>   // For std::less
#include <cmath>        // For floating point operations

namespace Lua {
    // Forward declarations
    class Table;
    class Function;
    class GCString;
    template<typename T> class GCRef;
    
    // Forward declare utility functions
    GCRef<GCString> make_gc_string(const Str& str);
    GCRef<GCString> make_gc_string(const char* str);
    
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
            GCRef<GCString>,     // String
            GCRef<Table>,        // Table
            GCRef<Function>      // Function
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
        Value(const Str& val) : data(make_gc_string(val)) {}
        Value(const char* val) : data(make_gc_string(val)) {}
        Value(GCRef<Table> val) : data(val) {}
        Value(GCRef<Function> val) : data(val) {}
        
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
        
        // GC object checking
        bool isGCObject() const { return isString() || isTable() || isFunction(); }
        GCObject* asGCObject() const;
        
        // Get values
        LuaBoolean asBoolean() const;
        LuaNumber asNumber() const;
        const Str& asString() const;
        GCRef<Table> asTable() const;
        GCRef<Function> asFunction() const;
        
        // GC integration
        void markReferences(class GarbageCollector* gc) const;
        
        // Convert to string for printing
        Str toString() const;
        
        // Equality comparison
        bool operator==(const Value& other) const;
        bool operator!=(const Value& other) const { return !(*this == other); }
        
        // Less than comparison, for map sorting
        bool operator<(const Value& other) const;
        
        // Truthiness test (Lua semantics: only nil and false are falsy)
        bool isTruthy() const;
    };
}
