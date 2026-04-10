/**
 * @file packagelib.cpp
 * @brief Lua package/module library implementation
 *
 * Implements the Lua 5.1.5 package system: require(), module(),
 * and the package.* table fields.
 *
 * Design notes:
 * - package.loaded caches all successfully loaded modules
 * - package.preload allows C code to pre-register loader functions
 * - package.loaders is a sequence of searcher functions tried in order
 * - Default loaders: [1] preload, [2] Lua file, [3] C library (stub)
 * - require() is idempotent: repeated calls return the cached value
 * - module() creates a module table and adjusts the environment
 *
 * Reference Implementation:
 * - lua_c_analysis/src/loadlib.c — Lua 5.1.5 loadlib
 *
 * @author Lua C++ Project
 * @date 2026-04-10
 */

#include "lib/packagelib.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "vm/global_state.hpp"
#include "vm/vm.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace Lua {

// =====================================================================
// Internal keys — used to retrieve package sub-tables from the registry
// or global "package" table.
// =====================================================================

static const char* const PACKAGE_TABLE_NAME = "package";

// Default paths
#ifdef _WIN32
static const char* const LUA_DEFAULT_PATH =
    ".\\?.lua;"
    ".\\?\\init.lua;"
    "!\\lua\\?.lua;"
    "!\\lua\\?\\init.lua";
static const char* const LUA_DEFAULT_CPATH =
    ".\\?.dll;"
    "!\\?.dll;"
    "!\\loadall.dll";
static const char* const LUA_PATH_SEP  = ";";
static const char* const LUA_PATH_MARK = "?";
static const char* const LUA_DIR_SEP   = "\\";
static const char* const LUA_EXEC_DIR  = "!";
static const char* const LUA_IGMARK    = "-";
#else
static const char* const LUA_DEFAULT_PATH =
    "./?.lua;"
    "./?/init.lua;"
    "/usr/local/share/lua/5.1/?.lua;"
    "/usr/local/share/lua/5.1/?/init.lua";
static const char* const LUA_DEFAULT_CPATH =
    "./?.so;"
    "/usr/local/lib/lua/5.1/?.so";
static const char* const LUA_PATH_SEP  = ";";
static const char* const LUA_PATH_MARK = "?";
static const char* const LUA_DIR_SEP   = "/";
static const char* const LUA_EXEC_DIR  = "!";
static const char* const LUA_IGMARK    = "-";
#endif

// Config string: sep \n dirsep \n mark \n execdir \n igmark
static const char* const LUA_CONFIG_STRING =
#ifdef _WIN32
    "\\\n;\n?\n!\n-";
#else
    "/\n;\n?\n!\n-";
#endif

// =====================================================================
// Helper: get the "package" table from global environment
// =====================================================================

static Table* getPackageTable(LuaState* L) {
    Value pkgVal = L->getGlobal(PACKAGE_TABLE_NAME);
    if (pkgVal.isTable()) {
        return pkgVal.asTable();
    }
    return nullptr;
}

// =====================================================================
// Helper: get a sub-table from the package table
// =====================================================================

static Table* getPackageSubTable(LuaState* L, const char* fieldName) {
    Table* pkg = getPackageTable(L);
    if (!pkg) return nullptr;

    GCString* key = L->getGlobalState().getStringPool().intern(fieldName);
    Value val = pkg->get(Value(key));
    if (val.isTable()) {
        return val.asTable();
    }
    return nullptr;
}

// =====================================================================
// Helper: get a string field from the package table
// =====================================================================

static Str getPackageStringField(LuaState* L, const char* fieldName) {
    Table* pkg = getPackageTable(L);
    if (!pkg) return "";

    GCString* key = L->getGlobalState().getStringPool().intern(fieldName);
    Value val = pkg->get(Value(key));
    if (val.isString()) {
        return val.asString()->c_str();
    }
    return "";
}

// =====================================================================
// Path searching: replace "?" with module name in each path template
// =====================================================================

