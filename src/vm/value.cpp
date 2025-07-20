#include "value.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "table.hpp"
#include "function.hpp"
#include "userdata.hpp"
#include <sstream>

namespace Lua {
    ValueType Value::type() const {
        if (std::holds_alternative<std::monostate>(data)) return ValueType::Nil;
        if (std::holds_alternative<LuaBoolean>(data)) return ValueType::Boolean;
        if (std::holds_alternative<LuaNumber>(data)) return ValueType::Number;
        if (std::holds_alternative<GCRef<GCString>>(data)) return ValueType::String;
        if (std::holds_alternative<GCRef<Table>>(data)) return ValueType::Table;
        if (std::holds_alternative<GCRef<Function>>(data)) return ValueType::Function;
        if (std::holds_alternative<GCRef<Userdata>>(data)) return ValueType::Userdata;
        return ValueType::Nil; // Default case, should not reach here
    }
    
    LuaBoolean Value::asBoolean() const {
        if (isBoolean()) {
            return std::get<LuaBoolean>(data);
        }
        
        // In Lua, only nil and false are falsy, all other values are truthy
        return !isNil();
    }
    
    LuaNumber Value::asNumber() const {
        if (isNumber()) {
            return std::get<LuaNumber>(data);
        }
        
        // Can add string to number conversion
        if (isString()) {
            try {
                return std::stod(asString());
            } catch (...) {
                // Conversion failed
            }
        }
        
        return 0.0;
    }
    
    const Str& Value::asString() const {
        static const Str empty;
        
        if (isString()) {
            return std::get<GCRef<GCString>>(data)->getString();
        }
        
        return empty;
    }
    
    GCRef<Table> Value::asTable() const {
        if (isTable()) {
            return std::get<GCRef<Table>>(data);
        }
        return GCRef<Table>(nullptr);
    }
    
    GCRef<Function> Value::asFunction() const {
        if (isFunction()) {
            return std::get<GCRef<Function>>(data);
        }
        return GCRef<Function>(nullptr);
    }

    GCRef<Userdata> Value::asUserdata() const {
        if (isUserdata()) {
            return std::get<GCRef<Userdata>>(data);
        }
        return GCRef<Userdata>(nullptr);
    }
    
    GCObject* Value::asGCObject() const {
        if (isString()) {
            return std::get<GCRef<GCString>>(data).get();
        }
        if (isTable()) {
            return asTable().get();
        }
        if (isFunction()) {
            return asFunction().get();
        }
        if (isUserdata()) {
            return asUserdata().get();
        }
        return nullptr;
    }
    
    Str Value::toString() const {
        switch (type()) {
            case ValueType::Nil:
                return "nil";
            case ValueType::Boolean:
                return std::get<LuaBoolean>(data) ? "true" : "false";
            case ValueType::Number: {
                std::stringstream ss;
                ss << std::get<LuaNumber>(data);
                return ss.str();
            }
            case ValueType::String:
                return std::get<GCRef<GCString>>(data)->getString();
            case ValueType::Table:
                return "table";
            case ValueType::Function:
                return "function";
            case ValueType::Userdata:
                return "userdata";
            default:
                return "unknown";
        }
    }

    Str Value::getTypeName() const {
        switch (type()) {
            case ValueType::Nil:
                return "nil";
            case ValueType::Boolean:
                return "boolean";
            case ValueType::Number:
                return "number";
            case ValueType::String:
                return "string";
            case ValueType::Table:
                return "table";
            case ValueType::Function:
                return "function";
            case ValueType::Userdata:
                return "userdata";
            default:
                return "unknown";
        }
    }

    bool Value::operator==(const Value& other) const {
        // Different types are never equal in Lua
        // (No automatic type conversion for equality comparison)
        if (type() != other.type()) {
            return false;
        }
        
        // Same type comparison
        switch (type()) {
            case ValueType::Nil:
                return true; // nil is always equal to nil
            case ValueType::Boolean:
                return std::get<LuaBoolean>(data) == std::get<LuaBoolean>(other.data);
            case ValueType::Number:
                return std::get<LuaNumber>(data) == std::get<LuaNumber>(other.data);
            case ValueType::String:
                return *std::get<GCRef<GCString>>(data) == *std::get<GCRef<GCString>>(other.data);
            case ValueType::Table:
                return std::get<GCRef<Table>>(data) == std::get<GCRef<Table>>(other.data); // Compare addresses
            case ValueType::Function:
                return std::get<GCRef<Function>>(data) == std::get<GCRef<Function>>(other.data); // Compare addresses
            case ValueType::Userdata:
                return std::get<GCRef<Userdata>>(data) == std::get<GCRef<Userdata>>(other.data); // Compare addresses
            default:
                return false;
        }
    }
    
    void Value::markReferences(GarbageCollector* gc) const {
        if (isString()) {
            gc->markObject(std::get<GCRef<GCString>>(data).get());
        } else if (isTable()) {
            gc->markObject(asTable().get());
        } else if (isFunction()) {
            gc->markObject(asFunction().get());
        } else if (isUserdata()) {
            gc->markObject(asUserdata().get());
        }
    }
    
    bool Value::operator<(const Value& other) const {
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
                // Compare pointer addresses
                // Use std::less to ensure pointer comparison safety
                return std::less<void*>()(asTable().get(), other.asTable().get());
            case ValueType::Function:
                // Compare pointer addresses
                // Use std::less to ensure pointer comparison safety
                return std::less<void*>()(asFunction().get(), other.asFunction().get());
            case ValueType::Userdata:
                // Compare pointer addresses
                // Use std::less to ensure pointer comparison safety
                return std::less<void*>()(asUserdata().get(), other.asUserdata().get());
            default:
                return false;
        }
    }
    
    bool Value::isTruthy() const {
        // In Lua, only nil and false are falsy, all other values are truthy
        if (isNil()) {
            return false;
        }
        if (isBoolean()) {
            return asBoolean();
        }
        // All other values (numbers, strings, tables, functions, userdata) are truthy
        return true;
    }
}
