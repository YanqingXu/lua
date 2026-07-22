/**
 * @file thread.hpp
 * @brief Lua 协程对象
 *
 * 协程是垃圾回收器管理的对象，每个协程持有一个独立的 Lua 状态
 * （独立值栈和调用栈，共享全局状态）。
 *
 * 核心设计：
 *   - 所有执行现场保存在 Lua 状态和调用信息中（不依赖 C++ 栈帧）
 *   - 虚拟机通过已挂起执行结果退出执行循环
 *   - 恢复和挂起通过显式值搬运及虚拟机重入实现
 */

#pragma once

#include "core/gc_object.hpp"
#include "common/types.hpp"
#include <memory>

namespace Lua {

class LuaState;
class Function;
class GarbageCollector;
enum class ThreadStatus : u8;

/** @brief 按所有权标志销毁 Lua 状态的删除器。 */
struct LuaStateOwnerDeleter {
    bool ownsState = true;
    void operator()(LuaState* state) const noexcept;
};

using LuaStateOwner = std::unique_ptr<LuaState, LuaStateOwnerDeleter>;

/**
 * @brief Lua 协程状态（与线程执行状态不同，这是 Lua 层面语义）
 */
enum class CoroutineStatus : u8 {
    Suspended, // 创建后或挂起后
    Running,   // 正在执行
    Normal,    // 恢复其他协程后自身暂停
    Dead       // 函数返回或出错
};

/** @brief 由垃圾回收器管理的 Lua 协程。 */
class Thread : public GCObject {
public:
    // === 工厂方法 ===

    /**
     * @brief 创建协程
     * @param parentL 创建者的 Lua 状态（用于共享全局状态）
     * @param func 协程要执行的 Lua 函数
     */
    static Thread* create(LuaState* parentL, Function* func);

    /**
     * @brief 创建由 C API 填充入口函数和参数的空协程。
     */
    static Thread* create(LuaState* parentL);

    /** @brief 垃圾回收工厂构造函数；调用处优先使用协程创建函数。 */
    explicit Thread(LuaStateOwner state);

    /** @brief 将主状态表示为 Lua 线程值的非拥有型外观。 */
    explicit Thread(LuaState* mainState);

    ~Thread();

    // === 核心操作 ===

    /**
     * @brief 恢复协程
     * @param callerL 调用者的 Lua 状态
     * @param nargs 恢复参数数量（已位于 callerL 栈顶）
     * @return true 表示成功（正常返回或挂起），false 表示错误
     *
     * 恢复操作会在内部向 callerL 栈压入成功标志及结果值。
     */
    bool resume(LuaState* callerL, i32 nargs);

    /**
     * @brief 宿主或 API 失败后将协程置于规范的死亡状态
     *
     * lua_resume 在准备调用帧或复制结果时可能分配内存。发生此类异常后，本回滚操作可防止
     * 构造不完整的调用信息栈通过公开 API 暴露。
     */
    void abortResume(ThreadStatus status) noexcept;

    // === 状态查询 ===

    CoroutineStatus getCoroutineStatus() const noexcept {
        return coStatus_;
    }
    LuaState* getLuaState() const noexcept {
        return state_.get();
    }
    bool ownsLuaState() const noexcept {
        return state_.get_deleter().ownsState;
    }
    bool isDead() const noexcept {
        return coStatus_ == CoroutineStatus::Dead;
    }
    bool isSuspended() const noexcept {
        return coStatus_ == CoroutineStatus::Suspended;
    }

    // === 恢复链管理 ===

    Thread* getCaller() const noexcept {
        return caller_;
    }
    void setCaller(Thread* t) noexcept {
        caller_ = t;
    }

    // === GCObject 接口 ===

    void mark(GarbageCollector& gc) override;
    usize getSize() const override;

private:
    LuaStateOwner state_;
    CoroutineStatus coStatus_;
    Thread* caller_ = nullptr;
    LuaState* callerState_ = nullptr;
    bool firstResume_ = true;
    i32 savedNexeccalls_ = 1;
};

} // namespace Lua
