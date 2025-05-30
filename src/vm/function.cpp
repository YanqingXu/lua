#include "function.hpp"
#include <cmath>        // For std::floor

namespace Lua {
    Function::Function(Type type) : type(type) {
        if (type == Type::Lua) {
            lua.code = nullptr;
            lua.nparams = 0;
            lua.nlocals = 0;
            lua.nupvalues = 0;
        } else {
            native.fn = nullptr;
        }
    }
    
    Ptr<Function> Function::createLua(
        Ptr<Vec<Instruction>> code, 
        const Vec<Value>& constants,
        u8 nparams,
        u8 nlocals,
        u8 nupvalues
    ) {
        // Create a new Function object
        Ptr<Function> func = make_ptr<Function>(Type::Lua);
        
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
        return func;
    }
    
    Ptr<Function> Function::createNative(NativeFn fn) {
        Ptr<Function> func = make_ptr<Function>(Type::Native);
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
    
    NativeFn Function::getNative() const {
        return type == Type::Native ? native.fn : nullptr;
    }
}
