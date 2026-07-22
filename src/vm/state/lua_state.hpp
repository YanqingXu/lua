/**
 * @file lua_state.hpp
 * @brief Lua状态管理：每个线程的独立执行环境
 *
 * 详细说明：
 * Lua 状态类表示一个 Lua 线程（协程）的完整执行状态，包括：
 * - 值栈：存储函数参数、局部变量和临时值
 * - 调用栈：管理函数调用层次
 * - 全局状态引用：访问共享资源
 * - 全局表：线程的全局变量表
 * - 执行状态：正常、挂起、错误等
 *
 * 核心特性：
 * - 独立执行：每个 Lua 状态有独立的栈和调用信息
 * - 资源共享：通过全局状态共享字符串驻留池、垃圾回收器等资源
 * - 协程支持：为后续协程实现预留接口
 * - 现代 C++：使用资源获取即初始化管理资源
 * @author Lua C++ 项目
 * @date 2025-11-12
 */

#pragma once

#include "common/types.hpp"
#include "common/lua_error.hpp"
#include "core/value.hpp"
#include "core/table.hpp"
#include "runtime/lua_allocator.hpp"
#include "vm/state/global_state.hpp"
#include "vm/state/stack.hpp"
#include "vm/state/call_info.hpp"
#include "vm/vm_constants.hpp"
#include <expected>

struct lua_State;
struct lua_Debug;

namespace Lua {

// 前向声明
class Upvalue;
class Userdata;
class Thread;
class EngineContext;
struct RuntimeServices;

/** @brief 按分配方式销毁拥有型运行时上下文的删除器。 */
struct EngineContextDeleter {
    bool allocatorBacked = false;
    void operator()(EngineContext* context) const noexcept;
};

/**
 * @brief Lua线程状态枚举
 */
enum class ThreadStatus : u8 {
    /** @brief 正常执行状态 */
    OK = 0,
    /** @brief 协程挂起状态 */
    Yield = 1,
    /** @brief 运行时错误 */
    ErrRun = 2,
    /** @brief 语法错误 */
    ErrSyntax = 3,
    /** @brief 内存错误 */
    ErrMem = 4,
    /** @brief 错误处理函数错误 */
    ErrErr = 5
};

/**
 * @brief 调试钩子掩码位
 */
enum DebugHookMask : u8 {
    HookMaskCall = 1 << 0,
    HookMaskReturn = 1 << 1,
    HookMaskLine = 1 << 2,
    HookMaskCount = 1 << 3
};

using ApiDebugHook = void (*)(::lua_State* L, ::lua_Debug* ar);

/**
 * @brief 调试钩子事件类型
 */
enum class DebugHookEvent : u8 { Call, Return, TailReturn, Line, Count };

/**
 * @brief Lua状态类
 *
 * 管理单个Lua线程的完整执行状态。
 *
 * 使用示例：
 * @code
 * // 创建主线程
 * UPtr<LuaState> L = LuaState::create();
 *
 * // 压入值到栈
 * L->pushNumber(42.0);
 * L->pushBoolean(true);
 *
 * // 访问栈
 * Value v = L->getStack().top();
 *
 * // 清理
 * // L 自动释放
 * @endcode
 */
class LuaState {
public:
    struct CtorToken {
    private:
        constexpr CtorToken() = default;
        friend class LuaState;
    };

    // =====================================================================
    // 构造函数和析构函数
    // =====================================================================

    /**
     * @brief 构造低层状态对象；普通调用者应使用 create()/newState() 工厂。
     *
     * CtorToken 只能由 LuaState 工厂构造，用于让 makeUnique 在不暴露裸 new 的情况下访问构造路径。
     */
    LuaState(CtorToken, GlobalState& globalState);
    LuaState(CtorToken, GlobalState& globalState, bool allocatorOwnedSelf);

    /**
     * @brief 构造拥有独立运行时上下文的状态
     *
     * 令牌将构造过程限制在工厂内部，同时保证拥有型上下文晚于所有引用它的状态成员析构。
     */
    LuaState(CtorToken, EngineContext* ownedContext, bool allocatorOwnedContext, bool allocatorOwnedSelf);

    /**
     * @brief 创建拥有型Lua状态（主线程）
     * @return 承载所有权的独占指针
     */
    [[nodiscard]] static UPtr<LuaState> create();

