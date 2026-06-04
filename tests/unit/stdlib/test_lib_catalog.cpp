#include "../framework/test_framework.hpp"

#include "core/gc_string.hpp"
#include "core/function.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "compiler/opcode.hpp"
#include "lib/lib_catalog.hpp"
#include "lib/lib_manager.hpp"
#include "lib/lib_registry.hpp"
#include "lib/testlib.hpp"
#include "vm/state/lua_state.hpp"

#include "lua.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <expected>
#include <functional>
#include <string>
#include <type_traits>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Standard Library Catalog";

static_assert(std::is_same_v<
                  decltype(findStandardLibrary(StrView{})),
                  Opt<std::reference_wrapper<const LibCatalogEntry>>>,
              "findStandardLibrary should expose absence as Opt<reference_wrapper<...>>, not a nullable pointer");

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

void openMathByCatalog(LuaState* L) {
    StandardLibrary::openCatalogLibrary(L, "math");
}

static i32 catalogDummyFunction(LuaState*) {
    return 0;
}

void testCatalogOrder(TestSuite& suite) {
    const auto catalog = getStandardLibraryCatalog();
    constexpr std::array<StrView, 9> expectedIds = {
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
        ASSERT_TRUE(suite, catalog[index].id == expectedIds[index], Str(expectedIds[index]));
        ASSERT_TRUE(suite, !catalog[index].name.empty(), Str(expectedIds[index]));
        ASSERT_TRUE(suite, catalog[index].open != nullptr, Str(expectedIds[index]));
    }
}

void testCatalogIdsAreUnique(TestSuite& suite) {
    const auto catalog = getStandardLibraryCatalog();
    bool unique = true;

    for (usize left = 0; left < catalog.size(); ++left) {
        for (usize right = left + 1; right < catalog.size(); ++right) {
            if (catalog[left].id == catalog[right].id) {
                unique = false;
            }
        }
    }

    ASSERT_TRUE(suite, unique, "catalog ids are unique");
}

void testFindStandardLibraryReturnsOptionalReference(TestSuite& suite) {
    auto math = findStandardLibrary("math");
    ASSERT_TRUE(suite, math.has_value(), "known library id should resolve");
    if (math) {
        ASSERT_TRUE(suite, math->get().id == "math", "lookup returns the math catalog row");
    }

    auto missing = findStandardLibrary("definitely_missing");
    ASSERT_TRUE(suite, !missing.has_value(), "unknown library id should return nullopt");
}

void testFunctionRegistrarExpectedErrors(TestSuite& suite) {
    static_assert(std::is_same_v<
                      decltype(FunctionRegistrar::tryCreateLibTable(nullptr, StrView{})),
                      std::expected<Table*, LibRegistrationError>>,
                  "FunctionRegistrar tryCreateLibTable should return expected<Table*, LibRegistrationError>");

    auto missingState = FunctionRegistrar::tryCreateLibTable(nullptr, "math");
    ASSERT_TRUE(suite, !missingState.has_value(), "missing state should be an expected error");
    if (!missingState) {
        ASSERT_EQ(suite, static_cast<int>(LibRegistrationErrorCode::NullState),
                  static_cast<int>(missingState.error().code), "missing state reports NullState");
    }

    UPtr<LuaState> L = LuaState::create();
    auto missingName = FunctionRegistrar::tryCreateLibTable(L.get(), StrView{});
    ASSERT_TRUE(suite, !missingName.has_value(), "missing library name should be an expected error");
    if (!missingName) {
        ASSERT_EQ(suite, static_cast<int>(LibRegistrationErrorCode::NullName),
                  static_cast<int>(missingName.error().code), "missing name reports NullName");
    }

    auto missingFunction = FunctionRegistrar::tryCreateClosure(L.get(), nullptr);
    ASSERT_TRUE(suite, !missingFunction.has_value(), "missing C function should be an expected error");
    if (!missingFunction) {
        ASSERT_EQ(suite, static_cast<int>(LibRegistrationErrorCode::NullFunction),
                  static_cast<int>(missingFunction.error().code), "missing C function reports NullFunction");
    }

    auto closure = FunctionRegistrar::tryCreateClosure(L.get(), catalogDummyFunction);
    ASSERT_TRUE(suite, closure.has_value() && *closure != nullptr, "valid C function creates a closure");

    L.reset();
    GlobalState::getInstance().getGC().clearAll();
}

void testOpenCatalogLibraryRegistersSingleLibrary(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathByCatalog);
    LuaState* L = ctx.getState();

    assertGlobalTable(suite, L, "math");
    assertTableFunction(suite, L, "math", "sin");
    ASSERT_TRUE(suite, L->getGlobal("string").isNil(), "catalog single open leaves string unopened");

    StandardLibrary::openCatalogLibrary(L, "missing");
    ASSERT_TRUE(suite, L->getGlobal("missing").isNil(), "unknown catalog id is ignored");
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

