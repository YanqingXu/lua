#include "lib/lib_manager.hpp"

#include "lib/baselib.hpp"
#include "lib/mathlib.hpp"
#include "lib/iolib.hpp"
#include "lib/stringlib.hpp"
#include "lib/tablelib.hpp"
#include "lib/oslib.hpp"
#include "lib/coroutinelib.hpp"
#include "lib/debuglib.hpp"
#include "lib/packagelib.hpp"
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

void StandardLibrary::openTable(LuaState* L) {
    if (!L) {
        return;
    }

    openTableLib(L);
}

void StandardLibrary::openOS(LuaState* L) {
    if (!L) {
        return;
    }

    openOSLib(L);
}

void StandardLibrary::openCoroutine(LuaState* L) {
    if (!L) {
        return;
    }

    openCoroutineLib(L);
}

void StandardLibrary::openDebug(LuaState* L) {
    if (!L) {
        return;
    }

    openDebugLib(L);
}

void StandardLibrary::openAll(LuaState* L) {
    if (!L) {
        return;
    }

    openBase(L);
    openMath(L);
    openIO(L);
    openString(L);
    openTable(L);
    openOS(L);
    openCoroutine(L);
    openDebug(L);
    openPackage(L);
}

void StandardLibrary::openPackage(LuaState* L) {
    if (!L) {
        return;
    }

    openPackageLib(L);
}

} // namespace Lua
