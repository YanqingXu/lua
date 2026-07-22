/**
 * @file garbage_collector.hpp
 * @brief Lua垃圾回收器：三色标记-清除算法实现
 *
 * 本文件实现了 Lua 的垃圾回收系统，采用三色标记-清除算法管理所有垃圾回收对象的生命周期。
 *
 * 核心功能：
 * - 管理所有垃圾回收对象（字符串、表等）
 * - 三色标记算法（白色、灰色、黑色）
 * - 标记-清除垃圾回收
 * - 根对象保护
 * - 内存统计
 *
 * 设计特点：
 * - 由全局状态显式拥有；保留兼容入口供旧代码使用
 * - 链表管理：使用侵入式链表管理所有垃圾回收对象
 * - 增量准备：为后续增量垃圾回收预留接口
 * - 现代 C++：使用资源获取即初始化和智能指针辅助管理
 * @author Lua C++ 项目
 * @date 2025-11-12
 */

#pragma once

#include "common/types.hpp"
#include "core/gc_object.hpp"
#include "runtime/lua_allocator.hpp"
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace Lua {

// 前向声明
class GCString;
class StringPool;
class Table;
class LuaState;
class Userdata;
class Value;
class GlobalState;
class GCStrategy;
class MarkSweepGC;
class IncrementalGC;

/**
 * @brief 垃圾回收器类
 *
 * 管理所有垃圾回收对象的生命周期，实现三色标记-清除算法。
 *
 * 三色标记算法：
 * - 白色：未访问的对象，可能是垃圾
 * - 灰色：已访问但未扫描的对象，待处理
 * - 黑色：已访问且已扫描的对象，确定存活
 *
 * 垃圾回收流程：
 * 1. 标记阶段：从根对象开始，标记所有可达对象
 * 2. 清除阶段：回收所有未标记（白色）的对象
 *
 * 使用示例：
 * @code
 * GarbageCollector& gc = L->getGlobalState().getGC();
 *
 * // 创建对象时注册到GC
 * GCString* str = new GCString("hello");
 * gc.registerObject(str);
 *
 * // 添加根对象（保护不被回收）
 * gc.addRoot(str);
 *
 * // 执行垃圾回收
 * gc.collect();
 *
 * // 移除根对象
 * gc.removeRoot(str);
 * @endcode
 */
class GarbageCollector {
public:
    // =====================================================================
    // 构造和兼容入口
    // =====================================================================

    /**
     * @brief 构造独立的GC实例
     */
    explicit GarbageCollector(LuaAllocator* allocator = nullptr);

    /**
     * @brief 获取旧的兼容GC实例
     *
     * 新代码应优先使用 GlobalState::getGC() 或 RuntimeServices::gc。
     */
    [[deprecated("Use GlobalState::getGC() or RuntimeServices::gc instead")]]
    static GarbageCollector& getInstance();

    // 禁止拷贝和赋值
    GarbageCollector(const GarbageCollector&) = delete;
    GarbageCollector& operator=(const GarbageCollector&) = delete;

    /**
     * @brief 析构函数
     *
     * 清理所有GC对象，释放内存。
     */
    ~GarbageCollector();

    // =====================================================================
    // 对象管理
    // =====================================================================

    /**
     * @brief 注册GC对象
     *
     * 将新创建的GC对象添加到GC管理链表中。
     *
     * @param obj GC对象指针
     */
    void registerObject(GCObject* obj);

    /**
     * @brief 从GC管理链表中摘除对象
     *
 * 主要用于尚未提交的资源获取即初始化守卫放弃垃圾回收器托管。该函数不释放对象本身。
     */
    void unregisterObject(GCObject* obj) noexcept;

    /**
     * @brief 将对象动态存储量与快速路径内存总量对账
     */
    void accountObjectSizeChange(GCObject* obj) noexcept;

    /**
     * @brief 通过原始分配路径销毁已注册对象
     *
     * 供回滚守卫在对象成为完整函数原型或对象图的一部分前放弃该对象时使用。
     */
    void destroyManagedObject(GCObject* obj) noexcept;

    template <typename T, typename... Args> [[nodiscard]] T* create(Args&&... args) {
        return createManaged<T>(false, false, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args> [[nodiscard]] T* createRoot(Args&&... args) {
        return createManaged<T>(true, false, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args> [[nodiscard]] T* createFixedRoot(Args&&... args) {
        return createManaged<T>(true, true, std::forward<Args>(args)...);
    }

    /**
     * @brief 添加根对象
     *
     * 根对象不会被GC回收，通常是全局变量、栈上的对象等。
     *
     * @param obj 根对象指针
     */
    void addRoot(GCObject* obj);

    /**
     * @brief 移除根对象
     *
     * 从根对象集合中移除对象，使其可以被GC回收。
     *
     * @param obj 根对象指针
     */
    void removeRoot(GCObject* obj);

    /**
     * @brief 检查对象是否为根对象
     *
     * @param obj 对象指针
     * @return 如果是根对象返回true，否则返回false
     */
    bool isRoot(GCObject* obj) const;

    // =====================================================================
    // 垃圾回收
    // =====================================================================

    /**
     * @brief 执行完整的垃圾回收
     *
     * 执行标记-清除算法，回收所有不可达对象。
     *
     * 流程：
     * 1. 重置所有对象为白色
     * 2. 标记所有根对象为灰色
     * 3. 传播标记：处理所有灰色对象
     * 4. 清除所有白色对象
     *
     * @return 回收的对象数量
     */
    [[nodiscard]] usize collect();

    /**
     * @brief 使用显式字符串驻留池执行完整垃圾回收
     *
     * 字符串驻留池负责驻留表，清扫阶段删除字符串时必须从同一个池中摘除。
     */
    [[nodiscard]] usize collect(StringPool& stringPool);

    /**
     * @brief 执行完整的垃圾回收，并将当前LuaState作为执行根
     *
     * collectgarbage("collect") 应使用此入口，这样当前线程栈、主线程栈
     * 和全局状态中的共享根都会参与标记。
     *
     * @param currentState 当前执行 collectgarbage 的 LuaState
     * @return 回收的对象数量
     */
    [[nodiscard]] usize collect(LuaState* currentState);

    /**
     * @brief 使用显式字符串驻留池执行完整垃圾回收，并将当前 Lua 状态作为执行根
     */
    [[nodiscard]] usize collect(StringPool& stringPool, LuaState* currentState);

    /**
     * @brief VM 分配路径使用的自动回收入口
     *
     * 自动回收会执行标记、弱表清理和清扫，但会延后运行 userdata
     * `__gc` 终结器，避免在任意字节码指令中重入 Lua 调用栈。
     */
    [[nodiscard]] usize collectAutomatic(LuaState* currentState);

    /**
     * @brief 使用显式字符串驻留池执行自动回收
     */
    [[nodiscard]] usize collectAutomatic(StringPool& stringPool, LuaState* currentState);

    /**
     * @brief VM 粗粒度自动垃圾回收使用的分配与写屏障入口
     */
    [[nodiscard]] usize maybeCollectAutomatic(LuaState* currentState);

    void stopAutomatic() noexcept;
    void restartAutomatic() noexcept;
    [[nodiscard]] bool isAutomaticStopped() const noexcept;
    [[nodiscard]] bool step(LuaState* currentState, i32 size);

    [[nodiscard]] i32 getPause() const noexcept;
    [[nodiscard]] i32 setPause(i32 pause) noexcept;
    [[nodiscard]] i32 getStepMultiplier() const noexcept;
    [[nodiscard]] i32 setStepMultiplier(i32 stepMultiplier) noexcept;
    [[nodiscard]] isize getDebtBytes() const noexcept;
    [[nodiscard]] usize getAutomaticThresholdBytes() const noexcept;
    /**
     * @brief TestC 托管大小预算；并非分配器存活字节数或宿主硬限制
     */
    [[nodiscard]] usize getManagedMemoryBudgetBytes() const noexcept;
    usize setManagedMemoryBudgetBytes(usize limit) noexcept;
    [[nodiscard]] bool canAccountManagedBytes(usize additionalBytes = 0) const noexcept;

    /**
     * @brief 获取当前 GC 策略对象
     */
    [[nodiscard]] const GCStrategy& getStrategy() const noexcept;

    /**
     * @brief 获取当前 GC 策略名称
     */
    [[nodiscard]] const char* getStrategyName() const noexcept;

    /**
     * @brief 按名称切换 GC 策略；未知名称返回 false 并保持原策略
     */
    bool useStrategy(StrView name) noexcept;

    /**
     * @brief 标记阶段
     *
     * 从根对象开始，标记所有可达对象。
     */
    void mark();

    /**
     * @brief 标记阶段，并额外扫描当前执行状态根集
     * @param currentState 当前执行状态；nullptr 时只扫描显式 roots_
     */
    void mark(LuaState* currentState);

    /**
     * @brief 清除阶段
     *
     * 回收所有未标记（白色）的对象。
     *
     * @return 回收的对象数量
     */
    usize sweep(StringPool& stringPool);

    /**
     * @brief 标记单个对象
     *
     * 该方法是所有子对象报告引用关系的统一入口。只有白色对象会进入灰色队列。
     */
    void markObject(GCObject* obj);

    /**
     * @brief 标记一个 Lua Value 中包含的 GCObject
     */
    void markValue(const Value& value);

    /**
     * @brief 保守的增量垃圾回收写屏障
     *
     * 当黑色所有者开始引用白色子对象时，立即标记并传播子图，避免同一周期后续清扫回收新近
     * 可达的对象。
     */
    void writeBarrier(GCObject* owner, GCObject* child);

    /**
     * @brief writeBarrier() 的 Value 重载
     */
    void writeBarrier(GCObject* owner, const Value& value);

    /**
     * @brief 无异常状态转换使用的免分配写屏障
     *
     * 上值关闭可能在栈展开或状态销毁期间运行。将白色子对象排入队列供稍后传播，且不执行分配；
     * 若增量队列不变量意外失效，则在该周期进入清扫前将其放弃。
     */
    void writeBarrierDeferredNoexcept(GCObject* owner, const Value& value) noexcept;

    /**
     * @brief GlobalState 辅助表等非垃圾回收根使用的写屏障
     */
    void writeRootBarrier(GCObject* child);

    /**
     * @brief 标记 Lua 状态中的活动栈、调用帧窗口和开放上值
     */
    void markState(LuaState* state);

    /**
     * @brief 标记表对象，并按 __mode 应用弱键/弱值语义
     */
    void markTable(Table* table);

    /**
     * @brief 检查对象是否会在当前清扫阶段被回收
     */
    bool isObjectDead(GCObject* obj) const;

    /**
     * @brief 检查值中的可回收对象是否会在当前清扫阶段被回收
     */
    bool isValueDead(const Value& value) const;

    /**
     * @brief 检查弱值槽位是否应被清理
     */
    bool isWeakValueDead(const Value& value) const;

    // =====================================================================
    // 统计信息
    // =====================================================================

    /**
     * @brief 获取当前管理的对象总数
     * @return 对象数量
     */
    usize getObjectCount() const noexcept;

    /**
     * @brief 获取根对象数量
     * @return 根对象数量
     */
    usize getRootCount() const noexcept;

    /**
     * @brief 垃圾回收对象报告的托管大小总和
     *
     * 此值并非分配器的精确存活字节数；对象载荷与实现或容器元数据可能采用不同计量方式。
     */
    usize getTotalMemory() const noexcept;

    /**
     * @brief 自动垃圾回收步调控制使用的 O(1) 内存总量
     *
     * getTotalMemory() 仍会精确遍历对象报告的大小；本访问器公开独立维护的快速路径托管大小
     * 账本，供测试与运行时遥测使用。
     */
    usize getAccountedMemory() const noexcept;

    /**
     * @brief 获取GC统计信息
     *
     * @param outObjectCount 输出：对象总数
     * @param outRootCount 输出：根对象数量
     * @param outTotalMemory 输出：总内存使用量
     */
    void getStatistics(usize& outObjectCount, usize& outRootCount, usize& outTotalMemory) const noexcept;

    // =====================================================================
    // 调试和测试
    // =====================================================================

    /**
     * @brief 清理所有对象（用于测试）
     *
     * 强制删除所有GC对象，不管是否为根对象。
     * 仅用于测试和程序退出时的清理。
     */
    void clearAll();

    /**
     * @brief 使用显式字符串驻留池清理所有对象（用于测试）
     */
    void clearAll(StringPool& stringPool);

    /**
     * @brief 关闭期间运行仍获准执行的用户数据终结器
     *
     * 关闭流程也会访问可达用户数据。现有队列就地消费，使 lua_close 在分配器失败时仍不抛出
     * 异常。终结器错误会限制在边界内；有限的单轮策略预算可以有意停止后续回调。
     */
    void finalizeAll(LuaState* state) noexcept;

    /**
     * @brief 仍排队等待后续 __gc 清理的用户数据对象数
     */
    [[nodiscard]] usize getPendingFinalizerCount() const noexcept {
        return pendingFinalizers_.size();
    }

    /**
     * @brief 打印GC统计信息（调试用）
     */
    void printStatistics() const;

    // =====================================================================
    // 所属全局状态
    // =====================================================================

    void setGlobalState(GlobalState* state) noexcept {
        globalState_ = state;
    }

    GlobalState* getGlobalState() const noexcept {
        return globalState_;
    }

    void setStringPool(StringPool* stringPool) noexcept {
        stringPool_ = stringPool;
    }

    StringPool* getStringPool() const noexcept {
        return stringPool_;
    }

    LuaAllocator* getAllocator() noexcept {
        return allocator_;
    }

    const LuaAllocator* getAllocator() const noexcept {
        return allocator_;
    }

private:
    friend class StringPool;
    friend class MarkSweepGC;
    friend class IncrementalGC;

    template <typename T, typename... Args> [[nodiscard]] T* createManaged(bool root, bool fixed, Args&&... args) {
        static_assert(std::is_base_of_v<GCObject, T>, "GarbageCollector::create<T> requires a GCObject type");

        usize requestedSize = sizeof(T);
        if constexpr (requires { T::getGCAllocationSize(args...); }) {
            requestedSize = static_cast<usize>(T::getGCAllocationSize(args...));
        }

        if (!canAccountManagedBytes(requestedSize)) {
            throw std::bad_alloc();
        }

        if (allocator_ != nullptr && allocator_->isConfigured()) {
            void* memory = allocator_->allocate(sizeof(T));
            if (memory == nullptr) {
                throw std::bad_alloc();
            }

            T* raw = nullptr;
            try {
                if constexpr (std::is_constructible_v<T, LuaAllocator*, Args...>) {
                    raw = std::construct_at(static_cast<T*>(memory), allocator_, std::forward<Args>(args)...);
                } else {
                    raw = std::construct_at(static_cast<T*>(memory), std::forward<Args>(args)...);
                }
            } catch (...) {
                allocator_->deallocate(memory, sizeof(T));
                throw;
            }

            raw->setAllocatorAllocation(allocator_, sizeof(T),
                                        [](GCObject* object) noexcept { std::destroy_at(static_cast<T*>(object)); });

            try {
                registerObject(raw);
                if (fixed) {
                    raw->setMarked(raw->getMarked() | GCBits::FIXED);
                }
                if (root) {
                    addRoot(raw);
                }
            } catch (...) {
                unregisterObject(raw);
                std::destroy_at(raw);
                allocator_->deallocate(raw, sizeof(T));
                throw;
            }

            return raw;
        }

        auto object = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = object.get();
        registerObject(raw);

        try {
            if (fixed) {
                raw->setMarked(raw->getMarked() | GCBits::FIXED);
            }
            if (root) {
                addRoot(raw);
            }
        } catch (...) {
            unregisterObject(raw);
            throw;
        }

        object.release();
        return raw;
    }

    enum class IncrementalPhase : u8 { Pause, Propagate, Atomic, Sweep, Finalize };

    // =====================================================================
    // 内部辅助方法
    // =====================================================================

    static bool valueContainsObject(const Value& value);
    static GCObject* objectFromValue(const Value& value);
    static GarbageCollector& legacyInstance();

    StringPool& stringPoolForCollection(LuaState* currentState) const;
    void destroyObject(GCObject* obj, StringPool& stringPool);
    void releaseObjectMemory(GCObject* obj) noexcept;
    [[nodiscard]] usize collectMarkSweep(StringPool& stringPool, LuaState* currentState);
    [[nodiscard]] usize collectMarkSweep(StringPool& stringPool, LuaState* currentState, bool runFinalizersNow);
    [[nodiscard]] usize collectIncrementalCycle(StringPool& stringPool, LuaState* currentState);
    void resetIncrementalCycle() noexcept;
    usize refreshMemoryAccounting() noexcept;
    void updateAutomaticThresholdAfterCycle() noexcept;
    void beginIncrementalMark(LuaState* currentState);
    [[nodiscard]] usize propagateMarks(usize budget);
    void performIncrementalAtomic(LuaState* currentState);
    void reconcileWeakTableModes();
    [[nodiscard]] usize sweepStep(StringPool& stringPool, usize budget);
    [[nodiscard]] bool incrementalStep(StringPool& stringPool, LuaState* currentState, usize budget);

    /**
     * @brief 传播标记
     *
     * 处理所有灰色对象，将其引用的白色对象标记为灰色，
     * 并将自己标记为黑色。
     */
    void propagateMarks();

    /**
     * @brief 清理所有已标记弱表中的死亡键/值
     */
    void clearWeakTableEntries();

    /**
     * @brief 将带 __gc 的不可达 userdata 复活并加入待终结队列
     */
    void prepareFinalizers();

    /**
     * @brief 解析当前上下文的单轮终结器回调上限
     */
    [[nodiscard]] usize finalizerDrainLimit() const noexcept;

    /**
     * @brief 查询 userdata 的 __gc 元方法
     */
    Value getFinalizer(Userdata* userdata) const;

    /**
     * @brief 运行本轮收集期间排队的终结器
     */
    void runFinalizers(LuaState* state);

    /**
     * @brief 调用一个用户数据终结器并恢复调用者状态
     */
    void callFinalizer(LuaState* state, Userdata* userdata);

    // =====================================================================
    // 数据成员
    // =====================================================================

    /**
     * @brief 所有GC对象的链表头
     */
    GCObject* allObjects_;

    /**
     * @brief 根对象集合（使用vector存储，简单实现）
     */
    LuaVector<GCObject*> roots_;

    /**
     * @brief 灰色对象列表（待处理）
     */
    LuaVector<GCObject*> grayList_;

    /**
     * @brief 本轮标记中发现的弱表
     */
    LuaVector<Table*> weakTables_;

    /**
     * @brief 等待执行 __gc 的 userdata
     */
    LuaVector<Userdata*> pendingFinalizers_;

    /**
     * @brief 本轮标记中已遍历的外部收集器对象
     */
    LuaVector<GCObject*> externalMarked_;

    /**
     * @brief 防止终结器递归执行
     */
    bool finalizersRunning_;

    /**
     * @brief 拥有此GC的全局状态；独立测试实例为空
     */
    GlobalState* globalState_;

    /**
     * @brief 字符串驻留池；用于清扫或全部清理时同步摘除驻留池条目
     */
    StringPool* stringPool_;

    /** @brief 所属 EngineContext 共享的可变 Lua 分配器。 */
    LuaAllocator* allocator_;

    /**
     * @brief 当前垃圾回收策略；默认采用标记-清扫，策略对象本身为静态共享实例
     */
    const GCStrategy* strategy_;

    bool automaticStopped_;
    bool automaticCollectionRunning_;
    bool preciseStackRoots_;
    usize automaticThresholdBytes_;
    /** @brief TestC 与诊断用托管大小故障注入预算；绝非硬限制。 */
    usize managedMemoryBudgetBytes_;
    isize gcDebtBytes_;
    i32 stepCountdown_;
    i32 pause_;
    i32 stepMultiplier_;
    IncrementalPhase incrementalPhase_;
    GCObject* incrementalSweepCurrent_;
    GCObject* incrementalSweepPrevious_;
    usize incrementalCollected_;
    usize lastCompletedCollected_;

    /**
     * @brief 统计信息：对象总数
     */
    usize objectCount_;

    /**
     * @brief 统计信息：总内存使用量
     */
    usize totalMemory_;
};

} // namespace Lua