/// Replace all occurrences of `pattern` with `replacement` in `str`
static Str replaceAll(const Str& str, const Str& pattern, const Str& replacement) {
    Str result;
    result.reserve(str.size());
    usize pos = 0;
    usize patLen = pattern.size();
    while (pos < str.size()) {
        usize found = str.find(pattern, pos);
        if (found == Str::npos) {
            result.append(str, pos, str.size() - pos);
            break;
        }
        result.append(str, pos, found - pos);
        result.append(replacement);
        pos = found + patLen;
    }
    return result;
}

/// Convert module name dots to directory separators for path searching
static Str moduleNameToPath(const Str& modname) {
    return replaceAll(modname, ".", LUA_DIR_SEP);
}

/// Search for a file along the given path template string.
/// Returns the file path that was found, or empty string.
/// On failure, appends tried paths to `errorBuf` for the error message.
static Str searchPath(const Str& name, const Str& pathStr, Str& errorBuf) {
    Str modPath = moduleNameToPath(name);

    // Split pathStr by ";"
    usize pos = 0;
    while (pos <= pathStr.size()) {
        usize sep = pathStr.find(';', pos);
        if (sep == Str::npos) sep = pathStr.size();

        Str tmpl = pathStr.substr(pos, sep - pos);
        pos = sep + 1;

        if (tmpl.empty()) continue;

        // Replace "?" with the module name (with dots replaced by dirsep)
        Str filePath = replaceAll(tmpl, LUA_PATH_MARK, modPath);

        // Try to open the file
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            file.close();
            return filePath;
        }

        // Accumulate error message
        errorBuf += "\n\tno file '";
        errorBuf += filePath;
        errorBuf += "'";
    }
    return "";
}

// =====================================================================
// Helper: load a Lua file and return a compiled function
// Returns nullptr on failure and pushes nil + error to the stack.
// =====================================================================

static Function* loadLuaFile(LuaState* L, const Str& filename) {
    auto& pool = L->getGlobalState().getStringPool();

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return nullptr;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    Str source;
    source.resize(static_cast<usize>(size));
    if (!file.read(&source[0], size)) {
        return nullptr;
    }

    try {
        Parser parser(source);
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Str chunkName = "@" + filename;
        Proto* proto = codegen.generate(chunk, chunkName);
        if (!proto) {
            return nullptr;
        }

        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        func->setEnv(L->getGlobalTable());
        return func;

    } catch (...) {
        return nullptr;
    }
}

// =====================================================================
// package.loaders[1] — preload searcher
// =====================================================================

i32 loader_preload(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1 || !L->at(1).isString()) {
        L->setTop(0);
        L->pushString(L->getGlobalState().getStringPool().intern(
            "\n\tno field package.preload"));
        return 1;
    }

    Str modname = L->at(1).asString()->c_str();
    auto& pool = L->getGlobalState().getStringPool();

    // Look up package.preload[modname]
    Table* preload = getPackageSubTable(L, "preload");
    if (!preload) {
        L->setTop(0);
        L->pushString(pool.intern("\n\tno field package.preload"));
        return 1;
    }

    GCString* key = pool.intern(modname.c_str());
    Value loaderVal = preload->get(Value(key));

    if (loaderVal.isFunction()) {
        L->setTop(0);
        L->pushValue(loaderVal);
        return 1;
    }

    // Not found — return error string (not a hard error)
    Str msg = "\n\tno field package.preload['" + modname + "']";
    L->setTop(0);
    L->pushString(pool.intern(msg.c_str()));
    return 1;
}

// =====================================================================
// package.loaders[2] — Lua file searcher
// =====================================================================

