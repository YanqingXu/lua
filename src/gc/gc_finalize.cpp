/**
 * @file gc_finalize.cpp
 * @brief 垃圾回收器终结器处理实现
 */

#include "gc/garbage_collector.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/userdata.hpp"
#include "core/value.hpp"
#include "vm/state/global_state.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/state/stack.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/vm.hpp"
#include <algorithm>
#include <exception>
#include <limits>

namespace Lua {

Value GarbageCollector::getFinalizer(Userdata* userdata) const {
    if (userdata == nullptr || userdata->getMetatable() == nullptr) {
        return Value();
    }

    GlobalState& state = globalState_ != nullptr ? *globalState_ : GlobalState::getInstance();
    GCString* gcName = state.getMetamethodName(TMS::TM_GC);
    return userdata->getMetatable()->get(Value(gcName));
}

void GarbageCollector::prepareFinalizers() {
    GCObject* obj = allObjects_;
    while (obj != nullptr) {
        if (obj->getType() == GCObjectType::Userdata && obj->getColor() == GCColor::White &&
            (obj->getMarked() & (GCBits::FIXED | GCBits::FINALIZED)) == 0) {
            auto* userdata = static_cast<Userdata*>(obj);
            Value finalizer = getFinalizer(userdata);
            if (!finalizer.isNil()) {
                pendingFinalizers_.push_back(userdata);
                /**
                 * @brief 队列发布成功后才标记 FINALIZED。
                 *
                 * 发布过程可能分配内存，如此可避免内存不足时永久且无提示地丢弃 __gc。
                 */
                obj->setMarked(obj->getMarked() | GCBits::FINALIZED);
                markObject(obj);
            }
        }

        obj = obj->getNext();
    }
}

usize GarbageCollector::finalizerDrainLimit() const noexcept {
    if (globalState_ == nullptr) {
        return std::numeric_limits<usize>::max();
    }

    const ExecutionPolicy::FinalizerCount configured = globalState_->getExecutionPolicy().finalizerBudgetPerDrain();
    const auto maximum = static_cast<ExecutionPolicy::FinalizerCount>(std::numeric_limits<usize>::max());
    return configured >= maximum ? std::numeric_limits<usize>::max() : static_cast<usize>(configured);
}

void GarbageCollector::runFinalizers(LuaState* state) {
    if (state == nullptr || finalizersRunning_ || pendingFinalizers_.empty()) {
        return;
    }

    const usize scheduledCount = std::min(pendingFinalizers_.size(), finalizerDrainLimit());
    if (scheduledCount == 0) {
        return;
    }

    /**
     * @brief 回调运行期间将已安排的用户数据保留在 pendingFinalizers_ 中。
     *
     * __gc 回调可能递归调用 collectgarbage；标记阶段会将此成员队列视为根。若将队列移动到局部
     * 向量，剩余回调会对嵌套收集不可见，并在外层终结器循环中留下悬空指针。
     */
    using Difference = LuaVector<Userdata*>::difference_type;
    LuaVector<Userdata*> finalizers(pendingFinalizers_.begin(),
                                    pendingFinalizers_.begin() + static_cast<Difference>(scheduledCount),
                                    pendingFinalizers_.get_allocator());
    /**
     * @brief 复制成功后才发布重入守卫。
     *
     * 复制可能分配内存，否则内存不足会永久抑制终结器。
     */
    finalizersRunning_ = true;

    for (usize i = 0; i < finalizers.size(); i++) {
        Userdata* userdata = finalizers[i];
        Value finalizer = getFinalizer(userdata);
        if (finalizer.isNil()) {
            pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                     pendingFinalizers_.end());
            continue;
        }

        try {
            callFinalizer(state, userdata);
        } catch (...) {
            pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                     pendingFinalizers_.end());
            finalizersRunning_ = false;
            throw;
        }

        pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                 pendingFinalizers_.end());
    }

    finalizersRunning_ = false;
}