    /**
     * @brief 使用显式运行时服务创建拥有型Lua状态（主线程）
     */
    [[nodiscard]] static UPtr<LuaState> create(RuntimeServices& services);

    /**
     * @brief 使用拥有资源的运行时上下文创建拥有型Lua状态（主线程）
     */
    [[nodiscard]] static UPtr<LuaState> create(EngineContext& context);

    /**
     * @brief 创建拥有独立 EngineContext 的状态
     */
    [[nodiscard]] static UPtr<LuaState> createIsolated();

    /**
     * @brief 创建新的Lua状态（主线程），兼容旧 C API 风格所有权
     * @return 由调用者负责销毁的 Lua 状态指针
     */
    static LuaState* newState();

    /**
     * @brief 使用显式运行时服务创建新的Lua状态（主线程）
     */
    static LuaState* newState(RuntimeServices& services);

    /**
     * @brief 使用拥有资源的运行时上下文创建新的Lua状态（主线程）
     */
    static LuaState* newState(EngineContext& context);

    /**
     * @brief 创建由调用者持有且拥有独立 EngineContext 的状态
     */
    static LuaState* newIsolatedState();

    /**
     * @brief 创建上下文与状态内存块均使用 lua_Alloc 的 C API 状态
     */
    static LuaState* newAllocatedState(LuaAllocatorFunction allocator, void* userData);

    /**
     * @brief 正确销毁普通状态或由分配器支撑的状态
     */
    static void destroyState(LuaState* state) noexcept;

    /**
     * @brief 析构函数
     */
    ~LuaState();

    // 禁止拷贝和赋值
    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    // =====================================================================
    // 栈访问
    // =====================================================================

    /**
     * @brief 获取值栈
     * @return 栈的引用
     */
    Stack& getStack() noexcept {
        return stack_;
    }

    const Stack& getStack() const noexcept {
        return stack_;
    }

    // =====================================================================
    // 栈操作（便捷方法）
    // =====================================================================

    /**
     * @brief 压入nil值
     */
    void pushNil() {
        pushValue(Value());
    }

    /**
     * @brief 压入布尔值
     */
    void pushBoolean(bool b) {
        pushValue(Value(b));
    }

    /**
     * @brief 压入数值
     */
    void pushNumber(LuaNumber n) {
        pushValue(Value(n));
    }

    /**
     * @brief 压入字符串
     */
    void pushString(GCString* str) {
        pushValue(Value(str));
    }

    /**
     * @brief 压入表
     */
    void pushTable(Table* table) {
        pushValue(Value(table));
    }

    /**
     * @brief 压入函数
     */
    void pushFunction(Function* func) {
        pushValue(Value(func));
    }

    /**
     * @brief 压入用户数据
     */
    void pushUserdata(Userdata* ud) {
        pushValue(Value(ud));
    }

    /**
     * @brief 通用压入方法（在 top_ 位置压入值）
     *
     * 注意：这个函数在 LuaState::top_ 位置设置值，
     * 这可能与 Stack::top_ 不同（当使用 setAbsoluteTop 调整栈顶后）。
     */
    void pushValue(const Value& v) {
        stack_.checkLimit(top_ + 1);
    /**
     * @brief 在普通压栈路径之外保留一个已分配的栈槽。
     *
     * 即使失败的分配操作正是栈扩容，保护 C API 边界仍可用该槽发布固定的内存错误对象。
     */
        if (stack_.capacity() == 0 || top_ >= stack_.capacity() - 1) {
            const usize available = stack_.capacity() - stack_.size();
            stack_.ensureSpace(available + STACK_GROW_MARGIN);
        }

        while (stack_.size() <= top_) {
            stack_.push(Value());
        }

        stack_[top_++] = v;
    }

    /**
     * @brief 仅使用当前状态已有的存储空间发布值
     * @param v 要发布的值
     * @return 仅当应急栈槽不变量被破坏时返回 false
     *
     * 此接口专供 C API 边界将异常转换为状态码。与 pushValue() 不同，它绝不会请求 Lua
     * 分配器扩展栈空间。
     */
    [[nodiscard]] bool tryPushValueNoAlloc(const Value& v) noexcept {
        if (top_ >= stack_.capacity()) {
            return false;
        }

        while (stack_.size() <= top_) {
            stack_.pushUnchecked(Value());
        }
        stack_[top_++] = v;
        return true;
    }