i32 loader_lua(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1 || !L->at(1).isString()) {
        L->setTop(0);
        L->pushString(L->getGlobalState().getStringPool().intern(
            "\n\tno field package.path"));
        return 1;
    }

    Str modname = L->at(1).asString()->c_str();
    auto& pool = L->getGlobalState().getStringPool();

    Str pathStr = getPackageStringField(L, "path");
    if (pathStr.empty()) {
        L->setTop(0);
        L->pushString(pool.intern("\n\tno field package.path"));
        return 1;
    }

    Str errorBuf;
    Str filename = searchPath(modname, pathStr, errorBuf);

    if (filename.empty()) {
        // Not found — return error string
        L->setTop(0);
        L->pushString(pool.intern(errorBuf.c_str()));
        return 1;
    }

    // Found — compile and return the loader function
    Function* func = loadLuaFile(L, filename);
    if (!func) {
        Str msg = "\n\terror loading module '" + modname + "' from file '" + filename + "'";
        L->setTop(0);
        L->pushString(pool.intern(msg.c_str()));
        return 1;
    }

    L->setTop(0);
    L->pushValue(Value(func));
    return 1;
}

// =====================================================================
// package.loaders[3] — C library searcher (stub)
// =====================================================================

i32 loader_clib(LuaState* L) {
    i32 nargs = L->getTop();
    Str modname = (nargs >= 1 && L->at(1).isString())
        ? L->at(1).asString()->c_str()
        : "?";

    auto& pool = L->getGlobalState().getStringPool();

    Str pathStr = getPackageStringField(L, "cpath");

    Str errorBuf;
    if (!pathStr.empty()) {
        // Try to find the file on disk (even though we can't load it)
        Str filename = searchPath(modname, pathStr, errorBuf);
        (void)filename;
    }

    if (errorBuf.empty()) {
        errorBuf = "\n\tno C loader available (not supported in this interpreter)";
    }

    L->setTop(0);
    L->pushString(pool.intern(errorBuf.c_str()));
    return 1;
}

// =====================================================================
// require(modname) — load and return a module
// =====================================================================

i32 luaP_require(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("bad argument #1 to 'require' (string expected, got no value)");
    }
    if (!L->at(1).isString()) {
        L->error("bad argument #1 to 'require' (string expected)");
    }

    Str modname = L->at(1).asString()->c_str();
    auto& pool = L->getGlobalState().getStringPool();

    // 1. Check package.loaded[modname]
    Table* loaded = getPackageSubTable(L, "loaded");
    if (!loaded) {
        L->error("'package.loaded' table is missing");
    }

    GCString* modKey = pool.intern(modname.c_str());
    Value cachedVal = loaded->get(Value(modKey));

    if (!cachedVal.isNil()) {
        // Already loaded — return cached value
        L->setTop(0);
        L->pushValue(cachedVal);
        return 1;
    }

    // 2. Try each loader in package.loaders
    Table* loaders = getPackageSubTable(L, "loaders");
    if (!loaders) {
        L->error("'package.loaders' table is missing");
    }

    Str errorAccum = "module '" + modname + "' not found:";

    // Iterate loaders[1], loaders[2], ...
    for (i32 i = 1; ; i++) {
        Value loaderEntry = loaders->get(Value(static_cast<f64>(i)));
        if (loaderEntry.isNil()) {
            break;  // No more loaders
        }

        if (!loaderEntry.isFunction()) {
            continue;  // Skip non-function entries
        }

        Function* searcherFunc = loaderEntry.asFunction();

        // Call the searcher: result = searcher(modname)
        // Use VM::call to properly set up a call frame so at(1) = modname
        L->setTop(0);
        L->pushValue(Value(searcherFunc));
        L->pushString(modKey);
        VM::call(L, 1, 1);  // 1 arg (modname), 1 result

        // After VM::call, result is on the stack
        if (L->getTop() >= 1) {
            Value result = L->at(1);

            if (result.isFunction()) {
                // Found a loader function — call it with modname
                Function* loaderFunc = result.asFunction();

                L->setTop(0);
                L->pushValue(Value(loaderFunc));
                L->pushString(modKey);

                Value moduleResult;

                try {
                    VM::call(L, 1, 1);  // 1 arg (modname), 1 result

                    if (L->getTop() >= 1) {
                        moduleResult = L->at(1);
                    }
                } catch (const std::exception& e) {
                    Str msg = "error loading module '" + modname + "': " + e.what();
                    L->error(msg.c_str());
                }

                // If the loader returned a non-nil value, store it
                if (!moduleResult.isNil()) {
                    loaded->set(Value(modKey), moduleResult);
                } else {
                    // If no explicit return, check if package.loaded[modname]
                    // was set by the module itself; if not, set it to true
                    Value check = loaded->get(Value(modKey));
                    if (check.isNil()) {
                        moduleResult = Value(true);
                        loaded->set(Value(modKey), moduleResult);
                    } else {
                        moduleResult = check;
                    }
                }

                L->setTop(0);
                L->pushValue(moduleResult);
                return 1;

            } else if (result.isString()) {
                // Searcher returned an error string
                errorAccum += result.asString()->c_str();
            }
            // else: unexpected type, skip
        }
    }

    // No loader succeeded
    L->error(errorAccum.c_str());
    return 0;  // unreachable
}

