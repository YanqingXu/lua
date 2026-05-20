/**
 * @file garbage_collector.cpp
 * @brief 垃圾回收器实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/userdata.hpp"
#include <algorithm>
#include <iostream>

namespace Lua {

// =====================================================================
// 单例模式实现
// =====================================================================

GarbageCollector& GarbageCollector::getInstance() {
    static GarbageCollector instance;
    return instance;
}

GarbageCollector::GarbageCollector()
    : allObjects_(nullptr)
    , roots_()
    , grayList_()
    , weakTables_()
    , pendingFinalizers_()
    , finalizersRunning_(false)
    , objectCount_(0)
    , totalMemory_(0)
{
}

GarbageCollector::~GarbageCollector() {
    clearAll();
}

// =====================================================================
// 对象管理
// =====================================================================

void GarbageCollector::registerObject(GCObject* obj) {
    if (obj == nullptr) {
        return;
    }

    // 防止同一个对象被重复挂入侵入式链表。
    // StringPool::intern() 现在会自动注册字符串，旧代码中仍可能显式注册一次。
    for (GCObject* current = allObjects_; current != nullptr; current = current->getNext()) {
        if (current == obj) {
            return;
        }
    }
    
    obj->setColor(GCColor::White);

    // 将对象添加到链表头部
    obj->setNext(allObjects_);
    allObjects_ = obj;
    
    // 更新统计信息
    ++objectCount_;
    totalMemory_ += obj->getSize();
}

void GarbageCollector::unregisterObject(GCObject* obj) noexcept {
    if (obj == nullptr) {
        return;
    }

    GCObject* prev = nullptr;
    GCObject* current = allObjects_;
    while (current != nullptr) {
        GCObject* next = current->getNext();
        if (current == obj) {
            if (prev == nullptr) {
                allObjects_ = next;
            } else {
                prev->setNext(next);
            }
            obj->setNext(nullptr);
            if (objectCount_ > 0) {
                --objectCount_;
            }
            totalMemory_ = 0;
            break;
        }
        prev = current;
        current = next;
    }

    roots_.erase(std::remove(roots_.begin(), roots_.end(), obj), roots_.end());
    grayList_.erase(std::remove(grayList_.begin(), grayList_.end(), obj), grayList_.end());
    if (obj->getType() == GCObjectType::Table) {
        auto* table = static_cast<Table*>(obj);
        weakTables_.erase(std::remove(weakTables_.begin(), weakTables_.end(), table), weakTables_.end());
    } else if (obj->getType() == GCObjectType::Userdata) {
        auto* userdata = static_cast<Userdata*>(obj);
        pendingFinalizers_.erase(
            std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
            pendingFinalizers_.end());
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
    return collect(nullptr);
}

usize GarbageCollector::collect(LuaState* currentState) {
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
    usize collected = sweep();
    weakTables_.clear();

    // 5. 在对象已被复活且本轮垃圾已释放后运行终结器。
    if (currentState != nullptr) {
        runFinalizers(currentState);
    }
    
    return collected;
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
    // 清空根对象列表
    roots_.clear();
    grayList_.clear();
    weakTables_.clear();
    pendingFinalizers_.clear();

    // 删除所有非固定对象
    GCObject* prev = nullptr;
    GCObject* obj = allObjects_;
    while (obj != nullptr) {
        GCObject* next = obj->getNext();

        // 检查是否为固定对象
        bool isFixed = (obj->getMarked() & GCBits::FIXED) != 0;

        if (!isFixed) {
            // 非固定对象，删除它
            if (prev == nullptr) {
                allObjects_ = next;
            } else {
                prev->setNext(next);
            }

            // 更新统计信息
            usize objSize = obj->getSize();
            totalMemory_ = totalMemory_ >= objSize ? totalMemory_ - objSize : 0;
            --objectCount_;

            if (obj->getType() == GCObjectType::String) {
                StringPool::getInstance().remove(static_cast<GCString*>(obj));
            }

            delete obj;
        } else {
            // 固定对象，保留它
            prev = obj;
        }

        obj = next;
    }

    // 清空临时列表
    grayList_.clear();
    weakTables_.clear();
    pendingFinalizers_.clear();
}

void GarbageCollector::printStatistics() const {
    std::cout << "GC Statistics:" << std::endl;
    std::cout << "  Total objects: " << objectCount_ << std::endl;
    std::cout << "  Root objects: " << roots_.size() << std::endl;
    std::cout << "  Total memory: " << totalMemory_ << " bytes" << std::endl;
}

} // namespace Lua
