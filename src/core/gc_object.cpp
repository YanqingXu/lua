/**
 * @file gc_object.cpp
 * @brief GCObject类的实现文件
 * 
 * 当前阶段：基础实现
 * 大部分功能已在头文件中通过内联实现，此文件预留用于后续添加：
 * - 复杂的GC算法实现
 * - 调试和统计功能
 * - 对象遍历和验证
 */

#include "core/gc_object.hpp"
#include "gc/garbage_collector.hpp"

namespace Lua {

GCObject::~GCObject() {
    if (GarbageCollector* owner = getOwnerCollector()) {
        owner->unregisterObject(this);
    }
}

} // namespace Lua

