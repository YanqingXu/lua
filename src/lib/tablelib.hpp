/**
 * @file tablelib.hpp
 * @brief Lua Table Library - 表操作库
 * 
 * 实现 Lua 5.1 标准的 table 库函数，提供表的插入、删除、排序、连接等操作。
 * 
 * 核心功能：
 * - table.insert: 在表中插入元素
 * - table.remove: 从表中移除元素
 * - table.concat: 连接表中的字符串
 * - table.sort: 对表进行排序
 * - table.pack: 打包可变参数为表
 * - table.unpack: 解包表为多个返回值
 * - table.move: 移动表元素
 * 
 * @author Lua C++ Project
 * @date 2026-01-23
 */

#ifndef LUA_TABLELIB_HPP
#define LUA_TABLELIB_HPP

#include "common/types.hpp"

namespace Lua {

// 前向声明
class LuaState;

// =====================================================================
// Table 库函数声明
// =====================================================================

/**
 * @brief table.insert(table, [pos,] value) - 在表中插入元素
 * 
 * 如果提供 pos，则在该位置插入 value，并将后续元素后移。
 * 如果不提供 pos，则在表末尾插入 value。
 * 
 * @param L Lua 状态机
 * @return 返回值数量（0）
 */
i32 table_insert(LuaState* L);

/**
 * @brief table.remove(table, [pos]) - 从表中移除元素
 * 
 * 移除并返回 table[pos] 的值，并将后续元素前移。
 * 如果不提供 pos，则移除表末尾的元素。
 * 
 * @param L Lua 状态机
 * @return 返回值数量（1，被移除的元素）
 */
i32 table_remove(LuaState* L);

/**
 * @brief table.concat(table, [sep, [i, [j]]]) - 连接表中的字符串
 * 
 * 将 table[i] 到 table[j] 的元素用 sep 连接成字符串。
 * 默认 sep 为空字符串，i 为 1，j 为表长度。
 * 
 * @param L Lua 状态机
 * @return 返回值数量（1，连接后的字符串）
 */
i32 table_concat(LuaState* L);

/**
 * @brief table.sort(table, [comp]) - 对表进行排序
 * 
 * 对 table[1] 到 table[#table] 进行原地排序。
 * 如果提供 comp 函数，则使用该函数比较元素。
 * 
 * @param L Lua 状态机
 * @return 返回值数量（0）
 */
i32 table_sort(LuaState* L);

/**
 * @brief table.pack(...) - 打包可变参数为表
 * 
 * 返回一个包含所有参数的表，表中有一个字段 "n" 表示参数数量。
 * 
 * @param L Lua 状态机
 * @return 返回值数量（1，打包后的表）
 */
i32 table_pack(LuaState* L);

/**
 * @brief table.unpack(table, [i, [j]]) - 解包表为多个返回值
 * 
 * 返回 table[i], table[i+1], ..., table[j]。
 * 默认 i 为 1，j 为表长度。
 * 
 * @param L Lua 状态机
 * @return 返回值数量（j - i + 1）
 */
i32 table_unpack(LuaState* L);

/**
 * @brief table.move(a1, f, e, t, [a2]) - 移动表元素
 * 
 * 将表 a1 的元素从 f 到 e 移动到表 a2（默认为 a1）的位置 t。
 * 
 * @param L Lua 状态机
 * @return 返回值数量（1，目标表）
 */
i32 table_move(LuaState* L);

// =====================================================================
// 库注册
// =====================================================================

/**
 * @brief Table 库模块类
 */
class TableLibModule {
public:
    /**
     * @brief 注册所有 table 库函数到 Lua 状态机
     * @param L Lua 状态机
     */
    void registerFunctions(LuaState* L);

    /**
     * @brief 初始化 table 库（如果需要）
     * @param L Lua 状态机
     */
    void initialize(LuaState* L);
};

/**
 * @brief 打开 table 库
 * @param L Lua 状态机
 */
void openTableLib(LuaState* L);

} // namespace Lua

#endif // LUA_TABLELIB_HPP