    /**
     * @brief 弹出栈顶值
     */
    Value pop() {
        if (top_ == 0) {
            throw RuntimeError("pop: stack is empty");
        }
        Value v = stack_.at(top_ - 1);
        --top_;
        return v;
    }

    /**
     * @brief 获取栈顶值
     */
    Value& top() {
        return at(-1);
    }

    /**
     * @brief 获取栈大小（栈顶索引）
     *
     * 返回当前调用帧中的栈元素数量（相对于当前base的栈顶位置）。
     * 返回值 = top - base。
     */
    i32 getTop() const;

    /**
     * @brief 设置栈大小
     * @param idx 新的栈顶索引（从1开始）
     */
    void setTop(i32 idx);

    /**
     * @brief 将栈顶元素插入到指定位置
     * @param idx 目标位置索引（1-based）
     * 将栈顶元素移动到指定位置，其他元素向上移动
     */
    void insert(i32 idx);

    /**
     * @brief 用栈顶元素替换指定位置的元素
     * @param idx 目标位置索引（1-based）
     * 用栈顶元素替换指定位置的元素，然后弹出栈顶
     */
    void replace(i32 idx);

    /**
     * @brief 保护调用函数
     *
     * 在保护模式下调用栈上的函数，捕获所有错误。
     *
     * @param nargs 参数数量
     * @param nresults 期望的返回值数量（MULTRET 表示接受所有返回值）
     * @param errfunc 错误处理函数的栈索引（0 表示无错误处理函数）
     * @return i32 状态码（LUA_OK=成功，LUA_ERRRUN=运行时错误）
     *
     * 栈布局：
     * - 调用前：[... func arg1 arg2 ...]
     * - 成功后：[... result1 result2 ...]
     * - 失败后：[... error_msg]
     */
    i32 pcall(i32 nargs, i32 nresults, i32 errfunc);

    /**
     * @brief 现代保护调用外观，并保持 pcall 的栈效果。
     */
    [[nodiscard]] std::expected<i32, RuntimeError> tryPCall(i32 nargs, i32 nresults, i32 errfunc);

    /**
     * @brief 获取绝对栈顶索引
     *
     * 返回栈顶的绝对索引（用于 VM 内部）
     */
    usize getAbsoluteTop() const noexcept {
        return top_;
    }

    /**
     * @brief 设置绝对栈顶索引
     *
     * 设置栈顶的绝对索引（用于 VM 内部）
     */
    void setAbsoluteTop(usize top) {
        stack_.checkLimit(top);
        top_ = top;
    }

    /**
     * @brief 增加栈顶
     */
    void incrTop() {
        stack_.checkLimit(top_ + 1);
        if (top_ >= stack_.size()) {
            stack_.push(Value());
        }
        top_++;
    }

    /**
     * @brief 压入栈中指定索引的值的副本
     * @param idx 栈索引（从1开始，负数表示从栈顶倒数）
     */
    void pushValue(i32 idx);

    /**
     * @brief 获取栈中指定索引的值
     * @param idx 栈索引（从1开始，负数表示从栈顶倒数）
     * @return 值的引用
     */
    Value& at(i32 idx);

    const Value& at(i32 idx) const;

    // =====================================================================
    // 全局状态访问
    // =====================================================================

    /**
     * @brief 获取全局状态
     */
    GlobalState& getGlobalState() noexcept {
        return globalState_;
    }

    /**
     * @brief 获取全局表
     */
    Table* getGlobalTable() noexcept {
        return globalTable_;
    }

    void setGlobalTable(Table* table);

    /**
     * @brief 设置全局变量
     * @param name 变量名
     * @param value 值
     */
    void setGlobal(StrView name, const Value& value);

    /**
     * @brief 获取全局变量
     * @param name 变量名
     * @return 值
     */
    Value getGlobal(StrView name);

    // =====================================================================
    // 类型检查和转换
    // =====================================================================

    /**
     * @brief 检查栈索引处的值是否为数字
     */
    bool isNumber(i32 idx) const;

    /**
     * @brief 检查栈索引处的值是否为字符串
     */
    bool isString(i32 idx) const;

