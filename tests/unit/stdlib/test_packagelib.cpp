/**
 * @file test_packagelib.cpp
 * @brief Unit tests for the Lua package/module library
 *
 * Tests cover:
 * - Package table registration and structure
 * - require() with package.preload
 * - require() caching via package.loaded
 * - require() file loading via package.path
 * - module() function
 * - package.seeall()
 * - package.loadlib()
 * - package.config, package.path, package.cpath
 * - Error handling for missing modules
 *
 * @author Lua C++ Project
 * @date 2026-04-10
 */

#include "../framework/test_framework.hpp"

#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "lib/baselib.hpp"
#include "lib/packagelib.hpp"
#include "lib/lib_manager.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <fstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

using namespace Lua;
using namespace LuaTest;

namespace {

Table* createNativeModuleTable(LuaState* L, const char* kind, f64 value) {
    auto& pool = L->getGlobalState().getStringPool();

    Table* table = new Table();
    L->getGlobalState().getGC().registerObject(table);

    table->set(Value(pool.intern("kind")), Value(pool.intern(kind)));
    table->set(Value(pool.intern("value")), Value(value));
    return table;
}

} // namespace

#if defined(_WIN32)
#define LUA_PACKAGE_TEST_EXPORT extern "C" __declspec(dllexport)
#else
#define LUA_PACKAGE_TEST_EXPORT extern "C" __attribute__((visibility("default")))
#endif

LUA_PACKAGE_TEST_EXPORT int luaopen_loadlibfixture(LuaState* L) {
    L->setTop(0);
    L->pushValue(Value(createNativeModuleTable(L, "loadlib", 701.0)));
    return 1;
}

LUA_PACKAGE_TEST_EXPORT int luaopen_pkgdirect(LuaState* L) {
    L->setTop(0);
    L->pushValue(Value(createNativeModuleTable(L, "direct", 702.0)));
    return 1;
}

LUA_PACKAGE_TEST_EXPORT int luaopen_pkgroot_child(LuaState* L) {
    L->setTop(0);
    L->pushValue(Value(createNativeModuleTable(L, "allinone", 703.0)));
    return 1;
}

namespace {

constexpr const char* kSuiteName = "Package Library";

// Helper: open both base library and package library
void openBaseAndPackage(LuaState* L) {
    StandardLibrary::openCatalogLibrary(L, "base");
    StandardLibrary::openCatalogLibrary(L, "package");
}

// Helper: open all standard libraries
void openAllLibs(LuaState* L) {
    StandardLibrary::openAll(L);
}

// Helper: get a table field
Value getField(LuaState* L, Table* table, const char* key) {
    GCString* k = L->getGlobalState().getStringPool().intern(key);
    return table->get(Value(k));
}

// Helper: run a Lua chunk and return success
bool runLuaChunk(LuaState* L, const char* source, const char* chunkName = "test") {
    try {
        Parser parser(source);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, chunkName);
        if (!proto) return false;

        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        func->setEnv(L->getGlobalTable());
        VM::execute(L, func);
        return true;
    } catch (...) {
        return false;
    }
}

// Helper: write a Lua file to disk for testing require()
bool writeLuaFile(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    file << content;
    file.close();
    return true;
}

// Helper: delete a file
void deleteFile(const std::string& path) {
    std::remove(path.c_str());
}

std::string currentExecutablePath() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, static_cast<DWORD>(sizeof(buffer)));
    if (len == 0 || len >= sizeof(buffer)) {
        return "";
    }
    return std::string(buffer, len);
#else
    char buffer[4096];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        return "";
    }
    buffer[len] = '\0';
    return std::string(buffer);
#endif
}

bool setPackageTestExecutablePath(LuaState* L) {
    std::string exePath = currentExecutablePath();
    if (exePath.empty()) {
        return false;
    }

    GCString* exe = L->getGlobalState().getStringPool().intern(exePath.c_str());
    L->setGlobal("package_test_exe", Value(exe));
    return true;
}

// =====================================================================
// Test: Package table structure
// =====================================================================

void testPackageTableRegistration(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    // package table exists
    Value pkgVal = L->getGlobal("package");
    ASSERT_TRUE(suite, pkgVal.isTable(), "package table exists");
    if (!pkgVal.isTable()) return;

    Table* pkg = pkgVal.asTable();

    // Sub-tables exist
    ASSERT_TRUE(suite, getField(L, pkg, "loaded").isTable(), "package.loaded exists");
    ASSERT_TRUE(suite, getField(L, pkg, "preload").isTable(), "package.preload exists");
    ASSERT_TRUE(suite, getField(L, pkg, "loaders").isTable(), "package.loaders exists");

    // String fields exist
    ASSERT_TRUE(suite, getField(L, pkg, "path").isString(), "package.path exists");
    ASSERT_TRUE(suite, getField(L, pkg, "cpath").isString(), "package.cpath exists");
    ASSERT_TRUE(suite, getField(L, pkg, "config").isString(), "package.config exists");

    // Functions exist
    ASSERT_TRUE(suite, getField(L, pkg, "loadlib").isFunction(), "package.loadlib exists");
    ASSERT_TRUE(suite, getField(L, pkg, "seeall").isFunction(), "package.seeall exists");
}

