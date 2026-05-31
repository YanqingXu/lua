/**
 * @file lua_state.hpp
 * @brief Lua状态管理：每个线程的独立执行环境
 * 
 * 详细说明：
 * LuaState类表示一个Lua线程（协程）的完整执行状态，包括：
 * - 值栈（Stack）：存储函数参数、局部变量和临时值
 * - 调用栈（CallInfo数组）：管理函数调用层次
 * - 全局状态引用：访问共享资源
 * - 全局表：线程的全局变量表
 * - 执行状态：正常、挂起、错误等
 * 
 * 核心特性：
 * - 独立执行：每个LuaState有独立的栈和调用信息
 * - 资源共享：通过GlobalState共享字符串池、GC等资源
 * - 协程支持：为后续协程实现预留接口
 * - 现代C++：使用RAII管理资源
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#pragma once

#include "common/types.hpp"
#include "common/lua_error.hpp"
#include "core/value.hpp"
#include "core/table.hpp"
#include "vm/state/global_state.hpp"
#include "vm/state/stack.hpp"
#include "vm/state/call_info.hpp"
#include "vm/vm_constants.hpp"

namespace Lua {

// 前向声明
class Upvalue;
class Userdata;
class Thread;
class EngineContext;
struct RuntimeServices;

/**
 * @brief Lua线程状态枚举
 */
enum class ThreadStatus : u8 {
    OK = 0,         ///< 正常执行状态
    Yield = 1,      ///< 协程挂起状态
    ErrRun = 2,     ///< 运行时错误
    ErrSyntax = 3,  ///< 语法错误
    ErrMem = 4,     ///< 内存错误
    ErrErr = 5      ///< 错误处理函数错误
};

/**
 * @brief Debug hook mask bits
 */
enum DebugHookMask : u8 {
    HookMaskCall = 1 << 0,
    HookMaskReturn = 1 << 1,
    HookMaskLine = 1 << 2
};

/**
 * @brief Debug hook event kinds
 */
enum class DebugHookEvent : u8 {
    Call,
    Return,
    TailReturn,
    Line,
    Count
};

/**
 * @brief Lua状态类
 * 
 * 管理单个Lua线程的完整执行状态。
 * 
 * 使用示例：
 * @code
 * // 创建主线程
 * LuaState* L = LuaState::newState();
 * 
 * // 压入值到栈
 * L->pushNumber(42.0);
 * L->pushBoolean(true);
 * 
 * // 访问栈
 * Value v = L->getStack().top();
 * 
 * // 清理
 * delete L;
 * @endcode
 */
class LuaState {
public:

    // =====================================================================
    // 构造函数和析构函数
    // =====================================================================
    