// =====================================================================
// module(name [, ...]) — create a module
// =====================================================================

i32 luaP_module(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("bad argument #1 to 'module' (string expected, got no value)");
    }
    if (!L->at(1).isString()) {
        L->error("bad argument #1 to 'module' (string expected)");
    }

    Str modname = L->at(1).asString()->c_str();
    auto& pool = L->getGlobalState().getStringPool();

    // 1. Check package.loaded[modname]; create table if absent
    Table* loaded = getPackageSubTable(L, "loaded");
    if (!loaded) {
        L->error("'package.loaded' table is missing");
    }

    GCString* modKey = pool.intern(modname.c_str());
    Value existingVal = loaded->get(Value(modKey));

    Table* modTable = nullptr;
    if (existingVal.isTable()) {
        modTable = existingVal.asTable();
    } else {
        // Create a new module table
        modTable = new Table();
        L->getGlobalState().getGC().registerObject(modTable);

        // Store it in package.loaded
        loaded->set(Value(modKey), Value(modTable));

        // Also register in the global table
        L->setGlobal(modname, Value(modTable));
    }

    // 2. Set _NAME and _M fields
    GCString* nameKey = pool.intern("_NAME");
    modTable->set(Value(nameKey), Value(modKey));

    GCString* mKey = pool.intern("_M");
    modTable->set(Value(mKey), Value(modTable));

    // 3. Apply option functions (e.g., package.seeall)
    for (i32 i = 2; i <= nargs; i++) {
        Value optVal = L->at(i);
        if (optVal.isFunction()) {
            Function* optFunc = optVal.asFunction();
            // Call optFunc(modTable)
            L->setTop(0);
            L->pushValue(Value(optFunc));
            L->pushValue(Value(modTable));

            if (optFunc->isCFunction()) {
                optFunc->getCFunction()(L);
            }
        }
    }

    L->setTop(0);
    return 0;
}

// =====================================================================
// package.loadlib(libname, funcname) — dynamic loading (stub)
// =====================================================================

i32 luaP_loadlib(LuaState* L) {
    // Dynamic C library loading is not supported in this interpreter.
    // Return nil, error_message, "absent" as Lua 5.1 does on failure.
    auto& pool = L->getGlobalState().getStringPool();

    L->setTop(0);
    L->pushNil();
    L->pushString(pool.intern("dynamic C library loading is not supported"));
    L->pushString(pool.intern("absent"));
    return 3;
}

// =====================================================================
// package.seeall(module) — set module env to see all globals
// =====================================================================

i32 luaP_seeall(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("bad argument #1 to 'package.seeall' (table expected)");
    }

    if (!L->at(1).isTable()) {
        L->error("bad argument #1 to 'package.seeall' (table expected)");
    }

    Table* modTable = L->at(1).asTable();
    auto& pool = L->getGlobalState().getStringPool();

    // Set __index of the module's metatable to _G
    // If the module table has no metatable, create one
    Table* mt = modTable->getMetatable();
    if (!mt) {
        mt = new Table();
        L->getGlobalState().getGC().registerObject(mt);
        modTable->setMetatable(mt);
    }

    GCString* indexKey = pool.intern("__index");
    mt->set(Value(indexKey), Value(L->getGlobalTable()));

    return 0;
}

