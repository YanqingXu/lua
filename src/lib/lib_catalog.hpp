#pragma once

#include "common/types.hpp"

#include <span>

namespace Lua {

class LuaState;

using LibOpenFunction = void (*)(LuaState*);

struct LibCatalogEntry {
    const char* id;
    const char* name;
    LibOpenFunction open;
};

std::span<const LibCatalogEntry> getStandardLibraryCatalog();

const LibCatalogEntry* findStandardLibrary(StrView id);

} // namespace Lua
