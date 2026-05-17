/**
 * @file garbage_collector.cpp
 * @brief 垃圾回收器实现
 */

#include "gc/garbage_collector.hpp"
#include "core/metatable.hpp"
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
#include "vm/vm.hpp"
#include <iostream>
#include <algorithm>

namespace Lua {

namespace {

bool valueContainsObject(const Value& value) {
    return value.isString() || value.isTable() || value.isFunction() ||
           value.isUserdata() || value.isThread();
}

GCObject* objectFromValue(const Value& value) {
    if (value.isString()) {
        return value.asString();
    }
    if (value.isTable()) {
        return value.asTable();
    }
    if (value.isFunction()) {
        return value.asFunction();
    }
    if (value.isUserdata()) {
        return value.asUserdata();
    }
    if (value.isThread()) {
        return value.asThread();
    }
    return nullptr;
}

} // namespace

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

void GarbageCollector::mark() {
    mark(nullptr);
}

void GarbageCollector::mark(LuaState* currentState) {
    // 1. 重置所有对象为白色（保留FIXED和FINALIZED，清除上一轮弱表模式）
    GCObject* obj = allObjects_;
    while (obj != nullptr) {
        u8 preserved = obj->getMarked() & (GCBits::FIXED | GCBits::FINALIZED);
        obj->setMarked(preserved);

        // 设置为白色
        obj->setColor(GCColor::White);

        obj = obj->getNext();
    }

    // 2. 清空本轮临时列表
    grayList_.clear();
    weakTables_.clear();

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

    // 终结器队列中的 userdata 已经被复活，必须在真正运行 __gc 前保持存活。
    for (Userdata* userdata : pendingFinalizers_) {
        markObject(userdata);
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
    if (valueContainsObject(value)) {
        markObject(objectFromValue(value));
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

void GarbageCollector::markTable(Table* table) {
    if (table == nullptr) {
        return;
    }

    bool weakKeys = false;
    bool weakValues = false;
    Table* mt = table->getMetatable();
    if (mt != nullptr) {
        GCString* modeName = GlobalState::getInstance().getMetamethodName(TMS::TM_MODE);
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

Value GarbageCollector::getFinalizer(Userdata* userdata) const {
    if (userdata == nullptr || userdata->getMetatable() == nullptr) {
        return Value();
    }

    GCString* gcName = GlobalState::getInstance().getMetamethodName(TMS::TM_GC);
    return userdata->getMetatable()->get(Value(gcName));
}

void GarbageCollector::prepareFinalizers() {
    GCObject* obj = allObjects_;
    while (obj != nullptr) {
        if (obj->getType() == GCObjectType::Userdata &&
            obj->getColor() == GCColor::White &&
            (obj->getMarked() & (GCBits::FIXED | GCBits::FINALIZED)) == 0) {
            auto* userdata = static_cast<Userdata*>(obj);
            Value finalizer = getFinalizer(userdata);
            if (!finalizer.isNil()) {
                obj->setMarked(obj->getMarked() | GCBits::FINALIZED);
                pendingFinalizers_.push_back(userdata);
                markObject(obj);
            }
        }

        obj = obj->getNext();
    }
}

void GarbageCollector::runFinalizers(LuaState* state) {
    if (state == nullptr || finalizersRunning_ || pendingFinalizers_.empty()) {
        return;
    }

    finalizersRunning_ = true;
    Vec<Userdata*> finalizers;
    finalizers.swap(pendingFinalizers_);

    Stack& stack = state->getStack();
    for (Userdata* userdata : finalizers) {
        Value finalizer = getFinalizer(userdata);
        if (finalizer.isNil()) {
            continue;
        }

        usize savedTop = state->getAbsoluteTop();
        usize savedStackTop = stack.size();
        usize savedCI = state->getCurrentCI();

        try {
            stack.setTop(savedTop);
            state->setAbsoluteTop(savedTop);
            state->pushValue(finalizer);
            state->pushUserdata(userdata);
            VM::call(state, 1, 0);
        } catch (...) {
            // Lua 5.1 的 GC 终结流程不应让单个 finalizer 错误打断整轮回收。
        }

        while (state->getCurrentCI() > savedCI) {
            state->popCallInfo();
        }
        stack.setTop(savedStackTop);
        state->setAbsoluteTop(savedTop);
    }

    finalizersRunning_ = false;
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

