#include "../framework/test_framework.hpp"

#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "lib/lib_catalog.hpp"
#include "lib/lib_manager.hpp"
#include "vm/state/lua_state.hpp"

#include <algorithm>
#include <array>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Standard Library Catalog";

Value getField(LuaState* L, Table* table, const char* key) {
    GCString* field = L->getGlobalState().getStringPool().intern(key);
    return table->get(Value(field));
}

void assertGlobalFunction(TestSuite& suite, LuaState* L, const char* name) {
    ASSERT_TRUE(suite, L->getGlobal(name).isFunction(), name);
}

void assertGlobalTable(TestSuite& suite, LuaState* L, const char* name) {
    ASSERT_TRUE(suite, L->getGlobal(name).isTable(), name);
}

void assertTableFunction(TestSuite& suite, LuaState* L, const char* tableName, const char* functionName) {
    Value tableValue = L->getGlobal(tableName);
    ASSERT_TRUE(suite, tableValue.isTable(), tableName);
    if (!tableValue.isTable()) {
        return;
    }

    Value functionValue = getField(L, tableValue.asTable(), functionName);
    ASSERT_TRUE(suite, functionValue.isFunction(), functionName);
}

void testCatalogOrder(TestSuite& suite) {
    const auto catalog = getStandardLibraryCatalog();
    constexpr std::array<const char*, 9> expectedIds = {
        "base",
        "math",
        "io",
        "string",
        "table",
        "os",
        "coroutine",
        "debug",
        "package",
    };

    ASSERT_EQ(suite, expectedIds.size(), catalog.size(), "catalog has expected library count");

    const usize count = std::min(expectedIds.size(), catalog.size());
    for (usize index = 0; index < count; ++index) {
        ASSERT_TRUE(suite, StrView(catalog[index].id) == expectedIds[index], expectedIds[index]);
        ASSERT_TRUE(suite, catalog[index].name != nullptr && catalog[index].name[0] != '\0', expectedIds[index]);
        ASSERT_TRUE(suite, catalog[index].open != nullptr, expectedIds[index]);
    }
}

void testOpenAllRegistersCatalogLibraries(TestSuite& suite) {
    LuaStdLibTestContext ctx(StandardLibrary::openAll);
    LuaState* L = ctx.getState();

    assertGlobalFunction(suite, L, "print");
    assertGlobalFunction(suite, L, "type");
    assertGlobalFunction(suite, L, "require");
    assertGlobalFunction(suite, L, "module");

    assertGlobalTable(suite, L, "math");
    assertGlobalTable(suite, L, "io");
    assertGlobalTable(suite, L, "string");
    assertGlobalTable(suite, L, "table");
    assertGlobalTable(suite, L, "os");
    assertGlobalTable(suite, L, "coroutine");
    assertGlobalTable(suite, L, "debug");
    assertGlobalTable(suite, L, "package");

    assertTableFunction(suite, L, "math", "sin");
    assertTableFunction(suite, L, "string", "len");
    assertTableFunction(suite, L, "table", "insert");
    assertTableFunction(suite, L, "io", "open");
    assertTableFunction(suite, L, "os", "date");
    assertTableFunction(suite, L, "coroutine", "create");
    assertTableFunction(suite, L, "debug", "getinfo");

    Value packageValue = L->getGlobal("package");
    ASSERT_TRUE(suite, packageValue.isTable(), "package table exists");
    if (!packageValue.isTable()) {
        return;
    }

    Value loadedValue = getField(L, packageValue.asTable(), "loaded");
    ASSERT_TRUE(suite, loadedValue.isTable(), "package.loaded table exists");
    if (!loadedValue.isTable()) {
        return;
    }

    ASSERT_TRUE(suite, getField(L, loadedValue.asTable(), "math").isTable(), "package.loaded has math");
    ASSERT_TRUE(suite, getField(L, loadedValue.asTable(), "string").isTable(), "package.loaded has string");
    ASSERT_TRUE(suite, getField(L, loadedValue.asTable(), "table").isTable(), "package.loaded has table");
}

} // namespace

void registerLibCatalogTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "catalog order", testCatalogOrder);
    registry.registerTest(kSuiteName, "openAll registrations", testOpenAllRegistersCatalogLibraries);
}
