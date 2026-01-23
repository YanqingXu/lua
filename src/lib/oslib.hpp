#pragma once

#include "common/types.hpp"
#include "vm/lua_state.hpp"
#include <ctime>

namespace Lua {

/**
 * @brief os.clock() - 获取CPU时间
 * @param L Lua状态机指针
 * @return 返回值数量（1个：CPU时间秒数）
 */
i32 luaOS_clock(LuaState* L);

/**
 * @brief os.date([format [, time]]) - 格式化日期时间
 * @param L Lua状态机指针
 * @return 返回值数量（1个：格式化字符串或日期表）
 */
i32 luaOS_date(LuaState* L);

/**
 * @brief os.difftime(t2, t1) - 计算时间差
 * @param L Lua状态机指针
 * @return 返回值数量（1个：时间差秒数）
 */
i32 luaOS_difftime(LuaState* L);

/**
 * @brief os.execute([command]) - 执行系统命令
 * @param L Lua状态机指针
 * @return 返回值数量（1个：退出码或nil）
 */
i32 luaOS_execute(LuaState* L);

/**
 * @brief os.exit([code]) - 退出程序
 * @param L Lua状态机指针
 * @return 不返回（程序退出）
 */
i32 luaOS_exit(LuaState* L);

/**
 * @brief os.getenv(varname) - 获取环境变量
 * @param L Lua状态机指针
 * @return 返回值数量（1个：环境变量值或nil）
 */
i32 luaOS_getenv(LuaState* L);

/**
 * @brief os.remove(filename) - 删除文件
 * @param L Lua状态机指针
 * @return 返回值数量（1个：true或nil）
 */
i32 luaOS_remove(LuaState* L);

/**
 * @brief os.rename(oldname, newname) - 重命名文件
 * @param L Lua状态机指针
 * @return 返回值数量（1个：true或nil）
 */
i32 luaOS_rename(LuaState* L);

/**
 * @brief os.setlocale(locale [, category]) - 设置区域设置
 * @param L Lua状态机指针
 * @return 返回值数量（1个：区域设置字符串或nil）
 */
i32 luaOS_setlocale(LuaState* L);

/**
 * @brief os.time([table]) - 获取时间戳或转换时间表
 * @param L Lua状态机指针
 * @return 返回值数量（1个：时间戳或nil）
 */
i32 luaOS_time(LuaState* L);

/**
 * @brief os.tmpname() - 生成临时文件名
 * @param L Lua状态机指针
 * @return 返回值数量（1个：临时文件名）
 */
i32 luaOS_tmpname(LuaState* L);

/**
 * @brief 打开OS库
 * @param L Lua状态机指针
 */
void openOSLib(LuaState* L);

} // namespace Lua