// =====================================================================
// Test: Global functions require and module exist
// =====================================================================

void testGlobalFunctionsExist(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    Value requireVal = L->getGlobal("require");
    ASSERT_TRUE(suite, requireVal.isFunction(), "require function exists");

    Value moduleVal = L->getGlobal("module");
    ASSERT_TRUE(suite, moduleVal.isFunction(), "module function exists");
}

// =====================================================================
// Test: package.config format
// =====================================================================

void testPackageConfig(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    Value pkgVal = L->getGlobal("package");
    Table* pkg = pkgVal.asTable();

    Value configVal = getField(L, pkg, "config");
    ASSERT_TRUE(suite, configVal.isString(), "package.config is string");

    std::string config = configVal.asString()->c_str();
    // Config should contain path separator info
    ASSERT_TRUE(suite, !config.empty(), "package.config is not empty");

    // On Windows, first char should be backslash
#ifdef _WIN32
    ASSERT_TRUE(suite, config[0] == '\\', "package.config starts with backslash on Windows");
#else
    ASSERT_TRUE(suite, config[0] == '/', "package.config starts with slash on Unix");
#endif
}

// =====================================================================
// Test: package.loaders has the right number of entries
// =====================================================================

void testPackageLoaders(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    Value pkgVal = L->getGlobal("package");
    Table* pkg = pkgVal.asTable();
    Table* loaders = getField(L, pkg, "loaders").asTable();

    // Should have 4 loaders: preload, lua, clib, all-in-one clib
    Value l1 = loaders->get(Value(1.0));
    Value l2 = loaders->get(Value(2.0));
    Value l3 = loaders->get(Value(3.0));
    Value l4 = loaders->get(Value(4.0));
    Value l5 = loaders->get(Value(5.0));

    ASSERT_TRUE(suite, l1.isFunction(), "loader[1] is function (preload)");
    ASSERT_TRUE(suite, l2.isFunction(), "loader[2] is function (lua)");
    ASSERT_TRUE(suite, l3.isFunction(), "loader[3] is function (clib)");
    ASSERT_TRUE(suite, l4.isFunction(), "loader[4] is function (all-in-one clib)");
    ASSERT_TRUE(suite, l5.isNil(), "loader[5] is nil (no more loaders)");
}

// =====================================================================
// Test: require() with package.preload
// =====================================================================

void testRequirePreload(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    auto& pool = L->getGlobalState().getStringPool();
    auto& gc = L->getGlobalState().getGC();

    // Register a preload function for "mymod"
    Value pkgVal = L->getGlobal("package");
    Table* pkg = pkgVal.asTable();
    Table* preload = getField(L, pkg, "preload").asTable();

    // Create a C function that returns a table with field "value" = 42
    auto modLoader = [](LuaState* L) -> i32 {
        Table* modTable = new Table();
        L->getGlobalState().getGC().registerObject(modTable);

        GCString* key = L->getGlobalState().getStringPool().intern("value");
        modTable->set(Value(key), Value(42.0));

        L->setTop(0);
        L->pushValue(Value(modTable));
        return 1;
    };

    Function* loaderFunc = new Function(static_cast<LibCFunction>(modLoader));
    gc.registerObject(loaderFunc);

    GCString* modKey = pool.intern("mymod");
    preload->set(Value(modKey), Value(loaderFunc));

    // Now require("mymod")
    bool ok = runLuaChunk(L, "result = require('mymod')");
    ASSERT_TRUE(suite, ok, "require preloaded module succeeds");

    Value resultVal = L->getGlobal("result");
    ASSERT_TRUE(suite, resultVal.isTable(), "preloaded module returns table");

    if (resultVal.isTable()) {
        Value val = getField(L, resultVal.asTable(), "value");
        ASSERT_TRUE(suite, val.isNumber(), "module table has value field");
        ASSERT_TRUE(suite, val.asNumber() == 42.0, "module value equals 42");
    }
}

// =====================================================================
// Test: require() caching — second call returns cached value
// =====================================================================

