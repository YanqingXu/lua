#pragma once

#include "lib_module.hpp"
#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"

namespace Lua {
    
    /**
     * 使用新接口重构的BaseLib
     * 展示简化后的模块实现方式
     */
    class BaseLib : public LibModule {
    public:
        /**
         * 获取模块名称
         */
        StrView getName() const noexcept override;
        
        /**
         * 注册函数到注册表
         * 使用新的注册机制，更加简洁
         */
        void registerFunctions(FunctionRegistry& registry) override;
        
        /**
         * 可选的初始化逻辑
         */
        void initialize(State* state) override;
        
    public:
        // 基础库函数实现（保持与原版相同的签名）
        static Value print(State* state, i32 nargs);
        static Value tonumber(State* state, i32 nargs);
        static Value tostring(State* state, i32 nargs);
        static Value type(State* state, i32 nargs);
        static Value ipairs(State* state, i32 nargs);
        static Value pairs(State* state, i32 nargs);
        static Value next(State* state, i32 nargs);
        static Value getmetatable(State* state, i32 nargs);
        static Value setmetatable(State* state, i32 nargs);
        static Value rawget(State* state, i32 nargs);
        static Value rawset(State* state, i32 nargs);
        static Value rawlen(State* state, i32 nargs);
        static Value rawequal(State* state, i32 nargs);
        static Value pcall(State* state, i32 nargs);
        static Value xpcall(State* state, i32 nargs);
        static Value error(State* state, i32 nargs);
        static Value assert_func(State* state, i32 nargs);
        static Value select(State* state, i32 nargs);
        static Value unpack(State* state, i32 nargs);
    };
    
    /**
     * 演示更现代的函数实现方式
     * 使用lambda和现代C++特性
     */
    class ModernBaseLib : public LibModule {
    public:
        StrView getName() const noexcept override;
        void registerFunctions(FunctionRegistry& registry) override;
    };
    
    /**
     * 演示命名空间函数的使用
     */
    class NamespacedBaseLib : public LibModule {
    public:
        StrView getName() const noexcept override;
        void registerFunctions(FunctionRegistry& registry) override;
        
    private:
        static Value print(State* state, i32 nargs);
        static Value type(State* state, i32 nargs);
    };
    
    /**
     * 注册基础库到状态
     */
    void registerBaseLib(State* state);
}