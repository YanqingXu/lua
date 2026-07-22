/**
 * @file global_state.hpp
 * @brief Lua全局状态管理：所有线程共享的系统级资源
 *
 * 详细说明：
 * GlobalState类管理Lua虚拟机中所有线程共享的全局资源，包括：
 * - 字符串驻留池
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
 * @author Lua C++ 项目
 * @date 2025-11-12
 */

#pragma once

#include "common/types.hpp"
#include "core/value.hpp"
#include "core/table.hpp"
#include "core/string_pool.hpp"
#include "core/metatable.hpp"
#include "gc/garbage_collector.hpp"
#include "runtime/native_module_registry.hpp"
#include "runtime/compilation_policy.hpp"
#include "runtime/execution_policy.hpp"
#include "runtime/runtime_random.hpp"
#include "runtime/resource_policy.hpp"
#include "runtime/sandbox_policy.hpp"
#include "runtime/trace_runtime.hpp"

#include <array>
#include <stdexcept>
#include <thread>

struct lua_State;

namespace Lua {

// 前向声明
class LuaState;
class Thread;
class LuaAllocator;

/**
 * @brief 外部线程访问运行时状态前抛出的宿主逻辑错误
 */
class RuntimeOwnerThreadError final : public std::logic_error {
public:
    RuntimeOwnerThreadError() : std::logic_error("Lua runtime accessed from non-owner thread") {}
};

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
 * // 访问字符串驻留池
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
    using PanicFunction = int (*)(::lua_State*);

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
     * @brief 创建由指定字符串驻留池支撑的运行时全局状态
     *
     * 默认参数保留原有的单例支撑构造路径。EngineContext 在此传入其拥有的 StringPool，
     * 以创建相互隔离的运行时上下文。
     */
    explicit GlobalState(StringPool& stringPool = StringPool::getInstance(), LuaAllocator* allocator = nullptr);

    /**
     * @brief 析构函数
     */
    ~GlobalState();

    /**
     * @brief 判断调用者是否为固定不变的运行时所有者线程
     * @return 调用者是所有者线程时返回 true，否则返回 false
     * @note 此身份查询可在访问任何可变运行时状态之前安全调用。
     */
    [[nodiscard]] bool isOwnerThread() const noexcept {
        return ownerThread_ == std::this_thread::get_id();
    }

    /**
     * @brief 在读取可变运行时状态前拒绝外部线程访问
     */
    void requireOwnerThread() const {
        if (!isOwnerThread()) {
            throw RuntimeOwnerThreadError();
        }
    }

    // =====================================================================
    // 字符串管理
    // =====================================================================

    /**
     * @brief 获取字符串驻留池
     * @return 字符串驻留池的引用
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

    LuaAllocator* getAllocator() noexcept {
        return gc_.getAllocator();
    }

    const LuaAllocator* getAllocator() const noexcept {
        return gc_.getAllocator();
    }

    ExecutionPolicy& getExecutionPolicy() noexcept {
        return executionPolicy_;
    }

    const ExecutionPolicy& getExecutionPolicy() const noexcept {
        return executionPolicy_;
    }

    SandboxPolicy& getSandboxPolicy() noexcept {
        return sandboxPolicy_;
    }

    const SandboxPolicy& getSandboxPolicy() const noexcept {
        return sandboxPolicy_;
    }

    RuntimeRandom& getRandom() noexcept {
        return random_;
    }

    const RuntimeRandom& getRandom() const noexcept {
        return random_;
    }

    ResourcePolicy& getResourcePolicy() noexcept {
        return resourcePolicy_;
    }

    const ResourcePolicy& getResourcePolicy() const noexcept {
        return resourcePolicy_;
    }

    CompilationPolicy& getCompilationPolicy() noexcept {
        return compilationPolicy_;
    }

    const CompilationPolicy& getCompilationPolicy() const noexcept {
        return compilationPolicy_;
    }

    TraceRuntime& getTraceRuntime() noexcept {
        return traceRuntime_;
    }

    const TraceRuntime& getTraceRuntime() const noexcept {
        return traceRuntime_;
    }

    NativeModuleRegistry& getNativeModules() noexcept {
        return nativeModules_;
    }

    const NativeModuleRegistry& getNativeModules() const noexcept {
        return nativeModules_;
    }

    /**
     * @brief 标记全局状态持有的 GC 根
     *
     * 由完整 GC 在 collectgarbage("collect") 路径调用。
     */
    void markRoots(GarbageCollector& gc, LuaState* currentState) const;

