#pragma once

#include "../common/types.hpp"
#include "../gc/core/gc_object.hpp"
#include "value.hpp"
#include "call_result.hpp"
#include <iostream>

namespace Lua {
    // Forward declarations
    class GarbageCollector;
    class VM;
    
    class State : public GCObject {
    private:
        Vec<Value> stack;
        int top;
        HashMap<Str, Value> globals;
        VM* currentVM; // Current VM instance (for context-aware calls)
        
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
        
        // Call function (single return value for backward compatibility)
        Value call(const Value& func, const Vec<Value>& args);

        // Call function with multiple return values support
        CallResult callMultiple(const Value& func, const Vec<Value>& args);

        // VM context-aware function calls (Lua 5.1 style)
        Value callSafe(const Value& func, const Vec<Value>& args);
        CallResult callSafeMultiple(const Value& func, const Vec<Value>& args);

        // Set current VM instance (for context-aware calls)
        void setCurrentVM(VM* vm) { currentVM = vm; }
        VM* getCurrentVM() const { return currentVM; }

        // Native function call with arguments already on stack (Lua 5.1 design)
        Value callNative(const Value& func, int nargs);

        // Lua function call with arguments already on stack
        Value callLua(const Value& func, int nargs);
        
        // Code execution
        bool doString(const Str& code);
        bool doFile(const Str& filename);
    };
}
