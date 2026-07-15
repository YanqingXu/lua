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

    // clearAll() intentionally preserves fixed roots for tests. Runtime
    // destruction must release them as well so lua_close balances every
    // allocator-backed object, including reserved strings and the registry.
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
        // The sweep cursor still points at the former list head.  Treat this
        // allocation as its retained predecessor so removing that old head
        // cannot overwrite allObjects_ and unlink the fresh object.  Objects
        // allocated during Sweep stay black and are visited by the next
        // cycle, matching the usual allocate-black invariant.
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
            // Objects are constructed before registration, so they may
            // already own references (for example Function -> Proto or a
            // closed Upvalue -> Value).  A black allocation must publish
            // that complete initial graph just like a normal write barrier.
            const usize weakTableCountBeforeMark = weakTables_.size();
            obj->mark(*this);
            propagateMarks();
            if (incrementalPhase_ == IncrementalPhase::Sweep && weakTables_.size() > weakTableCountBeforeMark) {
                // Atomic weak cleanup has already run.  A newly published,
                // pre-populated weak table may still contain white entries;
                // continuing this sweep would reclaim their targets without
                // clearing the slots.  Restart only for this exceptional
                // graph shape instead of restarting on every allocation.
                resetIncrementalCycle();
            }
        } catch (const std::bad_alloc&) {
            // Barrier bookkeeping is optional once the unfinished cycle is
            // abandoned.  Keep the successfully allocated object and let the
            // next cycle recolour and scan the whole heap.
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

// collect(LuaState*) phase contract:
//
// The order is mark roots -> schedule finalizers -> propagate the resurrection
// graph -> clean weak tables -> sweep -> run finalizers. `prepareFinalizers()`
// must happen before weak cleanup and sweep because unreachable userdata with
// `__gc` is not freed immediately; it is queued, marked FINALIZED, and marked
// again so metatables, closures, and objects reachable from the finalizer path
// survive this cycle. `runFinalizers()` is intentionally after sweep: other
// white garbage is already reclaimed, while finalized userdata can still be
// resurrected without being queued for `__gc` a second time.
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

    // This is a VM hot path (SETTABLE/SETLIST/global writes). totalMemory_
    // and gcDebtBytes_ are maintained incrementally as GC objects enter and
    // leave the collector; traversing the entire object list here turns a
    // growing table of strings/closures into quadratic runtime.
    // Thresholds decide when to start a cycle, never whether to finish one.
    // Sweep can lower both memory and debt below the threshold; pausing there
    // would strand the cursor indefinitely and make objects allocated during
    // that partial sweep black without ever reaching a fresh mark phase.
    if (incrementalPhase_ == IncrementalPhase::Pause && totalMemory_ < automaticThresholdBytes_ && gcDebtBytes_ <= 0) {
        return 0;
    }

    automaticCollectionRunning_ = true;
    try {
        // Automatic checkpoints are charged in bytes, while the incremental
        // core currently budgets whole objects.  A 1 KiB public step maps to
        // only two objects and can lag hundreds of allocations behind a
        // modest standard-library heap.  Use an 8 KiB minimum quantum for
        // automatic work; explicit collectgarbage("step", 0) remains tiny and
        // observable for callers that intentionally drive individual phases.
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
    // Both factors are at most 2^31, so this product fits in u64. Convert
    // percent-scaled work to bytes separately and saturate before size_t to
    // keep INT_MAX public steps well-defined even with an extreme stepmul.
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
    // Memory-limited TestC workloads exercise this on every VM write. The
    // collector maintains totalMemory_ incrementally and reconciles it at
    // cycle boundaries; an object-list traversal here makes those workloads
    // quadratic once a finite (but large) limit is installed.
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
    // LocVar ranges are not complete liveness maps: anonymous expression
    // temporaries can survive across a C call to collectgarbage(). Scan the
    // complete active register windows until the VM has proper stack maps;
    // retaining a dead weak value for one cycle is safer than sweeping a live
    // temporary and later dereferencing it from SETLIST/RETURN.
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

    // 3. 清理弱表条目。必须在 sweep 删除白色对象之前执行。
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
    // Reserve before changing any colours.  Every local object can enter the
    // gray queue at most once, so this also guarantees that the deferred
    // noexcept write barrier never needs to allocate during this cycle.
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

    // Automatic steps can run between any two VM instructions, including in
    // the middle of a table constructor before its temporary register has a
    // LocVar range. Use the complete active register windows for incremental
    // root snapshots; precise roots remain useful for explicit collections
    // that implement weak-table observability tests.
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
    // Stack roots do not have a write barrier. Rescan them at the atomic
    // boundary so values installed after beginIncrementalMark cannot remain
    // white when sweeping starts.
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

    // A weak table's metatable can change after it was first scanned.
    // Re-evaluate __mode before clearing entries so weak-to-strong transitions
    // mark their newly strong edges in this same collection.
    reconcileWeakTableModes();
    propagateMarks();

    // Finalizer reachability must be decided only after weak-mode changes have
    // promoted newly strong edges.  Otherwise a userdata that became strongly
    // reachable during this cycle could be finalized prematurely.
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
                // Destruction can close upvalues and conservatively abort the
                // cycle.  Do not resurrect the stale cursor or overwrite the
                // reset phase after returning from that reentrant path.
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
        // A finalizer may synchronously run a full collection, which resets
        // the incremental bookkeeping.  Snapshot the completed sweep before
        // invoking user code so the outer automatic checkpoint reports its
        // own collection count.
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

    // Threads own LuaState instances that may still reference open upvalues.
    // Destroy them before the generic object pass so LuaState::~LuaState can
    // close those upvalues while the Upvalue objects are still alive.
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
