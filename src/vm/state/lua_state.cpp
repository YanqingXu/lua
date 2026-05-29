/**
 * @file lua_state.cpp
 * @brief Lua状态管理实现
 *
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#include "vm/state/lua_state.hpp"
#include "common/lua_error.hpp"
#include "core/gc_string.hpp"
#include "core/userdata.hpp"
#include "core/function.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/vm.hpp"
#include "core/upvalue.hpp"
#include <algorithm>
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
        case DebugHookEvent::Line:
            return "line";
        case DebugHookEvent::Count:
            return "count";
    }

    return "unknown";
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

    Vec<CallInfo>& frames = L->getCallStack();
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

// =====================================================================
// 静态工厂方法
// =====================================================================

LuaState* LuaState::newState() {
    RuntimeServices services = RuntimeServices::fromSingletons();
    return newState(services);
}

LuaState* LuaState::newState(RuntimeServices& services) {
    LuaState* L = new LuaState(services.globalState);
    L->initialize();
    return L;
}

LuaState* LuaState::newThread(LuaState* parentL) {
    LuaState* L = new LuaState(parentL->getGlobalState());

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
    L->stack_.push(Value());  // nil
    L->top_ = 1;

    L->status_ = ThreadStatus::OK;

    return L;
}

// =====================================================================
// 构造函数和析构函数
// =====================================================================

LuaState::LuaState()
    : LuaState(GlobalState::getInstance())
{
}

LuaState::LuaState(GlobalState& globalState)
    : globalState_(globalState)
    , stack_(INITIAL_STACK_SIZE)
    , top_(0)
    , callStack_(INITIAL_CI_SIZE)
    , currentCI_(0)
    , globalTable_(nullptr)
    , status_(ThreadStatus::OK)
    , openUpvalues_(nullptr)
{
}

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
    globalTable_ = new Table();
    globalState_.getGC().registerObject(globalTable_);
    globalState_.getGC().addRoot(globalTable_);

    // 初始化第一个调用信息（虚拟的主函数）
    CallInfo& ci = callStack_[0];
    ci.func = 0;
    ci.base = 1;  // func 在位置 0，局部变量从 1 开始
    ci.top = MIN_STACK_SIZE;
    ci.savedpc = nullptr;
    ci.nresults = MULTRET;
    ci.tailcalls = 0;

    // 在栈上放置一个nil值作为虚拟函数
    stack_.push(Value());  // nil
    top_ = 1;  // 栈顶指向下一个可用位置

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
    Upvalue* newUpval = Upvalue::createOpen(stackIndex, stack_);

    // 4. 插入链表（保持降序）
    newUpval->setNext(curr);
    if (prev == nullptr) {
        // 插入到链表头
        openUpvalues_ = newUpval;
    } else {
        // 插入到prev之后
        prev->setNext(newUpval);
    }

    // 5. 注册到GC
    globalState_.getGC().registerObject(newUpval);

    return newUpval;
}

void LuaState::closeUpvalues(usize level) {
    // 关闭所有栈索引 >= level 的upvalue
    while (openUpvalues_ != nullptr &&
           openUpvalues_->getStackIndex() >= level) {
        Upvalue* uv = openUpvalues_;
        openUpvalues_ = uv->getNext();  // 从链表移除
        uv->close(stack_);               // ✅ 改进：传入stack_引用
        uv->setNext(nullptr);            // 清除链表指针
    }
}

// =====================================================================
// Debug hook support
// =====================================================================

void LuaState::setDebugHook(Function* hook, u8 mask, i32 count) {
    GarbageCollector& gc = globalState_.getGC();

    if (hookFunc_ != nullptr && hookFunc_ != hook) {
        gc.removeRoot(hookFunc_);
    }

    if (hook == nullptr) {
        if (hookFunc_ != nullptr) {
            gc.removeRoot(hookFunc_);
        }
        hookFunc_ = nullptr;
        hookMask_ = 0;
        hookCount_ = 0;
        hookCountdown_ = 0;
        hookActive_ = false;
        return;
    }

    gc.addRoot(hook);
    hookFunc_ = hook;
    hookMask_ = mask;
    hookCount_ = std::max(0, count);
    hookCountdown_ = hookCount_;
}

bool LuaState::consumeDebugHookCount() {
    if (hookFunc_ == nullptr || hookActive_ || hookCount_ <= 0) {
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
    if (hookFunc_ == nullptr || hookActive_) {
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

        VM::call(this, 2, 0);
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
        throw MemoryError(
            "stack overflow: maximum call depth exceeded (limit: " +
            std::to_string(MAX_CALL_DEPTH) + ")"
        );
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
        throw MemoryError("VM: stack overflow (too many nested C/Lua calls)");
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

    // 填充nil值或收缩栈
    while (static_cast<i32>(stack_.size()) < newTop) {
        stack_.push(Value());  // nil
    }
    while (static_cast<i32>(stack_.size()) > newTop) {
        stack_.pop();
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
        absIdx = static_cast<i32>(base) + idx - 1;  // 转换为0-based
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
        absIdx = static_cast<i32>(top_) + idx;  // 注意：replace 不加 1
    } else {
        usize base = 0;
        if (currentCI_ > 0) {
            base = callStack_[currentCI_].base;
        }
        absIdx = static_cast<i32>(base) + idx - 1;  // 转换为0-based
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
        auto& pool = getGlobalState().getStringPool();
        pushString(pool.intern("pcall: invalid function index"));
        return LUA_ERRRUN;
    }

    // 保护调用结束后只回收 func/args 之上的逻辑栈顶，不能覆盖外层帧槽位：
    // 被保护函数在抛错前可能已经写入了外层 local/upvalue。
    usize savedPrefixTop = static_cast<usize>(funcIdx);

    Vec<CallInfo> savedCallStack = callStack_;
    usize savedCurrentCI = currentCI_;
    auto& pool = getGlobalState().getStringPool();

    Value errorHandler;
    bool hasErrorHandler = false;
    if (errfunc != 0) {
        try {
            errorHandler = at(errfunc);
            hasErrorHandler = errorHandler.isFunction();
        } catch (...) {
            hasErrorHandler = false;
        }
    }

    auto restoreStackPrefix = [&]() {
        top_ = savedPrefixTop;
    };

    auto restoreCallFrames = [&]() {
        callStack_ = savedCallStack;
        currentCI_ = savedCurrentCI;
    };

    auto makeStringValue = [&](const Str& message) -> Value {
        return Value(pool.intern(message.c_str()));
    };

    auto invokeErrorHandler = [&](const Value& errorValue) -> Value {
        if (!hasErrorHandler) {
            return errorValue;
        }

        usize handlerSavedCI = currentCI_;
        if (currentCI_ + 1 >= MAX_CALL_DEPTH && currentCI_ > 0) {
            --currentCI_;
        }

        try {
            pushValue(errorHandler);
            pushValue(errorValue);
            VM::call(this, 1, 1);
            Value handled = top();
            currentCI_ = handlerSavedCI;
            return handled;
        } catch (...) {
            currentCI_ = handlerSavedCI;
            return makeStringValue("error in error handling");
        }
    };

    Value& funcVal = stack_.at(funcIdx);
    if (!funcVal.isFunction()) {
        Value errorValue = invokeErrorHandler(makeStringValue("attempt to call a non-function value"));
        restoreCallFrames();
        restoreStackPrefix();
        pushValue(errorValue);
        setStatus(ThreadStatus::OK);
        return LUA_ERRRUN;
    }

    Function* func = funcVal.asFunction();
    if (!func) {
        Value errorValue = invokeErrorHandler(makeStringValue("invalid function"));
        restoreCallFrames();
        restoreStackPrefix();
        pushValue(errorValue);
        setStatus(ThreadStatus::OK);
        return LUA_ERRRUN;
    }

    try {
        VM::call(this, nargs, nresults);
        setStatus(ThreadStatus::OK);
        return LUA_OK;

    } catch (const LuaError& e) {
        Value errorValue = e.hasErrorObject()
                         ? e.getErrorObject()
                         : makeStringValue(runtimeErrorWithLocation(this, e.what()));
        errorValue = invokeErrorHandler(errorValue);

        restoreCallFrames();
        restoreStackPrefix();
        pushValue(errorValue);
        setStatus(ThreadStatus::OK);
        return LUA_ERRRUN;

    } catch (const std::exception& e) {
        Value errorValue = makeStringValue(runtimeErrorWithLocation(this, e.what()));
        errorValue = invokeErrorHandler(errorValue);

        restoreCallFrames();
        restoreStackPrefix();
        pushValue(errorValue);
        setStatus(ThreadStatus::OK);
        return LUA_ERRRUN;
    }
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

void LuaState::setGlobal(const Str& name, const Value& value) {
    if (!globalTable_) {
        throw RuntimeError("global table not initialized");
    }

    // 创建字符串键
    GCString* key = globalState_.getStringPool().intern(name);
    globalTable_->set(Value(key), value);
}

Value LuaState::getGlobal(const Str& name) {
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
        return at(idx).isNumber();
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
        return true;  // 无效索引视为nil
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
        if (v.isNil()) return 0;        // LUA_TNIL
        if (v.isBoolean()) return 1;    // LUA_TBOOLEAN
        if (v.isNumber()) return 3;     // LUA_TNUMBER
        if (v.isString()) return 4;     // LUA_TSTRING
        if (v.isTable()) return 5;      // LUA_TTABLE
        if (v.isFunction()) return 6;   // LUA_TFUNCTION
        if (v.isUserdata()) return 7;   // LUA_TUSERDATA
        if (v.isThread()) return 8;     // LUA_TTHREAD
        return -1;  // LUA_TNONE
    } catch (...) {
        return -1;  // LUA_TNONE
    }
}

const char* LuaState::typeName(i32 tp) const {
    static const char* typeNames[] = {
        "nil", "boolean", "lightuserdata", "number",
        "string", "table", "function", "userdata", "thread"
    };
    if (tp >= 0 && tp < 9) {
        return typeNames[tp];
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
        // TODO: 字符串到数字的转换
        return 0.0;
    } catch (...) {
        return 0.0;
    }
}

const char* LuaState::toString(i32 idx) {
    try {
        Value& v = at(idx);
        if (v.isString()) {
            return v.asString()->c_str();
        }
        if (v.isNumber()) {
            // 将数字转换为字符串并替换栈上的值
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.14g", v.asNumber());
            GCString* str = globalState_.getStringPool().intern(buffer);
            v = Value(str);
            return str->c_str();
        }
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

bool LuaState::toBoolean(i32 idx) const {
    try {
        const Value& v = at(idx);
        // Lua中只有nil和false是假值
        if (v.isNil()) return false;
        if (v.isBoolean()) return v.asBoolean();
        return true;  // 其他所有值都是真值
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
        if (!v.isTable() && !v.isUserdata()) {
            return false;  // 只能为表设置元表
        }

        Value& mt = top();
        if (mt.isNil()) {
            if (v.isTable()) {
                v.asTable()->setMetatable(nullptr);
            } else {
                v.asUserdata()->setMetatable(nullptr);
            }
        } else if (mt.isTable()) {
            if (v.isTable()) {
                v.asTable()->setMetatable(mt.asTable());
            } else {
                v.asUserdata()->setMetatable(mt.asTable());
            }
        } else {
            return false;  // 元表必须是表或nil
        }

        pop();  // 弹出元表
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

i32 LuaState::error() {
    setStatus(ThreadStatus::ErrRun);
    throw RuntimeError(top());
}

} // namespace Lua

