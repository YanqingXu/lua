/**
 * @file garbage_collector.hpp
 * @brief Lua垃圾回收器：三色标记-清除算法实现
 * 
 * 本文件实现了Lua的垃圾回收系统，采用三色标记-清除算法管理所有GC对象的生命周期。
 * 
 * 核心功能：
 * - 管理所有GC对象（GCString、Table等）
 * - 三色标记算法（白色、灰色、黑色）
 * - 标记-清除垃圾回收
 * - 根对象保护
 * - 内存统计
 * 
 * 设计特点：
 * - 单例模式：全局唯一的GC实例
 * - 链表管理：使用侵入式链表管理所有GC对象
 * - 增量准备：为后续增量GC预留接口
 * - 现代C++：使用RAII和智能指针辅助管理
 * 
 * 参考实现：
 * - lua_c_analysis/src/lgc.h/c - Lua 5.1.5 GC实现
 * 
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#pragma once

#include "common/types.hpp"
#include "core/gc_object.hpp"
#include <vector>

namespace Lua {

// 前向声明
class GCString;
class Table;
class LuaState;
class Userdata;
class Value;

/**
 * @brief 垃圾回收器类
 * 
 * 管理所有GC对象的生命周期，实现三色标记-清除算法。
 * 
 * 三色标记算法：
 * - 白色（White）：未访问的对象，可能是垃圾
 * - 灰色（Gray）：已访问但未扫描的对象，待处理
 * - 黑色（Black）：已访问且已扫描的对象，确定存活
 * 
 * GC流程：
 * 1. 标记阶段：从根对象开始，标记所有可达对象
 * 2. 清除阶段：回收所有未标记（白色）的对象
 * 
 * 使用示例：
 * @code
 * GarbageCollector& gc = GarbageCollector::getInstance();
 * 
 * // 创建对象时注册到GC
 * GCString* str = new GCString("hello");
 * gc.registerObject(str);
 * 
 * // 添加根对象（保护不被回收）
 * gc.addRoot(str);
 * 
 * // 执行垃圾回收
 * gc.collect();
 * 
 * // 移除根对象
 * gc.removeRoot(str);
 * @endcode
 */
class GarbageCollector {
public:
    // =====================================================================
    // 单例模式
    // =====================================================================
    
    /**
     * @brief 获取GC单例实例
     * @return GC实例的引用
     */
    static GarbageCollector& getInstance();
    
    // 禁止拷贝和赋值
    GarbageCollector(const GarbageCollector&) = delete;
    GarbageCollector& operator=(const GarbageCollector&) = delete;
    
    /**
     * @brief 析构函数
     * 
     * 清理所有GC对象，释放内存。
     */
    ~GarbageCollector();
    
    // =====================================================================
    // 对象管理
    // =====================================================================
    
    /**
     * @brief 注册GC对象
     * 
     * 将新创建的GC对象添加到GC管理链表中。
     * 
     * @param obj GC对象指针
     */
    void registerObject(GCObject* obj);

    /**
     * @brief 从GC管理链表中摘除对象
     *
     * 主要用于兼容仍然手动 delete 的旧代码路径。该函数不释放对象本身。
     */
    void unregisterObject(GCObject* obj) noexcept;
    
    /**
     * @brief 添加根对象
     * 
     * 根对象不会被GC回收，通常是全局变量、栈上的对象等。
     * 
     * @param obj 根对象指针
     */
    void addRoot(GCObject* obj);
    
    /**
     * @brief 移除根对象
     * 
     * 从根对象集合中移除对象，使其可以被GC回收。
     * 
     * @param obj 根对象指针
     */
    void removeRoot(GCObject* obj);
    
    /**
     * @brief 检查对象是否为根对象
     * 
     * @param obj 对象指针
     * @return 如果是根对象返回true，否则返回false
     */
    bool isRoot(GCObject* obj) const;
    
    // =====================================================================
    // 垃圾回收
    // =====================================================================
    
    /**
     * @brief 执行完整的垃圾回收
     * 
     * 执行标记-清除算法，回收所有不可达对象。
     * 
     * 流程：
     * 1. 重置所有对象为白色
     * 2. 标记所有根对象为灰色
     * 3. 传播标记：处理所有灰色对象
     * 4. 清除所有白色对象
     * 
     * @return 回收的对象数量
     */
    usize collect();

