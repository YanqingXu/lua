#pragma once

#include "../common/types.hpp"
#include "value.hpp"
#include "../gc/core/gc_object.hpp"
#include <vector>
#include <unordered_map>

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    
    class State : public GCObject {
    private:
        Vec<Value> stack;
        int top;
        HashMap<Str, Value> globals;
        
    public:
        State();
        ~State();
        
        // Override GCObject virtual functions
        void markReferences(GarbageCollector* gc) override;
        usize getSize() const override;
        usize getAdditionalSize() const override;
        
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
        GCRef<Table> toTable(int idx);
        GCRef<Function> toFunction(int idx);
        
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
