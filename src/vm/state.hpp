#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include "value.hpp"

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
        Value* getPtr(int idx);  // Get pointer to stack element
        
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
        
        // Set stack size (for clearing stack)
        void setTop(int newTop) {
            if (newTop > top) {
                // Fill new positions with nil when increasing stack size
                for (int i = top; i < newTop; i++) {
                    stack[i] = Value();  // Default Value constructor creates nil
                }
            }
            top = newTop;
        }
        void clearStack() { top = 0; }
        
        // Call function
        Value call(const Value& func, const Vec<Value>& args);

        // Native function call with arguments already on stack (Lua 5.1 design)
        Value callNative(const Value& func, int nargs);
        
        // Code execution
        bool doString(const Str& code);
        bool doFile(const Str& filename);
    };
}
