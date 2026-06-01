/**
 * @file vm.hpp
 * @brief Lua虚拟机执行引擎：字节码解释器
 * 
 * 设计说明：
 * VM命名空间以自由函数的形式提供字节码执行引擎。
 * 与Lua 5.1 C实现的lvm.c设计一致：luaV_execute(L, nexeccalls)
 * 是接收lua_State*参数的自由函数，所有执行状态（pc, base, cl）
 * 作为局部变量存在于函数调用栈中。
 * 
 * 核心特性：
 * - 完整的Lua 5.1指令集支持（38条指令）
 * - 基于寄存器的虚拟机架构
 * - 高效的指令分发（switch-case）
 * - 函数调用和返回机制
 * - 栈帧管理和程序计数器
 * - 错误处理和异常传播
 * @author Lua C++ Project
 * @date 2025-11-13
 */

#pragma once

#include "common/lua_error.hpp"
#include "common/types.hpp"

#include <expected>

namespace Lua {

// 前向声明
class LuaState;
class Function;
class Proto;
class ITraceSink;
struct RuntimeServices;

/// VM 执行结果（替代 void 返回，让 yield 成为正常控制流）
enum class ExecResult : u8 {
    Returned,      // 函数正常返回
    Yielded        // 协程 yield
};

/**
 * @brief 虚拟机执行引擎命名空间
 * 
 * 使用示例：
 * @code
 * UPtr<LuaState> L = LuaState::create();
 * Function* func = ...;  // 从CodeGenerator获取
 * VM::execute(L.get(), func);
 * Value result = L->getStack().top();
 * @endcode
 */
namespace VM {

    /**
     * @brief 执行Lua函数
     * @param L Lua状态指针
     * @param func 要执行的函数对象
     */
    void execute(LuaState* L, Function* func);

    /**
     * @brief 使用显式运行时服务执行Lua函数
     */
    void execute(RuntimeServices& services, LuaState* L, Function* func);

    /**
     * @brief 执行字节码块（Proto）
     * @param L Lua状态指针
     * @param proto 函数原型
     * @param nexeccalls 嵌套调用计数（用于检测栈溢出）
     * @return ExecResult::Returned 或 ExecResult::Yielded
     */
    ExecResult executeProto(LuaState* L, Proto* proto, i32 nexeccalls = 1);

    /**
     * @brief 使用显式运行时服务执行字节码块（Proto）
     */
    ExecResult executeProto(RuntimeServices& services, LuaState* L, Proto* proto, i32 nexeccalls = 1);

    /**
     * @brief 执行字节码块（Proto），以 expected 返回运行期错误
     */
    [[nodiscard]] std::expected<ExecResult, RuntimeError> tryExecuteProto(
        LuaState* L, Proto* proto, i32 nexeccalls = 1);

    /**
     * @brief 使用显式运行时服务执行字节码块（Proto），以 expected 返回运行期错误
     */
    [[nodiscard]] std::expected<ExecResult, RuntimeError> tryExecuteProto(
        RuntimeServices& services, LuaState* L, Proto* proto, i32 nexeccalls = 1);

    /**
     * @brief 从CFunction内部调用栈上的函数（不清除栈）
     *
     * 调用方先将 func + args 压入栈，然后调用此函数。
     * 执行完毕后结果替换到原 func 位置。
     *
     * @param L Lua状态指针
     * @param nargs 参数个数（不含函数本身）
     * @param nresults 期望的返回值数量（-1 = MULTRET）
     */
    void call(LuaState* L, i32 nargs, i32 nresults);

    /**
     * @brief 使用显式运行时服务从CFunction内部调用栈上的函数
     */
    void call(RuntimeServices& services, LuaState* L, i32 nargs, i32 nresults);

    /**
     * @brief 设置全局 Trace Sink（nullptr 表示关闭 trace）
     */
    void setTraceSink(ITraceSink* sink);

    /**
     * @brief 获取当前 Trace Sink
     */
    ITraceSink* getTraceSink();

    /**
     * @brief 开关 trace 差异模式；开启后 instruction 事件包含 changedRegisters。
     */
    void setTraceDiffEnabled(bool enabled);

    /**
     * @brief 查询 trace 差异模式是否开启。
     */
    bool isTraceDiffEnabled();

} // namespace VM

} // namespace Lua

