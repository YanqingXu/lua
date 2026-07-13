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
#include "vm/state/global_state.hpp"
#include "vm/state/lua_state.hpp"
#include <algorithm>
#include <iostream>
#include <limits>

namespace Lua {

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
      preciseStackRoots_(true), automaticThresholdBytes_(64 * 1024), gcDebtBytes_(-static_cast<isize>(64 * 1024)),
      stepCountdown_(0), pause_(200), stepMultiplier_(200), incrementalPhase_(IncrementalPhase::Pause),
      incrementalSweepCurrent_(nullptr), incrementalSweepPrevious_(nullptr), incrementalCollected_(0), objectCount_(0),
      totalMemory_(0) {}

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

    // 更新统计信息
    ++objectCount_;
    const usize objectSize = obj->getSize();
    totalMemory_ += objectSize;
    gcDebtBytes_ += static_cast<isize>(objectSize);
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
            totalMemory_ = 0;
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
    usize collected = collectMarkSweep(stringPool, currentState, false);
    preciseStackRoots_ = previousPreciseStackRoots;
    automaticCollectionRunning_ = false;
    const usize liveBytes = getTotalMemory();
    const usize pausePercent = static_cast<usize>(std::max(100, pause_));
    automaticThresholdBytes_ = std::max<usize>(64 * 1024, (liveBytes * pausePercent) / 100 + 32 * 1024);
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
    if (automaticStopped_ || automaticCollectionRunning_) {
        return 0;
    }

    if (getTotalMemory() < automaticThresholdBytes_ && gcDebtBytes_ <= 0) {
        return 0;
    }

    bool finished = step(currentState, 0);
    return finished ? incrementalCollected_ : 0;
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
    const i64 requestedBytes = static_cast<i64>(normalizedSize + 1) * 1024;
    const i64 scaledBytes = (requestedBytes * static_cast<i64>(std::max(1, stepMultiplier_))) / 100;
    const usize budget =
        static_cast<usize>(std::min<i64>(std::max<i64>(1, scaledBytes / 1024), std::numeric_limits<i32>::max()));

    bool wasStopped = automaticStopped_;
    automaticStopped_ = false;
    StringPool& stringPool = stringPoolForCollection(currentState);

    bool finished = false;
    if (normalizedSize >= 10000) {
        usize largeBudget = std::max<usize>(getObjectCount() + grayList_.size() + 16, 1024);
        do {
            finished = incrementalStep(stringPool, currentState, largeBudget);
        } while (!finished);
    } else {
        finished = incrementalStep(stringPool, currentState, budget);
    }

    gcDebtBytes_ -= static_cast<isize>(scaledBytes);
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
    mark(currentState);

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

void GarbageCollector::updateAutomaticThresholdAfterCycle() noexcept {
    const usize liveBytes = getTotalMemory();
    const usize pausePercent = static_cast<usize>(std::max(100, pause_));
    automaticThresholdBytes_ = std::max<usize>(64 * 1024, (liveBytes * pausePercent) / 100 + 32 * 1024);
    gcDebtBytes_ = static_cast<isize>(liveBytes) - static_cast<isize>(automaticThresholdBytes_);
}

void GarbageCollector::beginIncrementalMark(LuaState* currentState) {
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

    if (currentState != nullptr) {
        currentState->getGlobalState().markRoots(*this, currentState);
    } else if (globalState_ != nullptr) {
        globalState_->markRoots(*this, nullptr);
    }

    for (Userdata* userdata : pendingFinalizers_) {
        markObject(userdata);
    }

    incrementalPhase_ = IncrementalPhase::Propagate;
}

void GarbageCollector::performIncrementalAtomic(LuaState* currentState) {
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

    case IncrementalPhase::Finalize:
        if (currentState != nullptr) {
            runFinalizers(currentState);
        }
        resetIncrementalCycle();
        return true;
    }

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

    const usize objSize = obj->getSize();
    totalMemory_ = totalMemory_ >= objSize ? totalMemory_ - objSize : 0;
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
        total += obj->getSize();
    }
    return total;
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
