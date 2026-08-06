/**
 * @file global_state.cpp
 * @brief Lua全局状态管理实现
 *
 * @author Lua C++ 项目
 * @date 2025-11-12
 */

#include "vm/state/global_state.hpp"
#include "debugger/debug_runtime.hpp"
#include "core/thread.hpp"
#include "vm/state/lua_state.hpp"
#include <array>
#include <exception>
#include <iostream> // 调试输出

namespace Lua {

// =====================================================================
// 单例实现
// =====================================================================

GlobalState& GlobalState::getInstance() {
    static GlobalState instance;
    return instance;
}

// =====================================================================
// 构造函数和析构函数
// =====================================================================

GlobalState::GlobalState(StringPool& stringPool, LuaAllocator* allocator)
    : ownerThread_(std::this_thread::get_id()), sandboxPolicy_(), nativeModules_(&sandboxPolicy_), gc_(allocator),
      stringPool_(stringPool), registry_(nullptr), mainThread_(nullptr), memerrmsg_(nullptr),
      apiExceptionMessage_(nullptr), instructionBudgetErrorMessage_(nullptr), nativeWorkBudgetErrorMessage_(nullptr),
      deadlineErrorMessage_(nullptr), cancellationErrorMessage_(nullptr), sandboxLibraryErrorMessage_(nullptr),
      sandboxFilesystemErrorMessage_(nullptr), sandboxProcessErrorMessage_(nullptr),
      sandboxNativeModuleErrorMessage_(nullptr), sandboxRuntimeCompilationErrorMessage_(nullptr),
      sandboxBinaryChunksErrorMessage_(nullptr), sandboxGCControlErrorMessage_(nullptr) {
    stringPool_.setResourcePolicy(&resourcePolicy_);
    stringPool_.setGarbageCollector(&gc_);

    /** @brief 子任务 1.1：将字符串驻留池调整到初始大小。 */
    stringPool_.resize(32);

    // 子任务1.2：初始化元方法名称
    initMetamethodNames();

    // 子任务1.3：初始化保留字
    initReservedWords();

    // 子任务1.4：固定内存错误消息
    memerrmsg_ = stringPool_.intern("not enough memory");
    gc_.registerObject(memerrmsg_);
    memerrmsg_->markFixed(); // 标记为固定，防止在内存不足时被GC回收

    apiExceptionMessage_ = stringPool_.intern("unhandled C++ exception in protected Lua API");
    gc_.registerObject(apiExceptionMessage_);
    apiExceptionMessage_->markFixed();

    instructionBudgetErrorMessage_ = stringPool_.intern("execution instruction budget exceeded");
    gc_.registerObject(instructionBudgetErrorMessage_);
    instructionBudgetErrorMessage_->markFixed();

    nativeWorkBudgetErrorMessage_ = stringPool_.intern("execution native work budget exceeded");
    gc_.registerObject(nativeWorkBudgetErrorMessage_);
    nativeWorkBudgetErrorMessage_->markFixed();

    deadlineErrorMessage_ = stringPool_.intern("execution deadline exceeded");
    gc_.registerObject(deadlineErrorMessage_);
    deadlineErrorMessage_->markFixed();

    cancellationErrorMessage_ = stringPool_.intern("execution cancelled");
    gc_.registerObject(cancellationErrorMessage_);
    cancellationErrorMessage_->markFixed();

    sandboxLibraryErrorMessage_ = stringPool_.intern(SandboxPolicy::libraryDeniedMessage());
    gc_.registerObject(sandboxLibraryErrorMessage_);
    sandboxLibraryErrorMessage_->markFixed();

    sandboxFilesystemErrorMessage_ = stringPool_.intern(SandboxPolicy::deniedMessage(SandboxCapability::Filesystem));
    gc_.registerObject(sandboxFilesystemErrorMessage_);
    sandboxFilesystemErrorMessage_->markFixed();

    sandboxProcessErrorMessage_ = stringPool_.intern(SandboxPolicy::deniedMessage(SandboxCapability::Process));
    gc_.registerObject(sandboxProcessErrorMessage_);
    sandboxProcessErrorMessage_->markFixed();

    sandboxNativeModuleErrorMessage_ =
        stringPool_.intern(SandboxPolicy::deniedMessage(SandboxCapability::NativeModules));
    gc_.registerObject(sandboxNativeModuleErrorMessage_);
    sandboxNativeModuleErrorMessage_->markFixed();

    sandboxRuntimeCompilationErrorMessage_ =
        stringPool_.intern(SandboxPolicy::deniedMessage(SandboxCapability::RuntimeCompilation));
    gc_.registerObject(sandboxRuntimeCompilationErrorMessage_);
    sandboxRuntimeCompilationErrorMessage_->markFixed();

    sandboxBinaryChunksErrorMessage_ =
        stringPool_.intern(SandboxPolicy::deniedMessage(SandboxCapability::BinaryChunks));
    gc_.registerObject(sandboxBinaryChunksErrorMessage_);
    sandboxBinaryChunksErrorMessage_->markFixed();

    sandboxGCControlErrorMessage_ = stringPool_.intern(SandboxPolicy::deniedMessage(SandboxCapability::GCControl));
    gc_.registerObject(sandboxGCControlErrorMessage_);
    sandboxGCControlErrorMessage_->markFixed();

    // 创建注册表
    registry_ = gc_.createFixedRoot<Table>(); // 注册表永远不被回收

    /**
     * @brief 仅在构造成功后发布反向引用。
     *
     * 这样可以避免分配器失败引发栈展开时，在垃圾回收器成员析构期间调用尚未构造完整的
     * GlobalState。
     */
    gc_.setGlobalState(this);
}

GlobalState::~GlobalState() {
    if (!isOwnerThread()) {
        std::terminate();
    }

    if (debugger_ != nullptr) {
        debugger_->shutdown(Debugger::DisconnectAction::ContinueExecution);
        debugger_.reset();
    }

    // 注意：不需要手动删除registry_，因为GC会处理
    // 但需要从根对象中移除
    if (registry_) {
        gc_.removeRoot(registry_);
    }
    stringPool_.setGarbageCollector(nullptr);
    stringPool_.setResourcePolicy(nullptr);
}

Debugger::DebugController& GlobalState::enableDebugger() {
    return enableDebugger(Debugger::DebugResourceLimits{});
}

Debugger::DebugController& GlobalState::enableDebugger(const Debugger::DebugResourceLimits& limits) {
    requireOwnerThread();
    if (debugger_ == nullptr) {
        debugger_ = makeUnique<Debugger::DebugController>(limits);
        if (mainThread_ != nullptr) {
            (void)debugger_->registerState(*mainThread_, "main", "root Lua state");
        }
    }
    return *debugger_;
}

void GlobalState::disableDebugger(Debugger::DisconnectAction action) {
    requireOwnerThread();
    if (debugger_ != nullptr) {
        debugger_->shutdown(action);
        debugger_.reset();
    }
}

// =====================================================================
// 元表管理
// =====================================================================

Table* GlobalState::getMetatable(ValueType type) const noexcept {
    usize index = static_cast<usize>(type);
    if (index < metatables_.size()) {
        return metatables_[index];
    }
    return nullptr;
}

void GlobalState::setMetatable(ValueType type, Table* metatable) noexcept {
    usize index = static_cast<usize>(type);
    if (index < metatables_.size()) {
        gc_.writeRootBarrier(metatable);
        metatables_[index] = metatable;
    }
}

void GlobalState::setMainThread(LuaState* mainThread) noexcept {
    mainThread_ = mainThread;
}

void GlobalState::setRunningThread(Thread* t) noexcept {
    gc_.writeRootBarrier(t);
    runningThread_ = t;
}

void GlobalState::markRoots(GarbageCollector& gc, LuaState* currentState) const {
    gc.markObject(registry_);
    gc.markObject(memerrmsg_);
    gc.markObject(apiExceptionMessage_);
    gc.markObject(instructionBudgetErrorMessage_);
    gc.markObject(nativeWorkBudgetErrorMessage_);
    gc.markObject(deadlineErrorMessage_);
    gc.markObject(cancellationErrorMessage_);
    gc.markObject(sandboxLibraryErrorMessage_);
    gc.markObject(sandboxFilesystemErrorMessage_);
    gc.markObject(sandboxProcessErrorMessage_);
    gc.markObject(sandboxNativeModuleErrorMessage_);
    gc.markObject(sandboxRuntimeCompilationErrorMessage_);
    gc.markObject(sandboxBinaryChunksErrorMessage_);
    gc.markObject(sandboxGCControlErrorMessage_);

    for (GCString* name : tmname_) {
        gc.markObject(name);
    }

    for (Table* metatable : metatables_) {
        gc.markObject(metatable);
    }

    if (currentState != nullptr) {
        gc.markState(currentState);
    }

    if (mainThread_ != nullptr && mainThread_ != currentState) {
        gc.markState(mainThread_);
    }

    gc.markObject(runningThread_);
}

GCString* GlobalState::getExecutionPolicyErrorMessage(ExecutionStopReason reason) const noexcept {
    switch (reason) {
    case ExecutionStopReason::InstructionBudgetExceeded:
        return instructionBudgetErrorMessage_;
    case ExecutionStopReason::NativeWorkBudgetExceeded:
        return nativeWorkBudgetErrorMessage_;
    case ExecutionStopReason::DeadlineExceeded:
        return deadlineErrorMessage_;
    case ExecutionStopReason::Cancelled:
        return cancellationErrorMessage_;
    case ExecutionStopReason::None:
        return apiExceptionMessage_;
    }

    return apiExceptionMessage_;
}

GCString* GlobalState::getSandboxCapabilityErrorMessage(SandboxCapability capability) const noexcept {
    switch (capability) {
    case SandboxCapability::Filesystem:
        return sandboxFilesystemErrorMessage_;
    case SandboxCapability::Process:
        return sandboxProcessErrorMessage_;
    case SandboxCapability::NativeModules:
        return sandboxNativeModuleErrorMessage_;
    case SandboxCapability::RuntimeCompilation:
        return sandboxRuntimeCompilationErrorMessage_;
    case SandboxCapability::BinaryChunks:
        return sandboxBinaryChunksErrorMessage_;
    case SandboxCapability::GCControl:
        return sandboxGCControlErrorMessage_;
    }
    return sandboxLibraryErrorMessage_;
}

void GlobalState::resetRuntimeReferencesForClearAll() noexcept {
    mainThread_ = nullptr;
    runningThread_ = nullptr;
    metatables_.fill(nullptr);
    if (registry_ != nullptr) {
        registry_->clear();
    }
}

// =====================================================================
// 元方法名称管理
// =====================================================================

/**
 * @brief 初始化元方法名称
 *
 * 创建并固定所有17个元方法名称字符串，防止GC回收。
 */
void GlobalState::initMetamethodNames() {
    // 元方法名称数组（与TMS枚举顺序一致）
    static constexpr std::array<StrView, static_cast<usize>(TMS::TM_N)> metamethodNames{
        {"__index", "__newindex", "__gc", "__mode", "__eq", "__add", "__sub", "__mul", "__div", "__mod", "__pow",
         "__unm", "__len", "__lt", "__le", "__concat", "__call"}};

    // 创建并固定所有元方法名称字符串
    for (usize i = 0; i < metamethodNames.size(); i++) {
        tmname_[i] = stringPool_.intern(metamethodNames[i]);
        gc_.registerObject(tmname_[i]);
        tmname_[i]->markFixed(); // 标记为固定，防止GC回收
    }
}

/**
 * @brief 获取元方法名称字符串
 */
GCString* GlobalState::getMetamethodName(TMS event) const noexcept {
    usize index = static_cast<usize>(event);
    if (index < tmname_.size()) {
        return tmname_[index];
    }
    return nullptr;
}

/**
 * @brief 初始化保留字（关键字）
 *
 * 创建并固定所有21个Lua关键字字符串，防止GC回收。
 */
void GlobalState::initReservedWords() {
    // Lua 5.1的21个保留字（按字母顺序）
    static constexpr std::array<StrView, 21> reservedWords{
        {"and",   "break", "do",  "else", "elseif", "end",    "false", "for",  "function", "if",   "in",
         "local", "nil",   "not", "or",   "repeat", "return", "then",  "true", "until",    "while"}};

    // 创建并固定所有保留字字符串
    for (StrView word : reservedWords) {
        GCString* str = stringPool_.intern(word);
        gc_.registerObject(str);
        str->markFixed(); // 标记为固定，防止GC回收
    }
}

} // namespace Lua
