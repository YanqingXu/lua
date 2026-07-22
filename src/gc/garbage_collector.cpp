/**
 * @file garbage_collector.cpp
 * @brief 垃圾回收器实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "gc/gc_strategy.hpp"
#include "common/lua_error.hpp"
#include "vm/state/global_state.hpp"
#include "vm/state/lua_state.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <new>

namespace Lua {

namespace {

void addDebt(isize& debt, usize bytes) noexcept {
    const usize maxDelta = static_cast<usize>(std::numeric_limits<isize>::max());
    if (bytes > maxDelta || debt > std::numeric_limits<isize>::max() - static_cast<isize>(bytes)) {
        debt = std::numeric_limits<isize>::max();
        return;
    }
    debt += static_cast<isize>(bytes);
}

void subtractDebt(isize& debt, usize bytes) noexcept {
    const usize maxDelta = static_cast<usize>(std::numeric_limits<isize>::max());
    if (bytes > maxDelta || debt < std::numeric_limits<isize>::min() + static_cast<isize>(bytes)) {
        debt = std::numeric_limits<isize>::min();
        return;
    }
    debt -= static_cast<isize>(bytes);
}

void addLiveBytes(usize& total, usize bytes) noexcept {
    total = bytes > std::numeric_limits<usize>::max() - total ? std::numeric_limits<usize>::max() : total + bytes;
}

void subtractLiveBytes(usize& total, usize bytes) noexcept {
    total = total >= bytes ? total - bytes : 0;
}

usize pausedThreshold(usize liveBytes, i32 pause) noexcept {
    constexpr usize kMinimumThreshold = usize{64} * 1024;
    const usize pausePercent = static_cast<usize>(std::max(100, pause));
    if (liveBytes > std::numeric_limits<usize>::max() / pausePercent) {
        return std::numeric_limits<usize>::max();
    }
    return std::max(kMinimumThreshold, (liveBytes * pausePercent) / 100);
}

usize saturatedStepBytes(u64 scaledKilobytePercent) noexcept {
    constexpr u64 kPercent = 100;
    constexpr usize kBytesPerKilobyte = 1024;
    const u64 wholeKilobytes = scaledKilobytePercent / kPercent;
    const usize remainderBytes = static_cast<usize>((scaledKilobytePercent % kPercent) * kBytesPerKilobyte / kPercent);
    const usize maximum = std::numeric_limits<usize>::max();

    if (wholeKilobytes > static_cast<u64>(maximum / kBytesPerKilobyte)) {
        return maximum;
    }

    const usize wholeBytes = static_cast<usize>(wholeKilobytes) * kBytesPerKilobyte;
    return remainderBytes > maximum - wholeBytes ? maximum : wholeBytes + remainderBytes;
}

} // namespace

// =====================================================================
// 单例模式实现
// =====================================================================

GarbageCollector& GarbageCollector::legacyInstance() {
    static GarbageCollector instance;
    return instance;
}

GarbageCollector& GarbageCollector::getInstance() {
    return legacyInstance();
}

GarbageCollector::GarbageCollector(LuaAllocator* allocator)
    : allObjects_(nullptr), roots_(LuaStdAllocator<GCObject*>(allocator)),
      grayList_(LuaStdAllocator<GCObject*>(allocator)), weakTables_(LuaStdAllocator<Table*>(allocator)),
      pendingFinalizers_(LuaStdAllocator<Userdata*>(allocator)), externalMarked_(LuaStdAllocator<GCObject*>(allocator)),
      finalizersRunning_(false), globalState_(nullptr), stringPool_(nullptr), allocator_(allocator),
      strategy_(&markSweepGCStrategy()), automaticStopped_(false), automaticCollectionRunning_(false),
      preciseStackRoots_(true), automaticThresholdBytes_(usize{64} * 1024),
      managedMemoryBudgetBytes_(std::numeric_limits<usize>::max()), gcDebtBytes_(-static_cast<isize>(64 * 1024)),
      stepCountdown_(0), pause_(200), stepMultiplier_(200), incrementalPhase_(IncrementalPhase::Pause),
      incrementalSweepCurrent_(nullptr), incrementalSweepPrevious_(nullptr), incrementalCollected_(0),
      lastCompletedCollected_(0), objectCount_(0), totalMemory_(0) {}

GarbageCollector::~GarbageCollector() {
    clearAll();

    /**
     * @brief clearAll() 为测试有意保留固定根。
     *
     * 运行时析构也必须释放这些根，使 lua_close 对每个由分配器支撑的对象完成配对释放，
     * 包括保留字符串与注册表。
     */
    StringPool& stringPool = stringPoolForCollection(nullptr);
    while (allObjects_ != nullptr) {
        GCObject* object = allObjects_;
        allObjects_ = object->getNext();
        object->setNext(nullptr);
        destroyObject(object, stringPool);
    }
}

