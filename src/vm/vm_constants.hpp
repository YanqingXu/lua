/**
 * @file vm_constants.hpp
 * @brief Lua虚拟机常量定义
 *
 * 集中管理VM执行引擎中使用的所有常量，
 * 与 Lua 5.1 C 实现的 luaconf.h / llimits.h 对应。
 */

#pragma once

#include "common/types.hpp"

namespace Lua {

/**
 * @brief 元方法链最大查找深度
 * 防止 __index / __newindex 元方法形成无限循环。
 * 对应 Lua C: MAXTAGLOOP (lvm.c)
 */
inline constexpr i32 MAXTAGLOOP = 100;

/**
 * @brief 最大嵌套调用深度
 * 防止递归调用导致 C++ 栈溢出。
 * 对应 Lua C: LUAI_MAXCCALLS (luaconf.h)
 */
inline constexpr i32 MAX_CALLS = 200;

/**
 * @brief SETLIST 指令每批次刷新的字段数
 * 即 LFIELDS_PER_FLUSH。
 * 对应 Lua C: LFIELDS_PER_FLUSH (lvm.c, = 50)
 */
inline constexpr i32 FIELDS_PER_FLUSH = 50;

// =====================================================================
// 栈相关常量（对应 Lua C: luaconf.h / lstate.h）
// =====================================================================

/**
 * @brief 最小栈大小
 * 对应 Lua C: LUA_MINSTACK (lua.h)
 */
inline constexpr usize MIN_STACK_SIZE = 20;

/**
 * @brief 初始栈大小
 * 对应 Lua C: BASIC_STACK_SIZE = 2*LUA_MINSTACK (lstate.c)
 */
inline constexpr usize INITIAL_STACK_SIZE = 40;

/**
 * @brief 额外栈空间（用于元方法调用等）
 * 对应 Lua C: EXTRA_STACK (llimits.h)
 */
inline constexpr usize EXTRA_STACK = 5;

/**
 * @brief 最大栈大小（防止无限递归）
 * 对应 Lua C: LUAI_MAXSTACK (luaconf.h)
 */
inline constexpr usize MAX_STACK_SIZE = 1000000;

/**
 * @brief 栈增长余量
 * pushValue 扩容时额外分配的槽位数，避免频繁重新分配。
 */
inline constexpr usize STACK_GROW_MARGIN = 16;

// =====================================================================
// LuaState 相关常量（对应 Lua C: lua.h / lstate.h）
// =====================================================================

/**
 * @brief 初始调用信息数组大小
 * 对应 Lua C: BASIC_CI_SIZE (lstate.c)
 */
inline constexpr usize INITIAL_CI_SIZE = 8;

/**
 * @brief 最大调用深度
 * 对应 Lua C: LUAI_MAXCALLS (luaconf.h)
 */
inline constexpr usize MAX_CALL_DEPTH = 20000;

/**
 * @brief 多返回值标记
 * 对应 Lua C: LUA_MULTRET (lua.h, = -1)
 */
inline constexpr i32 MULTRET = -1;

/**
 * @brief 执行状态码（与 Lua 5.1 兼容）
 * 对应 Lua C: lua.h 中的状态码定义
 */
/** @brief 成功 */
inline constexpr i32 LUA_OK     = 0;
/** @brief 运行时错误 */
inline constexpr i32 LUA_ERRRUN = 2;
/** @brief 内存错误 */
inline constexpr i32 LUA_ERRMEM = 4;
/** @brief 错误处理函数错误 */
inline constexpr i32 LUA_ERRERR = 5;

} // namespace Lua
