#include "function.hpp"
#include "value.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/core/gc_ref.hpp"
#include "../gc/memory/allocator.hpp"
#include "../api/lua51_gc_api.hpp"

namespace Lua {
    Function::Function(Type type) : GCObject(GCObjectType::Function, sizeof(Function)), type(type) {
        if (type == Type::Lua) {
            lua.code = nullptr;
            lua.nparams = 0;
            lua.nlocals = 0;
            lua.nupvalues = 0;
        } else {
            native.fn = nullptr;
        }
    }
    
    GCRef<Function> Function::createLua(
        Ptr<Vec<Instruction>> code, 
        const Vec<Value>& constants,
        const Vec<GCRef<Function>>& prototypes,
        u8 nparams,
        u8 nlocals,
        u8 nupvalues,
        bool isVariadic
    ) {
        // Create a new Function object using GC allocator
        extern GCAllocator* g_gcAllocator;
        GCRef<Function> func;
        if (g_gcAllocator) {
            Function* obj = g_gcAllocator->allocateObject<Function>(GCObjectType::Function, Type::Lua);
            func = GCRef<Function>(obj);
        } else {
            // Fallback to direct allocation
            Function* obj = new Function(Type::Lua);
            func = GCRef<Function>(obj);
        }
        
        // Set code
        func->lua.code = code;
        
        // Handle constants carefully, avoid using direct assignment
        // Reserve space first
        func->lua.constants.reserve(constants.size());
        
        // Add elements one by one instead of bulk assignment
        for (const auto& value : constants) {
            func->lua.constants.push_back(value);
        }
        
        // Handle prototypes
        func->lua.prototypes.reserve(prototypes.size());
        for (const auto& prototype : prototypes) {
            func->lua.prototypes.push_back(prototype);
        }
        
        // Through regular vector operations
        func->lua.nparams = nparams;
        func->lua.nlocals = nlocals;
        func->lua.nupvalues = nupvalues;
        func->lua.isVariadic = isVariadic;
        
        // Initialize upvalues array
        func->lua.upvalues.resize(nupvalues);
        
        return func;
    }
    
    GCRef<Function> Function::createNative(NativeFn fn) {
        // Create a new Function object using GC allocator
        extern GCAllocator* g_gcAllocator;
        GCRef<Function> func;
        if (g_gcAllocator) {
            Function* obj = g_gcAllocator->allocateObject<Function>(GCObjectType::Function, Type::Native);
            func = GCRef<Function>(obj);
        } else {
            // Fallback to direct allocation
            Function* obj = new Function(Type::Native);
            func = GCRef<Function>(obj);
        }
        func->native.fn = fn;
        func->native.isLegacy = false;
        return func;
    }

    GCRef<Function> Function::createNativeLegacy(NativeFnLegacy fn) {
        // Create a new Function object using GC allocator
        extern GCAllocator* g_gcAllocator;
        GCRef<Function> func;
        if (g_gcAllocator) {
            Function* obj = g_gcAllocator->allocateObject<Function>(GCObjectType::Function, Type::Native);
            func = GCRef<Function>(obj);
        } else {
            // Fallback to direct allocation
            Function* obj = new Function(Type::Native);
            func = GCRef<Function>(obj);
        }
        func->native.fnLegacy = fn;
        func->native.isLegacy = true;
        return func;
    }
    
    const Vec<Instruction>& Function::getCode() const {
        static const Vec<Instruction> empty;
        return type == Type::Lua && lua.code ? *lua.code : empty;
    }
    
    const Vec<Value>& Function::getConstants() const {
        static const Vec<Value> empty;
        return type == Type::Lua ? lua.constants : empty;
    }
    
    const Vec<GCRef<Function>>& Function::getPrototypes() const {
        static const Vec<GCRef<Function>> empty;
        return type == Type::Lua ? lua.prototypes : empty;
    }
    
    usize Function::getConstantCount() const {
        return type == Type::Lua ? lua.constants.size() : 0;
    }
    
    NativeFn Function::getNative() const {
        return (type == Type::Native && !native.isLegacy) ? native.fn : nullptr;
    }

    NativeFnLegacy Function::getNativeLegacy() const {
        return (type == Type::Native && native.isLegacy) ? native.fnLegacy : nullptr;
    }

