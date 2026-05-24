/**
 * @file debuglib.hpp
 * @brief Lua debug library: runtime introspection and traceback helpers
 *
 * Detailed Description:
 * This module implements the Lua `debug` standard library pieces that are
 * currently needed by the project. The first version focuses on stable and
 * already-available runtime metadata, including:
 * - Registry access: getregistry
 * - Upvalue inspection and mutation: getupvalue, setupvalue
 * - Debug info lookup: getinfo
 * - Stack trace formatting: traceback
 *
 * Design Goals:
 * - Expose only debug data that the current VM already maintains reliably
 * - Keep the public API shape close to Lua 5.1 where practical
 * - Local variable inspection and mutation: getlocal, setlocal
 * - Hook management: sethook, gethook, debug
 * - Raw metatable access: getmetatable, setmetatable
 * - Function environment wrappers: getfenv, setfenv
 *
 * API behavior follows the Lua 5.1 Reference Manual where practical.
 *
 * @author Lua C++ Project
 * @date 2026-04-10
 */

#pragma once

#include "common/types.hpp"
#include "lib/lib_module.hpp"
#include "vm/state/lua_state.hpp"

namespace Lua {

class DebugLibModule : public LibModule {
public:
    const char* getName() const override { return "debug"; }

    void registerFunctions(LuaState* L) override;
};

/**
 * @brief Register the debug library in the global environment
 * @param L Lua state pointer
 *
 * Creates the global `debug` table and registers all currently implemented
 * debug library functions into it.
 */
void openDebugLib(LuaState* L);

// =====================================================================
// Debug Library Function Declarations
// =====================================================================

/**
 * @brief debug.getregistry() - Get the registry table
 * @param L Lua state pointer
 * @return Number of return values (1: registry table)
 */
i32 luaDebug_getregistry(LuaState* L);

/**
 * @brief debug.getupvalue(func, up) - Get a function upvalue
 * @param L Lua state pointer
 * @return Number of return values (1 or 2: nil on failure, name + value on success)
 */
i32 luaDebug_getupvalue(LuaState* L);

/**
 * @brief debug.setupvalue(func, up, value) - Set a function upvalue
 * @param L Lua state pointer
 * @return Number of return values (1: nil on failure, upvalue name on success)
 */
i32 luaDebug_setupvalue(LuaState* L);

/**
 * @brief debug.getinfo(thread|func|level [, what]) - Get debug information
 * @param L Lua state pointer
 * @return Number of return values (1: info table, nil on failure)
 */
i32 luaDebug_getinfo(LuaState* L);

/**
 * @brief debug.getlocal(thread|func|level, local) - Get a local variable
 * @param L Lua state pointer
 * @return Number of return values (1 on failure/function query, 2 for active locals)
 */
i32 luaDebug_getlocal(LuaState* L);

/**
 * @brief debug.setlocal([thread,] level, local, value) - Set an active local variable
 * @param L Lua state pointer
 * @return Number of return values (1: local name or nil)
 */
i32 luaDebug_setlocal(LuaState* L);

/**
 * @brief debug.getmetatable(object) - Get the raw metatable
 * @param L Lua state pointer
 * @return Number of return values (1: metatable or nil)
 */
i32 luaDebug_getmetatable(LuaState* L);

/**
 * @brief debug.setmetatable(object, table|nil) - Set the raw metatable
 *
 * Current compatibility boundary: this VM supports raw metatable mutation for
 * tables and full userdata. Per-type metatables for numbers/strings/etc. are
 * not implemented yet.
 *
 * @param L Lua state pointer
 * @return Number of return values (1: original object)
 */
i32 luaDebug_setmetatable(LuaState* L);

/**
 * @brief debug.getfenv(f) - Get a function environment
 *
 * Compatibility boundary: delegates to the base getfenv implementation, which
 * currently supports function objects and the global fallback. Stack-level and
 * thread environment queries are not implemented yet.
 *
 * @param L Lua state pointer
 * @return Number of return values (1: environment table)
 */
i32 luaDebug_getfenv(LuaState* L);

/**
 * @brief debug.setfenv(f, table) - Set a function environment
 *
 * Compatibility boundary: delegates to the base setfenv implementation, which
 * currently supports Lua function objects. Stack-level/thread environment
 * mutation and C-function environments are not implemented yet.
 *
 * @param L Lua state pointer
 * @return Number of return values (1: function)
 */
i32 luaDebug_setfenv(LuaState* L);

/**
 * @brief debug.traceback([message [, level]]) - Build a traceback string
 * @param L Lua state pointer
 * @return Number of return values (1: traceback string)
 */
i32 luaDebug_traceback(LuaState* L);

/**
 * @brief debug.sethook([thread,] hook, mask [, count]) - Install a debug hook
 * @param L Lua state pointer
 * @return Number of return values (0)
 */
i32 luaDebug_sethook(LuaState* L);

/**
 * @brief debug.gethook([thread]) - Query the current debug hook
 * @param L Lua state pointer
 * @return Number of return values (3: hook, mask, count)
 */
i32 luaDebug_gethook(LuaState* L);

/**
 * @brief debug.debug() - Enter the interactive debug console
 * @param L Lua state pointer
 * @return Number of return values (0)
 */
i32 luaDebug_debug(LuaState* L);

} // namespace Lua
