/**
 * @file coroutinelib.hpp
 * @brief Lua协程库：coroutine.create/resume/yield/status/running/wrap
 */

#pragma once

#include "common/types.hpp"
#include "lib/lib_module.hpp"
#include "vm/lua_state.hpp"

namespace Lua {

class CoroutineLibModule : public LibModule {
public:
    const char* getName() const override { return "coroutine"; }

    void registerFunctions(LuaState* L) override;
};

void openCoroutineLib(LuaState* L);

} // namespace Lua
