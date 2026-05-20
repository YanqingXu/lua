/**
 * @file gc_weak.cpp
 * @brief 垃圾回收器弱表处理实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "vm/global_state.hpp"

namespace Lua {

void GarbageCollector::markTable(Table* table) {
    if (table == nullptr) {
        return;
    }

    bool weakKeys = false;
    bool weakValues = false;
    Table* mt = table->getMetatable();
    if (mt != nullptr) {
        GlobalState& state = globalState_ != nullptr ? *globalState_ : GlobalState::getInstance();
        GCString* modeName = state.getMetamethodName(TMS::TM_MODE);
        Value mode = mt->get(Value(modeName));
        if (mode.isString()) {
            const Str& modeText = mode.asString()->getData();
            weakKeys = modeText.find('k') != Str::npos;
            weakValues = modeText.find('v') != Str::npos;
        }
    }

    u8 marked = table->getMarked() & ~GCBits::WEAKBITS;
    if (weakKeys) {
        marked |= GCBits::WEAKKEY;
    }
    if (weakValues) {
        marked |= GCBits::WEAKVALUE;
    }
    table->setMarked(marked);

    if (weakKeys || weakValues) {
        weakTables_.push_back(table);
    }

    table->markContents(*this, weakKeys, weakValues);
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
    if (!valueContainsObject(value)) {
        return false;
    }
    return isObjectDead(objectFromValue(value));
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
