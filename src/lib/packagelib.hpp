/**
 * @file packagelib.hpp
 * @brief Lua package/module library: require() and module loading system
 *
 * Implements the Lua 5.1 package system including:
 * - require(modname)    — load and cache modules
 * - module(name, ...)   — create a module environment
 * - package.loaded      — table of already-loaded modules
 * - package.preload     — table of preload functions
 * - package.path        — search path for Lua modules
 * - package.cpath       — search path for C modules
 * - package.config      — path configuration string
 * - package.loaders     — table of searcher functions
 * - package.loadlib     — dynamic library loader
 * - package.seeall      — open module environment to _G
 *
 * API behavior follows the Lua 5.1 Reference Manual §5.3.
 *
 * @author Lua C++ Project
 * @date 2026-04-10
 */

#pragma once

#include "common/types.hpp"
#include "lib/lib_module.hpp"
#include "vm/state/lua_state.hpp"

namespace Lua {

class PackageLibModule : public LibModule {
public:
    const char* getName() const override { return "package"; }

    void registerFunctions(LuaState* L) override;

    void initialize(LuaState* L) override;
};

/**
 * @brief Register the package library in the global environment
 * @param L Lua state pointer
 *
 * Creates the global `package` table, registers require() and module()
 * as global functions, and sets up the default loaders.
 */
void openPackageLib(LuaState* L);

} // namespace Lua