    /**
     * @brief 检查栈索引处的值是否为表
     */
    bool isTable(i32 idx) const;

    /**
     * @brief 检查栈索引处的值是否为函数
     */
    bool isFunction(i32 idx) const;

    /**
     * @brief 检查栈索引处的值是否为nil
     */
    bool isNil(i32 idx) const;

    /**
     * @brief 检查栈索引处的值是否为布尔值
     */
    bool isBoolean(i32 idx) const;

    /**
     * @brief 检查栈索引处的值是否为用户数据
     */
    bool isUserdata(i32 idx) const;

    /**
     * @brief 获取值的类型
     * @param idx 栈索引
     * @return 类型枚举值
     */
    i32 type(i32 idx) const;

    /**
     * @brief 获取类型名称
     * @param tp 类型枚举值
     * @return 类型名称字符串
     */
    const char* typeName(i32 tp) const;

    /**
     * @brief 将栈索引处的值转换为数字
     */
    LuaNumber toNumber(i32 idx) const;

    /**
     * @brief 将栈索引处的值转换为字符串
     */
    Opt<StrView> tryToString(i32 idx);
    const char* toString(i32 idx);

    /**
     * @brief 将栈索引处的值转换为布尔值
     */
    bool toBoolean(i32 idx) const;

    // =====================================================================
    // 元表操作
    // =====================================================================

    /**
     * @brief 获取对象的元表
     * @param idx 栈索引
     * @return 如果有元表返回true，否则返回false
     */
    bool getMetatable(i32 idx);

    /**
     * @brief 设置表的元表
     * @param idx 栈索引（必须是表）
     * @return 成功返回true
     */
    bool setMetatable(i32 idx);

    // =====================================================================
    // 错误处理
    // =====================================================================

    /**
     * @brief 抛出错误
     * @param msg 错误消息
     * @return 不返回
     */
    [[noreturn]] void error(const char* msg);

    /**
     * @brief 拒绝当前上下文禁用的 Lua 特权操作
     */
    void requireSandboxCapability(SandboxCapability capability);

    /**
     * @brief 统计原生标准库循环内部执行的工作量
     */
    void consumeNativeWork(ExecutionPolicy::NativeWorkCount units = 1);

    /**
     * @brief 拒绝显式打开已禁用的标准库
     */
    void requireStandardLibrary(StrView id);

    /**
     * @brief 抛出错误（使用栈顶的值作为错误消息）
     * @return 不返回
     */
    i32 error();

    // =====================================================================
    // 调用信息管理
    // =====================================================================

    /**
     * @brief 获取当前调用信息
     */
    CallInfo& getCurrentCallInfo() noexcept {
        return callStack_[currentCI_];
    }

    const CallInfo& getCurrentCallInfo() const noexcept {
        return callStack_[currentCI_];
    }

    /**
     * @brief 获取调用栈大小
     */
    usize getCallStackSize() const noexcept {
        return currentCI_ + 1;
    }

    /**
     * @brief 分配新的CallInfo（用于函数调用）
     * @return 新的CallInfo引用
     *
     * 详细说明：
     * 当进行函数调用时，需要分配新的CallInfo来存储调用上下文。
     * 如果调用栈已满，会自动扩展（双倍增长）。
     */
    CallInfo& pushCallInfo();

    /**
     * @brief 弹出当前CallInfo（用于函数返回）
     *
     * 详细说明：
     * 当函数返回时，需要弹出当前CallInfo，恢复到调用者的上下文。
     */
    void popCallInfo();

    /**
     * @brief 获取当前CallInfo索引
     */
    usize getCurrentCI() const noexcept {
        return currentCI_;
    }

    // =====================================================================
    // 线程状态管理
    // =====================================================================

    /**
     * @brief 获取线程状态
     */
    ThreadStatus getStatus() const noexcept {
        return status_;
    }

    /**
     * @brief 设置线程状态
     */
    void setStatus(ThreadStatus status) noexcept {
        status_ = status;
    }

    // =====================================================================
    // Upvalue管理
    // =====================================================================

