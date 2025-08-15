#include "allocator.hpp"
#include "../core/garbage_collector.hpp"
#include "../../vm/lua_state.hpp"
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace Lua {
    GCAllocator* g_gcAllocator = nullptr;

    GCAllocator::GCAllocator()
        : totalAllocated(0), gcThreshold(1024*1024), gc(nullptr), luaState(nullptr), stats{} {
        initializeMemoryPools();
    }

    GCAllocator::~GCAllocator() {
        destroyMemoryPools();
    }

    void GCAllocator::setGarbageCollector(GarbageCollector* collector) {
        gc = collector;
    }

    void GCAllocator::setLuaState(LuaState* state) {
        luaState = state;
    }

    void GCAllocator::initializeMemoryPools() {
        // 初始化小对象内存池
        size_t poolSizes[] = {16, 32, 64, 128, 192, 256, 384, 512};
        for (int i = 0; i < 8; ++i) {
            smallObjectPools[i].blockSize = poolSizes[i];
            smallObjectPools[i].totalBlocks = 0;
            smallObjectPools[i].usedBlocks = 0;
            smallObjectPools[i].freeList = nullptr;
        }
    }

    void GCAllocator::destroyMemoryPools() {
        // 清理内存池（简化实现）
        for (int i = 0; i < 8; ++i) {
            smallObjectPools[i].freeList = nullptr;
            smallObjectPools[i].totalBlocks = 0;
            smallObjectPools[i].usedBlocks = 0;
        }
    }

    size_t GCAllocator::getPoolIndex(size_t size) const {
        // 根据大小选择合适的内存池
        if (size <= 16) return 0;
        if (size <= 32) return 1;
        if (size <= 64) return 2;
        if (size <= 128) return 3;
        if (size <= 192) return 4;
        if (size <= 256) return 5;
        if (size <= 384) return 6;
        return 7;  // 最大的小对象池
    }

    void* GCAllocator::allocateSmallObject(size_t size) {
        // 简化的小对象分配实现
        // 在完整实现中，这里会使用内存池
        void* ptr = std::malloc(alignSize(size));
        if (ptr) {
            updateMemoryStats(size, true);
        }
        return ptr;
    }

    void* GCAllocator::allocateLargeObject(size_t size) {
        // 大对象直接使用系统分配器
        void* ptr = std::malloc(alignSize(size));
        if (ptr) {
            updateMemoryStats(size, true);
        }
        return ptr;
    }

    void GCAllocator::deallocateSmallObject(void* ptr, size_t size) {
        if (ptr) {
            updateMemoryStats(size, false);
            std::free(ptr);
        }
    }

    void GCAllocator::deallocateLargeObject(void* ptr, size_t size) {
        if (ptr) {
            updateMemoryStats(size, false);
            std::free(ptr);
        }
    }

    void* GCAllocator::allocateRaw(size_t size, GCObjectType type, bool isGCObject) {
        if (size == 0) return nullptr;

        // 检查内存限制
        if (!checkMemoryLimits(size)) {
            return nullptr;
        }

        // 大小对象分离策略
        void* ptr = nullptr;
        if (size <= SMALL_OBJECT_THRESHOLD) {
            ptr = allocateSmallObject(size);
        } else {
            ptr = allocateLargeObject(size);
        }

        // 检查是否需要触发GC
        if (ptr) {
            triggerGCIfNeeded();
        }

        return ptr;
    }

    void GCAllocator::deallocate(void* ptr, size_t size) {
        if (!ptr) return;

        // 大小对象分离策略
        if (size <= SMALL_OBJECT_THRESHOLD) {
            deallocateSmallObject(ptr, size);
        } else {
            deallocateLargeObject(ptr, size);
        }
    }

    void* GCAllocator::reallocate(void* ptr, size_t oldSize, size_t newSize) {
        if (newSize == 0) {
            deallocate(ptr, oldSize);
            return nullptr;
        }

        if (!ptr) {
            return allocateRaw(newSize);
        }

        // 简化的重新分配实现
        void* newPtr = std::realloc(ptr, alignSize(newSize));
        if (newPtr) {
            // 更新统计信息
            if (newSize > oldSize) {
                updateMemoryStats(newSize - oldSize, true);
            } else if (oldSize > newSize) {
                updateMemoryStats(oldSize - newSize, false);
            }
        }

        return newPtr;
    }

    void GCAllocator::updateMemoryStats(size_t size, bool isAllocation) {
        if (isAllocation) {
            stats.totalAllocated += size;
            stats.currentUsage += size;
            stats.allocationCount++;

            // 更新峰值使用量
            if (stats.currentUsage > stats.peakUsage) {
                stats.peakUsage = stats.currentUsage;
            }

            totalAllocated += size;
        } else {
            stats.totalDeallocated += size;
            stats.currentUsage = (stats.currentUsage >= size) ?
                                 (stats.currentUsage - size) : 0;
            stats.deallocationCount++;
        }
    }

    bool GCAllocator::isMemoryPressureHigh() const {
        // 内存压力检查：当前使用量超过阈值的80%
        return stats.currentUsage > (gcThreshold * 4 / 5);
    }

    void GCAllocator::triggerGCIfNeeded() {
        if (shouldTriggerGC() && gc) {
            // 触发垃圾收集
            gc->collectGarbage();
        }
    }

    bool GCAllocator::checkMemoryLimits(size_t size) const {
        // 检查是否会超过内存限制
        return (stats.currentUsage + size) <= (gcThreshold * 2);
    }

    void GCAllocator::updateStats(int delta) {
        if (delta > 0) {
            totalAllocated += delta;
        }
    }

    bool GCAllocator::shouldTriggerGC() const {
        return isMemoryPressureHigh() || (stats.currentUsage > gcThreshold);
    }

    size_t GCAllocator::getTotalAllocated() const { return totalAllocated; }
    size_t GCAllocator::getGCThreshold() const { return gcThreshold; }
    void GCAllocator::setGCThreshold(size_t threshold) {
        gcThreshold = threshold;
        stats.gcThreshold = threshold;
    }

    GCAllocator* GCAllocator::getInstance() {
        if (!g_gcAllocator) g_gcAllocator = new GCAllocator();
        return g_gcAllocator;
    }

    void GCAllocator::destroyInstance() {
        delete g_gcAllocator;
        g_gcAllocator = nullptr;
    }

    // Lua 5.1兼容的全局内存分配函数实现

    /**
     * @brief 核心内存重新分配函数 - 对应官方luaM_realloc_
     * @param L Lua状态
     * @param block 原始内存块
     * @param oldsize 原始大小
     * @param size 新大小
     * @return 重新分配的内存块指针
     */
    void* luaM_realloc_(LuaState* L, void* block, size_t oldsize, size_t size) {
        GCAllocator* allocator = GCAllocator::getInstance();

        if (size == 0) {
            // 释放内存
            if (block) {
                allocator->deallocate(block, oldsize);
            }
            return nullptr;
        }

        if (!block) {
            // 新分配
            return allocator->allocateRaw(size);
        }

        // 重新分配
        return allocator->reallocate(block, oldsize, size);
    }

    /**
     * @brief 内存过大错误处理 - 对应官方luaM_toobig
     * @param L Lua状态
     * @return 总是返回nullptr
     */
    void* luaM_toobig(LuaState* L) {
        // 在官方Lua中，这里会抛出"not enough memory"错误
        // 简化实现：直接返回nullptr
        (void)L;  // 避免未使用参数警告
        return nullptr;
    }

    /**
     * @brief 辅助增长函数 - 对应官方luaM_growaux_
     * @param L Lua状态
     * @param block 原始内存块
     * @param size 当前大小指针
     * @param size_elem 元素大小
     * @param limit 最大限制
     * @param errormsg 错误消息
     * @return 重新分配的内存块指针
     */
    void* luaM_growaux_(LuaState* L, void* block, int* size, size_t size_elem,
                        int limit, const char* errormsg) {
        void* newblock;
        int newsize;

        if (*size >= limit/2) {  // 不能翻倍？
            if (*size >= limit) {  // 已经达到限制？
                return luaM_toobig(L);
            }
            newsize = limit;  // 仍然有空间
        } else {
            newsize = (*size) * 2;
            if (newsize < 4) {
                newsize = 4;  // 最小大小
            }
        }

        newblock = luaM_reallocv(L, block, *size, newsize, size_elem);
        if (newblock == nullptr) {
            return luaM_toobig(L);
        }

        *size = newsize;
        return newblock;
    }

}
