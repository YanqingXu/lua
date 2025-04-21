#pragma once

#include "../types.hpp"
#include "../common/opcodes.hpp"
#include "value.hpp"

namespace Lua {
    // 前向声明
    class State;
    
    // 原生函数类型
    using NativeFn = Value(*)(State* state, int nargs);
    
    // 函数类
    class Function {
    public:
        enum class Type { Lua, Native };
        
    private:
        Type type;
        
        // Lua函数数据
        struct LuaData {
            Ptr<Vec<Instruction>> code;
            Vec<Value> constants;
            u8 nparams;
            u8 nlocals;
            u8 nupvalues;
            
            // 添加默认构造函数
            LuaData() : code(nullptr), constants{}, nparams(0), nlocals(0), nupvalues(0) {}
        } lua;
        
        // 原生函数数据
        struct NativeData {
            NativeFn fn;
            
            // 添加默认构造函数
            NativeData() : fn(nullptr) {}
        } native;
        
    public:
        // 构造函数
        explicit Function(Type type);
        
        // 创建Lua函数
        static Ptr<Function> createLua(
            Ptr<Vec<Instruction>> code, 
            const Vec<Value>& constants,
            u8 nparams = 0,
            u8 nlocals = 0,
            u8 nupvalues = 0
        );
        
        // 创建原生函数
        static Ptr<Function> createNative(NativeFn fn);
        
        // 获取函数类型
        Type getType() const { return type; }
        
        // 获取Lua函数的字节码
        const Vec<Instruction>& getCode() const;
        
        // 获取常量表
        const Vec<Value>& getConstants() const;
        
        // 获取原生函数
        NativeFn getNative() const;
        
        // 获取参数数量
        u8 getParamCount() const { return type == Type::Lua ? lua.nparams : 0; }
        
        // 获取局部变量数量
        u8 getLocalCount() const { return type == Type::Lua ? lua.nlocals : 0; }
        
        // 获取上值数量
        u8 getUpvalueCount() const { return type == Type::Lua ? lua.nupvalues : 0; }
    };
}