    /**
     * @brief 查找或创建指向栈位置的上值
     * @param stackIndex 栈索引位置
     * @return 上值指针
     *
     * 详细说明：
     * 这是上值管理的核心函数，实现了上值的共享机制。
     * 多个闭包可以共享指向同一栈位置的上值，确保变量语义的正确性。
     *
     * 查找策略：
     * 1. 遍历openUpvalues_链表（按栈索引降序排列）
     * 2. 如果找到匹配的栈位置，返回现有Upvalue
     * 3. 如果没找到，创建新的Upvalue并插入链表
     *
     * 链表维护：
     * - 链表按stackIndex降序排列
     * - 新Upvalue插入到正确位置以保持有序
     *
     * GC集成：
     * - 新创建的Upvalue会注册到GC系统
     */
    Upvalue* findOrCreateUpvalue(usize stackIndex);

    /**
     * @brief 关闭指定栈层级及以上的所有开放上值
     * @param level 栈层级阈值
     *
     * 详细说明：
     * 当函数返回或栈收缩时，需要关闭相应的上值。
     * 关闭操作将开放状态的上值转换为关闭状态。
     *
     * 处理策略：
     * 1. 遍历openUpvalues_链表
     * 2. 找到所有栈索引 >= level的Upvalue
     * 3. 调用Upvalue::close()关闭它们
     * 4. 从链表中移除
     *
     * 调用时机：
     * - 函数返回时
     * - 块结束时
     * - 栈收缩时
     */
    void closeUpvalues(usize level);

    // =====================================================================
    /** @brief 协程支持。 */
    // =====================================================================

    /**
     * @brief 创建子线程（协程用）
     * 共享全局状态和全局表，拥有独立值栈和调用栈。
     */
    static LuaState* newThread(LuaState* parentL);

    /**
     * @brief 挂起许可计数器
     */
    void incAllowYield() noexcept {
        allowYield_++;
    }
    void decAllowYield() noexcept {
        if (allowYield_ > 0)
            allowYield_--;
    }
    bool canYield() const noexcept {
        return allowYield_ > 0;
    }

    /**
     * @brief 挂起值数量
     */
    void setYieldResults(i32 n) noexcept {
        yieldResults_ = n;
    }
    i32 getYieldResults() const noexcept {
        return yieldResults_;
    }

    /**
     * @brief nexeccalls 保存/恢复
     */
    void setSavedNexeccalls(i32 n) noexcept {
        savedNexeccalls_ = n;
    }
    i32 getSavedNexeccalls() const noexcept {
        return savedNexeccalls_;
    }

    /** @brief 通过 VM::call 重新进入 Lua 的 C/C++ 宿主调用帧。 */
    void enterHostCall();
    void leaveHostCall() noexcept;
    i32 getHostCallDepth() const noexcept {
        return hostCallDepth_;
    }
    void setHostCallDepth(i32 depth) noexcept {
        hostCallDepth_ = depth;
    }

    /**
     * @brief 当前 Lua 状态对应的协程对象（主线程为空指针）
     */
    Thread* getThread() const noexcept {
        return thread_;
    }
    void setThread(Thread* t) noexcept {
        thread_ = t;
    }

    Thread* getMainThreadFacade() const noexcept {
        return mainThreadFacade_;
    }
    void setMainThreadFacade(Thread* thread) noexcept {
        mainThreadFacade_ = thread;
    }

    /**
     * @brief 调用栈访问（供协程对象执行垃圾回收标记时使用）
     */
    LuaVector<CallInfo>& getCallStack() noexcept {
        return callStack_;
    }

    /**
     * @brief 开放上值链表头访问
     */
    Upvalue* getOpenUpvalues() const noexcept {
        return openUpvalues_;
    }

    // =====================================================================
    /** @brief 调试钩子支持。 */
    // =====================================================================

    /**
     * @brief 安装或清除线程级调试钩子
     * @param hook 钩子函数；空指针表示清除钩子
     * @param mask 由调试钩子掩码值组成的位掩码
     * @param count 计数钩子的指令间隔
     */
    void setDebugHook(Function* hook, u8 mask, i32 count);

    /**
     * @brief 安装或清除公开的 Lua 5.1 C 调试钩子
     */
    void setApiDebugHook(ApiDebugHook hook, u8 mask, i32 count);

    /**
     * @brief 获取已安装的调试钩子函数
     */
    Function* getDebugHook() const noexcept {
        return hookFunc_;
    }

    ApiDebugHook getApiDebugHook() const noexcept {
        return apiDebugHook_;
    }