void testRequireCaching(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    auto& pool = L->getGlobalState().getStringPool();
    auto& gc = L->getGlobalState().getGC();

    // Set up a counter that increments on each call
    static int callCount = 0;
    callCount = 0;

    auto modLoader = [](LuaState* L) -> i32 {
        callCount++;
        Table* modTable = new Table();
        L->getGlobalState().getGC().registerObject(modTable);

        GCString* key = L->getGlobalState().getStringPool().intern("calls");
        modTable->set(Value(key), Value(static_cast<f64>(callCount)));

        L->setTop(0);
        L->pushValue(Value(modTable));
        return 1;
    };

    Value pkgVal = L->getGlobal("package");
    Table* preload = getField(L, pkgVal.asTable(), "preload").asTable();

    Function* loaderFunc = new Function(static_cast<LibCFunction>(modLoader));
    gc.registerObject(loaderFunc);
    GCString* modKey = pool.intern("countmod");
    preload->set(Value(modKey), Value(loaderFunc));

    // First require
    runLuaChunk(L, "r1 = require('countmod')");
    ASSERT_EQ(suite, 1, callCount, "loader called once on first require");

    // Second require — should use cache
    runLuaChunk(L, "r2 = require('countmod')");
    ASSERT_EQ(suite, 1, callCount, "loader NOT called again on second require");

    // Both results should be the same table
    Value r1 = L->getGlobal("r1");
    Value r2 = L->getGlobal("r2");
    ASSERT_TRUE(suite, r1.isTable() && r2.isTable(), "both results are tables");
    ASSERT_TRUE(suite, r1.asTable() == r2.asTable(), "cached result is same object");
}

// =====================================================================
// Test: require() does not treat package.loaded[name] = false as cached
// =====================================================================

void testRequireFalseResultReloads(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    auto& pool = L->getGlobalState().getStringPool();
    auto& gc = L->getGlobalState().getGC();

    static int falseCallCount = 0;
    falseCallCount = 0;

    auto falseLoader = [](LuaState* L) -> i32 {
        falseCallCount++;
        L->setTop(0);
        L->pushBoolean(false);
        return 1;
    };

    Value pkgVal = L->getGlobal("package");
    Table* preload = getField(L, pkgVal.asTable(), "preload").asTable();

    Function* loaderFunc = new Function(static_cast<LibCFunction>(falseLoader));
    gc.registerObject(loaderFunc);
    GCString* modKey = pool.intern("falsemod");
    preload->set(Value(modKey), Value(loaderFunc));

    bool ok = runLuaChunk(L, "r1 = require('falsemod'); r2 = require('falsemod')");
    ASSERT_TRUE(suite, ok, "require false-valued module runs");
    ASSERT_EQ(suite, 2, falseCallCount, "false package.loaded entry does not cache require");

    Value r1 = L->getGlobal("r1");
    Value r2 = L->getGlobal("r2");
    ASSERT_TRUE(suite, r1.isBoolean() && !r1.asBoolean(), "first require returns false");
    ASSERT_TRUE(suite, r2.isBoolean() && !r2.asBoolean(), "second require returns false");
}

// =====================================================================
// Test: require() error for missing module
// =====================================================================

void testRequireMissingModule(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    // require a module that doesn't exist — should error
    bool ok = runLuaChunk(L, "require('nonexistent_module_xyz')");
    ASSERT_FALSE(suite, ok, "require missing module throws error");
}

// =====================================================================
// Test: require() loads from file via package.path
// =====================================================================

void testRequireFromFile(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    auto& pool = L->getGlobalState().getStringPool();

    // Write a test module file
    std::string filename = "test_require_mod_tmp.lua";
    std::string content = "local M = {}\nM.greeting = \"hello from file\"\nreturn M\n";

    bool written = writeLuaFile(filename, content);
    ASSERT_TRUE(suite, written, "test module file written");

    if (written) {
        // Set package.path to current directory
        Value pkgVal = L->getGlobal("package");
        Table* pkg = pkgVal.asTable();
        GCString* pathKey = pool.intern("path");
        GCString* pathVal = pool.intern(".\\?.lua");
        pkg->set(Value(pathKey), Value(pathVal));

        // require the module
        bool ok = runLuaChunk(L, "filemod = require('test_require_mod_tmp')");
        ASSERT_TRUE(suite, ok, "require from file succeeds");

        Value resultVal = L->getGlobal("filemod");
        ASSERT_TRUE(suite, resultVal.isTable(), "file module returns table");

        if (resultVal.isTable()) {
            Value greet = getField(L, resultVal.asTable(), "greeting");
            ASSERT_TRUE(suite, greet.isString(), "module has greeting field");
            if (greet.isString()) {
                std::string s = greet.asString()->c_str();
                ASSERT_TRUE(suite, s == "hello from file", "greeting value correct");
            }
        }

        deleteFile(filename);
    }
}

// =====================================================================
// Test: package.loaded direct manipulation
// =====================================================================

