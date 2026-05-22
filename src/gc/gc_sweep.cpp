/**
 * @file gc_sweep.cpp
 * @brief 垃圾回收器清扫阶段实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"

namespace Lua {

usize GarbageCollector::sweep(StringPool& stringPool) {
    usize collected = 0;
    GCObject* prev = nullptr;
    GCObject* obj = allObjects_;

    while (obj != nullptr) {
        GCObject* next = obj->getNext();

        // 检查是否为固定对象（FIXED标记）
        bool isFixed = (obj->getMarked() & GCBits::FIXED) != 0;

        // 如果是白色对象（未标记）且不是固定对象，则回收
        if (obj->getColor() == GCColor::White && !isFixed) {
            // 从链表中移除
            if (prev == nullptr) {
                allObjects_ = next;
            } else {
                prev->setNext(next);
            }

            // 更新统计信息
            usize objSize = obj->getSize();
            totalMemory_ = totalMemory_ >= objSize ? totalMemory_ - objSize : 0;
            --objectCount_;
            obj->setOwnerCollector(nullptr);

            if (obj->getType() == GCObjectType::String) {
                stringPool.remove(static_cast<GCString*>(obj));
            }
            
            // 删除对象
            delete obj;
            ++collected;
            
            // prev不变，因为当前对象已删除
        } else {
            // 保留对象，重置为白色（为下次GC准备）
            obj->setColor(GCColor::White);
            prev = obj;
        }
        
        obj = next;
    }
    
    return collected;
}

} // namespace Lua
