/**
 * @file lua_state.cpp
 * @brief Lua状态管理实现
 *
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#include "vm/lua_state.hpp"
#include "common/lua_error.hpp"
#include "core/gc_string.hpp"
#include "core/userdata.hpp"
#include "core/function.hpp"
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

} // namespace

// =====================================================================
// 静态工厂方法
// =====================================================================

LuaState* LuaState::newState() {
    LuaState* L = new LuaState();
    L->initialize();
    return L;
}

LuaState* LuaState::newThread(LuaState* parentL) {
    LuaState* L = new LuaState();

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
    : globalState_(GlobalState::getInstance())
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
    ci.base = 1;  // ⭐ P0修复：base应该是1（func在位置0，局部变量从1开始）
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
        throw std::runtime_error(
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
        throw std::runtime_error("LuaState::popCallInfo: cannot pop base CallInfo");
    }

    // ✅ 改进：清理当前CallInfo（调试模式）
    #ifdef DEBUG
    callStack_[currentCI_].reset();
    #endif

    currentCI_--;
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
    // 参考：lua_c_analysis/src/lapi.c:618 lua_settop
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
        throw std::runtime_error("invalid stack index");
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
    stack_.push(at(idx));
    top_ = stack_.size();  // 同步 top_
}

void LuaState::insert(i32 idx) {
    // 参考：lua_c_analysis/src/lapi.c:753 lua_insert
    // 官方实现：
    //   p = index2adr(L, idx);
    //   for (q = L->top; q>p; q--) setobjs2s(L, q, q-1);
    //   setobjs2s(L, p, L->top);
    //
    // 将栈顶元素插入到指定位置，其他元素向上移动
    // 注意：栈大小不变，栈顶元素被移动到目标位置

    if (stack_.size() == 0) {
        throw std::runtime_error("insert: stack is empty");
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

    if (absIdx < 0 || absIdx >= static_cast<i32>(stack_.size())) {
        throw std::runtime_error("insert: invalid index");
    }

    // 保存栈顶元素
    Value topValue = stack_.at(stack_.size() - 1);

    // 将元素从 absIdx 到 top-1 向上移动一位
    // 从栈顶向下移动（避免覆盖）
    for (i32 i = static_cast<i32>(stack_.size()) - 1; i > absIdx; i--) {
        stack_.at(i) = stack_.at(i - 1);
    }

    // 将原栈顶元素放到目标位置
    stack_.at(absIdx) = topValue;

    // 栈大小不变，top_ 也不变
}

void LuaState::replace(i32 idx) {
    // 参考：lua_c_analysis/src/lapi.c lua_replace
    // 用栈顶元素替换指定位置的元素，然后弹出栈顶

    if (stack_.size() == 0) {
        throw std::runtime_error("replace: stack is empty");
    }

    // 获取栈顶元素
    Value top = stack_.top();

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

    if (absIdx < 0 || absIdx >= static_cast<i32>(stack_.size()) - 1) {
        throw std::runtime_error("replace: invalid index");
    }

    // 替换目标位置的元素
    stack_.at(absIdx) = top;

    // 弹出栈顶
    stack_.pop();
    top_ = stack_.size();  // 同步 top_
}

i32 LuaState::pcall(i32 nargs, i32 nresults, i32 errfunc) {
    // 参考：lua_c_analysis/src/lapi.c:3027 lua_pcall

    // 计算函数在栈中的绝对位置（0-based）
    // 栈布局：[... func arg1 arg2 ...]
    // 函数在参数之前，所以 funcIdx = top_ - nargs - 1
    i32 funcIdx = static_cast<i32>(top_) - nargs - 1;

    if (funcIdx < 0 || funcIdx >= static_cast<i32>(stack_.size())) {
        // 栈索引无效
        // 移除 func 和 args，压入错误消息
        while (static_cast<i32>(top_) > funcIdx) {
            pop();
        }
        auto& pool = getGlobalState().getStringPool();
        pushString(pool.intern("pcall: invalid function index"));
        return LUA_ERRRUN;
    }

    Value& funcVal = stack_.at(funcIdx);
    if (!funcVal.isFunction()) {
        // 不是函数
        // 移除 func 和 args，压入错误消息
        while (static_cast<i32>(top_) > funcIdx) {
            pop();
        }
        auto& pool = getGlobalState().getStringPool();
        pushString(pool.intern("attempt to call a non-function value"));
        return LUA_ERRRUN;
    }

    Function* func = funcVal.asFunction();
    if (!func) {
        auto& pool = getGlobalState().getStringPool();
        setTop(funcIdx);
        pushString(pool.intern("invalid function"));
        return LUA_ERRRUN;
    }

    // 保存 funcIdx 之前的栈内容（包括可能的 errfunc）
    Vec<Value> savedStack;
    for (i32 i = 0; i < funcIdx; i++) {
        savedStack.push_back(stack_.at(i));
    }

    // 保存参数
    Vec<Value> args;
    for (i32 i = 0; i < nargs; i++) {
        i32 argIdx = funcIdx + 1 + i;
        if (argIdx >= 0 && argIdx < static_cast<i32>(stack_.size())) {
            args.push_back(stack_.at(argIdx));
        }
    }

    Vec<CallInfo> savedCallStack = callStack_;
    usize savedCurrentCI = currentCI_;

    auto restoreStackPrefix = [&](const Vec<Value>& prefix) {
        stack_.clear();
        top_ = 0;
        for (const auto& v : prefix) {
            pushValue(v);
        }
    };

    auto restoreCallFrames = [&]() {
        callStack_ = savedCallStack;
        currentCI_ = savedCurrentCI;
    };

    try {
        // 使用干净的绝对栈执行 protected call，避免当前 C 栈帧残留值干扰结果布局。
        stack_.clear();
        top_ = 0;
        pushValue(Value(func));
        for (const auto& arg : args) {
            pushValue(arg);
        }

        if (func->isCFunction()) {
            CallInfo& ci = pushCallInfo();
            ci.func = 0;
            ci.base = 1;
            ci.top = 1 + static_cast<usize>(nargs) + 20;
            ci.nresults = MULTRET;
            ci.savedpc = nullptr;
            ci.tailcalls = 0;

            while (stack_.size() < ci.top) {
                stack_.push(Value());
            }
            setAbsoluteTop(1 + static_cast<usize>(nargs));

            if (hasDebugHookMask(HookMaskCall)) {
                callDebugHook(DebugHookEvent::Call);
            }

            i32 nReturnValues = func->getCFunction()(this);
            if (hasDebugHookMask(HookMaskReturn)) {
                callDebugHook(DebugHookEvent::Return);
            }
            usize currentTop = getAbsoluteTop();
            usize firstResult = currentTop - static_cast<usize>(nReturnValues);
            for (i32 i = 0; i < nReturnValues; i++) {
                stack_.at(static_cast<usize>(i)) =
                    stack_.at(firstResult + static_cast<usize>(i));
            }
            stack_.setTop(static_cast<usize>(nReturnValues));
            setAbsoluteTop(static_cast<usize>(nReturnValues));
            popCallInfo();
        } else {
            Proto* proto = func->getProto();
            if (!proto) {
                throw std::runtime_error("invalid function");
            }

            CallInfo& ci = pushCallInfo();
            ci.func = 0;
            ci.base = 1;
            ci.top = ci.base;
            ci.savedpc = nullptr;
            ci.nresults = MULTRET;
            ci.tailcalls = 0;

            usize requiredTop = ci.base + proto->getMaxStackSize();
            if (stack_.capacity() < requiredTop) {
                stack_.checkSpace(requiredTop - stack_.size());
            }
            while (stack_.size() < requiredTop) {
                stack_.push(Value());
            }

            ci.top = requiredTop;
            setAbsoluteTop(requiredTop);

            if (hasDebugHookMask(HookMaskCall)) {
                callDebugHook(DebugHookEvent::Call);
            }

            VM::executeProto(this, proto, 1);
            popCallInfo();
        }

        Vec<Value> results;
        for (usize i = 0; i < getAbsoluteTop(); i++) {
            results.push_back(stack_.at(i));
        }

        restoreCallFrames();
        restoreStackPrefix(savedStack);
        for (const auto& v : results) {
            pushValue(v);
        }

        setStatus(ThreadStatus::OK);

        // 调整返回值数量
        if (nresults != MULTRET) {
            i32 actualResults = static_cast<i32>(results.size());
            i32 savedSize = static_cast<i32>(savedStack.size());
            if (actualResults < nresults) {
                // 补充 nil
                for (i32 i = actualResults; i < nresults; i++) {
                    pushValue(Value());
                }
            } else if (actualResults > nresults) {
                // 截断：保留 saved + nresults 个结果
                setTop(savedSize + nresults);
                setAbsoluteTop(savedSize + nresults);
            }
        }

        return LUA_OK;

    } catch (const LuaError& e) {
        // LuaError: 直接使用 Lua Value 作为错误对象
        restoreCallFrames();
        restoreStackPrefix(savedStack);
        pushValue(e.getErrorObject());
        setStatus(ThreadStatus::OK);
        return LUA_ERRRUN;

    } catch (const std::exception& e) {
        // 捕获异常并返回错误
        // 恢复栈：[saved...] [error_msg]
        restoreCallFrames();
        restoreStackPrefix(savedStack);

        auto& pool = getGlobalState().getStringPool();

        // 如果有错误处理函数，调用它
        if (errfunc != 0) {
            // TODO: 实现错误处理函数调用
            // 当前简化版本直接返回错误消息
        }

        pushString(pool.intern(e.what()));
        setStatus(ThreadStatus::OK);
        return LUA_ERRRUN;
    }
}

i32 LuaState::getTop() const {
    // 参考：lua_c_analysis/src/lapi.c:608 lua_gettop
    // 返回值 = L->top - L->base
    if (currentCI_ > 0) {
        const CallInfo& ci = callStack_[currentCI_];
        return static_cast<i32>(top_ - ci.base);
    }
    // 没有调用帧时，返回 top_
    return static_cast<i32>(top_);
}

Value& LuaState::at(i32 idx) {
    // 参考：lua_c_analysis/src/lapi.c:164-186 index2adr
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
        throw std::runtime_error("global table not initialized");
    }

    // 创建字符串键
    GCString* key = globalState_.getStringPool().intern(name);
    globalTable_->set(Value(key), value);
}

Value LuaState::getGlobal(const Str& name) {
    if (!globalTable_) {
        throw std::runtime_error("global table not initialized");
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
    throw std::runtime_error(msg);
}

i32 LuaState::error() {
    setStatus(ThreadStatus::ErrRun);
    const char* msg = toString(-1);
    if (msg) {
        throw std::runtime_error(msg);
    }
    throw std::runtime_error("error object is not a string");
}

} // namespace Lua

