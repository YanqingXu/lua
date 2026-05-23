#pragma once

#include "lib/lib_module.hpp"

namespace Lua {

class StandardLibrary {
public:
    static void openAll(LuaState* L);

    static void openCatalogLibrary(LuaState* L, StrView id);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"base\") instead.")]]
    static void openBase(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"math\") instead.")]]
    static void openMath(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"io\") instead.")]]
    static void openIO(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"string\") instead.")]]
    static void openString(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"table\") instead.")]]
    static void openTable(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"os\") instead.")]]
    static void openOS(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"coroutine\") instead.")]]
    static void openCoroutine(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"debug\") instead.")]]
    static void openDebug(LuaState* L);

    [[deprecated("Use StandardLibrary::openCatalogLibrary(L, \"package\") instead.")]]
    static void openPackage(LuaState* L);

    static void openModule(LuaState* L, LibModule& module);
};

} // namespace Lua
