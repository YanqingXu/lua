/**
 * @file debuglib.hpp
 * @brief Lua 调试库：运行时内省与调用栈回溯辅助接口
 *
 * 详细说明：
 * 本模块实现项目当前所需的 Lua `debug` 标准库功能。首个版本专注于稳定且运行时已经提供的
 * 元数据，包括：
 * - 注册表访问：getregistry
 * - 上值检查与修改：getupvalue、setupvalue
 * - 调试信息查询：getinfo
 * - 调用栈回溯格式化：traceback
 *
 * 设计目标：
 * - 仅公开当前 VM 已可靠维护的调试数据
 * - 在可行范围内保持公共 API 形态接近 Lua 5.1
 * - 局部变量检查与修改：getlocal、setlocal
 * - 钩子管理：sethook、gethook、debug
 * - 原始元表访问：getmetatable、setmetatable
 * - 函数环境封装：getfenv、setfenv
 *
 * API 行为在可行范围内遵循 Lua 5.1 参考手册。
 *
 * @author Lua C++ 项目
 * @date 2026-04-10
 */

#pragma once

#include "common/types.hpp"
#include "lib/lib_module.hpp"
#include "vm/state/lua_state.hpp"

namespace Lua {

/** @brief Lua 调试库模块。 */
class DebugLibModule : public LibModule {
public:
    const char* getName() const override {
        return "debug";
    }

    void registerFunctions(LuaState* L) override;
};

/**
 * @brief 在全局环境中注册调试库
 * @param L Lua 状态指针
 *
 * 创建全局 `debug` 表，并将当前已实现的全部调试库函数注册到其中。
 */
void openDebugLib(LuaState* L);

// =====================================================================
// 调试库函数声明
// =====================================================================

/**
 * @brief debug.getregistry()——获取注册表
 * @param L Lua 状态指针
 * @return 返回值数量（1：注册表）
 */
i32 luaDebug_getregistry(LuaState* L);

/**
 * @brief debug.getupvalue(func, up)——获取函数上值
 * @param L Lua 状态指针
 * @return 返回值数量（失败时为 1 个 nil，成功时为名称和值共 2 个）
 */
i32 luaDebug_getupvalue(LuaState* L);

/**
 * @brief debug.setupvalue(func, up, value)——设置函数上值
 * @param L Lua 状态指针
 * @return 返回值数量（1：失败时为 nil，成功时为上值名称）
 */
i32 luaDebug_setupvalue(LuaState* L);

/**
 * @brief debug.getinfo(thread|func|level [, what])——获取调试信息
 * @param L Lua 状态指针
 * @return 返回值数量（1：信息表，失败时为 nil）
 */
i32 luaDebug_getinfo(LuaState* L);

/**
 * @brief debug.getlocal(thread|func|level, local)——获取局部变量
 * @param L Lua 状态指针
 * @return 返回值数量（失败或查询函数时为 1，活动局部变量为 2）
 */
i32 luaDebug_getlocal(LuaState* L);

/**
 * @brief debug.setlocal([thread,] level, local, value)——设置活动局部变量
 * @param L Lua 状态指针
 * @return 返回值数量（1：局部变量名称或 nil）
 */
i32 luaDebug_setlocal(LuaState* L);

/**
 * @brief debug.getmetatable(object)——获取原始元表
 * @param L Lua 状态指针
 * @return 返回值数量（1：元表或 nil）
 */
i32 luaDebug_getmetatable(LuaState* L);

/**
 * @brief debug.setmetatable(object, table|nil)——设置原始元表
 *
 * 当前兼容边界：此 VM 支持修改表与完整用户数据的原始元表，尚未实现数值、字符串等类型的
 * 类型级元表。
 *
 * @param L Lua 状态指针
 * @return 返回值数量（1：原对象）
 */
i32 luaDebug_setmetatable(LuaState* L);

/**
 * @brief debug.getfenv(f)——获取函数环境
 *
 * 兼容边界：委托给基础库 getfenv 实现，后者支持 Lua/C 函数对象、调用栈级别与全局后备环境。
 *
 * @param L Lua 状态指针
 * @return 返回值数量（1：环境表）
 */
i32 luaDebug_getfenv(LuaState* L);

/**
 * @brief debug.setfenv(f, table)——设置函数环境
 *
 * 兼容边界：委托给基础库 setfenv 实现，后者支持 Lua/C 函数对象与调用栈级别。线程环境修改
 * 仍由协程或线程路径处理。
 *
 * @param L Lua 状态指针
 * @return 返回值数量（1：函数）
 */
i32 luaDebug_setfenv(LuaState* L);

/**
 * @brief debug.traceback([message [, level]])——构建调用栈回溯字符串
 * @param L Lua 状态指针
 * @return 返回值数量（1：调用栈回溯字符串）
 */
i32 luaDebug_traceback(LuaState* L);

/**
 * @brief debug.sethook([thread,] hook, mask [, count])——安装调试钩子
 * @param L Lua 状态指针
 * @return 返回值数量（0）
 */
i32 luaDebug_sethook(LuaState* L);

/**
 * @brief debug.gethook([thread])——查询当前调试钩子
 * @param L Lua 状态指针
 * @return 返回值数量（3：钩子、掩码与计数）
 */
i32 luaDebug_gethook(LuaState* L);

/**
 * @brief debug.debug()——进入交互式调试控制台
 * @param L Lua 状态指针
 * @return 返回值数量（0）
 */
i32 luaDebug_debug(LuaState* L);

} // namespace Lua
