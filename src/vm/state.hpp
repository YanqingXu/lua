#pragma once

#include "../common/types.hpp"
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
        
        // Stack operations
        void push(const Value& value);
        Value pop();
        Value& get(int idx);
        void set(int idx, const Value& value);
        
        // Check stack element types
        bool isNil(int idx) const;
        bool isBoolean(int idx) const;
        bool isNumber(int idx) const;
        bool isString(int idx) const;
        bool isTable(int idx) const;
        bool isFunction(int idx) const;
        
        // Convert stack elements
        LuaBoolean toBoolean(int idx) const;
        LuaNumber toNumber(int idx) const;
        Str toString(int idx) const;
        Ptr<Table> toTable(int idx);
        Ptr<Function> toFunction(int idx);
        
        // Global variable operations
        void setGlobal(const Str& name, const Value& value);
        Value getGlobal(const Str& name);
        
        // Get stack size
        int getTop() const { return top; }
        
        // Call function
        Value call(const Value& func, const Vec<Value>& args);
        
        // Code execution
        bool doString(const Str& code);
        bool doFile(const Str& filename);
    };
}
