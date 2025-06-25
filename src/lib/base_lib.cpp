#include "base_lib.hpp"
#include "type_conversion.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include "../vm/table.hpp"
#include "../vm/function.hpp"
#include "../gc/core/gc_ref.hpp"
#include <iostream>
#include <sstream>

namespace Lua {
    
    // BaseLib 类的实现
    
    StrView BaseLib::getName() const noexcept {
        return "base";
    }
    
    void BaseLib::registerFunctions(FunctionRegistry& registry) {
        // 使用简化的宏注册函数
        REGISTER_FUNCTION(registry, print, print);
        REGISTER_FUNCTION(registry, tonumber, tonumber);
        REGISTER_FUNCTION(registry, tostring, tostring);
        REGISTER_FUNCTION(registry, type, type);
        REGISTER_FUNCTION(registry, ipairs, ipairs);
        REGISTER_FUNCTION(registry, pairs, pairs);
        REGISTER_FUNCTION(registry, next, next);
        REGISTER_FUNCTION(registry, getmetatable, getmetatable);
        REGISTER_FUNCTION(registry, setmetatable, setmetatable);
        REGISTER_FUNCTION(registry, rawget, rawget);
        REGISTER_FUNCTION(registry, rawset, rawset);
        REGISTER_FUNCTION(registry, rawlen, rawlen);
        REGISTER_FUNCTION(registry, rawequal, rawequal);
        REGISTER_FUNCTION(registry, select, select);
        REGISTER_FUNCTION(registry, unpack, unpack);
    }
    
    void BaseLib::initialize(State* state) {
        // 可以在这里进行特殊的初始化
        // 例如设置全局变量、初始化状态等
    }
    
    // 基础库函数实现（保持与原版相同的签名）
    Value BaseLib::print(State* state, i32 nargs) {
        // Arguments are at the top of the stack
        // Read from stack top backwards: top-nargs+1, top-nargs+2, ..., top
        int stackTop = state->getTop();
        for (i32 i = 0; i < nargs; i++) {
            if (i > 0) std::cout << "\t";
            int argIndex = stackTop - nargs + 1 + i;
            auto val = state->get(argIndex);
            std::cout << val.toString();
        }
        std::cout << std::endl;
        return Value(nullptr);
    }
    
    Value BaseLib::tonumber(State* state, i32 nargs) {
        if (nargs < 1) {
            return Value(nullptr);
        }
        auto val = state->get(1);
        // 实现数字转换逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::tostring(State* state, i32 nargs) {
        if (nargs < 1) {
            return Value("");
        }
        auto val = state->get(1);
        return Value(val.toString());
    }
    
    Value BaseLib::type(State* state, i32 nargs) {
        if (nargs < 1) {
            return Value("nil");
        }
        auto val = state->get(1);
        return Value(TypeConverter::getTypeName(val));
    }
    
    Value BaseLib::ipairs(State* state, i32 nargs) {
        // 实现ipairs逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::pairs(State* state, i32 nargs) {
        // 实现pairs逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::next(State* state, i32 nargs) {
        // 实现next逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::getmetatable(State* state, i32 nargs) {
        // 实现getmetatable逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::setmetatable(State* state, i32 nargs) {
        // 实现setmetatable逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::rawget(State* state, i32 nargs) {
        // 实现rawget逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::rawset(State* state, i32 nargs) {
        // 实现rawset逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::rawlen(State* state, i32 nargs) {
        // 实现rawlen逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::rawequal(State* state, i32 nargs) {
        // 实现rawequal逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::select(State* state, i32 nargs) {
        // 实现select逻辑
        return Value(nullptr); // 占位符
    }
    
    Value BaseLib::unpack(State* state, i32 nargs) {
        // 实现unpack逻辑
        return Value(nullptr); // 占位符
    }
    
    // ModernBaseLib 类的实现
    
    StrView ModernBaseLib::getName() const noexcept {
        return "modern_base";
    }
    
    void ModernBaseLib::registerFunctions(FunctionRegistry& registry) {
        // 使用lambda直接注册，减少静态函数的需要
        registry.registerFunction("print", [](State* state, i32 nargs) -> Value {
            for (i32 i = 1; i <= nargs; i++) {
                if (i > 1) std::cout << "\t";
                auto val = state->get(i);
                std::cout << val.toString();
            }
            std::cout << std::endl;
            return Value(nullptr);
        });
        
        registry.registerFunction("type", [](State* state, i32 nargs) -> Value {
            if (nargs < 1) {
                return Value("nil");
            }
            auto val = state->get(1);
            return Value(TypeConverter::getTypeName(val));
        });
        
        // 可以继续添加更多函数...
    }
    
    // NamespacedBaseLib 类的实现
    
    StrView NamespacedBaseLib::getName() const noexcept {
        return "namespaced_base";
    }
    
    void NamespacedBaseLib::registerFunctions(FunctionRegistry& registry) {
        // 使用命名空间避免函数名冲突
        REGISTER_NAMESPACED_FUNCTION(registry, "base", print, print);
        REGISTER_NAMESPACED_FUNCTION(registry, "base", type, type);
        // 这样注册的函数名为 "base.print", "base.type" 等
    }
    
    Value NamespacedBaseLib::print(State* state, i32 nargs) {
        // 实现...
        return Value(nullptr);
    }
    
    Value NamespacedBaseLib::type(State* state, i32 nargs) {
        // 简单的类型检查实现
        if (nargs < 1) {
            return Value("nil");
        }
        auto val = state->get(1);
        return Value(TypeConverter::getTypeName(val));
    }
    
    // 注册基础库到状态
    void registerBaseLib(State* state) {
        // 注册print函数到全局环境
        auto printFunc = Function::createNative([](State* s, i32 nargs) -> Value {
            return BaseLib::print(s, nargs);
        });
        state->setGlobal("print", Value(printFunc));
        
        // 注册其他基础函数
        auto tonumberFunc = Function::createNative([](State* s, i32 nargs) -> Value {
            return BaseLib::tonumber(s, nargs);
        });
        state->setGlobal("tonumber", Value(tonumberFunc));
        
        auto tostringFunc = Function::createNative([](State* s, i32 nargs) -> Value {
            return BaseLib::tostring(s, nargs);
        });
        state->setGlobal("tostring", Value(tostringFunc));
        
        auto typeFunc = Function::createNative([](State* s, i32 nargs) -> Value {
            return BaseLib::type(s, nargs);
        });
        state->setGlobal("type", Value(typeFunc));
    }
}