    /**
     * @brief 执行完整的垃圾回收，并将当前LuaState作为执行根
     *
     * collectgarbage("collect") 应使用此入口，这样当前线程栈、主线程栈
     * 和全局状态中的共享根都会参与标记。
     *
     * @param currentState 当前执行 collectgarbage 的 LuaState
     * @return 回收的对象数量
     */
    usize collect(LuaState* currentState);
    
    /**
     * @brief 标记阶段
     * 
     * 从根对象开始，标记所有可达对象。
     */
    void mark();

    /**
     * @brief 标记阶段，并额外扫描当前执行状态根集
     * @param currentState 当前执行状态；nullptr 时只扫描显式 roots_
     */
    void mark(LuaState* currentState);
    
    /**
     * @brief 清除阶段
     * 
     * 回收所有未标记（白色）的对象。
     * 
     * @return 回收的对象数量
     */
    usize sweep();

    /**
     * @brief 标记单个对象
     *
     * 该方法是所有子对象报告引用关系的统一入口。只有白色对象会进入灰色队列。
     */
    void markObject(GCObject* obj);

    /**
     * @brief 标记一个 Lua Value 中包含的 GCObject
     */
    void markValue(const Value& value);

    /**
     * @brief 标记 LuaState 中的活动栈、调用帧窗口和 open upvalue
     */
    void markState(LuaState* state);

    /**
     * @brief 标记表对象，并按 __mode 应用弱键/弱值语义
     */
    void markTable(Table* table);

    /**
     * @brief 检查对象是否会在当前 sweep 中被回收
     */
    bool isObjectDead(GCObject* obj) const;

    /**
     * @brief 检查 Value 中的可回收对象是否会在当前 sweep 中被回收
     */
    bool isValueDead(const Value& value) const;
    
    // =====================================================================
    // 统计信息
    // =====================================================================
    
    /**
     * @brief 获取当前管理的对象总数
     * @return 对象数量
     */
    usize getObjectCount() const noexcept;
    
    /**
     * @brief 获取根对象数量
     * @return 根对象数量
     */
    usize getRootCount() const noexcept;
    
    /**
     * @brief 获取总内存使用量（估算）
     * @return 内存字节数
     */
    usize getTotalMemory() const noexcept;
    
    /**
     * @brief 获取GC统计信息
     * 
     * @param outObjectCount 输出：对象总数
     * @param outRootCount 输出：根对象数量
     * @param outTotalMemory 输出：总内存使用量
     */
    void getStatistics(usize& outObjectCount, usize& outRootCount, usize& outTotalMemory) const noexcept;
    
    // =====================================================================
    // 调试和测试
    // =====================================================================
    
    /**
     * @brief 清理所有对象（用于测试）
     * 
     * 强制删除所有GC对象，不管是否为根对象。
     * 仅用于测试和程序退出时的清理。
     */
    void clearAll();
    
    /**
     * @brief 打印GC统计信息（调试用）
     */
    void printStatistics() const;

private:
    // =====================================================================
    // 私有构造函数（单例模式）
    // =====================================================================
    
    GarbageCollector();
    
    // =====================================================================
    // 内部辅助方法
    // =====================================================================

    static bool valueContainsObject(const Value& value);
    static GCObject* objectFromValue(const Value& value);
    
    /**
     * @brief 传播标记
     * 
     * 处理所有灰色对象，将其引用的白色对象标记为灰色，
     * 并将自己标记为黑色。
     */
    void propagateMarks();

    /**
     * @brief 清理所有已标记弱表中的死亡键/值
     */
    void clearWeakTableEntries();

    /**
     * @brief 将带 __gc 的不可达 userdata 复活并加入待终结队列
     */
    void prepareFinalizers();

    /**
     * @brief 查询 userdata 的 __gc 元方法
     */
    Value getFinalizer(Userdata* userdata) const;

    /**
     * @brief 运行本轮收集期间排队的终结器
     */
    void runFinalizers(LuaState* state);
    
    // =====================================================================
    // 数据成员
    // =====================================================================
    
    /// 所有GC对象的链表头
    GCObject* allObjects_;
    
    /// 根对象集合（使用vector存储，简单实现）
    Vec<GCObject*> roots_;
    
    /// 灰色对象列表（待处理）
    Vec<GCObject*> grayList_;

    /// 本轮标记中发现的弱表
    Vec<Table*> weakTables_;

    /// 等待执行 __gc 的 userdata
    Vec<Userdata*> pendingFinalizers_;

    /// 防止终结器递归执行
    bool finalizersRunning_;
    
    /// 统计信息：对象总数
    usize objectCount_;
    
    /// 统计信息：总内存使用量
    usize totalMemory_;
};

} // namespace Lua

