/**
 * @file gc_weak.cpp
 * @brief 垃圾回收器弱表处理实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "vm/state/global_state.hpp"
#include <algorithm>

namespace Lua {

namespace {

struct WeakMode {
    bool keys = false;
    bool values = false;
};

WeakMode readWeakMode(Table* table, GlobalState* globalState) {
    WeakMode result;
    if (table == nullptr) {
        return result;
    }

    Table* mt = table->getMetatable();
    if (mt == nullptr) {
        return result;
    }

    GlobalState& state = globalState != nullptr ? *globalState : GlobalState::getInstance();
    GCString* modeName = state.getMetamethodName(TMS::TM_MODE);
    Value mode = mt->get(Value(modeName));
    if (mode.isString()) {
        const StrView modeText = mode.asString()->getData();
        result.keys = modeText.find('k') != Str::npos;
        result.values = modeText.find('v') != Str::npos;
    }
    return result;
}

void setWeakBits(Table* table, const WeakMode& mode) {
    u8 marked = table->getMarked() & ~GCBits::WEAKBITS;
    if (mode.keys) {
        marked |= GCBits::WEAKKEY;
    }
    if (mode.values) {
        marked |= GCBits::WEAKVALUE;
    }
    table->setMarked(marked);
}

} // namespace

void GarbageCollector::markTable(Table* table) {
    if (table == nullptr) {
        return;
    }

    const WeakMode mode = readWeakMode(table, globalState_);
    setWeakBits(table, mode);

    if (mode.keys || mode.values) {
        weakTables_.push_back(table);
    }

    table->markContents(*this, mode.keys, mode.values);
}

void GarbageCollector::reconcileWeakTableModes() {
    usize writeIndex = 0;
    const usize originalSize = weakTables_.size();
    for (usize readIndex = 0; readIndex < originalSize; ++readIndex) {
        Table* table = weakTables_[readIndex];
        if (table == nullptr || isObjectDead(table)) {
            continue;
        }

        const WeakMode mode = readWeakMode(table, globalState_);
        setWeakBits(table, mode);

    /**
     * @brief 使用当前模式重新扫描。
     *
     * 若传播期间为弱引用的边在原子阶段前变为强引用，此操作必不可少。
     */
        table->markContents(*this, mode.keys, mode.values);
        if (mode.keys || mode.values) {
            weakTables_[writeIndex++] = table;
        }
    }
    weakTables_.resize(writeIndex);
}

bool GarbageCollector::isObjectDead(GCObject* obj) const {
    if (obj == nullptr) {
        return false;
    }
    if ((obj->getMarked() & GCBits::FIXED) != 0) {
        return false;
    }
    return obj->getColor() == GCColor::White;
}

bool GarbageCollector::isValueDead(const Value& value) const {
    if (value.isString()) {
        return false;
    }
    if (!valueContainsObject(value)) {
        return false;
    }
    return isObjectDead(objectFromValue(value));
}

bool GarbageCollector::isWeakValueDead(const Value& value) const {
    if (value.isString()) {
        return false;
    }
    if (value.isUserdata()) {
        auto* userdata = value.asUserdata();
        if (std::find(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata) != pendingFinalizers_.end()) {
            return true;
        }
    }
    if (!valueContainsObject(value)) {
        return false;
    }
    return isValueDead(value);
}

void GarbageCollector::clearWeakTableEntries() {
    for (Table* table : weakTables_) {
        if (table == nullptr || isObjectDead(table)) {
            continue;
        }

        u8 marked = table->getMarked();
        bool weakKeys = (marked & GCBits::WEAKKEY) != 0;
        bool weakValues = (marked & GCBits::WEAKVALUE) != 0;
        table->removeWeakEntries(*this, weakKeys, weakValues);
    }
}

} // namespace Lua