void GarbageCollector::callFinalizer(LuaState* state, Userdata* userdata) {
    Value finalizer = getFinalizer(userdata);
    if (finalizer.isNil()) {
        return;
    }

    Stack& stack = state->getStack();
    const usize savedTop = state->getAbsoluteTop();
    const usize savedStackTop = stack.size();
    const usize savedCI = state->getCurrentCI();
    std::exception_ptr finalizerError;

    try {
        stack.setTop(savedTop);
        state->setAbsoluteTop(savedTop);
        state->pushValue(finalizer);
        state->pushUserdata(userdata);
        RuntimeServices services(state->getGlobalState());
        VM::call(services, state, 1, 0);
    } catch (...) {
        finalizerError = std::current_exception();
    }

    while (state->getCurrentCI() > savedCI) {
        state->popCallInfo();
    }
    /**
     * @brief 恢复前清除回调窗口。
     *
     * 物理栈可预留比状态逻辑栈顶更宽的辅助窗口；清除后，后续宽根扫描不会继续持有终结器
     * 或其用户数据参数。
     */
    const usize clearTop = std::max(savedStackTop, stack.size());
    for (usize slot = savedTop; slot < clearTop; ++slot) {
        stack[slot] = Value();
    }
    stack.setTop(savedStackTop);
    state->setAbsoluteTop(savedTop);

    if (finalizerError) {
        std::rethrow_exception(finalizerError);
    }
}

void GarbageCollector::finalizeAll(LuaState* state) noexcept {
    if (state == nullptr || finalizersRunning_) {
        return;
    }

    resetIncrementalCycle();

    /**
     * @brief 清空栈前关闭开放上值，以匹配 Lua 5.1。
     *
     * lua_close 不保留执行中的调用帧；即使分配器持续失败，此顺序也使已拥有的基础调用帧存储
     * 保持可用。
     */
    try {
        state->closeUpvalues(0);
    } catch (...) {
        /** @brief 即使受损状态无法正确关闭某个上值，关闭流程仍须继续并释放运行时。 */
    }
    while (state->getCurrentCI() > 0) {
        try {
            state->popCallInfo();
        } catch (...) {
            break;
        }
    }

    Stack& stack = state->getStack();
    for (usize slot = 0; slot < stack.capacity(); ++slot) {
        stack[slot] = Value();
    }
    stack.clear();
    state->setAbsoluteTop(0);
    (void)state->tryPushValueNoAlloc(Value());
    state->setStatus(ThreadStatus::OK);

    const usize callbackLimit = finalizerDrainLimit();
    usize callbacksEntered = 0;
    finalizersRunning_ = true;
    try {
        while (callbacksEntered < callbackLimit) {
            Userdata* userdata = nullptr;
            if (!pendingFinalizers_.empty()) {
                userdata = pendingFinalizers_.back();
                pendingFinalizers_.pop_back();
            } else {
                /**
                 * @brief 每轮只选择一个对象，因为终结器可能运行嵌套收集并修改侵入式对象链表。
                 */
                for (GCObject* object = allObjects_; object != nullptr; object = object->getNext()) {
                    if (object->getType() != GCObjectType::Userdata || (object->getMarked() & GCBits::FINALIZED) != 0) {
                        continue;
                    }
                    userdata = static_cast<Userdata*>(object);
                    object->setMarked(object->getMarked() | GCBits::FINALIZED);
                    break;
                }
            }

            if (userdata == nullptr) {
                break;
            }

            pendingFinalizers_.erase(std::remove(pendingFinalizers_.begin(), pendingFinalizers_.end(), userdata),
                                     pendingFinalizers_.end());
            if (getFinalizer(userdata).isNil()) {
                continue;
            }

            ++callbacksEntered;
            try {
                callFinalizer(state, userdata);
            } catch (...) {
                /**
                 * @brief Lua 5.1 会保护关闭期终结器；继续清理以免损坏的 __gc 泄漏后续用户数据资源。
                 */
            }
        }
    } catch (...) {
        /** @brief 任何异常均不得逸出关闭流程，包括终结器查找或队列记账意外损坏。 */
    }
    finalizersRunning_ = false;
}

} // namespace Lua
