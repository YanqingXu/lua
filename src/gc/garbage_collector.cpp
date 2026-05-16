/**
 * @file garbage_collector.cpp
 * @brief 垃圾回收器实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/function.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/thread.hpp"
#include "core/userdata.hpp"
#include "core/value.hpp"
#include "core/upvalue.hpp"
#include "vm/global_state.hpp"
#include "vm/lua_state.hpp"
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
    
    // 2. 清除阶段
    usize collected = sweep();
    
    return collected;
}

void GarbageCollector::mark() {
    mark(nullptr);
}

void GarbageCollector::mark(LuaState* currentState) {
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

    // 4. 标记当前执行状态及全局状态中的共享根
    if (currentState != nullptr) {
        currentState->getGlobalState().markRoots(*this, currentState);
    }

    // 5. 传播标记
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
        
        // 调用对象的mark方法，由对象通过gc.markObject/markValue报告引用关系。
        obj->mark(*this);
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

void GarbageCollector::markValue(const Value& value) {
    if (value.isString()) {
        markObject(value.asString());
    } else if (value.isTable()) {
        markObject(value.asTable());
    } else if (value.isFunction()) {
        markObject(value.asFunction());
    } else if (value.isUserdata()) {
        markObject(value.asUserdata());
    } else if (value.isThread()) {
        markObject(value.asThread());
    }
}

void GarbageCollector::markState(LuaState* state) {
    if (state == nullptr) {
        return;
    }

    Stack& stack = state->getStack();
    usize scanTop = std::min(state->getAbsoluteTop(), stack.size());

    Vec<CallInfo>& callStack = state->getCallStack();
    usize callStackSize = state->getCallStackSize();
    for (usize i = 0; i < callStackSize && i < callStack.size(); i++) {
        scanTop = std::max(scanTop, std::min(callStack[i].top, stack.size()));
        if (callStack[i].func < stack.size()) {
            markValue(stack.at(callStack[i].func));
        }
    }

    for (usize i = 0; i < scanTop; i++) {
        markValue(stack.at(i));
    }

    Upvalue* uv = state->getOpenUpvalues();
    while (uv != nullptr) {
        markObject(uv);
        uv = uv->getNext();
    }

    markObject(state->getDebugHook());
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
            usize objSize = obj->getSize();
            totalMemory_ = totalMemory_ >= objSize ? totalMemory_ - objSize : 0;
            --objectCount_;

            if (obj->getType() == GCObjectType::String) {
                StringPool::getInstance().remove(static_cast<GCString*>(obj));
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

