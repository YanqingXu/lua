#include "allocator.hpp"
#include "../core/garbage_collector.hpp"
#include <cstdlib>

namespace Lua {
    GCAllocator* g_gcAllocator = nullptr;

    GCAllocator::GCAllocator() : totalAllocated(0), gcThreshold(1024*1024), gc(nullptr), luaState(nullptr) {}

    GCAllocator::~GCAllocator() {}

    void GCAllocator::setGarbageCollector(GarbageCollector* collector) { gc = collector; }
    void GCAllocator::setLuaState(LuaState* state) { luaState = state; }
    bool GCAllocator::checkMemoryLimits(size_t size) const { return true; }
    void GCAllocator::updateStats(int delta) { if (delta > 0) totalAllocated += delta; }
    bool GCAllocator::shouldTriggerGC() const { return false; }

    void* GCAllocator::allocateRaw(size_t size, GCObjectType type, bool isGCObject) {
        return std::malloc(size);
    }

    void GCAllocator::deallocate(void* ptr, size_t size) {
        std::free(ptr);
    }

    void* GCAllocator::reallocate(void* ptr, size_t oldSize, size_t newSize) {
        return std::realloc(ptr, newSize);
    }

    size_t GCAllocator::getTotalAllocated() const { return totalAllocated; }
    size_t GCAllocator::getGCThreshold() const { return gcThreshold; }
    void GCAllocator::setGCThreshold(size_t threshold) { gcThreshold = threshold; }

    GCAllocator* GCAllocator::getInstance() {
        if (!g_gcAllocator) g_gcAllocator = new GCAllocator();
        return g_gcAllocator;
    }

    void GCAllocator::destroyInstance() {
        delete g_gcAllocator;
        g_gcAllocator = nullptr;
    }
}
