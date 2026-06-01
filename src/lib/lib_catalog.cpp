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

#include <array>
#include <functional>
#include <ranges>

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

Opt<std::reference_wrapper<const LibCatalogEntry>> findStandardLibrary(StrView id) {
    const auto catalog = getStandardLibraryCatalog();
    const auto iter = std::ranges::find_if(catalog, [id](const LibCatalogEntry& entry) {
        return entry.id == id;
    });

    if (iter == catalog.end()) {
        return std::nullopt;
    }
    return std::cref(*iter);
}

} // namespace Lua
