#include "function.hpp"
#include "value.hpp"
#include "../gc/core/garbage_collector.hpp"
#include "../gc/core/gc_ref.hpp"
#include <cmath>        // For std::floor

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
        u8 nparams,
        u8 nlocals,
        u8 nupvalues
    ) {
        // Create a new Function object
        GCRef<Function> func = make_gc_ref<Function>(GCObjectType::Function, Type::Lua);
        
        // Set code
        func->lua.code = code;
        
        // Handle constants carefully, avoid using direct assignment
        // Reserve space first
        func->lua.constants.reserve(constants.size());
        
        // Add elements one by one instead of bulk assignment
        for (const auto& value : constants) {
            func->lua.constants.push_back(value);
        }
        
        // Through regular vector operations
        func->lua.nparams = nparams;
        func->lua.nlocals = nlocals;
        func->lua.nupvalues = nupvalues;
        
        // Initialize upvalues array
        func->lua.upvalues.resize(nupvalues, nullptr);
        
        return func;
    }
    
    GCRef<Function> Function::createNative(NativeFn fn) {
        GCRef<Function> func = make_gc_ref<Function>(GCObjectType::Function, Type::Native);
        func->native.fn = fn;
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
    
    usize Function::getConstantCount() const {
        return type == Type::Lua ? lua.constants.size() : 0;
    }
    
    NativeFn Function::getNative() const {
        return type == Type::Native ? native.fn : nullptr;
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
            
            // Mark all upvalues that are GC objects
             for (Value* upvalue : lua.upvalues) {
                 if (upvalue != nullptr && upvalue->isGCObject()) {
                     gc->markObject(upvalue->asGCObject());
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
            
            // Calculate size of upvalues vector
            additionalSize += lua.upvalues.capacity() * sizeof(Value*);
            
            // Calculate size of code vector if it exists
            if (lua.code) {
                additionalSize += lua.code->capacity() * sizeof(Instruction);
            }
        }
        
        return additionalSize;
    }
    
    Value* Function::getUpvalue(usize index) const {
        if (type != Type::Lua || index >= lua.upvalues.size()) {
            return nullptr;
        }
        return lua.upvalues[index];
    }
    
    void Function::setUpvalue(usize index, Value* upvalue) {
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
             // Close all upvalues by setting them to nil
             // In a full implementation, this would properly close upvalues
             // by moving their values from the stack to the upvalue itself
             for (size_t i = 0; i < lua.upvalues.size(); ++i) {
                 // For now, we just set them to nullptr
                 // A full implementation would handle the upvalue closing protocol
                 lua.upvalues[i] = nullptr;
             }
         }
     }
}