void testPackageLoadedDirectSet(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    auto& pool = L->getGlobalState().getStringPool();
    auto& gc = L->getGlobalState().getGC();

    // Manually set package.loaded["fakemod"] = { x = 99 }
    Value pkgVal = L->getGlobal("package");
    Table* loaded = getField(L, pkgVal.asTable(), "loaded").asTable();

    Table* fakeTable = new Table();
    gc.registerObject(fakeTable);
    GCString* xKey = pool.intern("x");
    fakeTable->set(Value(xKey), Value(99.0));

    GCString* modKey = pool.intern("fakemod");
    loaded->set(Value(modKey), Value(fakeTable));

    // require("fakemod") should return the pre-set table
    bool ok = runLuaChunk(L, "fm = require('fakemod')");
    ASSERT_TRUE(suite, ok, "require pre-loaded module succeeds");

    Value fmVal = L->getGlobal("fm");
    ASSERT_TRUE(suite, fmVal.isTable(), "pre-loaded module returns table");
    if (fmVal.isTable()) {
        Value xVal = getField(L, fmVal.asTable(), "x");
        ASSERT_TRUE(suite, xVal.isNumber() && xVal.asNumber() == 99.0, "pre-loaded value correct");
    }
}

// =====================================================================
// Test: package.loadlib opens libraries and resolves symbols
// =====================================================================

void testPackageLoadlib(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();
    bool hasExePath = setPackageTestExecutablePath(L);
    ASSERT_TRUE(suite, hasExePath, "current executable path is available");
    if (!hasExePath) return;

    bool ok = runLuaChunk(L, R"(
        local missing, openerr, openwhere = package.loadlib("definitely_missing_package_test_library.dll", "luaopen_missing")
        loadlib_missing_nil = (missing == nil)
        loadlib_missing_has_error = (type(openerr) == "string")
        loadlib_missing_where = openwhere

        local nosym, symerr, symwhere = package.loadlib(package_test_exe, "luaopen_missing_symbol_for_test")
        loadlib_symbol_nil = (nosym == nil)
        loadlib_symbol_has_error = (type(symerr) == "string")
        loadlib_symbol_where = symwhere

        local linked = package.loadlib(package_test_exe, "*")
        loadlib_star_ok = (linked == true)

        local f, err, where = package.loadlib(package_test_exe, "luaopen_loadlibfixture")
        loadlib_result_function = (type(f) == "function")
        loadlib_result_error_nil = (err == nil and where == nil)

        local mod = f("loadlibfixture")
        loadlib_result_kind = mod.kind
        loadlib_result_value = mod.value
    )");
    ASSERT_TRUE(suite, ok, "package.loadlib runs without crash");

    Value missingNil = L->getGlobal("loadlib_missing_nil");
    ASSERT_TRUE(suite, missingNil.isBoolean() && missingNil.asBoolean(),
                "loadlib returns nil for missing library");

    Value missingErr = L->getGlobal("loadlib_missing_has_error");
    ASSERT_TRUE(suite, missingErr.isBoolean() && missingErr.asBoolean(),
                "loadlib returns error message for missing library");

    Value missingWhere = L->getGlobal("loadlib_missing_where");
    ASSERT_TRUE(suite, missingWhere.isString() &&
                std::string(missingWhere.asString()->c_str()) == "open",
                "loadlib reports open failures");

    Value symNil = L->getGlobal("loadlib_symbol_nil");
    ASSERT_TRUE(suite, symNil.isBoolean() && symNil.asBoolean(),
                "loadlib returns nil for missing symbol");

    Value symErr = L->getGlobal("loadlib_symbol_has_error");
    ASSERT_TRUE(suite, symErr.isBoolean() && symErr.asBoolean(),
                "loadlib returns error message for missing symbol");

    Value symWhere = L->getGlobal("loadlib_symbol_where");
    ASSERT_TRUE(suite, symWhere.isString() &&
                std::string(symWhere.asString()->c_str()) == "init",
                "loadlib reports symbol lookup failures");

    Value starOk = L->getGlobal("loadlib_star_ok");
    ASSERT_TRUE(suite, starOk.isBoolean() && starOk.asBoolean(),
                "loadlib with '*' links and returns true");

    Value isFunction = L->getGlobal("loadlib_result_function");
    ASSERT_TRUE(suite, isFunction.isBoolean() && isFunction.asBoolean(),
                "loadlib returns a callable C function");

    Value noError = L->getGlobal("loadlib_result_error_nil");
    ASSERT_TRUE(suite, noError.isBoolean() && noError.asBoolean(),
                "loadlib success does not return error values");

    Value kind = L->getGlobal("loadlib_result_kind");
    ASSERT_TRUE(suite, kind.isString() &&
                std::string(kind.asString()->c_str()) == "loadlib",
                "loadlib returned function can be called");

    Value value = L->getGlobal("loadlib_result_value");
    ASSERT_TRUE(suite, value.isNumber() && value.asNumber() == 701.0,
                "loadlib returned function produces module value");
}