// =====================================================================
// package.searchpath(name, path [, sep [, rep]])
// Not in Lua 5.1 proper, but useful helper — skip for now.
// =====================================================================

// =====================================================================
// Library Registration
// =====================================================================

void PackageLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    auto& pool = L->getGlobalState().getStringPool();
    auto& gc = L->getGlobalState().getGC();

    // ---- Create the package table ----
    Table* pkgTable = FunctionRegistrar::createLibTable(L, PACKAGE_TABLE_NAME);
    if (!pkgTable) {
        L->error("Failed to create package library table");
        return;
    }

    // ---- Register functions into the package table ----
    FunctionRegistrar(L)
        .addGlobal("loadlib", luaP_loadlib)
        .addGlobal("seeall", luaP_seeall)
        .commitToTable(pkgTable);

    // ---- Register global functions: require, module ----
    FunctionRegistrar(L)
        .addGlobal("require", luaP_require)
        .addGlobal("module", luaP_module)
        .commit();

    // ---- package.loaded ----
    Table* loadedTable = new Table();
    gc.registerObject(loadedTable);
    GCString* loadedKey = pool.intern("loaded");
    pkgTable->set(Value(loadedKey), Value(loadedTable));

    // Pre-populate package.loaded with already-opened standard libraries.
    // We store _G for the base library, and the lib tables for named libs.
    // These will be detected at the point require() is called. For now,
    // we just leave loaded empty — modules register on first require().

    // ---- package.preload ----
    Table* preloadTable = new Table();
    gc.registerObject(preloadTable);
    GCString* preloadKey = pool.intern("preload");
    pkgTable->set(Value(preloadKey), Value(preloadTable));

    // ---- package.path ----
    GCString* pathKey = pool.intern("path");
    GCString* pathVal = pool.intern(LUA_DEFAULT_PATH);
    pkgTable->set(Value(pathKey), Value(pathVal));

    // ---- package.cpath ----
    GCString* cpathKey = pool.intern("cpath");
    GCString* cpathVal = pool.intern(LUA_DEFAULT_CPATH);
    pkgTable->set(Value(cpathKey), Value(cpathVal));

    // ---- package.config ----
    GCString* configKey = pool.intern("config");
    GCString* configVal = pool.intern(LUA_CONFIG_STRING);
    pkgTable->set(Value(configKey), Value(configVal));

    // ---- package.loaders ----
    Table* loadersTable = new Table();
    gc.registerObject(loadersTable);
    GCString* loadersKey = pool.intern("loaders");
    pkgTable->set(Value(loadersKey), Value(loadersTable));

    // loader[1] = preload searcher
    Function* preloadSearcher = new Function(loader_preload);
    gc.registerObject(preloadSearcher);
    loadersTable->set(Value(1.0), Value(preloadSearcher));

    // loader[2] = Lua file searcher
    Function* luaSearcher = new Function(loader_lua);
    gc.registerObject(luaSearcher);
    loadersTable->set(Value(2.0), Value(luaSearcher));

    // loader[3] = C library searcher (stub)
    Function* clibSearcher = new Function(loader_clib);
    gc.registerObject(clibSearcher);
    loadersTable->set(Value(3.0), Value(clibSearcher));
}

void PackageLibModule::initialize(LuaState* L) {
    if (!L) {
        return;
    }

    // Pre-populate package.loaded with standard libraries that are
    // already open. This allows  require("math")  etc. to work.
    Table* loaded = getPackageSubTable(L, "loaded");
    if (!loaded) return;

    auto& pool = L->getGlobalState().getStringPool();

    // Map library names to their global table entries
    static const char* stdlibs[] = {
        "math", "io", "os", "string", "table",
        "coroutine", "debug", "package",
        nullptr
    };

    for (const char** p = stdlibs; *p; ++p) {
        Value libVal = L->getGlobal(*p);
        if (!libVal.isNil()) {
            GCString* key = pool.intern(*p);
            loaded->set(Value(key), libVal);
        }
    }
}

void openPackageLib(LuaState* L) {
    if (!L) {
        return;
    }

    PackageLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
