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
 * - 支持 Ctrl+C 中断信号
 * - 支持可配置的提示符 (_PROMPT, _PROMPT2)
 *
 * 参考实现：
 * - lua_c_analysis/src/lua.c - dotty(), loadline(), pushline(), incomplete()
 * - lua_with_cpp/src/repl.cpp - 信号处理、可配置提示符
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

/// 默认主提示符（对应 LUA_PROMPT）
constexpr const char* DEFAULT_PROMPT1 = "> ";

/// 默认续行提示符（对应 LUA_PROMPT2）
constexpr const char* DEFAULT_PROMPT2 = ">> ";

/// 版本信息
constexpr const char* VERSION = "Lua 5.1 (C++ Implementation)";

/// 版权信息
constexpr const char* COPYRIGHT = "Copyright (c) 2025 Lua C++ Project";

/// Lua 版本字符串（用于 _VERSION 全局变量）
constexpr const char* LUA_VERSION = "Lua 5.1";

// ============================================================================
// REPL 公共接口
// ============================================================================

/**
 * @brief 初始化 REPL 环境
 *
 * 设置 REPL 所需的全局变量和函数：
 * - _VERSION: Lua 版本信息
 * - _PROMPT: 主提示符（可由用户修改）
 * - _PROMPT2: 续行提示符（可由用户修改）
 * - exit(): 退出 REPL 的函数
 *
 * @param L Lua 状态机指针
 */
void initialize(LuaState* L);

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
 * 支持的特性：
 * - Ctrl+C 中断当前输入
 * - 可配置的提示符（通过 _PROMPT 和 _PROMPT2 全局变量）
 * - exit() 函数退出
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

