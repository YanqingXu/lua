/**
 * @file repl.hpp
 * @brief REPL（交互式解释器）模块头文件
 *
 * 详细说明：
 * 本模块实现了 Lua 的交互式 REPL（Read-Eval-Print Loop）功能，
 * 参考官方 Lua 5.1.5 的 lua.c 中的 dotty()、loadline()、pushline() 等函数。
 *
 * 主要功能：
 * - 显示欢迎信息和提示符
 * - 读取用户输入（支持多行输入）
 * - 检测输入是否完整
 * - 解析并执行代码
 * - 打印表达式结果
 * - 处理错误并继续运行
 *
 * 参考实现：
 * - lua_c_analysis/src/lua.c - dotty(), loadline(), pushline(), incomplete()
 *
 * @author Lua C++ Project
 * @date 2025-12-04
 */

#ifndef LUA_REPL_HPP
#define LUA_REPL_HPP

#include "vm/lua_state.hpp"

namespace Lua {

/**
 * @brief REPL 模块命名空间
 *
 * 包含所有 REPL 相关的函数和常量。
 */
namespace REPL {

// ============================================================================
// REPL 常量定义
// ============================================================================

/// 主提示符（对应 LUA_PROMPT）
constexpr const char* PROMPT1 = "> ";

/// 续行提示符（对应 LUA_PROMPT2）
constexpr const char* PROMPT2 = ">> ";

/// 版本信息
constexpr const char* VERSION = "Lua 5.1 (C++ Implementation)";

/// 版权信息
constexpr const char* COPYRIGHT = "Copyright (c) 2025 Lua C++ Project";

// ============================================================================
// REPL 公共接口
// ============================================================================

/**
 * @brief 运行交互式 REPL 模式
 *
 * 实现完整的 REPL（Read-Eval-Print Loop）功能：
 * 1. 显示欢迎信息
 * 2. 显示提示符并读取用户输入
 * 3. 检测输入是否完整（支持多行输入）
 * 4. 解析并执行代码
 * 5. 打印表达式结果
 * 6. 处理错误并继续运行
 *
 * 参考官方 Lua 的 dotty() 函数实现。
 *
 * @param L Lua 状态机指针
 * @return 执行状态码（0=成功）
 *
 * @see lua_c_analysis/src/lua.c - dotty()
 */
int run(LuaState* L);

}  // namespace REPL

}  // namespace Lua

#endif  // LUA_REPL_HPP

