/**
 * @file thread.hpp
 * @brief Lua Thread (协程) 对象
 *
 * Thread 是 GC 管理的协程对象，每个 Thread 持有一个独立的 LuaState
 * （独立栈 + 调用栈，共享 GlobalState）。
 *
 * 核心设计：
 *   - 所有执行现场保存在 LuaState/CallInfo 中（不依赖 C++ 栈帧）
 *   - VM 通过 ExecResult::Yielded 退出执行循环
 *   - resume/yield 通过显式的值搬运 + VM 重入实现
 */

#pragma once

#include "core/gc_object.hpp"
#include "common/types.hpp"

namespace Lua {

class LuaState;
class Function;
class GarbageCollector;

/// Lua 协程状态（与 ThreadStatus 不同，这是 Lua 层面语义）
enum class CoroutineStatus : u8 {
    Suspended,  // 创建后 / yield 后
    Running,    // 正在执行
    Normal,     // resume 了其他协程，自身暂停
    Dead        // 函数返回或出错
};

class Thread : public GCObject {
public:
    // === 工厂方法 ===

    /// 创建协程
    /// @param parentL 创建者的 LuaState（用于共享 GlobalState）
    /// @param func 协程要执行的 Lua 函数
    static Thread* create(LuaState* parentL, Function* func);

    ~Thread();

    // === 核心操作 ===

    /// resume 协程
    /// @param callerL 调用者的 LuaState
    /// @param nargs resume 参数数量（已在 callerL 栈顶）
    /// @return true=成功（正常返回或 yield），false=错误
    ///
    /// resume 内部会 push true/false + 结果值到 callerL 栈。
    bool resume(LuaState* callerL, i32 nargs);

    // === 状态查询 ===

    CoroutineStatus getCoroutineStatus() const noexcept { return coStatus_; }
    LuaState* getLuaState() const noexcept { return state_; }
    bool isDead() const noexcept { return coStatus_ == CoroutineStatus::Dead; }
    bool isSuspended() const noexcept { return coStatus_ == CoroutineStatus::Suspended; }

    // === resume 链管理 ===

    Thread* getCaller() const noexcept { return caller_; }
    void setCaller(Thread* t) noexcept { caller_ = t; }

    // === GCObject 接口 ===

    void mark(GarbageCollector& gc) override;
    usize getSize() const override;

private:
    explicit Thread(LuaState* state);

    LuaState*       state_;
    CoroutineStatus coStatus_;
    Thread*         caller_ = nullptr;
    bool            firstResume_ = true;
    i32             savedNexeccalls_ = 1;
};

} // namespace Lua
