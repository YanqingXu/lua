#include "lib/lib_catalog.hpp"

#include "lib/baselib.hpp"
#include "lib/coroutinelib.hpp"
#include "lib/debuglib.hpp"
#include "lib/iolib.hpp"
#include "lib/mathlib.hpp"
#include "lib/oslib.hpp"
#include "lib/packagelib.hpp"
#include "lib/stringlib.hpp"
#include "lib/tablelib.hpp"

#include <algorithm>
#include <array>

namespace Lua {

namespace {

constexpr std::array<LibCatalogEntry, 9> kStandardLibraryCatalog = {
    {
        { "base", "Base Library", openBaseLib },
        { "math", "Math Library", openMathLib },
        { "io", "IO Library", openIOLib },
        { "string", "String Library", openStringLib },
        { "table", "Table Library", openTableLib },
        { "os", "OS Library", openOSLib },
        { "coroutine", "Coroutine Library", openCoroutineLib },
        { "debug", "Debug Library", openDebugLib },
        { "package", "Package Library", openPackageLib },
    },
};

} // namespace

std::span<const LibCatalogEntry> getStandardLibraryCatalog() {
    return std::span<const LibCatalogEntry>(kStandardLibraryCatalog.data(), kStandardLibraryCatalog.size());
}

const LibCatalogEntry* findStandardLibrary(StrView id) {
    const auto catalog = getStandardLibraryCatalog();
    const auto iter = std::find_if(catalog.begin(), catalog.end(), [id](const LibCatalogEntry& entry) {
        return StrView(entry.id) == id;
    });

    return iter == catalog.end() ? nullptr : &(*iter);
}

} // namespace Lua
