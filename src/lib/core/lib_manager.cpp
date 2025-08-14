#include "lib_manager.hpp"
#include "lib/base/base_lib.hpp"
#include "lib/string/string_lib.hpp"
#include "lib/math/math_lib.hpp"
#include "lib/table/table_lib.hpp"
#include "lib/io/io_lib.hpp"
#include "lib/os/os_lib.hpp"
#include "lib/debug/debug_lib.hpp"
#include "lib/package/package_lib.hpp"
#include "../../vm/lua_state.hpp"
#include "../../common/defines.hpp"
#include <iostream>

namespace Lua {

// ===================================================================
// StandardLibrary Implementation
// ===================================================================

void StandardLibrary::initializeAll(LuaState* state) {
    if (!state) {
        return;
    }

    initializeBase(state);
    initializeString(state);
    initializeMath(state);
    initializeTable(state);
    initializeIO(state);
    initializeOS(state);
    initializeDebug(state);
    initializePackage(state);
}

void StandardLibrary::initializeBase(LuaState* state) {
    if (!state) {
        return;
    }
    initializeBaseLib(state);
}

void StandardLibrary::initializeString(LuaState* state) {
    if (!state) {
        return;
    }
    initializeStringLib(state);
}

void StandardLibrary::initializeMath(LuaState* state) {
    if (!state) {
        return;
    }
    initializeMathLib(state);
}

void StandardLibrary::initializeTable(LuaState* state) {
    if (!state) {
        return;
    }
    initializeTableLib(state);
}

void StandardLibrary::initializeIO(LuaState* state) {
    if (!state) {
        return;
    }
    initializeIOLib(state);
}

void StandardLibrary::initializeOS(LuaState* state) {
    if (!state) {
        return;
    }
    initializeOSLib(state);
}

void StandardLibrary::initializeDebug(LuaState* state) {
    if (!state) {
        return;
    }
    initializeDebugLib(state);
}

void StandardLibrary::initializePackage(LuaState* state) {
    if (!state) {
        return;
    }
    initializePackageLib(state);
}

} // namespace Lua