// =====================================================================
// 对象管理
// =====================================================================

void GarbageCollector::registerObject(GCObject* obj) {
    if (obj == nullptr) {
        return;
    }

    if (GarbageCollector* owner = obj->getOwnerCollector(); owner == this) {
        return;
    } else if (owner != nullptr) {
        owner->unregisterObject(obj);
    }

    obj->setColor(incrementalPhase_ == IncrementalPhase::Pause ? GCColor::White : GCColor::Black);
    obj->setOwnerCollector(this);

    // 将对象添加到链表头部
    obj->setNext(allObjects_);
    allObjects_ = obj;

    if (incrementalPhase_ == IncrementalPhase::Sweep && incrementalSweepPrevious_ == nullptr) {
        /**
         * @brief 将清扫期间的新分配视为当前游标保留的前驱。
         *
         * 清扫游标仍指向原链表头；如此处理后，移除旧链表头不会覆盖 allObjects_ 并断开新对象。
         * 清扫期间分配的对象保持黑色并由下一周期访问，符合通常的“分配即黑色”不变量。
         */
        incrementalSweepPrevious_ = obj;
    }

    const usize objectSize = obj->getSize();
    obj->setAccountedSize(objectSize);

    // 更新统计信息
    ++objectCount_;
    addLiveBytes(totalMemory_, objectSize);
    addDebt(gcDebtBytes_, objectSize);

    if (incrementalPhase_ != IncrementalPhase::Pause) {
        try {
        /**
         * @brief 黑色新分配必须像普通写屏障一样发布完整初始对象图。
         *
         * 对象先构造后注册，因此可能已拥有引用，例如 Function 指向 Proto，或已关闭上值指向
         * Value。
         */
            const usize weakTableCountBeforeMark = weakTables_.size();
            obj->mark(*this);
            propagateMarks();
            if (incrementalPhase_ == IncrementalPhase::Sweep && weakTables_.size() > weakTableCountBeforeMark) {
            /**
             * @brief 原子弱引用清理后遇到预填充弱表时，仅针对该异常对象图重启周期。
             *
             * 新发布的预填充弱表可能仍含白色条目；继续清扫会回收其目标却不清除对应槽。无需在
             * 每次分配时都重启。
             */
                resetIncrementalCycle();
            }
        } catch (const std::bad_alloc&) {
            /**
             * @brief 放弃未完成周期后无需继续写屏障记账。
             *
             * 保留成功分配的对象，由下一周期重新着色并扫描整个堆。
             */
            resetIncrementalCycle();
        } catch (...) {
            resetIncrementalCycle();
            unregisterObject(obj);
            throw;
        }
    }
}

void GarbageCollector::unregisterObject(GCObject* obj) noexcept {
    if (obj == nullptr) {
        return;
    }

    GCObject* prev = nullptr;
    GCObject* current = allObjects_;
    bool removed = false;
    while (current != nullptr) {
        GCObject* next = current->getNext();
        if (current == obj) {
            if (prev == nullptr) {
                allObjects_ = next;
            } else {
                prev->setNext(next);
            }
            obj->setNext(nullptr);
            obj->setOwnerCollector(nullptr);
            if (objectCount_ > 0) {
                --objectCount_;
            }
            const usize accountedSize = obj->getAccountedSize();
            subtractLiveBytes(totalMemory_, accountedSize);
            subtractDebt(gcDebtBytes_, accountedSize);
            obj->setAccountedSize(0);
            removed = true;
            break;
        }
        prev = current;
        current = next;
    }

    if (removed || obj->getOwnerCollector() == this) {
        roots_.erase(std::remove(roots_.begin(), roots_.end(), obj), roots_.end());
        grayList_.erase(std::remove(grayList_.begin(), grayList_.end(), obj), grayList_.end());
        if (obj->getType() == GCObjectType::Table) {
            auto* table = static_cast<Table*>(obj);
            weakTables_.erase(std::remove(weakTables_.begin(), weakTables_.end(), table), weakTables_.end());
        } else if (obj->getType() == GCObjectType::Userdata) {
            auto* userdata = static_cast<Userdata*>(obj);
            pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                     pendingFinalizers_.end());
        }
    }
}

