#include "lib_manager.hpp"
#include "lib/base/base_lib.hpp"
#include "lib/string/string_lib.hpp"
#include "lib/math/math_lib.hpp"
#include "lib/table/table_lib.hpp"
#include "lib/io/io_lib.hpp"
#include "lib/os/os_lib.hpp"
#include "lib/debug/debug_lib.hpp"
#include "lib/package/package_lib.hpp"
#include "../../common/defines.hpp"
#include <iostream>

namespace Lua {

// ===================================================================
// StandardLibrary Implementation
// ===================================================================

void StandardLibrary::initializeAll(State* state) {
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

void StandardLibrary::initializeBase(State* state) {
    if (!state) {
        return;
    }
    initializeBaseLib(state);
}

void StandardLibrary::initializeString(State* state) {
    if (!state) {
        return;
    }
    initializeStringLib(state);
}

void StandardLibrary::initializeMath(State* state) {
    if (!state) {
        return;
    }
    initializeMathLib(state);
}

void StandardLibrary::initializeTable(State* state) {
    if (!state) {
        return;
    }
    initializeTableLib(state);
}

void StandardLibrary::initializeIO(State* state) {
    if (!state) {
        return;
    }
    initializeIOLib(state);
}

void StandardLibrary::initializeOS(State* state) {
    if (!state) {
        return;
    }
    initializeOSLib(state);
}

void StandardLibrary::initializeDebug(State* state) {
    if (!state) {
        return;
    }
    initializeDebugLib(state);
}

void StandardLibrary::initializePackage(State* state) {
    if (!state) {
        return;
    }
    initializePackageLib(state);
}

} // namespace Lua