/**
 * @file baselib.hpp
 * @brief Lua基础库：核心函数和基本操作
 * 
 * 详细说明：
 * 本模块实现了Lua的基础库函数，提供了最核心和最常用的标准库功能。
 * 这些函数构成了Lua编程的基础，包括输入输出、类型操作、元表管理、错误处理等核心功能。
 * 
 * 主要功能：
 * 1. 输入输出：print
 * 2. 类型操作：type、tonumber、tostring
 * 3. 元表操作：getmetatable、setmetatable
 * 4. 错误处理：error、assert
 * @author Lua C++ 项目
 * @date 2025-11-13
 */

#pragma once

#include "common/types.hpp"
#include "lib/lib_module.hpp"
#include "vm/state/lua_state.hpp"

namespace Lua {

/** @brief Lua 基础库模块。 */
class BaseLibModule : public LibModule {
public:
	const char* getName() const override { return "base"; }

	void registerFunctions(LuaState* L) override;

	void initialize(LuaState* L) override;
};

/**
 * @brief 注册基础库到全局环境
 * @param L Lua状态机指针
 * 
 * 将所有基础库函数注册到全局表中，使其可以从Lua代码中调用。
 */
void openBaseLib(LuaState* L);

// =====================================================================
// 基础库函数声明
// =====================================================================

/**
 * @brief print(...) - 打印任意数量的参数到标准输出
 * @param L Lua状态机指针
 * @return 返回值数量（总是0）
 */
i32 luaB_print(LuaState* L);

/**
 * @brief type(v) - 返回值的类型字符串
 * @param L Lua状态机指针
 * @return 返回值数量（1个：类型字符串）
 */
i32 luaB_type(LuaState* L);

/**
 * @brief tostring(v) - 将值转换为字符串
 * @param L Lua状态机指针
 * @return 返回值数量（1个：字符串）
 */
i32 luaB_tostring(LuaState* L);

/**
 * @brief tonumber(e [, base]) - 将值转换为数字
 * @param L Lua状态机指针
 * @return 返回值数量（1个：数字或nil）
 */
i32 luaB_tonumber(LuaState* L);

/**
 * @brief error(message [, level]) - 抛出错误
 * @param L Lua状态机指针
 * @return 不返回（抛出错误）
 */
i32 luaB_error(LuaState* L);

/**
 * @brief assert(v [, message]) - 断言
 * @param L Lua状态机指针
 * @return 返回值数量（返回所有参数）
 */
i32 luaB_assert(LuaState* L);

/**
 * @brief setmetatable(table, metatable) - 设置表的元表
 * @param L Lua状态机指针
 * @return 返回值数量（1个：设置元表的表）
 */
i32 luaB_setmetatable(LuaState* L);

/**
 * @brief getmetatable(object) - 获取对象的元表
 * @param L Lua状态机指针
 * @return 返回值数量（1个：元表或nil）
 */
i32 luaB_getmetatable(LuaState* L);

/**
 * @brief rawget(table, index) - 绕过元方法直接获取表元素
 * @param L Lua状态机指针
 * @return 返回值数量（1个：table[index]的值）
 */
i32 luaB_rawget(LuaState* L);

/**
 * @brief rawset(table, index, value) - 绕过元方法直接设置表元素
 * @param L Lua状态机指针
 * @return 返回值数量（1个：表本身）
 */
i32 luaB_rawset(LuaState* L);

/**
 * @brief rawequal(v1, v2) - 绕过元方法直接比较两个值
 * @param L Lua状态机指针
 * @return 返回值数量（1个：布尔值）
 */
i32 luaB_rawequal(LuaState* L);

/**
 * @brief select(index, ...) - 从可变参数中选择特定范围的参数
 * @param L Lua状态机指针
 * @return 返回值数量（可变，取决于选择的参数数量）
 */
i32 luaB_select(LuaState* L);

/**
 * @brief pcall(f, arg1, ...) - 保护模式调用函数
 * @param L Lua状态机指针
 * @return 返回值数量（成功时返回 true + 结果，失败时返回 false + 错误消息）
 */
i32 luaB_pcall(LuaState* L);

/**
 * @brief xpcall(f, msgh, arg1, ...) - 带错误处理器的保护调用
 * @param L Lua状态机指针
 * @return 返回值数量（成功时返回 true + 结果，失败时返回 false + msgh结果）
 */
i32 luaB_xpcall(LuaState* L);

/**
 * @brief loadstring(string [, chunkname]) - 编译字符串为函数
 * @param L Lua状态机指针
 * @return 返回值数量（成功时返回函数，失败时返回 nil + 错误消息）
 */
i32 luaB_loadstring(LuaState* L);

/**
 * @brief loadfile([filename]) - 编译文件为函数
 * @param L Lua状态机指针
 * @return 返回值数量（成功时返回函数，失败时返回 nil + 错误消息）
 */
i32 luaB_loadfile(LuaState* L);

/**
 * @brief dofile([filename]) - 加载并执行文件
 * @param L Lua状态机指针
 * @return 返回值数量（文件执行的结果）
 */
i32 luaB_dofile(LuaState* L);

/**
 * @brief gcinfo() - 获取GC内存使用量（已废弃）
 * @param L Lua状态机指针
 * @return 返回值数量（1个：内存使用量KB）
 */
i32 luaB_gcinfo(LuaState* L);

/**
 * @brief getfenv(f) - 获取函数环境表
 * @param L Lua状态机指针
 * @return 返回值数量（1个：环境表）
 */
i32 luaB_getfenv(LuaState* L);

/**
 * @brief setfenv(f, table) - 设置函数环境表
 * @param L Lua状态机指针
 * @return 返回值数量（1个：被修改的函数对象）
 */
i32 luaB_setfenv(LuaState* L);

/**
 * @brief collectgarbage(opt [, arg]) - 垃圾回收控制
 * @param L Lua状态机指针
 * @return 返回值数量（1个：操作结果）
 */
i32 luaB_collectgarbage(LuaState* L);

} // namespace Lua

