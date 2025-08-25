#include "value.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/core/gc_string.hpp"
#include "../api/lua51_gc_api.hpp"
#include "table.hpp"
#include "function.hpp"
#include "userdata.hpp"
#include "lua_state.hpp"
#include "lua_coroutine.hpp"
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
        if (std::holds_alternative<GCRef<LuaCoroutine>>(data)) return ValueType::Thread;
        if (std::holds_alternative<void*>(data)) return ValueType::LightUserdata;
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

    GCRef<LuaCoroutine> Value::asThread() const {
        if (isThread()) {
            return std::get<GCRef<LuaCoroutine>>(data);
        }
        return GCRef<LuaCoroutine>(nullptr);
    }

    void* Value::asLightUserdata() const {
        if (isLightUserdata()) {
            return std::get<void*>(data);
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
            case ValueType::Thread:
                return "thread";
            case ValueType::LightUserdata:
                return "userdata";  // Light userdata also shows as "userdata" in Lua 5.1
            default:
                // Add debug information for unknown types
                #ifdef DEBUG
                std::cerr << "Warning: Value::toString() encountered unknown type: "
                         << static_cast<int>(type()) << std::endl;
                #endif
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
            case ValueType::Thread:
                return "thread";
            case ValueType::LightUserdata:
                return "userdata";  // Light userdata also reports as "userdata" type in Lua 5.1
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
            case ValueType::Thread:
                return std::get<GCRef<LuaCoroutine>>(data) == std::get<GCRef<LuaCoroutine>>(other.data); // Compare addresses
            case ValueType::LightUserdata:
                return std::get<void*>(data) == std::get<void*>(other.data); // Compare pointer values
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
        } else if (isThread()) {
            gc->markObject(asThread().get());
        }
        // Note: Light userdata (void*) is not managed by GC, so no marking needed
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

    // === Write Barrier Support Implementation ===

    GCObject* Value::asGCObject() const {
        if (!isGCObject()) {
            return nullptr;
        }

        switch (type()) {
            case ValueType::String:
                return static_cast<GCObject*>(std::get<GCRef<GCString>>(data).get());
            case ValueType::Table:
                return static_cast<GCObject*>(asTable().get());
            case ValueType::Function:
                return static_cast<GCObject*>(asFunction().get());
            case ValueType::Userdata:
                return static_cast<GCObject*>(asUserdata().get());
            default:
                return nullptr;
        }
    }

    void Value::assignWithBarrier(const Value& other, LuaState* L) {
        // 如果当前值和新值都是GC对象，需要检查写屏障
        if (L && isGCObject() && other.isGCObject()) {
            GCObject* currentObj = asGCObject();
            GCObject* newObj = other.asGCObject();

            // 应用写屏障 - 检查当前对象是否为黑色，新对象是否为白色
            if (currentObj && newObj) {
                luaC_barrier(L, currentObj, newObj);
            }
        }

        // 执行实际赋值
        *this = other;
    }

    template<typename T>
    void Value::setGCObjectWithBarrier(GCRef<T> gcObj, LuaState* L) {
        // 如果当前值是GC对象且新对象也是GC对象，检查写屏障
        if (L && isGCObject() && gcObj) {
            GCObject* currentObj = asGCObject();
            GCObject* newObj = gcObj.get();

            if (currentObj && newObj) {
                luaC_barrier(L, currentObj, newObj);
            }
        }

        // 执行实际赋值
        data = gcObj;
    }

    // 显式实例化模板方法
    template void Value::setGCObjectWithBarrier<GCString>(GCRef<GCString>, LuaState*);
    template void Value::setGCObjectWithBarrier<Table>(GCRef<Table>, LuaState*);
    template void Value::setGCObjectWithBarrier<Function>(GCRef<Function>, LuaState*);
    template void Value::setGCObjectWithBarrier<Userdata>(GCRef<Userdata>, LuaState*);
}
