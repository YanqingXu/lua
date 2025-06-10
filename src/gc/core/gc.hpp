#pragma once

#include "../../types.hpp"
#include "../../vm/value.hpp"
#include "../utils/gc_types.hpp"

namespace Lua {
    // Forward declarations
    class State;
    class Table;
    class Function;

    class GCObject {
    public:
        bool marked = false;
        virtual ~GCObject() = default;
        virtual void mark() = 0;
    };

    class GarbageCollector {
    private:
        HashSet<GCObject*> allObjects;
        Vec<GCObject*> grayStack;
        usize bytesAllocated = 0;
        usize nextGC = 1024 * 1024; // 1MB
        State* globalState = nullptr;

    public:
        void addObject(GCObject* obj);
        void removeObject(GCObject* obj);
        void collectGarbage();
        void markValue(const Value& value);
        void markObject(GCObject* obj);

        // Additional public methods
        void setState(State* state);
        void forceGC();
        usize getBytesAllocated() const;
        usize getObjectCount() const;

    private:
        void markRoots();
        void traceReferences();
        void sweep();

        // Helper methods for marking specific types
        void markStateRoots(State* state);
        void markTableContents(Table* table);
        void markFunctionContents(Function* func);
        void markGlobals(State* state);
    };
}