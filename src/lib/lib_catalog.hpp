#pragma once

#include "common/types.hpp"

#include <functional>
#include <span>

namespace Lua {

class LuaState;

using LibOpenFunction = void (*)(LuaState*);

struct LibCatalogEntry {
    StrView id;
    StrView name;
    LibOpenFunction open;
};

std::span<const LibCatalogEntry> getStandardLibraryCatalog();

Opt<std::reference_wrapper<const LibCatalogEntry>> findStandardLibrary(StrView id);

} // namespace Lua