    /**
     * @brief 在 GarbageCollector::clearAll() 前重置运行时原始引用
     *
     * 测试与关闭路径使用 clearAll() 跳过标记阶段并删除非固定垃圾回收对象。
     * GlobalState 保存了部分指向这些对象的原始指针，因此必须在释放对象前清空它们。
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

    /**
     * @brief 返回分配失败时使用的固定错误字符串
     */
    GCString* getMemoryErrorMessage() const noexcept {
        return memerrmsg_;
    }

    /**
     * @brief 返回保护 API 无法为 C++ 异常分配文本时使用的固定后备值
     */
    GCString* getApiExceptionMessage() const noexcept {
        return apiExceptionMessage_;
    }

    /**
     * @brief 返回与执行策略停止原因对应的固定错误对象
     */
    GCString* getExecutionPolicyErrorMessage(ExecutionStopReason reason) const noexcept;

    /**
     * @brief 返回沙箱能力被拒绝时使用的固定 Lua 错误对象
     */
    GCString* getSandboxCapabilityErrorMessage(SandboxCapability capability) const noexcept;

    /**
     * @brief 返回标准库暴露被禁用时使用的固定 Lua 错误对象
     */
    GCString* getSandboxLibraryErrorMessage() const noexcept {
        return sandboxLibraryErrorMessage_;
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

    PanicFunction setPanicFunction(PanicFunction function) noexcept {
        PanicFunction previous = panicFunction_;
        panicFunction_ = function;
        return previous;
    }

    PanicFunction getPanicFunction() const noexcept {
        return panicFunction_;
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

    Thread* getRunningThread() const noexcept {
        return runningThread_;
    }
    void setRunningThread(Thread* t) noexcept;

private:
    static constexpr usize kMetatableCount = static_cast<usize>(ValueType::Thread) + 1;

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

    /** @brief 除取消操作外，所有操作固定使用的构造线程。 */
    const std::thread::id ownerThread_;

    /** @brief 运行时级标准库与特权操作策略。 */
    SandboxPolicy sandboxPolicy_;

    /** @brief 原生模块晚于垃圾回收器析构，以确保所有 C 函数先行销毁。 */
    NativeModuleRegistry nativeModules_;

    /** @brief 主状态与所有协程共享的运行时级限制。 */
    ExecutionPolicy executionPolicy_;

    /** @brief 隔离在当前运行时上下文内的确定性随机流。 */
    RuntimeRandom random_;

    /** @brief 应用于脚本控制资源增长的统一限制。 */
    ResourcePolicy resourcePolicy_;

    /** @brief 词法分析、语法分析与代码生成期间使用的统一限制。 */
    CompilationPolicy compilationPolicy_;

    /** @brief 当前上下文拥有的跟踪输出端、序列号与调试开关。 */
    TraceRuntime traceRuntime_;

    /**
     * @brief 垃圾回收器（由GlobalState拥有）
     */
    GarbageCollector gc_;

    /**
     * @brief 字符串驻留池（单例引用）
     */
    StringPool& stringPool_;

    /**
     * @brief 注册表（C代码专用的全局表）
     */
    Table* registry_;

    /**
     * @brief 主线程指针
     */
    LuaState* mainThread_;

    /** @brief 由 lua_atpanic 安装的未保护错误回调。 */
    PanicFunction panicFunction_ = nullptr;

    /**
     * @brief 当前正在执行的协程（主线程时为 nullptr）
     */
    Thread* runningThread_ = nullptr;

    /**
     * @brief 基础类型的元表数组（索引对应ValueType枚举值）
     */
    std::array<Table*, kMetatableCount> metatables_{}; // 9种基础类型

    /**
     * @brief 元方法名称数组（17个元方法）
     */
    std::array<GCString*, static_cast<usize>(TMS::TM_N)> tmname_{};

    /**
     * @brief 内存错误消息（固定字符串，防止在内存不足时被GC回收）
     */
    GCString* memerrmsg_;

    /** @brief 保护 API 的应急错误文本；在状态创建时分配并固定。 */
    GCString* apiExceptionMessage_;

    /**
     * @brief 固定的执行策略错误对象，在分配器失败期间仍保持可用。
     */
    GCString* instructionBudgetErrorMessage_;
    GCString* nativeWorkBudgetErrorMessage_;
    GCString* deadlineErrorMessage_;
    GCString* cancellationErrorMessage_;

    /** @brief 固定沙箱错误可在拒绝操作时保持稳定且无需分配。 */
    GCString* sandboxLibraryErrorMessage_;
    GCString* sandboxFilesystemErrorMessage_;
    GCString* sandboxProcessErrorMessage_;
    GCString* sandboxNativeModuleErrorMessage_;
    GCString* sandboxRuntimeCompilationErrorMessage_;
    GCString* sandboxBinaryChunksErrorMessage_;
    GCString* sandboxGCControlErrorMessage_;
};

} // namespace Lua
