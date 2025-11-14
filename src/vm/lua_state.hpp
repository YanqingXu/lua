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
 * 
 * 参考实现：
 * - lua_c_analysis/src/lstate.h 中的 lua_State 结构
 * - lua_with_cpp/src/vm/lua_state.hpp 中的实现
 * 
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#pragma once

#include "common/types.hpp"
#include "core/value.hpp"
#include "core/table.hpp"
#include "vm/global_state.hpp"
#include "vm/stack.hpp"
#include "vm/call_info.hpp"

namespace Lua {

// 前向声明
class Upvalue;

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
    // 常量定义
    // =====================================================================
    
    /// 初始调用信息数组大小
    static constexpr usize INITIAL_CI_SIZE = 8;
    
    /// 多返回值标记
    static constexpr i32 MULTRET = -1;
    
    // =====================================================================
    // 构造函数和析构函数
    // =====================================================================
    
    /**
     * @brief 创建新的Lua状态（主线程）
     * @return 新创建的LuaState指针
     */
    static LuaState* newState();
    
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
        stack_.push(Value());
    }
    
    /**
     * @brief 压入布尔值
     */
    void pushBoolean(bool b) {
        stack_.push(Value(b));
    }
    
    /**
     * @brief 压入数值
     */
    void pushNumber(LuaNumber n) {
        stack_.push(Value(n));
    }
    
    /**
     * @brief 压入字符串
     */
    void pushString(GCString* str) {
        stack_.push(Value(str));
    }
    
    /**
     * @brief 压入表
     */
    void pushTable(Table* table) {
        stack_.push(Value(table));
    }
    
    /**
     * @brief 压入函数
     */
    void pushFunction(Function* func) {
        stack_.push(Value(func));
    }
    
    /**
     * @brief 弹出栈顶值
     */
    Value pop() {
        return stack_.pop();
    }

    /**
     * @brief 获取栈顶值
     */
    Value& top() {
        return stack_.top();
    }

    /**
     * @brief 获取栈大小（栈顶索引）
     */
    i32 getTop() const {
        return static_cast<i32>(stack_.size());
    }

    /**
     * @brief 设置栈大小
     * @param idx 新的栈顶索引（从1开始）
     */
    void setTop(i32 idx);

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

private:
    /**
     * @brief 私有构造函数
     */
    LuaState();
    
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
};

} // namespace Lua