void GarbageCollector::accountObjectSizeChange(GCObject* obj) noexcept {
    if (obj == nullptr || obj->getOwnerCollector() != this) {
        return;
    }

    const usize previousSize = obj->getAccountedSize();
    const usize currentSize = obj->getSize();
    if (currentSize > previousSize) {
        const usize growth = currentSize - previousSize;
        addLiveBytes(totalMemory_, growth);
        addDebt(gcDebtBytes_, growth);
    } else if (currentSize < previousSize) {
        const usize shrinkage = previousSize - currentSize;
        subtractLiveBytes(totalMemory_, shrinkage);
        subtractDebt(gcDebtBytes_, shrinkage);
    }
    obj->setAccountedSize(currentSize);
}

void GarbageCollector::addRoot(GCObject* obj) {
    if (obj == nullptr) {
        return;
    }

    // 检查是否已经是根对象
    if (isRoot(obj)) {
        return;
    }

    roots_.push_back(obj);

    if (incrementalPhase_ != IncrementalPhase::Pause) {
        writeRootBarrier(obj);
    }
}

void GarbageCollector::removeRoot(GCObject* obj) {
    if (obj == nullptr) {
        return;
    }

    // 从根对象列表中移除
    auto it = std::find(roots_.begin(), roots_.end(), obj);
    if (it != roots_.end()) {
        roots_.erase(it);
    }
}

bool GarbageCollector::isRoot(GCObject* obj) const {
    if (obj == nullptr) {
        return false;
    }

    return std::find(roots_.begin(), roots_.end(), obj) != roots_.end();
}

// =====================================================================
// 垃圾回收
// =====================================================================

usize GarbageCollector::collect() {
    return collect(stringPoolForCollection(nullptr), nullptr);
}

usize GarbageCollector::collect(StringPool& stringPool) {
    return collect(stringPool, nullptr);
}

/**
 * @brief collect(LuaState*) 的阶段约定
 *
 * 顺序为：标记根 → 安排终结器 → 传播复活对象图 → 清理弱表 → 清扫 → 运行终结器。
 * prepareFinalizers() 必须先于弱引用清理与清扫，因为带 __gc 的不可达用户数据不会立即释放；
 * 它会入队、标记为 FINALIZED 并再次标记，使元表、闭包及终结器路径可达对象在本周期存活。
 * runFinalizers() 有意置于清扫之后：其他白色垃圾已回收，而已终结用户数据仍可复活，且不会
 * 第二次排入 __gc 队列。
 */
usize GarbageCollector::collect(LuaState* currentState) {
    return collect(stringPoolForCollection(currentState), currentState);
}

usize GarbageCollector::collect(StringPool& stringPool, LuaState* currentState) {
    GCContext context{*this, stringPool, currentState};
    return strategy_->collect(context);
}

usize GarbageCollector::collectAutomatic(LuaState* currentState) {
    return collectAutomatic(stringPoolForCollection(currentState), currentState);
}

usize GarbageCollector::collectAutomatic(StringPool& stringPool, LuaState* currentState) {
    if (automaticStopped_ || automaticCollectionRunning_) {
        return 0;
    }

    automaticCollectionRunning_ = true;
    bool previousPreciseStackRoots = preciseStackRoots_;
    preciseStackRoots_ = false;
    usize collected = 0;
    try {
        collected = collectMarkSweep(stringPool, currentState, false);
    } catch (...) {
        resetIncrementalCycle();
        preciseStackRoots_ = previousPreciseStackRoots;
        automaticCollectionRunning_ = false;
        throw;
    }
    preciseStackRoots_ = previousPreciseStackRoots;
    automaticCollectionRunning_ = false;
    const usize liveBytes = refreshMemoryAccounting();
    automaticThresholdBytes_ = pausedThreshold(liveBytes, pause_);
    gcDebtBytes_ = static_cast<isize>(liveBytes) - static_cast<isize>(automaticThresholdBytes_);
    return collected;
}

void GarbageCollector::destroyManagedObject(GCObject* obj) noexcept {
    if (obj == nullptr) {
        return;
    }

    if (obj->getOwnerCollector() == this) {
        unregisterObject(obj);
    }
    if (obj->getType() == GCObjectType::String && stringPool_ != nullptr) {
        stringPool_->remove(static_cast<GCString*>(obj));
    }
    releaseObjectMemory(obj);
}

