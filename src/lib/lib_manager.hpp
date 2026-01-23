#pragma once

#include "lib/lib_module.hpp"

namespace Lua {

class StandardLibrary {
public:
    static void openAll(LuaState* L);

    static void openBase(LuaState* L);

    static void openMath(LuaState* L);

    static void openIO(LuaState* L);

    static void openString(LuaState* L);

    static void openModule(LuaState* L, LibModule& module);
};

} // namespace Lua
