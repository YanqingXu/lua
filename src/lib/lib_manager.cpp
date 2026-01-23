#include "lib/lib_manager.hpp"

#include "lib/baselib.hpp"
#include "lib/mathlib.hpp"
#include "lib/iolib.hpp"
#include "lib/stringlib.hpp"
#include "vm/lua_state.hpp"

namespace Lua {

void StandardLibrary::openModule(LuaState* L, LibModule& module) {
    if (!L) {
        return;
    }

    module.registerFunctions(L);
    module.initialize(L);
}

void StandardLibrary::openBase(LuaState* L) {
    if (!L) {
        return;
    }

    openBaseLib(L);
}

void StandardLibrary::openMath(LuaState* L) {
    if (!L) {
        return;
    }

    openMathLib(L);
}

void StandardLibrary::openIO(LuaState* L) {
    if (!L) {
        return;
    }

    openIOLib(L);
}

void StandardLibrary::openString(LuaState* L) {
    if (!L) {
        return;
    }

    openStringLib(L);
}

void StandardLibrary::openAll(LuaState* L) {
    if (!L) {
        return;
    }

    openBase(L);
    openMath(L);
    openIO(L);
    openString(L);
}

} // namespace Lua
