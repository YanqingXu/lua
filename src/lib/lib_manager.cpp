#include "lib/lib_manager.hpp"

#include "lib/lib_catalog.hpp"
#include "vm/state/lua_state.hpp"

namespace Lua {

namespace {

void openCatalogEntry(LuaState* L, const LibCatalogEntry& entry) {
    if (!L || !entry.open) {
        return;
    }

    entry.open(L);
}

} // namespace

void StandardLibrary::openModule(LuaState* L, LibModule& module) {
    if (!L) {
        return;
    }

    module.registerFunctions(L);
    module.initialize(L);
}

void StandardLibrary::openCatalogLibrary(LuaState* L, StrView id) {
    auto entry = findStandardLibrary(id);
    if (!entry) {
        return;
    }

    openCatalogEntry(L, entry->get());
}

void StandardLibrary::openBase(LuaState* L) {
    openCatalogLibrary(L, "base");
}

void StandardLibrary::openMath(LuaState* L) {
    openCatalogLibrary(L, "math");
}

void StandardLibrary::openIO(LuaState* L) {
    openCatalogLibrary(L, "io");
}

void StandardLibrary::openString(LuaState* L) {
    openCatalogLibrary(L, "string");
}

void StandardLibrary::openTable(LuaState* L) {
    openCatalogLibrary(L, "table");
}

void StandardLibrary::openOS(LuaState* L) {
    openCatalogLibrary(L, "os");
}

void StandardLibrary::openCoroutine(LuaState* L) {
    openCatalogLibrary(L, "coroutine");
}

void StandardLibrary::openDebug(LuaState* L) {
    openCatalogLibrary(L, "debug");
}

void StandardLibrary::openAll(LuaState* L) {
    if (!L) {
        return;
    }

    for (const LibCatalogEntry& entry : getStandardLibraryCatalog()) {
        openCatalogEntry(L, entry);
    }
}

void StandardLibrary::openPackage(LuaState* L) {
    openCatalogLibrary(L, "package");
}

} // namespace Lua
