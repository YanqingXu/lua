/**
 * @file global_state.hpp
 * @brief Lua全局状态管理：所有线程共享的系统级资源
 * 
 * 详细说明：
 * GlobalState类管理Lua虚拟机中所有线程共享的全局资源，包括：
 * - 字符串池（StringPool）
 * - 垃圾回收器（GarbageCollector，由GlobalState拥有）
 * - 元表（metatable）
 * - 注册表（registry）
 * - 主线程引用
 * 
 * 设计特点：
 * - 单例模式：全局唯一的GlobalState实例
 * - 资源共享：所有LuaState共享同一个GlobalState
 * - 现代C++：使用智能指针和RAII管理资源
 * - 线程安全：为后续多线程支持预留接口
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#pragma once

#include "common/types.hpp"
#include "core/value.hpp"
#include "core/table.hpp"
#include "core/string_pool.hpp"
#include "core/metatable.hpp"
#include "gc/garbage_collector.hpp"

namespace Lua {

// 前向声明
class LuaState;
class Thread;

/**
 * @brief 全局状态类
 * 
 * 管理所有线程共享的全局资源和配置信息。
 * 
 * 核心职责：
 * 1. 字符串管理：通过StringPool实现字符串驻留
 * 2. 垃圾回收：管理GC状态和配置
 * 3. 元表管理：为基础类型提供元表支持
 * 4. 注册表：提供C代码专用的全局存储
 * 5. 主线程：维护主线程的引用
 * 
 * 使用示例：
 * @code
 * GlobalState& gs = GlobalState::getInstance();
 * 
 * // 访问字符串池
 * StringPool& pool = gs.getStringPool();
 * 
 * // 访问垃圾回收器
 * GarbageCollector& gc = gs.getGC();
 * 
 * // 访问注册表
 * Table* registry = gs.getRegistry();
 * @endcode
 */
class GlobalState {
public:
    // =====================================================================
    // 单例模式
    // =====================================================================
    
    /**
     * @brief 获取全局状态单例实例
     * @return GlobalState实例的引用
     */
    static GlobalState& getInstance();
    
    // 禁止拷贝和赋值
    GlobalState(const GlobalState&) = delete;
    GlobalState& operator=(const GlobalState&) = delete;

    /**
     * @brief Create a runtime global state backed by the supplied string pool.
     *
     * The default argument preserves the historical singleton-backed
     * construction path. EngineContext passes its owned StringPool here to
     * create an isolated runtime context.
     */
    explicit GlobalState(StringPool& stringPool = StringPool::getInstance());
    
    /**
     * @brief 析构函数
     */
    ~GlobalState();
    
    // =====================================================================
    // 字符串管理
    // =====================================================================
    
    /**
     * @brief 获取字符串池
     * @return 字符串池的引用
     */
    StringPool& getStringPool() noexcept {
        return stringPool_;
    }
    
    // =====================================================================
    // 垃圾回收管理
    // =====================================================================
    
    /**
     * @brief 获取垃圾回收器
     * @return 垃圾回收器的引用
     */
    GarbageCollector& getGC() noexcept {
        return gc_;
    }

    /**
     * @brief 标记全局状态持有的 GC 根
     *
     * 由完整 GC 在 collectgarbage("collect") 路径调用。
     */
    void markRoots(GarbageCollector& gc, LuaState* currentState) const;

    /**
     * @brief Reset raw runtime references before GarbageCollector::clearAll().
     *
     * clearAll() is used by tests and shutdown paths to delete non-fixed GC
     * objects without a mark phase. GlobalState stores a few raw pointers to
     * those objects, so they must be cleared before the objects are freed.
     */
    void resetRuntimeReferencesForClearAll() noexcept;
    
    // =====================================================================
    // 注册表管理
    // =====================================================================
    
    /**
     * @brief 获取注册表
     * 
     * 注册表是一个全局表，只能从C代码访问，用于存储C扩展的私有数据。
     * 
     * @return 注册表指针
     */
    Table* getRegistry() noexcept {
        return registry_;
    }
    
    // =====================================================================
    // 主线程管理
    // =====================================================================
    
    /**
     * @brief 设置主线程
     * @param mainThread 主线程指针
     */
    void setMainThread(LuaState* mainThread) noexcept;
    
    /**
     * @brief 获取主线程
     * @return 主线程指针
     */
    LuaState* getMainThread() const noexcept {
        return mainThread_;
    }
    
    // =====================================================================
    // 元表管理
    // =====================================================================
    
    /**
     * @brief 获取基础类型的元表
     * @param type 值类型
     * @return 元表指针（如果没有设置则返回nullptr）
     */
    Table* getMetatable(ValueType type) const noexcept;
    
    /**
     * @brief 设置基础类型的元表
     * @param type 值类型
     * @param metatable 元表指针
     */
    void setMetatable(ValueType type, Table* metatable) noexcept;

    // =====================================================================
    // 元方法名称管理
    // =====================================================================

    /**
     * @brief 获取元方法名称字符串
     * @param event 元方法类型
     * @return 元方法名称的GCString指针
     */
    GCString* getMetamethodName(TMS event) const noexcept;

    // =====================================================================
    // 当前运行协程追踪
    // =====================================================================

    Thread* getRunningThread() const noexcept { return runningThread_; }
    void setRunningThread(Thread* t) noexcept;

private:
    /**
     * @brief 初始化元方法名称
     *
     * 创建并固定所有元方法名称字符串，防止GC回收。
     */
    void initMetamethodNames();

    /**
     * @brief 初始化保留字（关键字）
     *
     * 创建并固定所有Lua关键字字符串，防止GC回收。
     */
    void initReservedWords();

    // =====================================================================
    // 成员变量
    // =====================================================================

    /// 垃圾回收器（由GlobalState拥有）
    GarbageCollector gc_;

    /// 字符串池（单例引用）
    StringPool& stringPool_;

    /// 注册表（C代码专用的全局表）
    Table* registry_;

    /// 主线程指针
    LuaState* mainThread_;

    /// 当前正在执行的协程（主线程时为 nullptr）
    Thread* runningThread_ = nullptr;

    /// 基础类型的元表数组（索引对应ValueType枚举值）
    Table* metatables_[9];  // 9种基础类型

    /// 元方法名称数组（17个元方法）
    GCString* tmname_[static_cast<usize>(TMS::TM_N)];

    /// 内存错误消息（固定字符串，防止在内存不足时被GC回收）
    GCString* memerrmsg_;
};

} // namespace Lua

