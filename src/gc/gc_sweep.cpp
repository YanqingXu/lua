/**
 * @file gc_sweep.cpp
 * @brief 垃圾回收器清扫阶段实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/upvalue.hpp"

namespace Lua {

namespace {

bool isOpenUpvalue(GCObject* obj) {
    return obj != nullptr
        && obj->getType() == GCObjectType::Upval
        && static_cast<Upvalue*>(obj)->isOpen();
}

} // namespace

usize GarbageCollector::sweep(StringPool& stringPool) {
    usize collected = 0;

    auto sweepMatching = [&](auto shouldSweep) {
        usize passCollected = 0;
        GCObject* prev = nullptr;
        GCObject* obj = allObjects_;

        while (obj != nullptr) {
            GCObject* next = obj->getNext();

            if (!shouldSweep(obj)) {
                prev = obj;
                obj = next;
                continue;
            }

            // 检查是否为固定对象（FIXED标记）
            bool isFixed = (obj->getMarked() & GCBits::FIXED) != 0;

            // 如果是白色对象（未标记）且不是固定对象，则回收
            if (obj->getColor() == GCColor::White && !isFixed) {
                if (isOpenUpvalue(obj)) {
                    // Open upvalues point into a LuaState stack. If their
                    // owning thread is also unreachable, thread destruction
                    // closes them before a later pass can reclaim them.
                    prev = obj;
                    obj = next;
                    continue;
                }

                // 从链表中移除
                if (prev == nullptr) {
                    allObjects_ = next;
                } else {
                    prev->setNext(next);
                }

                destroyObject(obj, stringPool);
                ++passCollected;

                // prev不变，因为当前对象已删除
            } else {
                // 保留对象，重置为白色（为下次GC准备）
                obj->setColor(GCColor::White);
                prev = obj;
            }

            obj = next;
        }

        return passCollected;
    };

    // Threads own LuaState stacks that may still be referenced by open
    // upvalues. Reclaim unreachable threads before the generic object pass so
    // LuaState::~LuaState() closes those upvalues while they are still alive.
    collected += sweepMatching([](GCObject* obj) {
        return obj->getType() == GCObjectType::Thread;
    });
    collected += sweepMatching([](GCObject* obj) {
        return obj->getType() != GCObjectType::Thread;
    });
    
    return collected;
}

} // namespace Lua