    /**
     * @brief 获取调试钩子掩码位
     */
    u8 getDebugHookMask() const noexcept {
        return hookMask_;
    }

    /**
     * @brief 获取计数钩子的触发间隔
     */
    i32 getDebugHookCount() const noexcept {
        return hookCount_;
    }

    /**
     * @brief 检查调试钩子当前是否正在运行
     */
    bool isDebugHookActive() const noexcept {
        return hookActive_;
    }

    /**
     * @brief 检查指定的钩子掩码位是否启用
     */
    bool hasDebugHookMask(u8 mask) const noexcept {
        return (hookFunc_ != nullptr || apiDebugHook_ != nullptr) && (hookMask_ & mask) != 0;
    }

    /**
     * @brief 为计数钩子消耗一条指令配额
     * @return 应触发计数钩子时返回 true
     */
    bool consumeDebugHookCount();

    /**
     * @brief 针对 VM 事件调用已安装的调试钩子
     * @param event 钩子事件类型
     * @param line 当前行号；非行事件传入 -1
     */
    void callDebugHook(DebugHookEvent event, i32 line = -1);

private:
    /**
     * @brief 私有构造函数
     */
    LuaState();
    explicit LuaState(GlobalState& globalState);

    /**
     * @brief 初始化状态
     */
    void initialize();

    // =====================================================================
    // 成员变量
    // =====================================================================

    /** @brief 独立创建的公开 C API 状态可选拥有的上下文。 */
    std::unique_ptr<EngineContext, EngineContextDeleter> ownedContext_;
    bool allocatorOwnedSelf_;

    /**
     * @brief 全局状态引用
     */
    GlobalState& globalState_;

    /**
     * @brief 值栈
     */
    Stack stack_;

    /**
     * @brief 栈顶索引（指向下一个可用位置）
     */
    usize top_;

    /**
     * @brief 调用信息栈
     */
    LuaVector<CallInfo> callStack_;

    /**
     * @brief 当前调用信息索引
     */
    usize currentCI_;

    /**
     * @brief 全局表
     */
    Table* globalTable_;

    /**
     * @brief 线程状态
     */
    ThreadStatus status_;

    /**
     * @brief 开放上值链表头（按栈索引降序排列）
     * @note 上值由垃圾回收器管理，这里只持有指针。
     */
    Upvalue* openUpvalues_;

    // =====================================================================
    // Coroutine 相关字段
    // =====================================================================

    /**
     * @brief 允许挂起的嵌套层数（大于 0 可挂起，等于 0 不可挂起）
     */
    u16 allowYield_ = 0;

    /**
     * @brief 挂起返回值数量
     */
    i32 yieldResults_ = 0;

    /**
     * @brief 保存的执行深度（挂起时写入，恢复时还原）
     */
    i32 savedNexeccalls_ = 1;

    /**
     * @brief 嵌套 C/C++ 到 Lua 的调用深度，用于在宿主栈耗尽前报告 Lua 栈溢出。
     */
    i32 hostCallDepth_ = 0;

    /**
     * @brief 所属协程对象（主线程为空指针）
     */
    Thread* thread_ = nullptr;
    Thread* mainThreadFacade_ = nullptr;

    /**
     * @brief 是否由新建线程接口创建（析构时不移除全局表根对象）
     */
    bool isChildThread_ = false;

    /** @brief 已安装的调试钩子函数。 */
    Function* hookFunc_ = nullptr;

    /** @brief 已安装的公开 C 调试钩子回调。 */
    ApiDebugHook apiDebugHook_ = nullptr;

    /** @brief 钩子掩码位（调用、返回、行、计数）。 */
    u8 hookMask_ = 0;

    /** @brief 计数钩子的指令间隔。 */
    i32 hookCount_ = 0;

    /** @brief 距离下一次计数钩子触发的剩余指令数。 */
    i32 hookCountdown_ = 0;

    /** @brief 防止钩子运行期间递归调用钩子。 */
    bool hookActive_ = false;

public:
    // =====================================================================
    // ✅ 改进：调试支持
    // =====================================================================

#ifdef DEBUG
    /**
     * @brief 打印调用栈（用于调试）
     */
    void dumpCallStack() const;

    /**
     * @brief 验证调用栈状态
     */
    void validateCallStack() const;
#endif
};

} // namespace Lua
