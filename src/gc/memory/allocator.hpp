#pragma once

#include "../../common/types.hpp"
#include "../utils/gc_types.hpp"
#include <memory>
#include <optional>
#include <cstddef>

namespace Lua {
    // 前向声明
    class GarbageCollector;
    class LuaState;
    class GCObject;

    // Lua 5.1兼容的内存错误消息
    constexpr const char* MEMERRMSG = "not enough memory";

    // 内存分配结果类型 - 使用现代C++特性
    template<typename T>
    using AllocResult = std::optional<std::unique_ptr<T>>;

    // 内存分配统计信息
    struct MemoryStats {
        size_t totalAllocated = 0;
        size_t totalDeallocated = 0;
        size_t currentUsage = 0;
        size_t peakUsage = 0;
        size_t gcThreshold = 0;
        size_t allocationCount = 0;
        size_t deallocationCount = 0;
    };

    // Lua 5.1兼容的内存分配宏定义

    /**
     * @brief 安全的向量重新分配宏 - 对应官方luaM_reallocv
     * @param L Lua状态
     * @param b 原始内存块
     * @param on 原始元素数量
     * @param n 新元素数量
     * @param e 元素大小
     */
    #define luaM_reallocv(L,b,on,n,e) \
        ((static_cast<size_t>((n)+1) <= SIZE_MAX/(e)) ? \
            luaM_realloc_(L, (b), (on)*(e), (n)*(e)) : \
            luaM_toobig(L))

    /**
     * @brief 释放内存宏 - 对应官方luaM_freemem
     */
    #define luaM_freemem(L, b, s) luaM_realloc_(L, (b), (s), 0)

    /**
     * @brief 释放单个对象宏 - 对应官方luaM_free
     */
    #define luaM_free(L, b) luaM_realloc_(L, (b), sizeof(*(b)), 0)

    /**
     * @brief 释放数组宏 - 对应官方luaM_freearray
     */
    #define luaM_freearray(L, b, n, t) luaM_reallocv(L, (b), n, 0, sizeof(t))

    /**
     * @brief 分配内存宏 - 对应官方luaM_malloc
     */
    #define luaM_malloc(L,t) luaM_realloc_(L, nullptr, 0, (t))

    /**
     * @brief 分配新对象宏 - 对应官方luaM_new
     */
    #define luaM_new(L,t) static_cast<t*>(luaM_malloc(L, sizeof(t)))

    /**
     * @brief 分配新向量宏 - 对应官方luaM_newvector
     */
    #define luaM_newvector(L,n,t) \
        static_cast<t*>(luaM_reallocv(L, nullptr, 0, n, sizeof(t)))

    /**
     * @brief 增长向量宏 - 对应官方luaM_growvector
     */
    #define luaM_growvector(L,v,nelems,size,t,limit,e) \
        do { \
            if ((nelems)+1 > (size)) { \
                (v) = static_cast<t*>(luaM_growaux_(L,v,&(size),sizeof(t),limit,e)); \
            } \
        } while(0)

    /**
     * @brief 重新分配向量宏 - 对应官方luaM_reallocvector
     */
    #define luaM_reallocvector(L, v,oldn,n,t) \
        ((v) = static_cast<t*>(luaM_reallocv(L, v, oldn, n, sizeof(t))))

    // Lua 5.1兼容的核心内存分配函数声明
    void* luaM_realloc_(LuaState* L, void* block, size_t oldsize, size_t size);
    void* luaM_toobig(LuaState* L);
    void* luaM_growaux_(LuaState* L, void* block, int* size, size_t size_elem,
                        int limit, const char* errormsg);

    /**
     * @brief 极简化的内存分配器 - 专注于编译成功
     *
     * 这是一个最小化实现，只包含必要的方法来确保编译成功。
     * 复杂功能将在编译成功后逐步添加。
     */
    class GCAllocator {
    private:
        // 基本成员变量
        size_t totalAllocated;
        size_t gcThreshold;
        GarbageCollector* gc;
        LuaState* luaState;

        // 大小对象分离策略相关
        static constexpr size_t SMALL_OBJECT_THRESHOLD = 256;  // 小对象阈值
        static constexpr size_t LARGE_OBJECT_THRESHOLD = 4096; // 大对象阈值

        // 内存统计信息
        MemoryStats stats;

        // 内存池管理（简化实现）
        struct MemoryPool {
            size_t blockSize;
            size_t totalBlocks;
            size_t usedBlocks;
            void* freeList;
        };

        // 小对象内存池
        MemoryPool smallObjectPools[8];  // 不同大小的小对象池

        // 内存对齐辅助函数
        static constexpr size_t alignSize(size_t size, size_t alignment = sizeof(void*)) {
            return (size + alignment - 1) & ~(alignment - 1);
        }

    public:
        // 静态方法 - 移到前面
        static GCAllocator* getInstance();
        static void destroyInstance();

        // 构造和析构
        GCAllocator();
        ~GCAllocator();

        // 禁用拷贝
        GCAllocator(const GCAllocator&) = delete;
        GCAllocator& operator=(const GCAllocator&) = delete;

        // 基本方法
        void setGarbageCollector(GarbageCollector* collector);
        void setLuaState(LuaState* state);

        // 核心内存分配方法
        void* allocateRaw(size_t size, GCObjectType type = GCObjectType::String, bool isGCObject = false);
        void deallocate(void* ptr, size_t size);
        void* reallocate(void* ptr, size_t oldSize, size_t newSize);

        // GC对象分配方法 - 简化的stub实现
        template<typename T, typename... Args>
        T* allocateObject(GCObjectType type, Args&&... args) {
            void* ptr = allocateRaw(sizeof(T), type, true);
            if (!ptr) return nullptr;
            return new(ptr) T(std::forward<Args>(args)...);
        }

        // 大小对象分离策略方法
        void* allocateSmallObject(size_t size);
        void* allocateLargeObject(size_t size);
        void deallocateSmallObject(void* ptr, size_t size);
        void deallocateLargeObject(void* ptr, size_t size);

        // 内存池管理方法
        void initializeMemoryPools();
        void destroyMemoryPools();
        size_t getPoolIndex(size_t size) const;

        // 内存统计和监控方法
        void updateMemoryStats(size_t size, bool isAllocation);
        bool isMemoryPressureHigh() const;
        void triggerGCIfNeeded();

        // 现代C++特性支持
        template<typename T>
        AllocResult<T> allocateUnique(size_t count = 1) {
            size_t totalSize = sizeof(T) * count;
            void* ptr = allocateRaw(totalSize, GCObjectType::String, false);
            if (!ptr) {
                return std::nullopt;
            }
            return std::make_unique<T[]>(static_cast<T*>(ptr));
        }

        // 辅助方法
        bool shouldTriggerGC() const;
        bool checkMemoryLimits(size_t size) const;
        void updateStats(int delta);
        size_t getTotalAllocated() const;
        size_t getGCThreshold() const;
        void setGCThreshold(size_t threshold);

        // 获取内存统计信息
        const MemoryStats& getMemoryStats() const { return stats; }
    };

    // 全局实例
    extern GCAllocator* g_gcAllocator;

} // namespace Lua
