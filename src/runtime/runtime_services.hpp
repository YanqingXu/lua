#pragma once

/**
 * @file runtime_services.hpp
 * @brief 编译器与 VM 入口使用的显式运行时级服务集合
 */

#include "runtime/lua_allocator.hpp"
#include "runtime/runtime_configuration.hpp"
#include "vm/state/global_state.hpp"

#include <exception>

namespace Lua {

namespace VM {
class DispatchStrategy;
}

namespace Debugger {
class DebugController;
}

class EngineContext;

/**
 * @brief 在编译器与 VM 边界之间传递的运行时服务
 *
 * 此类型有意作为当前 GlobalState 支撑运行时之上的轻量兼容层。它在调用处明确服务依赖，
 * 同时保留既有的 GlobalState、StringPool 与垃圾回收器所有权模型。
 */
struct RuntimeServices {
    GlobalState& globalState;
    StringPool& strings;
    GarbageCollector& gc;
    VM::DispatchStrategy* dispatchStrategy;
    Debugger::DebugController* debugger;

    explicit RuntimeServices(GlobalState& global, VM::DispatchStrategy* dispatch = nullptr)
        : globalState(global), strings((global.requireOwnerThread(), global.getStringPool())), gc(global.getGC()),
          dispatchStrategy(dispatch), debugger(global.getDebugController()) {}

    RuntimeServices(GlobalState& global, StringPool& stringPool, GarbageCollector& collector,
                    VM::DispatchStrategy* dispatch = nullptr)
        : globalState(global), strings((global.requireOwnerThread(), stringPool)), gc(collector),
          dispatchStrategy(dispatch), debugger(global.getDebugController()) {}

    static RuntimeServices fromSingletons() {
        return RuntimeServices(GlobalState::getInstance());
    }
};

/**
 * @brief 隔离 Lua 状态的拥有型运行时上下文
 *
 * 此类型进一步扩展了 RuntimeServices 的非拥有型兼容集合：每个 EngineContext 拥有自己的
 * 字符串驻留池和全局状态，后者再拥有垃圾回收器、注册表、基础类型元表、保留字符串与当前
 * 线程记录。
 */
class EngineContext {
public:
    explicit EngineContext(LuaAllocatorFunction allocator = nullptr, void* allocatorUserData = nullptr)
        : allocator_(allocator, allocatorUserData), strings_(&allocator_), globalState_(strings_, &allocator_) {}

    EngineContext(LuaAllocatorFunction allocator, void* allocatorUserData, const RuntimeConfiguration& configuration)
        : allocator_(allocator, allocatorUserData), strings_(&allocator_), globalState_(strings_, &allocator_) {
        globalState_.getSandboxPolicy().configure(configuration.sandbox);
        globalState_.getExecutionPolicy().configure(configuration.execution);
        globalState_.getResourcePolicy() = configuration.resources;
        globalState_.getCompilationPolicy() = configuration.compilation;
    }

    EngineContext(const EngineContext&) = delete;
    EngineContext& operator=(const EngineContext&) = delete;
    EngineContext(EngineContext&&) = delete;
    EngineContext& operator=(EngineContext&&) = delete;

    ~EngineContext() noexcept {
        if (!globalState_.isOwnerThread()) {
            std::terminate();
        }
    }

    [[nodiscard]] RuntimeServices services(VM::DispatchStrategy* dispatch = nullptr) {
        globalState_.requireOwnerThread();
        return RuntimeServices(globalState_, strings_, globalState_.getGC(), dispatch);
    }

    [[nodiscard]] GlobalState& globalState() {
        globalState_.requireOwnerThread();
        return globalState_;
    }

    [[nodiscard]] StringPool& strings() {
        globalState_.requireOwnerThread();
        return strings_;
    }

    [[nodiscard]] GarbageCollector& gc() {
        globalState_.requireOwnerThread();
        return globalState_.getGC();
    }

    [[nodiscard]] NativeModuleRegistry& nativeModules() {
        globalState_.requireOwnerThread();
        return globalState_.getNativeModules();
    }

    [[nodiscard]] const NativeModuleRegistry& nativeModules() const {
        globalState_.requireOwnerThread();
        return globalState_.getNativeModules();
    }

    [[nodiscard]] LuaAllocator& allocator() {
        globalState_.requireOwnerThread();
        return allocator_;
    }

    [[nodiscard]] const LuaAllocator& allocator() const {
        globalState_.requireOwnerThread();
        return allocator_;
    }

    /**
     * @brief 由所有者线程访问运行时级执行限制
     */
    [[nodiscard]] ExecutionPolicy& executionPolicy() {
        globalState_.requireOwnerThread();
        return globalState_.getExecutionPolicy();
    }

    [[nodiscard]] const ExecutionPolicy& executionPolicy() const {
        globalState_.requireOwnerThread();
        return globalState_.getExecutionPolicy();
    }

    /**
     * @brief 由所有者线程配置标准库与宿主资源访问权限
     */
    [[nodiscard]] SandboxPolicy& sandboxPolicy() {
        globalState_.requireOwnerThread();
        return globalState_.getSandboxPolicy();
    }

    [[nodiscard]] const SandboxPolicy& sandboxPolicy() const {
        globalState_.requireOwnerThread();
        return globalState_.getSandboxPolicy();
    }

    [[nodiscard]] RuntimeRandom& random() {
        globalState_.requireOwnerThread();
        return globalState_.getRandom();
    }

    [[nodiscard]] const RuntimeRandom& random() const {
        globalState_.requireOwnerThread();
        return globalState_.getRandom();
    }

    [[nodiscard]] ResourcePolicy& resourcePolicy() {
        globalState_.requireOwnerThread();
        return globalState_.getResourcePolicy();
    }

    [[nodiscard]] const ResourcePolicy& resourcePolicy() const {
        globalState_.requireOwnerThread();
        return globalState_.getResourcePolicy();
    }

    [[nodiscard]] CompilationPolicy& compilationPolicy() {
        globalState_.requireOwnerThread();
        return globalState_.getCompilationPolicy();
    }

    [[nodiscard]] const CompilationPolicy& compilationPolicy() const {
        globalState_.requireOwnerThread();
        return globalState_.getCompilationPolicy();
    }

    [[nodiscard]] TraceRuntime& trace() {
        globalState_.requireOwnerThread();
        return globalState_.getTraceRuntime();
    }

    [[nodiscard]] const TraceRuntime& trace() const {
        globalState_.requireOwnerThread();
        return globalState_.getTraceRuntime();
    }

    /**
     * @brief 创建析构安全且仅允许跨线程取消的句柄
     */
    [[nodiscard]] ExecutionCancellationHandle cancellationHandle() {
        return executionPolicy().cancellationHandle();
    }

private:
    LuaAllocator allocator_;
    StringPool strings_;
    GlobalState globalState_;
};

} // namespace Lua