usize GarbageCollector::maybeCollectAutomatic(LuaState* currentState) {
    if (!canAccountManagedBytes()) {
        throw MemoryError("not enough memory");
    }
    if (automaticStopped_ || automaticCollectionRunning_) {
        return 0;
    }

    /**
     * @brief 避免在 VM 热路径中遍历整个对象链表，并保证已开始的周期能够完成。
     *
     * 此处是 SETTABLE、SETLIST 与全局写入的 VM 热路径。垃圾回收对象进出收集器时会增量维护
     * totalMemory_ 与 gcDebtBytes_；在此遍历整个对象链表会使不断增长的字符串或闭包表退化为
     * 二次时间。阈值仅决定何时开始周期，而不决定是否完成周期。清扫可能让内存与债务同时
     * 降到阈值以下；若此时暂停，游标会永久搁置，且局部清扫期间分配的对象会一直保持黑色，
     * 无法进入新的标记阶段。
     */
    if (incrementalPhase_ == IncrementalPhase::Pause && totalMemory_ < automaticThresholdBytes_ && gcDebtBytes_ <= 0) {
        return 0;
    }

    automaticCollectionRunning_ = true;
    try {
    /**
     * @brief 为自动工作使用 8 KiB 最小时间片。
     *
     * 自动检查点按字节计费，而增量核心当前按完整对象分配预算。公开的 1 KiB 步长仅映射为两个
     * 对象，在普通标准库堆中可能落后数百次分配。显式 collectgarbage("step", 0) 仍保持极小且
     * 可观察，供有意逐阶段驱动的调用者使用。
     */
        constexpr i32 kAutomaticStepKilobytes = 8;
        bool finished = step(currentState, kAutomaticStepKilobytes - 1);
        automaticCollectionRunning_ = false;
        return finished ? lastCompletedCollected_ : 0;
    } catch (...) {
        automaticCollectionRunning_ = false;
        throw;
    }
}

void GarbageCollector::stopAutomatic() noexcept {
    automaticStopped_ = true;
}

void GarbageCollector::restartAutomatic() noexcept {
    automaticStopped_ = false;
    stepCountdown_ = 0;
}

bool GarbageCollector::isAutomaticStopped() const noexcept {
    return automaticStopped_;
}

bool GarbageCollector::step(LuaState* currentState, i32 size) {
    const i32 normalizedSize = std::max(0, size);
    const u64 requestedKilobytes = static_cast<u64>(normalizedSize) + 1;
    const u64 multiplier = static_cast<u64>(std::max(1, stepMultiplier_));
    /**
     * @brief 对百分比缩放后的工作量执行饱和字节转换。
     *
     * 两个因子最大均为 2^31，因此乘积可容纳于 u64。转换到 size_t 前先饱和，使极端 stepmul
     * 下的 INT_MAX 公开步长仍有明确定义。
     */
    const u64 scaledKilobytePercent = requestedKilobytes * multiplier;
    const u64 scaledKilobytes = scaledKilobytePercent / 100;
    const usize scaledBytes = saturatedStepBytes(scaledKilobytePercent);
    const usize budget = static_cast<usize>(
        std::min<u64>(std::max<u64>(1, scaledKilobytes), static_cast<u64>(std::numeric_limits<i32>::max())));

    bool wasStopped = automaticStopped_;
    automaticStopped_ = false;
    StringPool& stringPool = stringPoolForCollection(currentState);

    bool finished = false;
    try {
        if (normalizedSize >= 10000) {
            usize largeBudget = std::max<usize>(getObjectCount() + grayList_.size() + 16, 1024);
            do {
                finished = incrementalStep(stringPool, currentState, largeBudget);
            } while (!finished);
        } else {
            finished = incrementalStep(stringPool, currentState, budget);
        }
    } catch (...) {
        resetIncrementalCycle();
        automaticStopped_ = wasStopped;
        throw;
    }

    subtractDebt(gcDebtBytes_, scaledBytes);
    if (finished) {
        updateAutomaticThresholdAfterCycle();
    }

    automaticStopped_ = wasStopped;
    return finished;
}

i32 GarbageCollector::getPause() const noexcept {
    return pause_;
}

i32 GarbageCollector::setPause(i32 pause) noexcept {
    i32 previous = pause_;
    pause_ = std::max(0, pause);
    return previous;
}

