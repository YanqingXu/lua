/**
 * @file garbage_collector.cpp
 * @brief 垃圾回收器实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include <iostream>
#include <algorithm>

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
    
    // 将对象添加到链表头部
    obj->setNext(allObjects_);
    allObjects_ = obj;
    
    // 更新统计信息
    ++objectCount_;
    totalMemory_ += obj->getSize();
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
    // 1. 标记阶段
    mark();
    
    // 2. 清除阶段
    usize collected = sweep();
    
    return collected;
}

void GarbageCollector::mark() {
    // 1. 重置所有对象为白色（但保留FIXED等特殊标志）
    GCObject* obj = allObjects_;
    while (obj != nullptr) {
        // 保存FIXED标志
        u8 marked = obj->getMarked();
        bool isFixed = (marked & GCBits::FIXED) != 0;

        // 设置为白色
        obj->setColor(GCColor::White);

        // 恢复FIXED标志
        if (isFixed) {
            obj->setMarked(obj->getMarked() | GCBits::FIXED);
        }

        obj = obj->getNext();
    }

    // 2. 清空灰色列表
    grayList_.clear();

    // 3. 标记所有根对象为灰色
    for (GCObject* root : roots_) {
        if (root != nullptr) {
            markObject(root);
        }
    }

    // 4. 传播标记
    propagateMarks();
}

void GarbageCollector::propagateMarks() {
    // 处理所有灰色对象
    while (!grayList_.empty()) {
        // 取出一个灰色对象
        GCObject* obj = grayList_.back();
        grayList_.pop_back();
        
        // 标记为黑色
        obj->setColor(GCColor::Black);
        
        // 调用对象的mark方法，标记其引用的对象
        obj->mark();
        
        // 注意：obj->mark()内部会调用其引用对象的setColor(Gray)
        // 我们需要将这些灰色对象添加到grayList_
        // 但当前实现中，我们直接在GCObject::mark()中设置颜色
        // 这里需要改进：让mark()方法通知GC添加到灰色列表
        
        // 临时解决方案：遍历所有对象，找出新的灰色对象
        GCObject* current = allObjects_;
        while (current != nullptr) {
            if (current->getColor() == GCColor::Gray) {
                // 检查是否已在灰色列表中
                bool inList = false;
                for (GCObject* gray : grayList_) {
                    if (gray == current) {
                        inList = true;
                        break;
                    }
                }
                if (!inList) {
                    grayList_.push_back(current);
                }
            }
            current = current->getNext();
        }
    }
}

void GarbageCollector::markObject(GCObject* obj) {
    if (obj == nullptr) {
        return;
    }
    
    // 如果已经是灰色或黑色，不需要重复标记
    if (obj->getColor() != GCColor::White) {
        return;
    }
    
    // 标记为灰色
    obj->setColor(GCColor::Gray);
    
    // 添加到灰色列表
    grayList_.push_back(obj);
}

usize GarbageCollector::sweep() {
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
            totalMemory_ -= obj->getSize();
            --objectCount_;
            
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
    return totalMemory_;
}

void GarbageCollector::getStatistics(usize& outObjectCount, usize& outRootCount, usize& outTotalMemory) const noexcept {
    outObjectCount = objectCount_;
    outRootCount = roots_.size();
    outTotalMemory = totalMemory_;
}

// =====================================================================
// 调试和测试
// =====================================================================

void GarbageCollector::clearAll() {
    // 清空根对象列表
    roots_.clear();

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
            totalMemory_ -= obj->getSize();
            --objectCount_;

            delete obj;
        } else {
            // 固定对象，保留它
            prev = obj;
        }

        obj = next;
    }

    // 清空灰色列表
    grayList_.clear();
}

void GarbageCollector::printStatistics() const {
    std::cout << "GC Statistics:" << std::endl;
    std::cout << "  Total objects: " << objectCount_ << std::endl;
    std::cout << "  Root objects: " << roots_.size() << std::endl;
    std::cout << "  Total memory: " << totalMemory_ << " bytes" << std::endl;
}

} // namespace Lua

