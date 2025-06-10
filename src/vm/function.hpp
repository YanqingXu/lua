#pragma once

#include "../common/types.hpp"
#include "instruction.hpp"
#include "value.hpp"

namespace Lua {
    // Forward declaration
    class State;
    
    // Native function type
    using NativeFn = Value(*)(State* state, int nargs);
    
    // Function class
    class Function {
    public:
        enum class Type { Lua, Native };
        
    private:
        Type type;
        
        // Lua function data
        struct LuaData {
            Ptr<Vec<Instruction>> code;
            Vec<Value> constants;
            u8 nparams;
            u8 nlocals;
            u8 nupvalues;
            
            // Add default constructor
            LuaData() : code(nullptr), constants{}, nparams(0), nlocals(0), nupvalues(0) {}
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
        
        // Create Lua function
        static Ptr<Function> createLua(
            Ptr<Vec<Instruction>> code, 
            const Vec<Value>& constants,
            u8 nparams = 0,
            u8 nlocals = 0,
            u8 nupvalues = 0
        );
        
        // Create native function
        static Ptr<Function> createNative(NativeFn fn);
        
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
    };
}
