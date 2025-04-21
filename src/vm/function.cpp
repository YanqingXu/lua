#include "function.hpp"
#include <cmath>        // 用于std::floor

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
        // 创建一个新的Function对象
        Ptr<Function> func = make_ptr<Function>(Type::Lua);
        
        // 设置代码
        func->lua.code = code;
        
        // 小心处理constants，避免使用直接赋值
        // 先预留空间
        func->lua.constants.reserve(constants.size());
        
        // 逐个添加元素而不是整体赋值
        for (const auto& value : constants) {
            func->lua.constants.push_back(value);
        }
        
        // 遣过常规的向量操作
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
