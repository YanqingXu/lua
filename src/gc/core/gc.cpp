#include "gc.hpp"
#include "../../vm/table.hpp"
#include "../../vm/function.hpp"
#include "../../vm/state.hpp"
#include <algorithm>
#include <iostream>

namespace Lua {

    // GarbageCollector implementation

    void GarbageCollector::addObject(GCObject* obj) {
        if (obj != nullptr) {
            allObjects.insert(obj);
            bytesAllocated += sizeof(*obj); // Approximate size

            // Check if we need to trigger GC
            if (bytesAllocated >= nextGC) {
                collectGarbage();
            }
        }
    }

    void GarbageCollector::removeObject(GCObject* obj) {
        if (obj != nullptr) {
            auto it = allObjects.find(obj);
            if (it != allObjects.end()) {
                allObjects.erase(it);
                if (bytesAllocated >= sizeof(*obj)) {
                    bytesAllocated -= sizeof(*obj);
                }
            }
        }
    }

    void GarbageCollector::collectGarbage() {
        // Three-phase mark and sweep garbage collection

        // Phase 1: Mark roots
        markRoots();

        // Phase 2: Trace references
        traceReferences();

        // Phase 3: Sweep unreachable objects
        sweep();

        // Update next GC threshold
        nextGC = bytesAllocated * 2;
        if (nextGC < 1024 * 1024) { // Minimum 1MB
            nextGC = 1024 * 1024;
        }
    }

    void GarbageCollector::markValue(const Value& value) {
        switch (value.type()) {
            case ValueType::String: {
                // Strings are managed by shared_ptr, no direct GC needed
                // But we might want to track them for debugging
                break;
            }
            case ValueType::Table: {
                auto table = value.asTable();
                if (table) {
                    // Mark table if it's a GCObject
                    // Note: In this implementation, Table doesn't inherit from GCObject
                    // but we can still traverse its contents
                    markTableContents(table.get());
                }
                break;
            }
            case ValueType::Function: {
                auto func = value.asFunction();
                if (func) {
                    // Mark function if it's a GCObject
                    markFunctionContents(func.get());
                }
                break;
            }
            default:
                // Nil, Boolean, Number don't need marking
                break;
        }
    }

    void GarbageCollector::markObject(GCObject* obj) {
        if (obj != nullptr && !obj->marked) {
            obj->marked = true;
            grayStack.push_back(obj);
        }
    }

    void GarbageCollector::markRoots() {
        // Clear all marks first
        for (auto* obj : allObjects) {
            obj->marked = false;
        }
        grayStack.clear();

        // Mark global state roots
        if (globalState != nullptr) {
            markStateRoots(globalState);
        }
    }

    void GarbageCollector::traceReferences() {
        // Process gray stack until empty
        while (!grayStack.empty()) {
            GCObject* obj = grayStack.back();
            grayStack.pop_back();

            // Let the object mark its references
            obj->mark();
        }
    }

    void GarbageCollector::sweep() {
        auto it = allObjects.begin();
        while (it != allObjects.end()) {
            GCObject* obj = *it;
            if (!obj->marked) {
                // Object is unreachable, delete it
                it = allObjects.erase(it);
                if (bytesAllocated >= sizeof(*obj)) {
                    bytesAllocated -= sizeof(*obj);
                }
                delete obj;
            } else {
                ++it;
            }
        }
    }

    void GarbageCollector::markStateRoots(State* state) {
        if (state == nullptr) return;

        // Mark stack values
        for (int i = 0; i < state->getTop(); ++i) {
            markValue(state->get(i));
        }

        // Mark global variables
        // Note: We need access to State's globals, which might require
        // adding a friend declaration or getter method
        markGlobals(state);
    }

    void GarbageCollector::markTableContents(Table* table) {
        if (table == nullptr) return;

        // Mark array part
        for (usize i = 0; i < table->length(); ++i) {
            Value key(static_cast<LuaNumber>(i + 1));
            Value value = table->get(key);
            markValue(value);
        }

        // Note: To properly mark hash part, we'd need access to Table's entries
        // This might require adding a friend declaration or iterator methods
    }

    void GarbageCollector::markFunctionContents(Function* func) {
        if (func == nullptr) return;

        if (func->getType() == Function::Type::Lua) {
            // Mark constants in the function
            const auto& constants = func->getConstants();
            for (const auto& constant : constants) {
                markValue(constant);
            }

            // Mark upvalues if they exist
            // Note: This would require extending Function class to track upvalues
        }
    }

    void GarbageCollector::markGlobals(State* state) {
        // This would require access to State's globals map
        // For now, we'll leave this as a placeholder
        // In a real implementation, State would need to provide
        // an iterator or friend access to its globals
    }

    void GarbageCollector::setState(State* state) {
        globalState = state;
    }

    usize GarbageCollector::getBytesAllocated() const {
        return bytesAllocated;
    }

    usize GarbageCollector::getObjectCount() const {
        return allObjects.size();
    }

    void GarbageCollector::forceGC() {
        collectGarbage();
    }

    // Example GCObject implementations

    // If Table were to inherit from GCObject:
    /*
    class GCTable : public Table, public GCObject {
    public:
        void mark() override {
            // Mark all values in the table
            for (usize i = 0; i < length(); ++i) {
                Value key(static_cast<LuaNumber>(i + 1));
                Value value = get(key);
                if (gc) gc->markValue(value);
            }
            // Mark hash part entries...
        }
    private:
        GarbageCollector* gc = nullptr;
    };
    */

    // If Function were to inherit from GCObject:
    /*
    class GCFunction : public Function, public GCObject {
    public:
        void mark() override {
            if (getType() == Type::Lua) {
                const auto& constants = getConstants();
                for (const auto& constant : constants) {
                    if (gc) gc->markValue(constant);
                }
            }
        }
    private:
        GarbageCollector* gc = nullptr;
    };
    */
}