// =====================================================================
// Test: require() loads native modules via package.loaders[3]
// =====================================================================

void testRequireFromCLoader(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();
    bool hasExePath = setPackageTestExecutablePath(L);
    ASSERT_TRUE(suite, hasExePath, "current executable path is available");
    if (!hasExePath) return;

    bool ok = runLuaChunk(L, R"(
        package.cpath = package_test_exe
        local mod = require("pkgdirect")
        cloader_kind = mod.kind
        cloader_value = mod.value
        cloader_cached = (package.loaded.pkgdirect == mod)
    )");
    ASSERT_TRUE(suite, ok, "require loads module through C loader");

    Value kind = L->getGlobal("cloader_kind");
    ASSERT_TRUE(suite, kind.isString() &&
                std::string(kind.asString()->c_str()) == "direct",
                "C loader resolves luaopen_<module>");

    Value value = L->getGlobal("cloader_value");
    ASSERT_TRUE(suite, value.isNumber() && value.asNumber() == 702.0,
                "C loader returns native module result");

    Value cached = L->getGlobal("cloader_cached");
    ASSERT_TRUE(suite, cached.isBoolean() && cached.asBoolean(),
                "require caches C loader result");
}

// =====================================================================
// Test: package.loaders[4] supports all-in-one C modules
// =====================================================================

void testAllInOneCLoader(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();
    bool hasExePath = setPackageTestExecutablePath(L);
    ASSERT_TRUE(suite, hasExePath, "current executable path is available");
    if (!hasExePath) return;

    bool ok = runLuaChunk(L, R"(
        package.cpath = package_test_exe
        local loader = package.loaders[4]
        local f = loader("pkgroot.child")
        allinone_loader_function = (type(f) == "function")

        local mod = f("pkgroot.child")
        allinone_kind = mod.kind
        allinone_value = mod.value
    )");
    ASSERT_TRUE(suite, ok, "all-in-one C loader returns a native loader");

    Value isFunction = L->getGlobal("allinone_loader_function");
    ASSERT_TRUE(suite, isFunction.isBoolean() && isFunction.asBoolean(),
                "all-in-one loader returns a function for dotted modules");

    Value kind = L->getGlobal("allinone_kind");
    ASSERT_TRUE(suite, kind.isString() &&
                std::string(kind.asString()->c_str()) == "allinone",
                "all-in-one loader resolves luaopen_root_child");

    Value value = L->getGlobal("allinone_value");
    ASSERT_TRUE(suite, value.isNumber() && value.asNumber() == 703.0,
                "all-in-one loader function produces module value");
}

// =====================================================================
// Test: module() creates module table
// =====================================================================

void testModuleCreation(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    bool ok = runLuaChunk(L, "module('testmod')");
    ASSERT_TRUE(suite, ok, "module() runs successfully");

    // Check that package.loaded["testmod"] exists
    Value pkgVal = L->getGlobal("package");
    Table* loaded = getField(L, pkgVal.asTable(), "loaded").asTable();

    auto& pool = L->getGlobalState().getStringPool();
    GCString* modKey = pool.intern("testmod");
    Value modVal = loaded->get(Value(modKey));

    ASSERT_TRUE(suite, modVal.isTable(), "module table stored in package.loaded");

    if (modVal.isTable()) {
        Table* modTable = modVal.asTable();
        Value nameVal = getField(L, modTable, "_NAME");
        ASSERT_TRUE(suite, nameVal.isString(), "_NAME field exists");
        if (nameVal.isString()) {
            ASSERT_TRUE(suite, std::string(nameVal.asString()->c_str()) == "testmod",
                         "_NAME equals module name");
        }

        Value mVal = getField(L, modTable, "_M");
        ASSERT_TRUE(suite, mVal.isTable(), "_M field exists");
        ASSERT_TRUE(suite, mVal.asTable() == modTable, "_M references module table");
    }

    // Also check global
    Value globalMod = L->getGlobal("testmod");
    ASSERT_TRUE(suite, globalMod.isTable(), "module registered as global");
}

// =====================================================================
// Test: module() switches the caller's environment to the module table
// =====================================================================

