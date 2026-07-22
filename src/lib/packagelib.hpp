/**
 * @file packagelib.hpp
 * @brief Lua 包与模块库：require() 和模块加载系统
 *
 * 实现 Lua 5.1 包系统，包括：
 * - require(modname)——加载并缓存模块
 * - module(name, ...)——创建模块环境
 * - package.loaded——已加载模块表
 * - package.preload——预加载函数表
 * - package.path——Lua 模块搜索路径
 * - package.cpath——C 模块搜索路径
 * - package.config——路径配置字符串
 * - package.loaders——搜索器函数表
 * - package.loadlib——动态库加载器
 * - package.seeall——向 _G 开放模块环境
 *
 * API 行为遵循 Lua 5.1 参考手册第 5.3 节。
 *
 * @author Lua C++ 项目
 * @date 2026-04-10
 */

#pragma once

#include "common/types.hpp"
#include "lib/lib_module.hpp"
#include "vm/state/lua_state.hpp"

namespace Lua {

/** @brief Lua 包与模块加载库。 */
class PackageLibModule : public LibModule {
public:
    const char* getName() const override { return "package"; }

    void registerFunctions(LuaState* L) override;

    void initialize(LuaState* L) override;
};

/**
 * @brief 在全局环境中注册包库
 * @param L Lua 状态指针
 *
 * 创建全局 `package` 表，将 require() 与 module() 注册为全局函数，并配置默认加载器。
 */
void openPackageLib(LuaState* L);

} // namespace Lua