void testTestLibListcodeProvidesOfficialShape(TestSuite& suite) {
    UPtr<LuaState> L = LuaState::create();
    openTestLib(L.get());

    Value tableValue = L->getGlobal("T");
    ASSERT_TRUE(suite, tableValue.isTable(), "T helper table is registered explicitly");
    if (!tableValue.isTable()) {
        return;
    }

    Value listcodeValue = getField(L.get(), tableValue.asTable(), "listcode");
    ASSERT_TRUE(suite, listcodeValue.isFunction(), "T.listcode is a C function");
    if (!listcodeValue.isFunction()) {
        return;
    }

    L->setTop(0);

    Proto* proto = L->getGlobalState().getGC().create<Proto>();
    proto->setMaxStackSize(2);
    proto->setNumParams(1);
    proto->addInstruction(CREATE_ABC(OpCode::RETURN, 0, 2, 0));

    Function* function = L->getGlobalState().getGC().create<Function>(proto);
    function->setEnv(L->getGlobalTable());
    L->pushFunction(function);

    i32 results = listcodeValue.asFunction()->getCFunction()(L.get());
    ASSERT_EQ(suite, 1, results, "T.listcode returns one result table");
    ASSERT_TRUE(suite, L->at(-1).isTable(), "T.listcode result is a table");
    if (!L->at(-1).isTable()) {
        return;
    }

    Table* result = L->at(-1).asTable();
    ASSERT_EQ(suite, 2.0, getField(L.get(), result, "maxstack").asNumber(), "T.listcode exposes maxstack");
    ASSERT_EQ(suite, 1.0, getField(L.get(), result, "numparams").asNumber(), "T.listcode exposes numparams");

    Value firstLine = result->getArray(1);
    ASSERT_TRUE(suite, firstLine.isString(), "T.listcode stores instruction lines in the array part");
    if (firstLine.isString()) {
        std::string text(firstLine.asString()->c_str());
        ASSERT_TRUE(suite, text.find("- RETURN") != std::string::npos,
                    "T.listcode lines include official-style opcode text");
    }
}

void testTestLibTestCFirstStageStackCommands(TestSuite& suite) {
    UPtr<LuaState> L = LuaState::create();
    openTestLib(L.get());

    Value tableValue = L->getGlobal("T");
    ASSERT_TRUE(suite, tableValue.isTable(), "T helper table is registered explicitly");
    if (!tableValue.isTable()) {
        return;
    }

    Value testCValue = getField(L.get(), tableValue.asTable(), "testC");
    ASSERT_TRUE(suite, testCValue.isFunction(), "T.testC is a C function");
    if (!testCValue.isFunction()) {
        return;
    }

    L->setTop(0);

    L->pushString(L->getGlobalState().getStringPool().intern(
        "pushnum 1; pushnum 2; pushnum 3; settop 3; return 2"));
    i32 results = testCValue.asFunction()->getCFunction()(L.get());

    ASSERT_EQ(suite, 2, results, "T.testC returns requested top values");
    ASSERT_TRUE(suite, L->at(-2).isNumber(), "T.testC first returned value is numeric");
    ASSERT_TRUE(suite, L->at(-1).isNumber(), "T.testC second returned value is numeric");
    if (L->at(-2).isNumber() && L->at(-1).isNumber()) {
        ASSERT_EQ(suite, 1.0, L->at(-2).asNumber(), "T.testC preserves Lua stack semantics before return");
        ASSERT_EQ(suite, 2.0, L->at(-1).asNumber(), "T.testC preserves Lua stack semantics at top");
    }
}

void testLua51CAPIShimStackAndRawTableSmoke(TestSuite& suite) {
    lua_State* C = lua_open();
    ASSERT_TRUE(suite, C != nullptr, "lua_open creates a C API state");
    if (C == nullptr) {
        return;
    }

    const int initialTop = lua_gettop(C);
    lua_pushnumber(C, 10.0);
    lua_pushlstring(C, "a\0b", 3);

    size_t length = 0;
    const char* text = lua_tolstring(C, -1, &length);
    ASSERT_TRUE(suite, text != nullptr, "lua_tolstring returns string storage");
    ASSERT_EQ(suite, static_cast<size_t>(3), length, "lua_tolstring preserves binary string length");
    ASSERT_TRUE(suite, text != nullptr && std::memcmp(text, "a\0b", 3) == 0,
                "lua_pushlstring preserves embedded NUL bytes");

    lua_createtable(C, 0, 0);
    lua_pushnumber(C, 42.0);
    lua_rawseti(C, -2, 1);
    lua_rawgeti(C, -1, 1);

    ASSERT_EQ(suite, initialTop + 4, lua_gettop(C), "C API stack keeps raw table result on top");
    ASSERT_EQ(suite, 42.0, lua_tonumber(C, -1), "lua_rawgeti reads the raw array slot");

    lua_close(C);
}

} // namespace

void registerLibCatalogTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "catalog order", testCatalogOrder);
    registry.registerTest(kSuiteName, "catalog ids are unique", testCatalogIdsAreUnique);
    registry.registerTest(kSuiteName, "catalog lookup optional reference", testFindStandardLibraryReturnsOptionalReference);
    registry.registerTest(kSuiteName, "function registrar expected errors", testFunctionRegistrarExpectedErrors);
    registry.registerTest(kSuiteName, "openCatalogLibrary single library", testOpenCatalogLibraryRegistersSingleLibrary);
    registry.registerTest(kSuiteName, "openAll registrations", testOpenAllRegistersCatalogLibraries);
    registry.registerTest(kSuiteName, "T.listcode official shape", testTestLibListcodeProvidesOfficialShape);
    registry.registerTest(kSuiteName, "T.testC first-stage stack commands", testTestLibTestCFirstStageStackCommands);
    registry.registerTest(kSuiteName, "Lua 5.1 C API shim stack and raw table smoke",
                          testLua51CAPIShimStackAndRawTableSmoke);
}