void testModuleSetsCallerEnvironment(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    bool ok = runLuaChunk(L, R"lua(
        module('plainenv')
        plainValue = 321
    )lua");
    ASSERT_TRUE(suite, ok, "module() switches caller environment");

    Value pkgVal = L->getGlobal("package");
    Table* loaded = getField(L, pkgVal.asTable(), "loaded").asTable();
    auto& pool = L->getGlobalState().getStringPool();

    Value modVal = loaded->get(Value(pool.intern("plainenv")));
    ASSERT_TRUE(suite, modVal.isTable(), "plainenv table stored in package.loaded");

    if (modVal.isTable()) {
        Table* modTable = modVal.asTable();
        Value value = getField(L, modTable, "plainValue");
        ASSERT_TRUE(suite, value.isNumber() && value.asNumber() == 321.0,
                    "assignment after module() goes into module table");

        Value packageVal = getField(L, modTable, "_PACKAGE");
        ASSERT_TRUE(suite, packageVal.isString(), "_PACKAGE field exists");
        if (packageVal.isString()) {
            ASSERT_TRUE(suite, std::string(packageVal.asString()->c_str()).empty(),
                        "_PACKAGE is empty for top-level module");
        }
    }

    Value globalValue = L->getGlobal("plainValue");
    ASSERT_TRUE(suite, globalValue.isNil(), "assignment after module() does not leak to _G");
}

// =====================================================================
// Test: module() creates/reuses compound global paths
// =====================================================================

void testModuleCompoundName(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    bool ok = runLuaChunk(L, R"lua(
        module('alpha.beta.gamma', package.seeall)
        compoundValue = 99
        _G.compoundName = _NAME
        _G.compoundPackage = _PACKAGE
        _G.compoundCanSeeGlobal = (type(package) == 'table')
    )lua");
    ASSERT_TRUE(suite, ok, "module() supports compound names");

    Value pkgVal = L->getGlobal("package");
    Table* loaded = getField(L, pkgVal.asTable(), "loaded").asTable();
    auto& pool = L->getGlobalState().getStringPool();

    Value modVal = loaded->get(Value(pool.intern("alpha.beta.gamma")));
    ASSERT_TRUE(suite, modVal.isTable(), "compound module table stored in package.loaded");

    Value alphaVal = L->getGlobal("alpha");
    ASSERT_TRUE(suite, alphaVal.isTable(), "compound module creates global root table");

    if (alphaVal.isTable() && modVal.isTable()) {
        Value betaVal = getField(L, alphaVal.asTable(), "beta");
        ASSERT_TRUE(suite, betaVal.isTable(), "compound module creates nested table");

        if (betaVal.isTable()) {
            Value gammaVal = getField(L, betaVal.asTable(), "gamma");
            ASSERT_TRUE(suite, gammaVal.isTable(), "compound module creates leaf table");
            ASSERT_TRUE(suite, gammaVal.isTable() && gammaVal.asTable() == modVal.asTable(),
                        "global compound path points at package.loaded module table");
        }

        Value compound = getField(L, modVal.asTable(), "compoundValue");
        ASSERT_TRUE(suite, compound.isNumber() && compound.asNumber() == 99.0,
                    "compound module assignment goes into module table");

        Value packageVal = getField(L, modVal.asTable(), "_PACKAGE");
        ASSERT_TRUE(suite, packageVal.isString(), "compound module _PACKAGE exists");
        if (packageVal.isString()) {
            ASSERT_TRUE(suite, std::string(packageVal.asString()->c_str()) == "alpha.beta.",
                        "compound module _PACKAGE contains parent prefix");
        }
    }

    Value nameVal = L->getGlobal("compoundName");
    ASSERT_TRUE(suite, nameVal.isString() &&
                std::string(nameVal.asString()->c_str()) == "alpha.beta.gamma",
                "package.seeall allows module body to publish _NAME through _G");

    Value packageVal = L->getGlobal("compoundPackage");
    ASSERT_TRUE(suite, packageVal.isString() &&
                std::string(packageVal.asString()->c_str()) == "alpha.beta.",
                "package.seeall allows module body to publish _PACKAGE through _G");

    Value canSeeGlobal = L->getGlobal("compoundCanSeeGlobal");
    ASSERT_TRUE(suite, canSeeGlobal.isBoolean() && canSeeGlobal.asBoolean(),
                "package.seeall lets module environment read globals");
}

// =====================================================================
// Test: module() calls Lua option functions
// =====================================================================

void testModuleLuaOption(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    bool ok = runLuaChunk(L, R"lua(
        local function mark(M)
            M.optionCalled = true
        end

        module('optionmod', mark)
        optionValue = 17
    )lua");
    ASSERT_TRUE(suite, ok, "module() accepts Lua option functions");

    Value pkgVal = L->getGlobal("package");
    Table* loaded = getField(L, pkgVal.asTable(), "loaded").asTable();
    auto& pool = L->getGlobalState().getStringPool();

    Value modVal = loaded->get(Value(pool.intern("optionmod")));
    ASSERT_TRUE(suite, modVal.isTable(), "optionmod table stored in package.loaded");

    if (modVal.isTable()) {
        Value optionCalled = getField(L, modVal.asTable(), "optionCalled");
        ASSERT_TRUE(suite, optionCalled.isBoolean() && optionCalled.asBoolean(),
                    "Lua option function receives module table");

        Value optionValue = getField(L, modVal.asTable(), "optionValue");
        ASSERT_TRUE(suite, optionValue.isNumber() && optionValue.asNumber() == 17.0,
                    "caller environment still switches after Lua option");
    }
}