i32 GarbageCollector::getStepMultiplier() const noexcept {
    return stepMultiplier_;
}

i32 GarbageCollector::setStepMultiplier(i32 stepMultiplier) noexcept {
    i32 previous = stepMultiplier_;
    stepMultiplier_ = std::max(0, stepMultiplier);
    stepCountdown_ = 0;
    return previous;
}

isize GarbageCollector::getDebtBytes() const noexcept {
    return gcDebtBytes_;
}

usize GarbageCollector::getAutomaticThresholdBytes() const noexcept {
    return automaticThresholdBytes_;
}

usize GarbageCollector::getManagedMemoryBudgetBytes() const noexcept {
    return managedMemoryBudgetBytes_;
}

usize GarbageCollector::setManagedMemoryBudgetBytes(usize limit) noexcept {
    const usize previous = managedMemoryBudgetBytes_;
    managedMemoryBudgetBytes_ = limit;
    return previous;
}

bool GarbageCollector::canAccountManagedBytes(usize additionalBytes) const noexcept {
    if (managedMemoryBudgetBytes_ == std::numeric_limits<usize>::max()) {
        return true;
    }
    /**
     * @brief 避免受内存限制的 TestC 工作负载在每次 VM 写入时遍历对象链表。
     *
     * 收集器增量维护 totalMemory_ 并在周期边界对账；设置有限但较大的限制后，在此遍历对象
     * 链表会使这些工作负载退化为二次时间。
     */
    const usize liveBytes = totalMemory_;
    return liveBytes <= managedMemoryBudgetBytes_ && additionalBytes <= managedMemoryBudgetBytes_ - liveBytes;
}

const GCStrategy& GarbageCollector::getStrategy() const noexcept {
    return *strategy_;
}

const char* GarbageCollector::getStrategyName() const noexcept {
    return strategy_->name();
}

bool GarbageCollector::useStrategy(StrView name) noexcept {
    Opt<std::reference_wrapper<const GCStrategy>> strategy = findGCStrategy(name);
    if (!strategy.has_value()) {
        return false;
    }

    strategy_ = &strategy->get();
    return true;
}

usize GarbageCollector::collectMarkSweep(StringPool& stringPool, LuaState* currentState) {
    return collectMarkSweep(stringPool, currentState, true);
}

usize GarbageCollector::collectMarkSweep(StringPool& stringPool, LuaState* currentState, bool runFinalizersNow) {
    resetIncrementalCycle();

    // 1. 标记阶段
    /**
     * @brief 在 VM 尚无完整栈映射时扫描全部活动寄存器窗口。
     *
     * LocVar 区间不是完整的存活映射：匿名表达式临时值可跨越对 collectgarbage() 的 C 调用存活。
     * 让一个已死亡弱值多保留一周期，比清扫仍存活的临时值并随后由 SETLIST/RETURN 解引用更安全。
     */
    const bool previousPreciseStackRoots = preciseStackRoots_;
    preciseStackRoots_ = false;
    try {
        mark(currentState);
    } catch (...) {
        preciseStackRoots_ = previousPreciseStackRoots;
        throw;
    }
    preciseStackRoots_ = previousPreciseStackRoots;

    // 2. 带 __gc 的不可达 userdata 需要先复活一轮，并保留其引用图。
    if (currentState != nullptr) {
        prepareFinalizers();
        propagateMarks();
    }

    /** @brief 3. 清理弱表条目，必须在清扫阶段删除白色对象之前执行。 */
    clearWeakTableEntries();

    // 4. 清除阶段
    usize collected = sweep(stringPool);
    weakTables_.clear();

    // 5. 在对象已被复活且本轮垃圾已释放后运行终结器。
    if (currentState != nullptr && runFinalizersNow) {
        runFinalizers(currentState);
    }

    resetIncrementalCycle();

    updateAutomaticThresholdAfterCycle();
    return collected;
}

usize GarbageCollector::collectIncrementalCycle(StringPool& stringPool, LuaState* currentState) {
    usize objectsBefore = getObjectCount();
    usize budget = std::max<usize>(getObjectCount() + grayList_.size() + 16, 1024);
    bool finished = false;
    do {
        finished = incrementalStep(stringPool, currentState, budget);
    } while (!finished);
    usize objectsAfter = getObjectCount();
    usize collected = objectsBefore >= objectsAfter ? objectsBefore - objectsAfter : 0;
    updateAutomaticThresholdAfterCycle();
    return collected;
}

