#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include "instruction.hpp"
#include "../gc/core/gc_ref.hpp"
#include "../vm/Value.hpp"
#include <functional>

namespace Lua {
    // Forward declarations
    class State;
    //class Value;
    class GarbageCollector;
    template<typename T> class GCRef;
    
    // Native function type
    using NativeFn = std::function<Value(State* state, int nargs)>;
    
    // Function class
    class Function : public GCObject {
    public:
        enum class Type { Lua, Native };
        
    private:
        Type type;
        
        // Lua function data
        struct LuaData {
            Ptr<Vec<Instruction>> code;
            Vec<Value> constants;
            Vec<Value*> upvalues;  // Store upvalue references
            Function* prototype;    // Function prototype (parent function)
            u8 nparams;
            u8 nlocals;
            u8 nupvalues;
            
            // Add default constructor
            LuaData() : code(nullptr), constants{}, upvalues{}, prototype(nullptr), nparams(0), nlocals(0), nupvalues(0) {}
        } lua;
        
        // Native function data
        struct NativeData {
            NativeFn fn;
            
            // Add default constructor
            NativeData() : fn(nullptr) {}
        } native;
        
    public:
        // Constructor
        explicit Function(Type type);
        
        // Override GCObject virtual functions
        void markReferences(GarbageCollector* gc) override;
        usize getSize() const override;
        usize getAdditionalSize() const override;
        
        // Create Lua function
        static GCRef<Function> createLua(
            Ptr<Vec<Instruction>> code, 
            const Vec<Value>& constants,
            u8 nparams = 0,
            u8 nlocals = 0,
            u8 nupvalues = 0
        );
        
        // Create native function
        static GCRef<Function> createNative(NativeFn fn);
        
        // Get function type
        Type getType() const { return type; }
        
        // Get Lua function bytecode
        const Vec<Instruction>& getCode() const;
        
        // Get constant table
        const Vec<Value>& getConstants() const;
        
        // Get native function
        NativeFn getNative() const;
        
        // Get parameter count
        u8 getParamCount() const { return type == Type::Lua ? lua.nparams : 0; }
        
        // Get local variable count
        u8 getLocalCount() const { return type == Type::Lua ? lua.nlocals : 0; }
        
        // Get upvalue count
        u8 getUpvalueCount() const { return type == Type::Lua ? lua.nupvalues : 0; }
        
        // Get upvalue by index
        Value* getUpvalue(usize index) const;
        
        // Set upvalue by index
        void setUpvalue(usize index, Value* upvalue);
        
        // Get constant count
        usize getConstantCount() const;
        
        // Get constant by index
        const Value& getConstant(usize index) const;
        
        // Get function prototype
        Function* getPrototype() const { return type == Type::Lua ? lua.prototype : nullptr; }
        
        // Set function prototype
        void setPrototype(Function* proto);
        
        // Close upvalues (for garbage collection)
        void closeUpvalues();
    };
}
