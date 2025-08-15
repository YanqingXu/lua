#pragma once

#include "../../common/types.hpp"
#include "gc_object.hpp"
#include "../algorithms/gc_marker.hpp"
#include "../algorithms/gc_sweeper.hpp"
#include "../utils/gc_types.hpp"
#include "string_pool.hpp"

namespace Lua {
    // Forward declarations
    class LuaState;
    
    /**
     * @brief Main garbage collector class
     * 
     * This class implements the tri-color mark-and-sweep garbage collection
     * algorithm for the Lua interpreter.
     */
    class GarbageCollector {
    private:
        LuaState* luaState;
        
        // GC algorithm components
        GCMarker marker;
        GCSweeper sweeper;
        
        // GC state and configuration
        GCState gcState;
        GCColor currentWhite;
        GCStats stats;
        GCConfig config;
        
        // Object management
        GCObject* allObjectsHead;
        usize gcThreshold;
        usize totalAllocated;

        // Lua 5.1 compatible GC state management
        GCObject* grayAgainList;     // 需要重新遍历的灰色对象列表
        GCObject* weakList;          // 弱表列表
        usize estimate;              // 内存使用估计
        usize gcdept;                // GC债务
        i32 sweepStringPos;          // 字符串扫描位置
        GCObject** sweepPos;         // 对象扫描位置指针
        
    public:
        explicit GarbageCollector(LuaState* state)
            : luaState(state)
            , marker()
            , sweeper(1024)  // Process up to 1024 objects per incremental step
            , gcState(GCState::Pause)
            , currentWhite(GCColor::White0)
            , stats()
            , config()
            , allObjectsHead(nullptr)
            , gcThreshold(1024 * 1024)  // 1MB default threshold
            , totalAllocated(0)
            , grayAgainList(nullptr)
            , weakList(nullptr)
            , estimate(0)
            , gcdept(0)
            , sweepStringPos(0) {
            // 暂时不初始化sweepPos，避免编译错误
            sweepPos = nullptr;
            stats.reset();
        }
        
        /**
         * @brief Mark an object as reachable
         * @param obj Pointer to the object to mark
         */
        void markObject(GCObject* obj);
        
        /**
         * @brief Perform a full garbage collection cycle
         */
        void collectGarbage();
        
        /**
         * @brief Check if garbage collection should be triggered
         * @return true if GC should run
         */
        bool shouldCollect() const;
        
        /**
         * @brief Register a new object with the GC
         * @param obj Object to register
         */
        void registerObject(GCObject* obj);
        
        /**
         * @brief Update memory allocation statistics
         * @param delta Change in allocated memory (can be negative for deallocation)
         */
        void updateAllocatedMemory(isize delta);
        
        /**
         * @brief Get current GC statistics
         * @return Reference to GC statistics
         */
        const GCStats& getStats() const { return stats; }
        
        /**
         * @brief Get current GC configuration
         * @return Reference to GC configuration
         */
        const GCConfig& getConfig() const { return config; }
        
        /**
         * @brief Update GC configuration
         * @param newConfig New configuration to apply
         */
        void setConfig(const GCConfig& newConfig) { config = newConfig; }

        // === Lua 5.1 Compatible Incremental GC API ===

        /**
         * @brief 执行一步增量GC - 对应官方luaC_step
         * @param L Lua状态
         */
        void step(LuaState* L);

        /**
         * @brief 执行完整GC - 对应官方luaC_fullgc
         * @param L Lua状态
         */
        void fullGC(LuaState* L);

        /**
         * @brief 执行单步GC操作 - 对应官方singlestep
         * @return 处理的内存量
         */
        isize singleStep();

        /**
         * @brief 获取当前GC状态
         * @return GCState 当前状态
         */
        GCState getState() const { return gcState; }

        /**
         * @brief 添加对象到grayagain列表
         * @param obj 需要重新遍历的对象
         */
        void addToGrayAgain(GCObject* obj);

        /**
         * @brief 更新GC阈值 - 对应官方setthreshold
         */
        void updateThreshold();

        /**
         * @brief 获取当前白色标记 - Lua 5.1兼容
         * @return 当前白色标记
         */
        GCColor getCurrentWhite() const { return currentWhite; }

        /**
         * @brief 获取当前白色位 - Lua 5.1兼容
         * @return 当前白色位掩码
         */
        u8 getCurrentWhiteBits() const {
            return (currentWhite == GCColor::White0) ? GCMark::WHITE0 : GCMark::WHITE1;
        }

        /**
         * @brief 获取另一个白色位 - Lua 5.1兼容
         * @return 另一个白色位掩码
         */
        u8 getOtherWhiteBits() const {
            return (currentWhite == GCColor::White0) ? GCMark::WHITE1 : GCMark::WHITE0;
        }

        /**
         * @brief 切换白色 - 用于GC周期切换
         */
        void flipWhite() {
            currentWhite = (currentWhite == GCColor::White0) ? GCColor::White1 : GCColor::White0;
        }

        /**
         * @brief 将对象添加到灰色列表 - 用于写屏障
         * @param obj 要添加的对象
         */
        void addToGrayList(GCObject* obj) {
            if (obj && !GCUtils::isblack(obj) && !GCUtils::iswhite(obj)) {
                // 简化实现：直接标记为灰色
                // 在完整实现中，这里应该将对象添加到灰色队列
                GCUtils::white2gray(obj);
            }
        }

    private:
        /**
         * @brief Collect root objects for marking phase
         * @return Vector of root objects
         */
        std::vector<GCObject*> collectRootObjects();
        
        /**
         * @brief Perform the mark phase of GC
         */
        void markPhase();
        
        /**
         * @brief Perform the sweep phase of GC
         */
        void sweepPhase();
        
        /**
         * @brief Flip white colors for next collection cycle
         */
        void flipWhiteColors();

        // === Incremental GC Private Methods ===

        /**
         * @brief 标记根对象 - 对应官方markroot
         * @return 处理的内存量
         */
        isize markRoot();

        /**
         * @brief 执行一步标记传播 - 对应官方propagatemark
         * @return 处理的内存量
         */
        isize propagateMarkStep();

        /**
         * @brief 原子标记阶段 - 对应官方atomic
         * @return 处理的内存量
         */
        isize atomicStep();

        /**
         * @brief 清理字符串表步骤 - 对应官方sweepwholelist字符串部分
         * @return 处理的内存量
         */
        isize sweepStringStep();

        /**
         * @brief 清理对象步骤 - 对应官方sweeplist
         * @return 处理的内存量
         */
        isize sweepObjectStep();

        /**
         * @brief 终结化步骤 - 对应官方GCTM
         * @return 处理的内存量
         */
        isize finalizeStep();
    };

    // === 全局GC函数声明 ===

    /**
     * @brief 执行一步增量垃圾回收 - 对应官方luaC_step
     * @param L Lua状态
     */
    void luaC_step(LuaState* L);

    // === WriteBarrier命名空间声明 ===

    namespace WriteBarrier {
        /**
         * @brief 前向写屏障 - 确保编译成功
         */
        void barrierForward(LuaState* L, GCObject* parent, GCObject* child);

        /**
         * @brief 后向写屏障 - 确保编译成功
         */
        void barrierBackward(LuaState* L, GCObject* obj);

        /**
         * @brief 检查是否需要写屏障 - 确保编译成功
         */
        bool needsBarrier(GCObject* parent, GCObject* child);
    }
}