void GarbageCollector::resetIncrementalCycle() noexcept {
    incrementalPhase_ = IncrementalPhase::Pause;
    incrementalSweepCurrent_ = nullptr;
    incrementalSweepPrevious_ = nullptr;
    incrementalCollected_ = 0;
    stepCountdown_ = 0;
    grayList_.clear();
    weakTables_.clear();
    externalMarked_.clear();
}

usize GarbageCollector::refreshMemoryAccounting() noexcept {
    const usize previousTotal = totalMemory_;
    usize currentTotal = 0;
    for (GCObject* obj = allObjects_; obj != nullptr; obj = obj->getNext()) {
        const usize objectSize = obj->getSize();
        obj->setAccountedSize(objectSize);
        addLiveBytes(currentTotal, objectSize);
    }

    totalMemory_ = currentTotal;
    if (currentTotal > previousTotal) {
        addDebt(gcDebtBytes_, currentTotal - previousTotal);
    } else if (currentTotal < previousTotal) {
        subtractDebt(gcDebtBytes_, previousTotal - currentTotal);
    }
    return currentTotal;
}

void GarbageCollector::updateAutomaticThresholdAfterCycle() noexcept {
    const usize liveBytes = refreshMemoryAccounting();
    automaticThresholdBytes_ = pausedThreshold(liveBytes, pause_);
    gcDebtBytes_ = static_cast<isize>(liveBytes) - static_cast<isize>(automaticThresholdBytes_);
}

void GarbageCollector::beginIncrementalMark(LuaState* currentState) {
    /**
     * @brief 改变任何颜色前预留队列容量。
     *
     * 每个本地对象最多进入灰色队列一次，这也保证延迟的无异常写屏障在本周期内无需分配。
     */
    grayList_.reserve(objectCount_);
    weakTables_.reserve(objectCount_);

    GCObject* obj = allObjects_;
    while (obj != nullptr) {
        u8 preserved = obj->getMarked() & (GCBits::FIXED | GCBits::FINALIZED);
        obj->setMarked(preserved);
        obj->setColor(GCColor::White);
        obj = obj->getNext();
    }

    grayList_.clear();
    weakTables_.clear();
    externalMarked_.clear();
    incrementalCollected_ = 0;
    incrementalSweepCurrent_ = nullptr;
    incrementalSweepPrevious_ = nullptr;

    for (GCObject* root : roots_) {
        markObject(root);
    }

    /**
     * @brief 增量根快照使用完整活动寄存器窗口。
     *
     * 自动步进可在任意两条 VM 指令间运行，包括表构造器执行到一半且临时寄存器尚无 LocVar
     * 区间时。精确根仍用于实现弱表可观察性测试的显式收集。
     */
    const bool previousPreciseStackRoots = preciseStackRoots_;
    preciseStackRoots_ = false;
    try {
        if (currentState != nullptr) {
            currentState->getGlobalState().markRoots(*this, currentState);
        } else if (globalState_ != nullptr) {
            globalState_->markRoots(*this, nullptr);
        }
    } catch (...) {
        preciseStackRoots_ = previousPreciseStackRoots;
        throw;
    }
    preciseStackRoots_ = previousPreciseStackRoots;

    for (Userdata* userdata : pendingFinalizers_) {
        markObject(userdata);
    }

    incrementalPhase_ = IncrementalPhase::Propagate;
}

void GarbageCollector::performIncrementalAtomic(LuaState* currentState) {
    /**
     * @brief 在原子边界重新扫描没有写屏障的栈根。
     *
     * 这样可防止 beginIncrementalMark 后安装的值在清扫开始时仍保持白色。
     */
    const bool previousPreciseStackRoots = preciseStackRoots_;
    preciseStackRoots_ = false;
    try {
        for (GCObject* root : roots_) {
            markObject(root);
        }
        if (currentState != nullptr) {
            currentState->getGlobalState().markRoots(*this, currentState);
        } else if (globalState_ != nullptr) {
            globalState_->markRoots(*this, nullptr);
        }
        for (Userdata* userdata : pendingFinalizers_) {
            markObject(userdata);
        }
        propagateMarks();
    } catch (...) {
        preciseStackRoots_ = previousPreciseStackRoots;
        throw;
    }
    preciseStackRoots_ = previousPreciseStackRoots;

    /**
     * @brief 清理条目前重新评估弱表元表的 __mode。
     *
     * 弱表首次扫描后元表仍可变化，因此弱到强的转换必须在同一轮收集中标记新近变强的边。
     */
    reconcileWeakTableModes();
    propagateMarks();

    /**
     * @brief 仅在弱模式变化提升新强边后判断终结器可达性。
     *
     * 否则，本周期内变为强可达的用户数据可能被过早终结。
     */
    if (currentState != nullptr) {
        prepareFinalizers();
        propagateMarks();
    }

    clearWeakTableEntries();
    incrementalSweepCurrent_ = allObjects_;
    incrementalSweepPrevious_ = nullptr;
    incrementalPhase_ = IncrementalPhase::Sweep;
}

