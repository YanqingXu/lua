/**
 * @file stringlib.hpp
 * @brief Lua 字符串库：字符串操作函数
 *
 * 详细说明：
 * 本模块实现 Lua 字符串库，提供完整的字符串操作能力，包括：
 * - 基本操作：len、sub、reverse、rep
 * - 大小写转换：upper、lower
 * - 字节操作：byte、char
 * - 模式匹配：find、gsub、match、gmatch
 * - 格式化：format
 * - 高级功能：dump
 *
 * API 行为遵循 Lua 5.1 参考手册。
 *
 * @author Lua C++ 项目
 * @date 2026-01-23
 */

#pragma once

#include "common/types.hpp"
#include "lib/lib_module.hpp"
#include "vm/state/lua_state.hpp"

namespace Lua {

/** @brief Lua 字符串库模块。 */
class StringLibModule : public LibModule {
public:
    const char* getName() const override {
        return "string";
    }

    void registerFunctions(LuaState* L) override;

    void initialize(LuaState* L) override;
};

/**
 * @brief 将字符串库注册到全局环境
 * @param L Lua 状态指针
 *
 * 将全部字符串库函数注册到全局 `string` 表。
 */
void openStringLib(LuaState* L);

// =====================================================================
// 字符串库函数声明
// =====================================================================

/**
 * @brief string.len(s)——获取字符串长度
 * @param L Lua 状态指针
 * @return 返回值数量（1：长度）
 */
i32 str_len(LuaState* L);

/**
 * @brief string.sub(s, i [, j])——提取子字符串
 * @param L Lua 状态指针
 * @return 返回值数量（1：子字符串）
 */
i32 str_sub(LuaState* L);

/**
 * @brief string.upper(s)——转换为大写
 * @param L Lua 状态指针
 * @return 返回值数量（1：大写字符串）
 */
i32 str_upper(LuaState* L);

/**
 * @brief string.lower(s)——转换为小写
 * @param L Lua 状态指针
 * @return 返回值数量（1：小写字符串）
 */
i32 str_lower(LuaState* L);

/**
 * @brief string.reverse(s)——反转字符串
 * @param L Lua 状态指针
 * @return 返回值数量（1：反转后的字符串）
 */
i32 str_reverse(LuaState* L);

/**
 * @brief string.rep(s, n)——重复字符串 n 次
 * @param L Lua 状态指针
 * @return 返回值数量（1：重复后的字符串）
 */
i32 str_rep(LuaState* L);

/**
 * @brief string.byte(s [, i [, j]])——获取字节值
 * @param L Lua 状态指针
 * @return 返回值数量（可变：字节值）
 */
i32 str_byte(LuaState* L);

/**
 * @brief string.char(...)——根据字节值创建字符串
 * @param L Lua 状态指针
 * @return 返回值数量（1：字符串）
 */
i32 str_char(LuaState* L);

/**
 * @brief string.find(s, pattern [, init [, plain]])——在字符串中查找模式
 * @param L Lua 状态指针
 * @return 返回值数量（2 至 3：起点、终点与捕获值）
 */
i32 str_find(LuaState* L);

/**
 * @brief string.gsub(s, pattern, repl [, n])——执行全局替换
 * @param L Lua 状态指针
 * @return 返回值数量（2：结果字符串与替换次数）
 */
i32 str_gsub(LuaState* L);

/**
 * @brief string.match(s, pattern [, init])——匹配模式
 * @param L Lua 状态指针
 * @return 返回值数量（可变：捕获值或完整匹配）
 */
i32 str_match(LuaState* L);

/**
 * @brief string.gmatch(s, pattern)——创建模式匹配迭代器
 * @param L Lua 状态指针
 * @return 返回值数量（1：迭代器函数）
 */
i32 str_gmatch(LuaState* L);

/**
 * @brief string.format(formatstring, ...)——格式化字符串
 * @param L Lua 状态指针
 * @return 返回值数量（1：格式化后的字符串）
 */
i32 str_format(LuaState* L);

/**
 * @brief string.dump(function)——导出函数字节码
 * @param L Lua 状态指针
 * @return 返回值数量（1：字节码字符串）
 */
i32 str_dump(LuaState* L);

} // namespace Lua
