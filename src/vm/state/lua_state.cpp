/**
 * @file lua_state.cpp
 * @brief Lua状态管理实现
 *
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#include "vm/state/lua_state.hpp"
#include "lua.h"
#include "common/lua_error.hpp"
#include "common/number_conversion.hpp"
#include "core/gc_string.hpp"
#include "core/userdata.hpp"
#include "core/function.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/vm.hpp"
#include "core/upvalue.hpp"
#include <algorithm>
#include <array>
#ifdef DEBUG
#include <cassert>
#endif

namespace Lua {

namespace {

const char* hookEventName(DebugHookEvent event) {
    switch (event) {
    case DebugHookEvent::Call:
        return "call";
    case DebugHookEvent::Return:
        return "return";
    case DebugHookEvent::TailReturn:
        return "tail return";
    case DebugHookEvent::Line:
        return "line";
    case DebugHookEvent::Count:
        return "count";
    }

    return "unknown";
}

int hookEventCode(DebugHookEvent event) {
    switch (event) {
    case DebugHookEvent::Call:
        return LUA_HOOKCALL;
    case DebugHookEvent::Return:
        return LUA_HOOKRET;
    case DebugHookEvent::TailReturn:
        return LUA_HOOKTAILRET;
    case DebugHookEvent::Line:
        return LUA_HOOKLINE;
    case DebugHookEvent::Count:
        return LUA_HOOKCOUNT;
    }

    return LUA_HOOKCOUNT;
}

Str makeStringChunkSnippet(StrView source) {
    constexpr usize kChunkSnippetLimit = 60;
    Str snippet;
    bool truncated = false;

    for (char ch : source) {
        if (ch == '\n' || ch == '\r') {
            truncated = true;
            break;
        }
        if (snippet.size() >= kChunkSnippetLimit) {
            truncated = true;
            break;
        }
        if (ch == '"' || ch == '\\') {
            snippet.push_back('\\');
        }
        snippet.push_back(ch);
    }

    if (truncated) {
        snippet += "...";
    }
    return snippet;
}

Str makeLuaChunkId(StrView source) {
    if (!source.empty()) {
        if (source.front() == '=') {
            return Str(source.substr(1));
        }
        if (source.front() == '@') {
            return Str(source.substr(1));
        }
    }

    return Str("[string \"") + makeStringChunkSnippet(source) + "\"]";
}

Str runtimeErrorWithLocation(LuaState* L, const Str& message) {
    if (!L) {
        return message;
    }

    LuaVector<CallInfo>& frames = L->getCallStack();
    usize frameIndex = L->getCurrentCI();
    while (true) {
        if (frameIndex < frames.size()) {
            const CallInfo& ci = frames[frameIndex];
            Stack& stack = L->getStack();
            if (ci.func < stack.size()) {
                const Value& funcValue = stack[ci.func];
                if (funcValue.isFunction()) {
                    Function* func = funcValue.asFunction();
                    Proto* proto = func ? func->getProto() : nullptr;
                    if (proto && ci.savedpc) {
                        const auto code = proto->getInstructionSpan();
                        usize pc = static_cast<usize>(ci.savedpc - code.data());
                        usize errorPc = pc > 0 ? pc - 1 : 0;
                        i32 line = proto->getLine(errorPc);
                        StrView source = proto->getSource() ? proto->getSource()->view() : StrView("=?");
                        return makeLuaChunkId(source) + ":" + std::to_string(line) + ": " + message;
                    }
                }
            }
        }

        if (frameIndex == 0) {
            break;
        }
        --frameIndex;
    }

    return message;
}

} // namespace

void EngineContextDeleter::operator()(EngineContext* context) const noexcept {
    if (context == nullptr) {
        return;
    }

    LuaAllocator allocatorSnapshot = context->allocator();
    if (allocatorBacked) {
        std::destroy_at(context);
        allocatorSnapshot.deallocate(context, sizeof(EngineContext));
    } else {
        std::default_delete<EngineContext>{}(context);
    }
}

// =====================================================================
// 静态工厂方法
// =====================================================================

LuaState* LuaState::newState() {
    return create().release();
}

UPtr<LuaState> LuaState::create() {
    RuntimeServices services = RuntimeServices::fromSingletons();
    return create(services);
}

LuaState* LuaState::newState(RuntimeServices& services) {
    return create(services).release();
}

UPtr<LuaState> LuaState::create(RuntimeServices& services) {
    UPtr<LuaState> L = makeUnique<LuaState>(CtorToken{}, services.globalState);
    L->initialize();
    return L;
}

LuaState* LuaState::newState(EngineContext& context) {
    return create(context).release();
}

UPtr<LuaState> LuaState::create(EngineContext& context) {
    RuntimeServices services = context.services();
    return create(services);
}

UPtr<LuaState> LuaState::createIsolated() {
    UPtr<EngineContext> context = makeUnique<EngineContext>();
    UPtr<LuaState> L = makeUnique<LuaState>(CtorToken{}, context.get(), false, false);
    [[maybe_unused]] EngineContext* releasedContext = context.release();
    L->initialize();
    return L;
}

LuaState* LuaState::newIsolatedState() {
    return createIsolated().release();
}

LuaState* LuaState::newAllocatedState(LuaAllocatorFunction allocatorFunction, void* userData) {
    if (allocatorFunction == nullptr) {
        return {};
    }

    LuaAllocator bootstrapAllocator(allocatorFunction, userData);
    void* contextMemory = bootstrapAllocator.allocate(sizeof(EngineContext));
    if (contextMemory == nullptr) {
        return {};
    }

    EngineContext* context = nullptr;
    try {
        context = std::construct_at(static_cast<EngineContext*>(contextMemory), allocatorFunction, userData);
    } catch (...) {
        bootstrapAllocator.deallocate(contextMemory, sizeof(EngineContext));
        return {};
    }

    LuaAllocator& allocator = context->allocator();
    LuaAllocator allocatorSnapshot = allocator;
    void* stateMemory = allocator.allocate(sizeof(LuaState));
    if (stateMemory == nullptr) {
        LuaAllocator snapshot = allocator;
        std::destroy_at(context);
        snapshot.deallocate(contextMemory, sizeof(EngineContext));
        return {};
    }

    LuaState* state = nullptr;
    bool stateOwnsContext = false;
    try {
        // ownedContext_ is the first LuaState member and its construction is
        // non-throwing. From this point, a later member-construction failure
        // destroys the context during constructor unwinding.
        stateOwnsContext = true;
        state = std::construct_at(static_cast<LuaState*>(stateMemory), CtorToken{}, context, true, true);
        state->initialize();
        return state;
    } catch (...) {
        if (state != nullptr) {
            destroyState(state);
        } else {
            allocatorSnapshot.deallocate(stateMemory, sizeof(LuaState));
            if (!stateOwnsContext) {
                std::destroy_at(context);
                allocatorSnapshot.deallocate(contextMemory, sizeof(EngineContext));
            }
        }
        return {};
    }
}

void LuaState::destroyState(LuaState* state) noexcept {
    if (state == nullptr) {
        return;
    }

    if (!state->allocatorOwnedSelf_) {
        UPtr<LuaState> owner(state);
        return;
    }

    LuaAllocator* currentAllocator = state->globalState_.getAllocator();
    LuaAllocator snapshot = currentAllocator != nullptr ? *currentAllocator : LuaAllocator{};
    std::destroy_at(state);
    snapshot.deallocate(state, sizeof(LuaState));
}

LuaState* LuaState::newThread(LuaState* parentL) {
    if (parentL == nullptr) {
        return nullptr;
    }

    GlobalState& globalState = parentL->getGlobalState();
    LuaAllocator* allocator = globalState.getAllocator();
    LuaState* L = nullptr;

    if (allocator != nullptr && allocator->isConfigured()) {
        void* memory = allocator->allocate(sizeof(LuaState));
        if (memory == nullptr) {
            return nullptr;
        }
        try {
            L = std::construct_at(static_cast<LuaState*>(memory), CtorToken{}, globalState, true);
        } catch (...) {
            allocator->deallocate(memory, sizeof(LuaState));
            return nullptr;
        }
    } else {
        try {
            UPtr<LuaState> state = makeUnique<LuaState>(CtorToken{}, globalState, false);
            L = state.release();
        } catch (...) {
            return nullptr;
        }
    }

    try {
        // 共享全局表（不创建新的，不注册为 GC root）
        L->globalTable_ = parentL->globalTable_;
        L->isChildThread_ = true;

        // 初始化调用栈（虚拟主函数帧）
        CallInfo& ci = L->callStack_[0];
        ci.func = 0;
        ci.base = 1;
        ci.top = MIN_STACK_SIZE;
        ci.savedpc = nullptr;
        ci.nresults = MULTRET;
        ci.tailcalls = 0;

        // 虚拟主函数位
        L->stack_.push(Value()); // nil
        L->top_ = 1;
        L->status_ = ThreadStatus::OK;
    } catch (...) {
        destroyState(L);
        return nullptr;
    }

    return L;
}

// =====================================================================
// 构造函数和析构函数
// =====================================================================

LuaState::LuaState() : LuaState(GlobalState::getInstance()) {}

LuaState::LuaState(CtorToken, GlobalState& globalState) : LuaState(globalState) {}

LuaState::LuaState(CtorToken, GlobalState& globalState, bool allocatorOwnedSelf) : LuaState(globalState) {
    allocatorOwnedSelf_ = allocatorOwnedSelf;
}

LuaState::LuaState(CtorToken, EngineContext* ownedContext, bool allocatorOwnedContext, bool allocatorOwnedSelf)
    : ownedContext_(ownedContext, EngineContextDeleter{allocatorOwnedContext}), allocatorOwnedSelf_(allocatorOwnedSelf),
      globalState_(ownedContext_->globalState()), stack_(INITIAL_STACK_SIZE, globalState_.getAllocator()), top_(0),
      callStack_(INITIAL_CI_SIZE, LuaStdAllocator<CallInfo>(globalState_.getAllocator())), currentCI_(0),
      globalTable_(nullptr), status_(ThreadStatus::OK), openUpvalues_(nullptr) {}

LuaState::LuaState(GlobalState& globalState)
    : ownedContext_(nullptr, EngineContextDeleter{}), allocatorOwnedSelf_(false), globalState_(globalState),
      stack_(INITIAL_STACK_SIZE, globalState_.getAllocator()), top_(0),
      callStack_(INITIAL_CI_SIZE, LuaStdAllocator<CallInfo>(globalState_.getAllocator())), currentCI_(0),
      globalTable_(nullptr), status_(ThreadStatus::OK), openUpvalues_(nullptr) {}

LuaState::~LuaState() {
    // 关闭所有open upvalue
    closeUpvalues(0);

    if (globalState_.getMainThread() == this) {
        globalState_.setMainThread(nullptr);
    }

    if (hookFunc_ != nullptr) {
        globalState_.getGC().removeRoot(hookFunc_);
        hookFunc_ = nullptr;
    }

    // 子线程不拥有全局表的 root 引用
    if (globalTable_ && !isChildThread_) {
        globalState_.getGC().removeRoot(globalTable_);
    }
}

// =====================================================================
// 初始化
// =====================================================================

void LuaState::initialize() {
    // 创建全局表
    globalTable_ = globalState_.getGC().createRoot<Table>();

    // 初始化第一个调用信息（虚拟的主函数）
    CallInfo& ci = callStack_[0];
    ci.func = 0;
    ci.base = 1; // func 在位置 0，局部变量从 1 开始
    ci.top = MIN_STACK_SIZE;
    ci.savedpc = nullptr;
    ci.nresults = MULTRET;
    ci.tailcalls = 0;

    // 在栈上放置一个nil值作为虚拟函数
    stack_.push(Value()); // nil
    top_ = 1;             // 栈顶指向下一个可用位置

    // 如果这是第一个LuaState，设置为主线程
    if (globalState_.getMainThread() == nullptr) {
        globalState_.setMainThread(this);
    }
}

void LuaState::setGlobalTable(Table* table) {
    if (table == nullptr || table == globalTable_) {
        return;
    }

    auto& gc = globalState_.getGC();
    if (globalTable_ && !isChildThread_) {
        gc.removeRoot(globalTable_);
    }

    globalTable_ = table;

    if (!isChildThread_) {
        gc.addRoot(globalTable_);
    }
}

// =====================================================================
// Upvalue管理
// =====================================================================

Upvalue* LuaState::findOrCreateUpvalue(usize stackIndex) {
    // 1. 在链表中查找已存在的upvalue（按栈索引降序）
    Upvalue* prev = nullptr;
    Upvalue* curr = openUpvalues_;

    while (curr != nullptr && curr->getStackIndex() > stackIndex) {
        prev = curr;
        curr = curr->getNext();
    }

    // 2. 找到了，直接返回
    if (curr != nullptr && curr->getStackIndex() == stackIndex) {
        return curr;
    }

    // 3. 没找到，创建新的upvalue
    // ✅ 改进：只传递索引，不传递指针
    Upvalue* newUpval = globalState_.getGC().create<Upvalue>(stackIndex, stack_);

    // 4. 插入链表（保持降序）
    newUpval->setNext(curr);
    if (prev == nullptr) {
        // 插入到链表头
        openUpvalues_ = newUpval;
    } else {
        // 插入到prev之后
        prev->setNext(newUpval);
    }

    return newUpval;
}

void LuaState::closeUpvalues(usize level) {
    // 关闭所有栈索引 >= level 的upvalue
    while (openUpvalues_ != nullptr && openUpvalues_->getStackIndex() >= level) {
        Upvalue* uv = openUpvalues_;
        openUpvalues_ = uv->getNext(); // 从链表移除
        uv->close(stack_);             // ✅ 改进：传入stack_引用
        uv->setNext(nullptr);          // 清除链表指针
    }
}

// =====================================================================
// Debug hook support
// =====================================================================

void LuaState::setDebugHook(Function* hook, u8 mask, i32 count) {
    GarbageCollector& gc = globalState_.getGC();
    Function* previousHook = hookFunc_;

    if (previousHook != nullptr && previousHook != hook) {
        gc.removeRoot(previousHook);
    }

    if (hook == nullptr) {
        hookFunc_ = nullptr;
        apiDebugHook_ = nullptr;
        hookMask_ = 0;
        hookCount_ = 0;
        hookCountdown_ = 0;
        hookActive_ = false;
        return;
    }

    if (previousHook != hook) {
        gc.addRoot(hook);
    }
    hookFunc_ = hook;
    apiDebugHook_ = nullptr;
    hookMask_ = static_cast<u8>(mask | (count > 0 ? HookMaskCount : 0));
    hookCount_ = std::max(0, count);
    hookCountdown_ = hookCount_;
}

void LuaState::setApiDebugHook(ApiDebugHook hook, u8 mask, i32 count) {
    if (hookFunc_ != nullptr) {
        globalState_.getGC().removeRoot(hookFunc_);
        hookFunc_ = nullptr;
    }

    apiDebugHook_ = (hook != nullptr && mask != 0) ? hook : nullptr;
    hookMask_ = apiDebugHook_ != nullptr ? mask : 0;
    hookCount_ = count;
    hookCountdown_ = count;
}

bool LuaState::consumeDebugHookCount() {
    if ((hookFunc_ == nullptr && apiDebugHook_ == nullptr) || hookActive_ || (hookMask_ & HookMaskCount) == 0 ||
        hookCount_ <= 0) {
        return false;
    }

    if (hookCountdown_ <= 1) {
        hookCountdown_ = hookCount_;
        return true;
    }

    --hookCountdown_;
    return false;
}

void LuaState::callDebugHook(DebugHookEvent event, i32 line) {
    if ((hookFunc_ == nullptr && apiDebugHook_ == nullptr) || hookActive_) {
        return;
    }

    usize savedTop = getAbsoluteTop();
    usize savedSize = getStack().size();
    usize restoreStackTop = savedSize;
    if (currentCI_ < callStack_.size()) {
        restoreStackTop = std::max(restoreStackTop, callStack_[currentCI_].top);
    }
    hookActive_ = true;

    try {
        if (apiDebugHook_ != nullptr) {
            lua_Debug activation{};
            activation.event = hookEventCode(event);
            activation.currentline = line;
            activation.i_ci = event == DebugHookEvent::TailReturn ? 0 : static_cast<int>(currentCI_);
            apiDebugHook_(reinterpret_cast<lua_State*>(this), &activation);
            getStack().setTop(restoreStackTop);
            setAbsoluteTop(savedTop);
            hookActive_ = false;
            return;
        }

        usize hookTop = std::max(savedTop, savedSize);
        if (currentCI_ < callStack_.size()) {
            hookTop = std::max(hookTop, callStack_[currentCI_].top + EXTRA_STACK);
        }

        GCString* eventName = globalState_.getStringPool().intern(hookEventName(event));
        setAbsoluteTop(hookTop);
        pushFunction(hookFunc_);
        pushString(eventName);
        if (line >= 0) {
            pushNumber(static_cast<LuaNumber>(line));
        } else {
            pushNil();
        }

        RuntimeServices services(globalState_);
        VM::call(services, this, 2, 0);
        getStack().setTop(restoreStackTop);
        setAbsoluteTop(savedTop);
        hookActive_ = false;
    } catch (...) {
        getStack().setTop(restoreStackTop);
        setAbsoluteTop(savedTop);
        hookActive_ = false;
        throw;
    }
}

// =====================================================================
// CallInfo管理
// =====================================================================

CallInfo& LuaState::pushCallInfo() {
    // ✅ 改进：检查最大调用深度
    if (currentCI_ + 1 >= MAX_CALL_DEPTH) {
        throw StackOverflowError(
            "stack overflow: maximum call depth exceeded (limit: " + std::to_string(MAX_CALL_DEPTH) + ")");
    }

    // 检查是否需要扩展调用栈
    if (currentCI_ + 1 >= callStack_.size()) {
        // ✅ 改进：双倍扩展，但不超过最大限制
        usize newSize = std::min(callStack_.size() * 2, MAX_CALL_DEPTH);
        callStack_.resize(newSize);
    }

    // 移动到下一个CallInfo
    currentCI_++;

    // 重置新的CallInfo
    callStack_[currentCI_].reset();

    return callStack_[currentCI_];
}

void LuaState::popCallInfo() {
    if (currentCI_ == 0) {
        throw RuntimeError("LuaState::popCallInfo: cannot pop base CallInfo");
    }

// ✅ 改进：清理当前CallInfo（调试模式）
#ifdef DEBUG
    callStack_[currentCI_].reset();
#endif

    currentCI_--;
}

void LuaState::enterHostCall() {
    if (hostCallDepth_ >= MAX_CALLS) {
        throw StackOverflowError("VM: stack overflow (too many nested C/Lua calls)");
    }
    ++hostCallDepth_;
}

void LuaState::leaveHostCall() noexcept {
    if (hostCallDepth_ > 0) {
        --hostCallDepth_;
    }
}

// =====================================================================
// ✅ 改进：调试支持
// =====================================================================

#ifdef DEBUG
void LuaState::dumpCallStack() const {
    std::cout << "========================================\n";
    std::cout << "Call Stack Dump\n";
    std::cout << "========================================\n";
    std::cout << "Depth: " << (currentCI_ + 1) << " / " << MAX_CALL_DEPTH << "\n";
    std::cout << "----------------------------------------\n";

    for (usize i = 0; i <= currentCI_; i++) {
        std::cout << "  [" << i << "] " << callStack_[i].toString() << "\n";
    }

    std::cout << "========================================\n";
}

void LuaState::validateCallStack() const {
    assert(currentCI_ < callStack_.size());
    assert(currentCI_ < MAX_CALL_DEPTH);

    // 验证每个CallInfo
    for (usize i = 0; i <= currentCI_; i++) {
        callStack_[i].validate(stack_.size());
    }
}
#endif

// =====================================================================
// 栈操作
// =====================================================================

void LuaState::setTop(i32 idx) {
    // 计算新的栈顶位置
    usize base = 0;
    if (currentCI_ > 0) {
        base = callStack_[currentCI_].base;
    }

    i32 newTop;
    if (idx >= 0) {
        // 正索引：相对于当前 base
        newTop = static_cast<i32>(base) + idx;
    } else {
        // 负索引：相对于当前 top_
        newTop = static_cast<i32>(top_) + idx + 1;
    }

    if (newTop < static_cast<i32>(base)) {
        throw RuntimeError("invalid stack index");
    }

    const usize oldTop = top_;

    // Fill every newly exposed logical slot with nil. The backing Stack can
    // already contain reserved frame registers beyond top_, so merely growing
    // its physical size would leak stale register values through lua_settop.
    while (static_cast<i32>(stack_.size()) < newTop) {
        stack_.push(Value()); // nil
    }
    for (usize i = oldTop; i < static_cast<usize>(newTop); ++i) {
        stack_[i] = Value();
    }

    // Shrinking the logical top must not discard a VM frame's reserved
    // register area. Clear the removed logical values so they neither retain
    // GC objects nor reappear if the C API grows the top again.
    for (usize i = static_cast<usize>(newTop); i < oldTop && i < stack_.size(); ++i) {
        stack_[i] = Value();
    }

    // 关键：同步 top_ 变量
    top_ = static_cast<usize>(newTop);
}

void LuaState::pushValue(i32 idx) {
    Value value = at(idx);
    pushValue(value);
}

void LuaState::insert(i32 idx) {
    // 将栈顶元素插入目标位置，目标位置之后的元素整体后移。
    //
    // 将栈顶元素插入到指定位置，其他元素向上移动
    // 注意：栈大小不变，栈顶元素被移动到目标位置

    if (top_ == 0) {
        throw RuntimeError("insert: stack is empty");
    }

    // 计算绝对索引（0-based）
    i32 absIdx = idx;
    if (idx < 0) {
        // 负索引：相对于 top_
        absIdx = static_cast<i32>(top_) + idx;
    } else {
        // 正索引：相对于 base
        usize base = 0;
        if (currentCI_ > 0) {
            base = callStack_[currentCI_].base;
        }
        absIdx = static_cast<i32>(base) + idx - 1; // 转换为0-based
    }

    if (absIdx < 0 || absIdx >= static_cast<i32>(top_)) {
        throw RuntimeError("insert: invalid index");
    }

    // 保存栈顶元素
    Value topValue = stack_.at(top_ - 1);

    // 将元素从 absIdx 到 top-1 向上移动一位
    // 从栈顶向下移动（避免覆盖）
    for (i32 i = static_cast<i32>(top_) - 1; i > absIdx; i--) {
        stack_.at(i) = stack_.at(i - 1);
    }

    // 将原栈顶元素放到目标位置
    stack_.at(absIdx) = topValue;

    // 栈大小不变，top_ 也不变
}

void LuaState::replace(i32 idx) {
    // 用栈顶元素替换指定位置的元素，然后弹出栈顶

    if (top_ == 0) {
        throw RuntimeError("replace: stack is empty");
    }

    // 获取栈顶元素
    Value top = stack_.at(top_ - 1);

    // 计算绝对索引
    i32 absIdx = idx;
    if (idx < 0) {
        absIdx = static_cast<i32>(top_) + idx; // 注意：replace 不加 1
    } else {
        usize base = 0;
        if (currentCI_ > 0) {
            base = callStack_[currentCI_].base;
        }
        absIdx = static_cast<i32>(base) + idx - 1; // 转换为0-based
    }

    if (absIdx < 0 || absIdx >= static_cast<i32>(top_)) {
        throw RuntimeError("replace: invalid index");
    }

    // 替换目标位置的元素
    stack_.at(absIdx) = top;

    // 弹出逻辑栈顶；保留物理预留空间供当前调用帧复用。
    --top_;
}

i32 LuaState::pcall(i32 nargs, i32 nresults, i32 errfunc) {
    // 计算函数在栈中的绝对位置（0-based）
    // 栈布局：[... func arg1 arg2 ...]
    // 函数在参数之前，所以 funcIdx = top_ - nargs - 1
    i32 funcIdx = static_cast<i32>(top_) - nargs - 1;

    if (funcIdx < 0 || funcIdx >= static_cast<i32>(top_)) {
        // 栈索引无效
        // 移除 func 和 args，压入错误消息
        top_ = funcIdx > 0 ? static_cast<usize>(funcIdx) : 0;
        try {
            auto& pool = getGlobalState().getStringPool();
            pushString(pool.intern("pcall: invalid function index"));
            return LUA_ERRRUN;
        } catch (...) {
            pushValue(Value(globalState_.getMemoryErrorMessage()));
            return LUA_ERRMEM;
        }
    }

    // 保护调用结束后只回收 func/args 之上的逻辑栈顶，不能覆盖外层帧槽位：
    // 被保护函数在抛错前可能已经写入了外层 local/upvalue。
    usize savedPrefixTop = static_cast<usize>(funcIdx);

    const usize savedCallFrameCount = currentCI_ + 1;
    LuaStdAllocator<CallInfo> snapshotAllocator(callStack_.get_allocator());
    struct SnapshotDeleter {
        LuaStdAllocator<CallInfo> allocator;
        usize count;

        void operator()(CallInfo* frames) noexcept {
            if (frames == nullptr) {
                return;
            }
            std::destroy_n(frames, count);
            allocator.deallocate(frames, count);
        }
    };
    std::unique_ptr<CallInfo[], SnapshotDeleter> savedCallStack(
        nullptr, SnapshotDeleter{snapshotAllocator, savedCallFrameCount});
    try {
        static_assert(std::is_nothrow_copy_constructible_v<CallInfo>);
        CallInfo* frames = snapshotAllocator.allocate(savedCallFrameCount);
        std::uninitialized_copy_n(callStack_.begin(), savedCallFrameCount, frames);
        savedCallStack.reset(frames);
    } catch (const std::bad_alloc&) {
        top_ = savedPrefixTop;
        pushValue(Value(globalState_.getMemoryErrorMessage()));
        setStatus(ThreadStatus::OK);
        return LUA_ERRMEM;
    }
    usize savedCurrentCI = currentCI_;
    auto& pool = getGlobalState().getStringPool();

    Value errorHandler;
    const bool hasErrorHandler = errfunc != 0;
    if (errfunc != 0) {
        try {
            errorHandler = at(errfunc);
        } catch (...) {
            errorHandler = Value();
        }
    }

    auto restoreStackPrefix = [&]() { top_ = savedPrefixTop; };

    auto restoreCallFrames = [&]() {
        std::copy_n(savedCallStack.get(), savedCallFrameCount, callStack_.begin());
        currentCI_ = savedCurrentCI;
    };

    auto closeUnwoundUpvalues = [&]() noexcept {
        usize frame = currentCI_;
        while (frame > savedCurrentCI && frame < callStack_.size()) {
            closeUpvalues(callStack_[frame].base);
            --frame;
        }
    };

    auto makeStringValue = [&](const Str& message) -> Value { return Value(pool.intern(message.c_str())); };

    struct HandlerResult {
        Value value;
        bool failed = false;
        bool memoryFailure = false;
    };

    auto finishMemoryError = [&]() -> i32 {
        closeUnwoundUpvalues();
        restoreCallFrames();
        restoreStackPrefix();
        pushValue(Value(globalState_.getMemoryErrorMessage()));
        setStatus(ThreadStatus::OK);
        return LUA_ERRMEM;
    };

    auto invokeErrorHandler = [&](const Value& errorValue) -> HandlerResult {
        if (!hasErrorHandler) {
            return {errorValue, false, false};
        }

        const usize handlerSavedCI = currentCI_;
        if (currentCI_ + 1 >= MAX_CALL_DEPTH && currentCI_ > savedCurrentCI) {
            // Keep one emergency CallInfo slot for the message handler after
            // logical stack overflow. The faulting frame remains stored and
            // is restored after the handler has produced the error object.
            --currentCI_;
        }

        try {
            pushValue(errorHandler);
            pushValue(errorValue);
            RuntimeServices services(globalState_);
            VM::call(services, this, 1, 1);
            Value handled = top();
            currentCI_ = handlerSavedCI;
            return {handled, false, false};
        } catch (...) {
            closeUnwoundUpvalues();
            currentCI_ = handlerSavedCI;
            try {
                return {makeStringValue("error in error handling"), true, false};
            } catch (...) {
                return {Value(globalState_.getMemoryErrorMessage()), true, true};
            }
        }
    };

    auto finishError = [&](Value errorValue, i32 status) -> i32 {
        HandlerResult handled = invokeErrorHandler(errorValue);
        closeUnwoundUpvalues();
        restoreCallFrames();
        restoreStackPrefix();
        pushValue(handled.value);
        setStatus(ThreadStatus::OK);
        return handled.memoryFailure ? LUA_ERRMEM : (handled.failed ? LUA_ERRERR : status);
    };

    Value& funcVal = stack_.at(funcIdx);
    if (!funcVal.isFunction()) {
        try {
            return finishError(makeStringValue("attempt to call a non-function value"), LUA_ERRRUN);
        } catch (...) {
            return finishMemoryError();
        }
    }

    Function* func = funcVal.asFunction();
    if (!func) {
        try {
            return finishError(makeStringValue("invalid function"), LUA_ERRRUN);
        } catch (...) {
            return finishMemoryError();
        }
    }

    try {
        RuntimeServices services(globalState_);
        VM::call(services, this, nargs, nresults);
        setStatus(ThreadStatus::OK);
        return LUA_OK;

    } catch (const MemoryError&) {
        return finishMemoryError();

    } catch (const std::bad_alloc&) {
        return finishMemoryError();

    } catch (const LuaError& e) {
        try {
            Value errorValue =
                e.hasErrorObject() ? e.getErrorObject() : makeStringValue(runtimeErrorWithLocation(this, e.what()));
            return finishError(errorValue, LUA_ERRRUN);
        } catch (...) {
            try {
                return finishError(Value(globalState_.getApiExceptionMessage()), LUA_ERRRUN);
            } catch (...) {
                return finishMemoryError();
            }
        }

    } catch (const std::exception& e) {
        try {
            Value errorValue = makeStringValue(runtimeErrorWithLocation(this, e.what()));
            return finishError(errorValue, LUA_ERRRUN);
        } catch (...) {
            try {
                return finishError(Value(globalState_.getApiExceptionMessage()), LUA_ERRRUN);
            } catch (...) {
                return finishMemoryError();
            }
        }

    } catch (...) {
        try {
            return finishError(Value(globalState_.getApiExceptionMessage()), LUA_ERRRUN);
        } catch (...) {
            return finishMemoryError();
        }
    }
}

std::expected<i32, RuntimeError> LuaState::tryPCall(i32 nargs, i32 nresults, i32 errfunc) {
    const i32 status = pcall(nargs, nresults, errfunc);
    if (status == LUA_OK) {
        return status;
    }

    try {
        if (getTop() > 0) {
            return std::unexpected(RuntimeError(top()));
        }
    } catch (...) {
    }

    return std::unexpected(RuntimeError("pcall failed"));
}

i32 LuaState::getTop() const {
    // 返回值 = top - base
    if (currentCI_ > 0) {
        const CallInfo& ci = callStack_[currentCI_];
        return static_cast<i32>(top_ - ci.base);
    }
    // 没有调用帧时，返回 top_
    return static_cast<i32>(top_);
}

Value& LuaState::at(i32 idx) {
    // 正索引从当前调用帧的 base 开始（1-based）
    // 负索引从栈顶倒数

    usize base = 0;

    // 如果有活动的调用帧，使用其 base
    if (currentCI_ > 0) {
        base = callStack_[currentCI_].base;
    }

    if (idx > 0) {
        // 正索引：从当前 base 开始（1-based）
        usize absIdx = base + static_cast<usize>(idx - 1);
        if (absIdx >= top_) {
            throw std::out_of_range("Stack index out of range");
        }
        return stack_.at(absIdx);
    } else if (idx < 0) {
        // 负索引：从栈顶倒数
        i32 offset = static_cast<i32>(top_) + idx;
        if (offset < static_cast<i32>(base)) {
            throw std::out_of_range("Stack index out of range");
        }
        return stack_.at(static_cast<usize>(offset));
    } else {
        throw std::invalid_argument("stack index cannot be 0");
    }
}

const Value& LuaState::at(i32 idx) const {
    return const_cast<LuaState*>(this)->at(idx);
}

// =====================================================================
// 全局变量操作
// =====================================================================

void LuaState::setGlobal(StrView name, const Value& value) {
    if (!globalTable_) {
        throw RuntimeError("global table not initialized");
    }

    // 创建字符串键
    GCString* key = globalState_.getStringPool().intern(name);
    globalTable_->set(Value(key), value);
}

Value LuaState::getGlobal(StrView name) {
    if (!globalTable_) {
        throw RuntimeError("global table not initialized");
    }

    // 创建字符串键
    GCString* key = globalState_.getStringPool().intern(name);
    return globalTable_->get(Value(key));
}

// =====================================================================
// 类型检查
// =====================================================================

bool LuaState::isNumber(i32 idx) const {
    try {
        const Value& v = at(idx);
        if (v.isNumber()) {
            return true;
        }
        if (v.isString()) {
            LuaNumber number = 0.0;
            return luaStringToNumber(v.asString()->view(), number);
        }
        return false;
    } catch (...) {
        return false;
    }
}

bool LuaState::isString(i32 idx) const {
    try {
        return at(idx).isString();
    } catch (...) {
        return false;
    }
}

bool LuaState::isTable(i32 idx) const {
    try {
        return at(idx).isTable();
    } catch (...) {
        return false;
    }
}

bool LuaState::isFunction(i32 idx) const {
    try {
        return at(idx).isFunction();
    } catch (...) {
        return false;
    }
}

bool LuaState::isNil(i32 idx) const {
    try {
        return at(idx).isNil();
    } catch (...) {
        return true; // 无效索引视为nil
    }
}

bool LuaState::isBoolean(i32 idx) const {
    try {
        return at(idx).isBoolean();
    } catch (...) {
        return false;
    }
}

bool LuaState::isUserdata(i32 idx) const {
    try {
        return at(idx).isUserdata();
    } catch (...) {
        return false;
    }
}

i32 LuaState::type(i32 idx) const {
    try {
        const Value& v = at(idx);
        if (v.isNil())
            return 0; // LUA_TNIL
        if (v.isBoolean())
            return 1; // LUA_TBOOLEAN
        if (v.isNumber())
            return 3; // LUA_TNUMBER
        if (v.isString())
            return 4; // LUA_TSTRING
        if (v.isTable())
            return 5; // LUA_TTABLE
        if (v.isFunction())
            return 6; // LUA_TFUNCTION
        if (v.isUserdata())
            return 7; // LUA_TUSERDATA
        if (v.isThread())
            return 8; // LUA_TTHREAD
        return -1;    // LUA_TNONE
    } catch (...) {
        return -1; // LUA_TNONE
    }
}

const char* LuaState::typeName(i32 tp) const {
    static constexpr std::array<StrView, 9> typeNames{
        {"nil", "boolean", "lightuserdata", "number", "string", "table", "function", "userdata", "thread"}};
    if (tp >= 0 && static_cast<usize>(tp) < typeNames.size()) {
        return typeNames[static_cast<usize>(tp)].data();
    }
    return "no value";
}

// =====================================================================
// 类型转换
// =====================================================================

LuaNumber LuaState::toNumber(i32 idx) const {
    try {
        const Value& v = at(idx);
        if (v.isNumber()) {
            return v.asNumber();
        }
        if (v.isString()) {
            LuaNumber number = 0.0;
            if (luaStringToNumber(v.asString()->view(), number)) {
                return number;
            }
        }
        return 0.0;
    } catch (...) {
        return 0.0;
    }
}

Opt<StrView> LuaState::tryToString(i32 idx) {
    try {
        Value& v = at(idx);
        if (v.isString()) {
            return v.asString()->view();
        }
        if (v.isNumber()) {
            // 将数字转换为字符串并替换栈上的值
            Str text = luaNumberToString(v.asNumber());
            GCString* str = globalState_.getStringPool().intern(text);
            v = Value(str);
            return str->view();
        }
        return std::nullopt;
    } catch (...) {
        return std::nullopt;
    }
}

const char* LuaState::toString(i32 idx) {
    if (!tryToString(idx).has_value()) {
        return nullptr;
    }

    try {
        Value& v = at(idx);
        return v.isString() ? v.asString()->c_str() : nullptr;
    } catch (...) {
        return nullptr;
    }
}

bool LuaState::toBoolean(i32 idx) const {
    try {
        const Value& v = at(idx);
        // Lua中只有nil和false是假值
        if (v.isNil())
            return false;
        if (v.isBoolean())
            return v.asBoolean();
        return true; // 其他所有值都是真值
    } catch (...) {
        return false;
    }
}

// =====================================================================
// 元表操作
// =====================================================================

bool LuaState::getMetatable(i32 idx) {
    try {
        const Value& v = at(idx);
        if (v.isTable()) {
            Table* mt = v.asTable()->getMetatable();
            if (mt) {
                pushTable(mt);
                return true;
            }
        } else if (v.isUserdata()) {
            Table* mt = v.asUserdata()->getMetatable();
            if (mt) {
                pushTable(mt);
                return true;
            }
        } else {
            Table* mt = globalState_.getMetatable(v.getType());
            if (mt) {
                pushTable(mt);
                return true;
            }
        }
        return false;
    } catch (...) {
        return false;
    }
}

bool LuaState::setMetatable(i32 idx) {
    try {
        Value& v = at(idx);
        Value& mt = top();
        Table* newMetatable = nullptr;
        if (mt.isNil()) {
            newMetatable = nullptr;
        } else if (mt.isTable()) {
            newMetatable = mt.asTable();
        } else {
            return false; // 元表必须是表或nil
        }

        if (v.isTable()) {
            v.asTable()->setMetatable(newMetatable);
        } else if (v.isUserdata()) {
            v.asUserdata()->setMetatable(newMetatable);
        } else {
            globalState_.setMetatable(v.getType(), newMetatable);
        }

        pop(); // 弹出元表
        return true;
    } catch (...) {
        return false;
    }
}

// =====================================================================
// 错误处理
// =====================================================================

void LuaState::error(const char* msg) {
    setStatus(ThreadStatus::ErrRun);
    throw RuntimeError(msg);
}

void LuaState::requireSandboxCapability(SandboxCapability capability) {
    if (!globalState_.getSandboxPolicy().allows(capability)) {
        setStatus(ThreadStatus::ErrRun);
        throw RuntimeError(Value(globalState_.getSandboxCapabilityErrorMessage(capability)));
    }
}

void LuaState::requireStandardLibrary(StrView id) {
    if (!globalState_.getSandboxPolicy().allowsStandardLibrary(id)) {
        setStatus(ThreadStatus::ErrRun);
        throw RuntimeError(Value(globalState_.getSandboxLibraryErrorMessage()));
    }
}

i32 LuaState::error() {
    setStatus(ThreadStatus::ErrRun);
    throw RuntimeError(top());
}

} // namespace Lua