usize GarbageCollector::sweepStep(StringPool& stringPool, usize budget) {
    usize collected = 0;
    usize processed = 0;

    while (incrementalSweepCurrent_ != nullptr && processed < budget) {
        GCObject* obj = incrementalSweepCurrent_;
        GCObject* next = obj->getNext();
        bool isFixed = (obj->getMarked() & GCBits::FIXED) != 0;

        if (obj->getColor() == GCColor::White && !isFixed) {
            if (obj->getType() == GCObjectType::Upval && static_cast<Upvalue*>(obj)->isOpen()) {
                incrementalSweepPrevious_ = obj;
                incrementalSweepCurrent_ = next;
                ++processed;
                continue;
            }

            if (incrementalSweepPrevious_ == nullptr) {
                allObjects_ = next;
            } else {
                incrementalSweepPrevious_->setNext(next);
            }

            destroyObject(obj, stringPool);
            ++collected;
            if (incrementalPhase_ != IncrementalPhase::Sweep) {
            /**
             * @brief 析构可能关闭上值并保守地中止周期。
             *
             * 从该重入路径返回后，不得恢复陈旧游标或覆盖已重置的阶段。
             */
                return collected;
            }
        } else {
            obj->setColor(GCColor::White);
            incrementalSweepPrevious_ = obj;
        }

        incrementalSweepCurrent_ = next;
        ++processed;
    }

    incrementalCollected_ += collected;
    if (incrementalSweepCurrent_ == nullptr) {
        weakTables_.clear();
        incrementalPhase_ = IncrementalPhase::Finalize;
    }

    return collected;
}

bool GarbageCollector::incrementalStep(StringPool& stringPool, LuaState* currentState, usize budget) {
    budget = std::max<usize>(1, budget);

    switch (incrementalPhase_) {
    case IncrementalPhase::Pause:
        beginIncrementalMark(currentState);
        return false;

    case IncrementalPhase::Propagate: {
        [[maybe_unused]] const usize propagated = propagateMarks(budget);
        if (grayList_.empty()) {
            incrementalPhase_ = IncrementalPhase::Atomic;
        }
        return false;
    }

    case IncrementalPhase::Atomic:
        performIncrementalAtomic(currentState);
        return false;

    case IncrementalPhase::Sweep: {
        [[maybe_unused]] const usize swept = sweepStep(stringPool, budget);
        return false;
    }

    case IncrementalPhase::Finalize: {
                /**
                 * @brief 调用用户代码前记录已完成的清扫快照。
                 *
                 * 终结器可能同步运行完整收集并重置增量记账；预先记录可让外层自动检查点报告自身
                 * 的收集数量。
                 */
        const usize completedCollected = incrementalCollected_;
        if (currentState != nullptr) {
            runFinalizers(currentState);
        }
        lastCompletedCollected_ = completedCollected;
        resetIncrementalCycle();
        return true;
    }
    }

    lastCompletedCollected_ = incrementalCollected_;
    resetIncrementalCycle();
    return true;
}

StringPool& GarbageCollector::stringPoolForCollection(LuaState* currentState) const {
    if (currentState != nullptr) {
        return currentState->getGlobalState().getStringPool();
    }
    if (stringPool_ != nullptr) {
        return *stringPool_;
    }
    if (globalState_ != nullptr) {
        return globalState_->getStringPool();
    }
    return StringPool::getInstance();
}