// =====================================================================
// Test: package.seeall
// =====================================================================

void testPackageSeeall(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    auto& pool = L->getGlobalState().getStringPool();
    auto& gc = L->getGlobalState().getGC();

    // Create a module table
    Table* modTable = new Table();
    gc.registerObject(modTable);

    // Call package.seeall on it
    Value pkgVal = L->getGlobal("package");
    Table* pkg = pkgVal.asTable();
    Value seeallVal = getField(L, pkg, "seeall");
    ASSERT_TRUE(suite, seeallVal.isFunction(), "package.seeall is function");

    if (seeallVal.isFunction()) {
        Function* seeallFunc = seeallVal.asFunction();
        L->setTop(0);
        L->pushValue(Value(modTable));
        seeallFunc->getCFunction()(L);

        // modTable should now have a metatable with __index = _G
        Table* mt = modTable->getMetatable();
        ASSERT_TRUE(suite, mt != nullptr, "module has metatable after seeall");

        if (mt) {
            GCString* indexKey = pool.intern("__index");
            Value indexVal = mt->get(Value(indexKey));
            ASSERT_TRUE(suite, indexVal.isTable(), "__index is table");
            ASSERT_TRUE(suite, indexVal.asTable() == L->getGlobalTable(),
                         "__index points to global table");
        }
    }
}

// =====================================================================
// Test: require() with standard libraries via openAll
// =====================================================================

void testRequireStdlibs(TestSuite& suite) {
    LuaStdLibTestContext ctx(openAllLibs);
    LuaState* L = ctx.getState();

    // package.loaded should contain standard library entries
    Value pkgVal = L->getGlobal("package");
    if (!pkgVal.isTable()) {
        ASSERT_TRUE(suite, false, "package table exists for stdlib test");
        return;
    }

    Table* loaded = getField(L, pkgVal.asTable(), "loaded").asTable();
    if (!loaded) {
        ASSERT_TRUE(suite, false, "package.loaded exists for stdlib test");
        return;
    }

    auto& pool = L->getGlobalState().getStringPool();

    // Check that at least "math" is in package.loaded
    GCString* mathKey = pool.intern("math");
    Value mathVal = loaded->get(Value(mathKey));
    ASSERT_TRUE(suite, mathVal.isTable(), "package.loaded has math");

    // require("math") should return the cached table
    bool ok = runLuaChunk(L, "m = require('math')");
    ASSERT_TRUE(suite, ok, "require('math') succeeds");

    Value mVal = L->getGlobal("m");
    ASSERT_TRUE(suite, mVal.isTable(), "require('math') returns table");

    // Check other libraries
    GCString* ioKey = pool.intern("io");
    Value ioVal = loaded->get(Value(ioKey));
    ASSERT_TRUE(suite, ioVal.isTable(), "package.loaded has io");

    GCString* osKey = pool.intern("os");
    Value osVal = loaded->get(Value(osKey));
    ASSERT_TRUE(suite, osVal.isTable(), "package.loaded has os");

    GCString* stringKey = pool.intern("string");
    Value stringVal = loaded->get(Value(stringKey));
    ASSERT_TRUE(suite, stringVal.isTable(), "package.loaded has string");

    GCString* tableKey = pool.intern("table");
    Value tableVal = loaded->get(Value(tableKey));
    ASSERT_TRUE(suite, tableVal.isTable(), "package.loaded has table");
}

// =====================================================================
// Test: require() with a preload function that returns nothing
// =====================================================================

void testRequirePreloadNoReturn(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    auto& pool = L->getGlobalState().getStringPool();
    auto& gc = L->getGlobalState().getGC();

    // A loader that doesn't return anything
    auto modLoader = [](LuaState* L) -> i32 {
        // Set a global variable to prove we ran
        L->setGlobal("noreturn_ran", Value(true));
        L->setTop(0);
        return 0;
    };

    Value pkgVal = L->getGlobal("package");
    Table* preload = getField(L, pkgVal.asTable(), "preload").asTable();

    Function* loaderFunc = new Function(static_cast<LibCFunction>(modLoader));
    gc.registerObject(loaderFunc);
    GCString* modKey = pool.intern("noreturnmod");
    preload->set(Value(modKey), Value(loaderFunc));

    bool ok = runLuaChunk(L, "nr = require('noreturnmod')");
    ASSERT_TRUE(suite, ok, "require module with no return succeeds");

    // Should default to true
    Value nrVal = L->getGlobal("nr");
    ASSERT_TRUE(suite, nrVal.isBoolean() && nrVal.asBoolean() == true,
                 "module with no return defaults to true");
}