    /**
     * @brief 创建新的Lua状态（主线程）
     * @return 新创建的LuaState指针
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
        if (top_ >= stack_.capacity()) {
            stack_.ensureSpace(top_ - stack_.capacity() + STACK_GROW_MARGIN);
        }

        while (stack_.size() <= top_) {
            stack_.push(Value());
        }

        stack_[top_++] = v;
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
     * @brief 保护调用函数（Protected Call）
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
    void setAbsoluteTop(usize top) noexcept {
        top_ = top;
    }

    /**
     * @brief 增加栈顶
     */
    void incrTop() {
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
    void setGlobal(const Str& name, const Value& value);

    /**
     * @brief 获取全局变量
     * @param name 变量名
     * @return 值
     */
    Value getGlobal(const Str& name);

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
     * @brief 查找或创建指向栈位置的Upvalue
     * @param stackIndex 栈索引位置
     * @return Upvalue指针
     *
     * 详细说明：
     * 这是Upvalue管理的核心函数，实现了Upvalue的共享机制。
     * 多个闭包可以共享指向同一栈位置的Upvalue，确保变量语义的正确性。
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
     * @brief 关闭指定栈层级及以上的所有Open Upvalue
     * @param level 栈层级阈值
     *
     * 详细说明：
     * 当函数返回或栈收缩时，需要关闭相应的Upvalue。
     * 关闭操作将Open状态的Upvalue转换为Closed状态。
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
    // Coroutine support
    // =====================================================================

    /**
     * @brief 创建子线程（协程用）
     * 共享 GlobalState + globalTable，独立 Stack + CallStack
     */
    static LuaState* newThread(LuaState* parentL);

    /// yield 许可计数器
    void incAllowYield() noexcept { allowYield_++; }
    void decAllowYield() noexcept { if (allowYield_ > 0) allowYield_--; }
    bool canYield() const noexcept { return allowYield_ > 0; }

    /// yield 值数量
    void setYieldResults(i32 n) noexcept { yieldResults_ = n; }
    i32  getYieldResults() const noexcept { return yieldResults_; }

    /// nexeccalls 保存/恢复
    void setSavedNexeccalls(i32 n) noexcept { savedNexeccalls_ = n; }
    i32  getSavedNexeccalls() const noexcept { return savedNexeccalls_; }

    /// C/C++ host frames that re-enter Lua through VM::call.
    void enterHostCall();
    void leaveHostCall() noexcept;
    i32 getHostCallDepth() const noexcept { return hostCallDepth_; }

    /// 当前 LuaState 对应的 Thread 对象（主线程为 nullptr）
    Thread* getThread() const noexcept { return thread_; }
    void setThread(Thread* t) noexcept { thread_ = t; }

    /// 调用栈访问（供 Thread GC marking 使用）
    Vec<CallInfo>& getCallStack() noexcept { return callStack_; }

    /// Open Upvalue 链表头访问
    Upvalue* getOpenUpvalues() const noexcept { return openUpvalues_; }

    // =====================================================================
    // Debug hook support
    // =====================================================================

    /**
     * @brief Install or clear the per-thread debug hook
     * @param hook Hook function, nullptr clears the hook
     * @param mask Bitmask made of DebugHookMask values
     * @param count Instruction count interval for count hooks
     */
    void setDebugHook(Function* hook, u8 mask, i32 count);

    /**
     * @brief Get the installed debug hook function
     */
    Function* getDebugHook() const noexcept { return hookFunc_; }

    /**
     * @brief Get the debug hook mask bits
     */
    u8 getDebugHookMask() const noexcept { return hookMask_; }

    /**
     * @brief Get the count-hook interval
     */
    i32 getDebugHookCount() const noexcept { return hookCount_; }

    /**
     * @brief Check whether the debug hook is currently running
     */
    bool isDebugHookActive() const noexcept { return hookActive_; }

    /**
     * @brief Check whether the given hook mask bit is enabled
     */
    bool hasDebugHookMask(u8 mask) const noexcept {
        return hookFunc_ != nullptr && (hookMask_ & mask) != 0;
    }

    /**
     * @brief Consume one instruction for count hooks
     * @return true when a count hook should fire
     */
    bool consumeDebugHookCount();

    /**
     * @brief Call the installed debug hook for a VM event
     * @param event Hook event kind
     * @param line Current line number, or -1 for non-line events
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

    /// 全局状态引用
    GlobalState& globalState_;

    /// 值栈
    Stack stack_;

    /// 栈顶索引（指向下一个可用位置）
    usize top_;

    /// 调用信息栈
    Vec<CallInfo> callStack_;

    /// 当前调用信息索引
    usize currentCI_;

    /// 全局表
    Table* globalTable_;

    /// 线程状态
    ThreadStatus status_;

    /// Open Upvalue链表头（按栈索引降序排列）
    /// 注意：Upvalue由GC管理，这里只持有指针
    Upvalue* openUpvalues_;

    // =====================================================================
    // Coroutine 相关字段
    // =====================================================================

    /// 允许 yield 的嵌套层数（> 0 可 yield，== 0 不可 yield）
    u16 allowYield_ = 0;

    /// yield 返回值数量
    i32 yieldResults_ = 0;

    /// 保存的执行深度（yield 时写入，resume 时恢复）
    i32 savedNexeccalls_ = 1;

    /// 嵌套 C/C++ -> Lua 调用深度，用于在宿主栈耗尽前报 Lua stack overflow。
    i32 hostCallDepth_ = 0;

    /// 所属 Thread 对象（主线程为 nullptr）
    Thread* thread_ = nullptr;

    /// 是否由 newThread 创建（析构时不 removeRoot globalTable_）
    bool isChildThread_ = false;

    /// Installed debug hook function.
    Function* hookFunc_ = nullptr;

    /// Hook mask bits (call/return/line).
    u8 hookMask_ = 0;

    /// Instruction interval for count hooks.
    i32 hookCount_ = 0;

    /// Remaining instructions until the next count hook.
    i32 hookCountdown_ = 0;

    /// Prevent recursive hook invocation while the hook is running.
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