void GarbageCollector::destroyObject(GCObject* obj, StringPool& stringPool) {
    if (obj == nullptr) {
        return;
    }

    roots_.erase(std::remove(roots_.begin(), roots_.end(), obj), roots_.end());
    grayList_.erase(std::remove(grayList_.begin(), grayList_.end(), obj), grayList_.end());
    externalMarked_.erase(std::remove(externalMarked_.begin(), externalMarked_.end(), obj), externalMarked_.end());
    if (obj->getType() == GCObjectType::Table) {
        auto* table = static_cast<Table*>(obj);
        weakTables_.erase(std::remove(weakTables_.begin(), weakTables_.end(), table), weakTables_.end());
    } else if (obj->getType() == GCObjectType::Userdata) {
        auto* userdata = static_cast<Userdata*>(obj);
        pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                 pendingFinalizers_.end());
    }

    const usize objSize = obj->getAccountedSize();
    subtractLiveBytes(totalMemory_, objSize);
    subtractDebt(gcDebtBytes_, objSize);
    obj->setAccountedSize(0);
    if (objectCount_ > 0) {
        --objectCount_;
    }

    obj->setNext(nullptr);
    obj->setOwnerCollector(nullptr);

    if (obj->getType() == GCObjectType::String) {
        stringPool.remove(static_cast<GCString*>(obj));
    }

    releaseObjectMemory(obj);
}

void GarbageCollector::releaseObjectMemory(GCObject* obj) noexcept {
    LuaAllocator* allocationAllocator = obj->getAllocationAllocator();
    const usize allocationSize = obj->getAllocationSize();
    GCObject::AllocationDestructor allocationDestructor = obj->getAllocationDestructor();
    if (allocationAllocator != nullptr && allocationDestructor != nullptr) {
        allocationDestructor(obj);
        allocationAllocator->deallocate(obj, allocationSize);
    } else {
        delete obj;
    }
}

// =====================================================================
// 统计信息
// =====================================================================

usize GarbageCollector::getObjectCount() const noexcept {
    return objectCount_;
}

usize GarbageCollector::getRootCount() const noexcept {
    return roots_.size();
}

usize GarbageCollector::getTotalMemory() const noexcept {
    usize total = 0;
    for (GCObject* obj = allObjects_; obj != nullptr; obj = obj->getNext()) {
        addLiveBytes(total, obj->getSize());
    }
    return total;
}

usize GarbageCollector::getAccountedMemory() const noexcept {
    return totalMemory_;
}

void GarbageCollector::getStatistics(usize& outObjectCount, usize& outRootCount, usize& outTotalMemory) const noexcept {
    outObjectCount = objectCount_;
    outRootCount = roots_.size();
    outTotalMemory = getTotalMemory();
}

// =====================================================================
// 调试和测试
// =====================================================================

void GarbageCollector::clearAll() {
    clearAll(stringPoolForCollection(nullptr));
}

void GarbageCollector::clearAll(StringPool& stringPool) {
    resetIncrementalCycle();

    // 清空根对象列表
    roots_.clear();
    grayList_.clear();
    weakTables_.clear();
    pendingFinalizers_.clear();
    externalMarked_.clear();
    if (globalState_ != nullptr) {
        globalState_->resetRuntimeReferencesForClearAll();
    }

    auto deleteMatching = [&](auto shouldDelete) {
        GCObject* prev = nullptr;
        GCObject* obj = allObjects_;
        while (obj != nullptr) {
            GCObject* next = obj->getNext();

            bool isFixed = (obj->getMarked() & GCBits::FIXED) != 0;

            if (!isFixed && shouldDelete(obj)) {
                if (prev == nullptr) {
                    allObjects_ = next;
                } else {
                    prev->setNext(next);
                }

                destroyObject(obj, stringPool);
            } else {
                prev = obj;
            }

            obj = next;
        }
    };

    /**
     * @brief 在通用对象遍历前销毁拥有 LuaState 的线程。
     *
     * LuaState 实例可能仍引用开放上值；先销毁线程可让 LuaState::~LuaState 在 Upvalue 对象仍
     * 存活时关闭这些上值。
     */
    deleteMatching([](GCObject* obj) { return obj->getType() == GCObjectType::Thread; });
    deleteMatching([](GCObject*) { return true; });

    // 清空临时列表
    grayList_.clear();
    weakTables_.clear();
    pendingFinalizers_.clear();
    updateAutomaticThresholdAfterCycle();
}

void GarbageCollector::printStatistics() const {
    std::cout << "GC Statistics:" << std::endl;
    std::cout << "  Total objects: " << objectCount_ << std::endl;
    std::cout << "  Root objects: " << roots_.size() << std::endl;
    std::cout << "  Total memory: " << totalMemory_ << " bytes" << std::endl;
}

} // namespace Lua