void testRequireSurvivesDeletedPackageGlobal(TestSuite& suite) {
    LuaStdLibTestContext ctx(openAllLibs);
    LuaState* L = ctx.getState();

    bool ok = runLuaChunk(L, R"lua(
        package.preload.afterclear = function()
            return { value = 42 }
        end
        package = nil
        require_after_clear = require("afterclear").value
    )lua");

    ASSERT_TRUE(suite, ok, "require survives deleted global package table");
    Value result = L->getGlobal("require_after_clear");
    ASSERT_TRUE(suite, result.isNumber() && result.asNumber() == 42.0,
                "require still uses package tables after package global deletion");
}

// =====================================================================
// Test: package.path string format
// =====================================================================

void testPackagePath(TestSuite& suite) {
    LuaStdLibTestContext ctx(openBaseAndPackage);
    LuaState* L = ctx.getState();

    Value pkgVal = L->getGlobal("package");
    Table* pkg = pkgVal.asTable();

    Value pathVal = getField(L, pkg, "path");
    ASSERT_TRUE(suite, pathVal.isString(), "package.path is string");

    std::string path = pathVal.asString()->c_str();
    // Path should contain "?" as placeholder
    ASSERT_TRUE(suite, path.find('?') != std::string::npos, "package.path contains ? placeholder");

    // Path should contain ".lua"
    ASSERT_TRUE(suite, path.find(".lua") != std::string::npos, "package.path contains .lua");
}

void testRequireMissingErrorMatchesVisiblePaths(TestSuite& suite) {
    LuaStdLibTestContext ctx(openAllLibs);
    LuaState* L = ctx.getState();

    const char* script = R"lua(
local fname = "__missing_require_path_probe__"
local ok, err = pcall(require, fname)
require_path_error_matches = not ok
for template in string.gmatch(package.path .. ";" .. package.cpath, "[^;]+") do
    local expected = string.gsub(template, "?", fname)
    if not string.find(err, expected, 1, true) then
        require_path_error_matches = false
        break
    end
end
)lua";

    bool ok = runLuaChunk(L, script, "require_path_error_matches");
    ASSERT_TRUE(suite, ok, "require missing path probe runs");

    Value matches = L->getGlobal("require_path_error_matches");
    ASSERT_TRUE(suite, matches.isBoolean() && matches.asBoolean(), "require error matches visible package paths");
}

} // end anonymous namespace

// =====================================================================
// Test Registration
// =====================================================================

void registerPackageLibTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "package table", testPackageTableRegistration);
    registry.registerTest(kSuiteName, "global functions", testGlobalFunctionsExist);
    registry.registerTest(kSuiteName, "package.config", testPackageConfig);
    registry.registerTest(kSuiteName, "package.loaders", testPackageLoaders);
    registry.registerTest(kSuiteName, "require preload", testRequirePreload);
    registry.registerTest(kSuiteName, "require caching", testRequireCaching);
    registry.registerTest(kSuiteName, "require false reloads", testRequireFalseResultReloads);
    registry.registerTest(kSuiteName, "require missing", testRequireMissingModule);
    registry.registerTest(kSuiteName, "require from file", testRequireFromFile);
    registry.registerTest(kSuiteName, "package.loaded set", testPackageLoadedDirectSet);
    registry.registerTest(kSuiteName, "package.loadlib", testPackageLoadlib);
    registry.registerTest(kSuiteName, "require C loader", testRequireFromCLoader);
    registry.registerTest(kSuiteName, "all-in-one C loader", testAllInOneCLoader);
    registry.registerTest(kSuiteName, "module creation", testModuleCreation);
    registry.registerTest(kSuiteName, "module caller env", testModuleSetsCallerEnvironment);
    registry.registerTest(kSuiteName, "module compound name", testModuleCompoundName);
    registry.registerTest(kSuiteName, "module Lua option", testModuleLuaOption);
    registry.registerTest(kSuiteName, "package.seeall", testPackageSeeall);
    registry.registerTest(kSuiteName, "require stdlibs", testRequireStdlibs);
    registry.registerTest(kSuiteName, "preload no return", testRequirePreloadNoReturn);
    registry.registerTest(kSuiteName, "require without package global", testRequireSurvivesDeletedPackageGlobal);
    registry.registerTest(kSuiteName, "package.path", testPackagePath);
    registry.registerTest(kSuiteName, "require missing visible paths", testRequireMissingErrorMatchesVisiblePaths);
}
