#include "lib/lib_manager.hpp"

#include "lib/baselib.hpp"
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

void StandardLibrary::openAll(LuaState* L) {
    if (!L) {
        return;
    }

    openBase(L);
}

} // namespace Lua
