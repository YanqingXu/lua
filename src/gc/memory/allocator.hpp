#pragma once

#include "../../common/types.hpp"
#include "../utils/gc_types.hpp"

namespace Lua {
    // 前向声明
    class GarbageCollector;
    class LuaState;
    class GCObject;

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

        // 辅助方法
        bool shouldTriggerGC() const;
        bool checkMemoryLimits(size_t size) const;
        void updateStats(int delta);
        size_t getTotalAllocated() const;
        size_t getGCThreshold() const;
        void setGCThreshold(size_t threshold);
    };

    // 全局实例
    extern GCAllocator* g_gcAllocator;

} // namespace Lua