    bool Function::isNativeLegacy() const {
        return type == Type::Native && native.isLegacy;
    }
    
    // GCObject virtual function implementations
    void Function::markReferences(GarbageCollector* gc) {
        if (type == Type::Lua) {
            // Mark all constants that are GC objects
            for (const auto& constant : lua.constants) {
                if (constant.isGCObject()) {
                    gc->markObject(constant.asGCObject());
                }
            }
            
            // Mark all prototypes
            for (const auto& prototype : lua.prototypes) {
                if (prototype) {
                    gc->markObject(prototype.get());
                }
            }
            
            // Mark all upvalues
             for (const auto& upvalue : lua.upvalues) {
                 if (upvalue) {
                     gc->markObject(upvalue.get());
                 }
             }
             
             // Mark function prototype if present
             if (lua.prototype != nullptr) {
                 gc->markObject(lua.prototype);
             }
             
             // Note: lua.code is a Ptr<Vec<Instruction>> which doesn't contain GC objects
             // Instructions themselves don't reference GC objects directly
        }
        // Native functions don't have references to mark
    }
    
    usize Function::getSize() const {
        return sizeof(Function);
    }
    
    usize Function::getAdditionalSize() const {
        usize additionalSize = 0;
        
        if (type == Type::Lua) {
            // Calculate size of constants vector
            additionalSize += lua.constants.capacity() * sizeof(Value);
            
            // Calculate size of prototypes vector
            additionalSize += lua.prototypes.capacity() * sizeof(GCRef<Function>);
            
            // Calculate size of upvalues vector
            additionalSize += lua.upvalues.capacity() * sizeof(Value*);
            
            // Calculate size of code vector if it exists
            if (lua.code) {
                additionalSize += lua.code->capacity() * sizeof(Instruction);
            }
        }
        
        return additionalSize;
    }
    
    GCRef<Upvalue> Function::getUpvalue(usize index) const {
        if (type != Type::Lua || index >= lua.upvalues.size()) {
            return GCRef<Upvalue>();
        }
        return lua.upvalues[index];
    }
    
    void Function::setUpvalue(usize index, GCRef<Upvalue> upvalue) {
         if (type == Type::Lua && index < lua.upvalues.size()) {
             lua.upvalues[index] = upvalue;
         }
     }
     
     const Value& Function::getConstant(usize index) const {
         static const Value empty;
         if (type != Type::Lua || index >= lua.constants.size()) {
             return empty;
         }
         return lua.constants[index];
     }
     
     void Function::setPrototype(Function* proto) {
         if (type == Type::Lua) {
             lua.prototype = proto;
         }
     }
     
     void Function::closeUpvalues() {
        if (type == Type::Lua) {
            // Close all upvalues by calling their close method
            // This properly handles the upvalue closing protocol
            for (auto& upvalue : lua.upvalues) {
                if (upvalue) {
                    upvalue->close();
                }
            }
        }
    }
    
    usize Function::estimateMemoryUsage() const {
        if (type != Type::Lua) {
            return sizeof(Function);
        }
        
        usize totalSize = sizeof(Function);
        
        // Add code size
        if (lua.code) {
            totalSize += lua.code->size() * sizeof(Instruction);
        }
        
        // Add constants size (rough estimate)
        totalSize += lua.constants.size() * sizeof(Value);
        
        // Add upvalues size
        totalSize += lua.upvalues.size() * sizeof(GCRef<Upvalue>);
        
        // Add prototypes size (recursive)
        for (const auto& proto : lua.prototypes) {
            if (proto) {
                totalSize += proto->estimateMemoryUsage();
            }
        }
        
        return totalSize;
    }
    
    bool Function::validateUpvalueCount() const {
        return getUpvalueCount() <= MAX_UPVALUES_PER_CLOSURE;
    }
    
    bool Function::validateNestingDepth(u8 currentDepth) const {
        return currentDepth <= MAX_FUNCTION_NESTING_DEPTH;
    }
    
    bool Function::isValidUpvalueIndex(usize index) const {
        if (type != Type::Lua) {
            return false;
        }
        return index < lua.nupvalues;
    }
}
