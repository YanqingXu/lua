#pragma once

#include "../types.hpp"
#include "value.hpp"
#include <vector>
#include <unordered_map>

namespace Lua {
    class State {
    private:
        Vec<Value> stack;
        int top;
        std::unordered_map<Str, Value> globals;
        
    public:
        State();
        ~State();
        
        // 栈操作
        void push(const Value& value);
        Value pop();
        Value& get(int idx);
        void set(int idx, const Value& value);
        
        // 检查栈元素类型
        bool isNil(int idx) const;
        bool isBoolean(int idx) const;
        bool isNumber(int idx) const;
        bool isString(int idx) const;
        bool isTable(int idx) const;
        bool isFunction(int idx) const;
        
        // 转换栈元素
        LuaBoolean toBoolean(int idx) const;
        LuaNumber toNumber(int idx) const;
        Str toString(int idx) const;
        Ptr<Table> toTable(int idx);
        Ptr<Function> toFunction(int idx);
        
        // 全局变量操作
        void setGlobal(const Str& name, const Value& value);
        Value getGlobal(const Str& name);
        
        // 获取栈大小
        int getTop() const { return top; }
        
        // 调用函数
        Value call(const Value& func, const Vec<Value>& args);
        
        // Code execution
        bool doString(const Str& code);
        bool doFile(const Str& filename);
    };
}
