#include "value.hpp"
#include "table.hpp"
#include "function.hpp"
#include <sstream>

namespace Lua {
    ValueType Value::type() const {
        if (std::holds_alternative<std::monostate>(data)) return ValueType::Nil;
        if (std::holds_alternative<LuaBoolean>(data)) return ValueType::Boolean;
        if (std::holds_alternative<LuaNumber>(data)) return ValueType::Number;
        if (std::holds_alternative<Ptr<Str>>(data)) return ValueType::String;
        if (std::holds_alternative<Ptr<Table>>(data)) return ValueType::Table;
        if (std::holds_alternative<Ptr<Function>>(data)) return ValueType::Function;
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
            return *std::get<Ptr<Str>>(data);
        }
        
        return empty;
    }
    
    Ptr<Table> Value::asTable() const {
        if (isTable()) {
            return std::get<Ptr<Table>>(data);
        }
        return nullptr;
    }
    
    Ptr<Function> Value::asFunction() const {
        if (isFunction()) {
            return std::get<Ptr<Function>>(data);
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
                return *std::get<Ptr<Str>>(data);
            case ValueType::Table:
                return "table";
            case ValueType::Function:
                return "function";
            default:
                return "unknown";
        }
    }
    
    bool Value::operator==(const Value& other) const {
        // Different types are not equal (unless number and string can be converted)
        if (type() != other.type()) {
            // Special case: number and convertible string comparison
            if (isNumber() && other.isString()) {
                try {
                    double num = std::stod(other.asString());
                    return std::get<LuaNumber>(data) == num;
                } catch (...) {}
                return false;
            }
            
            if (isString() && other.isNumber()) {
                try {
                    double num = std::stod(asString());
                    return num == std::get<LuaNumber>(other.data);
                } catch (...) {}
                return false;
            }
            
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
                return *std::get<Ptr<Str>>(data) == *std::get<Ptr<Str>>(other.data);
            case ValueType::Table:
                return std::get<Ptr<Table>>(data) == std::get<Ptr<Table>>(other.data); // Compare addresses
            case ValueType::Function:
                return std::get<Ptr<Function>>(data) == std::get<Ptr<Function>>(other.data); // Compare addresses
            default:
                return false;
        }
    }
}
