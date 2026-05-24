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
 * - Default loaders: [1] preload, [2] Lua file, [3] C library,
 *   [4] all-in-one C library
 * - require() is idempotent: repeated calls return the cached value
 * - module() creates a module table and adjusts the environment
 * @author Lua C++ Project
 * @date 2026-04-10
 */

#include "lib/packagelib.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "vm/state/global_state.hpp"
#include "vm/vm.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <unordered_map>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <limits.h>
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
static const char* const LUA_OFSEP     = "_";
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
static const char* const LUA_OFSEP     = "_";
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

static Str executablePath() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    DWORD len = GetModuleFileNameA(nullptr, buffer, static_cast<DWORD>(sizeof(buffer)));
    if (len == 0 || len >= sizeof(buffer)) {
        return "";
    }
    return Str(buffer, len);
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len <= 0) {
        return "";
    }
    buffer[len] = '\0';
    return Str(buffer);
#endif
}

static Str executableDirectory() {
    Str path = executablePath();
    if (path.empty()) {
        return ".";
    }

    usize pos = path.find_last_of("/\\");
    if (pos == Str::npos) {
        return ".";
    }
    return path.substr(0, pos);
}

#ifdef _WIN32
static Str fullPath(const Str& path) {
    DWORD needed = GetFullPathNameA(path.c_str(), 0, nullptr, nullptr);
    if (needed == 0) {
        return path;
    }

    Vec<char> buffer(static_cast<usize>(needed) + 1, '\0');
    DWORD len = GetFullPathNameA(path.c_str(), needed + 1, buffer.data(), nullptr);
    if (len == 0) {
        return path;
    }
    return Str(buffer.data(), static_cast<usize>(len));
}

static Str lowerAscii(Str value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return value;
}

static bool isCurrentExecutablePath(const Str& path) {
    Str current = executablePath();
    if (current.empty()) {
        return false;
    }
    return lowerAscii(fullPath(path)) == lowerAscii(fullPath(current));
}
#else
static Str fullPath(const Str& path) {
    char buffer[PATH_MAX];
    if (realpath(path.c_str(), buffer) == nullptr) {
        return path;
    }
    return Str(buffer);
}

static bool isCurrentExecutablePath(const Str& path) {
    Str current = executablePath();
    if (current.empty()) {
        return false;
    }
    return fullPath(path) == fullPath(current);
}
#endif

static Str applyExecutableDirectory(const Str& pathTemplate) {
    return replaceAll(pathTemplate, LUA_EXEC_DIR, executableDirectory());
}

static Table* createPackageTableObject(LuaState* L) {
    Table* table = new Table();
    L->getGlobalState().getGC().registerObject(table);
    return table;
}

[[noreturn]] static void moduleNameConflict(LuaState* L, const Str& modname) {
    Str msg = "name conflict for module '" + modname + "'";
    L->error(msg.c_str());
}

static Str modulePackagePrefix(const Str& modname) {
    usize pos = modname.rfind('.');
    return pos == Str::npos ? Str() : modname.substr(0, pos + 1);
}

static Table* ensureChildTable(LuaState* L, Table* parent,
                               const Str& field, const Str& modname) {
    auto& pool = L->getGlobalState().getStringPool();
    GCString* key = pool.intern(field.c_str());
    Value existing = parent->get(Value(key));
    if (existing.isTable()) {
        return existing.asTable();
    }
    if (!existing.isNil()) {
        moduleNameConflict(L, modname);
    }

    Table* child = createPackageTableObject(L);
    parent->set(Value(key), Value(child));
    return child;
}

static Table* findOrCreateGlobalModuleTable(LuaState* L, const Str& modname) {
    auto& pool = L->getGlobalState().getStringPool();
    Table* parent = L->getGlobalTable();

    usize start = 0;
    while (start <= modname.size()) {
        usize dot = modname.find('.', start);
        bool isLast = dot == Str::npos;
        Str field = isLast ? modname.substr(start) : modname.substr(start, dot - start);
        if (field.empty()) {
            moduleNameConflict(L, modname);
        }

        if (isLast) {
            GCString* key = pool.intern(field.c_str());
            Value existing = parent->get(Value(key));
            if (existing.isTable()) {
                return existing.asTable();
            }
            if (!existing.isNil()) {
                moduleNameConflict(L, modname);
            }

            Table* modTable = createPackageTableObject(L);
            parent->set(Value(key), Value(modTable));
            return modTable;
        }

        parent = ensureChildTable(L, parent, field, modname);
        start = dot + 1;
    }

    moduleNameConflict(L, modname);
}

static void setGlobalModulePath(LuaState* L, const Str& modname, Table* modTable) {
    auto& pool = L->getGlobalState().getStringPool();
    Table* parent = L->getGlobalTable();

    usize start = 0;
    while (start <= modname.size()) {
        usize dot = modname.find('.', start);
        bool isLast = dot == Str::npos;
        Str field = isLast ? modname.substr(start) : modname.substr(start, dot - start);
        if (field.empty()) {
            moduleNameConflict(L, modname);
        }

        GCString* key = pool.intern(field.c_str());
        if (isLast) {
            parent->set(Value(key), Value(modTable));
            return;
        }

        Value existing = parent->get(Value(key));
        if (!existing.isNil() && !existing.isTable()) {
            moduleNameConflict(L, modname);
        }
        parent = existing.isTable()
            ? existing.asTable()
            : ensureChildTable(L, parent, field, modname);
        start = dot + 1;
    }

    moduleNameConflict(L, modname);
}

static void setCallingLuaFunctionEnv(LuaState* L, Table* env) {
    if (L->getCurrentCI() == 0) {
        L->error("module: no calling Lua function");
    }

    Vec<CallInfo>& frames = L->getCallStack();
    Stack& stack = L->getStack();

    for (usize i = L->getCurrentCI(); i > 0; --i) {
        const CallInfo& caller = frames[i - 1];
        if (caller.func >= stack.size()) {
            continue;
        }

        Value& funcVal = stack.at(caller.func);
        if (!funcVal.isFunction()) {
            continue;
        }

        Function* func = funcVal.asFunction();
        if (func->isLuaFunction()) {
            func->setEnv(env);
            return;
        }
    }

    L->error("module: no calling Lua function");
}

static void callModuleOption(LuaState* L, const Value& option, Table* modTable) {
    if (!option.isFunction()) {
        L->error("bad argument to 'module' option (function expected)");
    }

    usize savedTop = L->getAbsoluteTop();
    try {
        L->pushValue(option);
        L->pushValue(Value(modTable));
        VM::call(L, 1, 0);
        L->getStack().setTop(savedTop);
        L->setAbsoluteTop(savedTop);
    } catch (...) {
        L->getStack().setTop(savedTop);
        L->setAbsoluteTop(savedTop);
        throw;
    }
}

/// Search for a file along the given path template string.
/// Returns the file path that was found, or empty string.
/// On failure, appends tried paths to `errorBuf` for the error message.
static Str searchPath(const Str& name, const Str& pathStr, Str& errorBuf) {
    Str modPath = moduleNameToPath(name);

    // Split pathStr by the configured path separator.
    usize pos = 0;
    while (pos <= pathStr.size()) {
        usize sep = pathStr.find(LUA_PATH_SEP, pos);
        if (sep == Str::npos) sep = pathStr.size();

        Str tmpl = pathStr.substr(pos, sep - pos);
        pos = sep + 1;

        if (tmpl.empty()) continue;

        tmpl = applyExecutableDirectory(tmpl);

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
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);

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
// Dynamic C library support
// =====================================================================

#ifdef _WIN32
using DynamicLibraryHandle = HMODULE;
#else
using DynamicLibraryHandle = void*;
#endif

enum class DynamicLookupStatus {
    Success,
    OpenFailure,
    InitFailure
};

struct DynamicLookupResult {
    DynamicLookupStatus status;
    Function* function;
    Str message;
    bool linkedOnly;
};

static std::unordered_map<Str, DynamicLibraryHandle>& loadedDynamicLibraries() {
    static std::unordered_map<Str, DynamicLibraryHandle> libraries;
    return libraries;
}

#ifdef _WIN32
static Str lastDynamicLibraryError() {
    DWORD err = GetLastError();
    if (err == 0) {
        return "unknown dynamic library error";
    }

    LPSTR buffer = nullptr;
    DWORD len = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr);

    Str message = (len != 0 && buffer != nullptr)
        ? Str(buffer, static_cast<usize>(len))
        : ("Windows error " + std::to_string(err));

    if (buffer) {
        LocalFree(buffer);
    }

    while (!message.empty() &&
           (message.back() == '\r' || message.back() == '\n')) {
        message.pop_back();
    }
    return message;
}
#else
static Str lastDynamicLibraryError() {
    const char* err = dlerror();
    return err ? Str(err) : Str("unknown dynamic library error");
}
#endif

static bool loadDynamicLibrary(const Str& filename,
                               DynamicLibraryHandle& handle,
                               Str& errorMessage) {
    if (filename.empty()) {
        errorMessage = "empty dynamic library path";
        return false;
    }

    Str key = fullPath(filename);
    auto& libraries = loadedDynamicLibraries();
    auto existing = libraries.find(key);
    if (existing != libraries.end()) {
        handle = existing->second;
        return true;
    }

#ifdef _WIN32
    if (isCurrentExecutablePath(filename)) {
        handle = GetModuleHandleA(nullptr);
    } else {
        handle = LoadLibraryA(filename.c_str());
    }
#else
    dlerror();
    if (isCurrentExecutablePath(filename)) {
        handle = dlopen(nullptr, RTLD_NOW | RTLD_GLOBAL);
    } else {
        handle = dlopen(filename.c_str(), RTLD_NOW | RTLD_GLOBAL);
    }
#endif

    if (!handle) {
        errorMessage = lastDynamicLibraryError();
        return false;
    }

    libraries.emplace(key, handle);
    return true;
}

static void* loadDynamicSymbol(DynamicLibraryHandle handle,
                               const Str& symbolName,
                               Str& errorMessage) {
#ifdef _WIN32
    FARPROC proc = GetProcAddress(handle, symbolName.c_str());
    if (!proc) {
        errorMessage = lastDynamicLibraryError();
        return nullptr;
    }
    return reinterpret_cast<void*>(proc);
#else
    dlerror();
    void* symbol = dlsym(handle, symbolName.c_str());
    const char* err = dlerror();
    if (err != nullptr) {
        errorMessage = err;
        return nullptr;
    }
    return symbol;
#endif
}

static Function* createDynamicCFunction(LuaState* L, void* symbol) {
    CFunction cfunc = reinterpret_cast<CFunction>(symbol);
    Function* func = new Function(cfunc);
    L->getGlobalState().getGC().registerObject(func);
    return func;
}

static DynamicLookupResult lookForDynamicFunction(LuaState* L,
                                                  const Str& filename,
                                                  const Str& functionName) {
    DynamicLibraryHandle handle = nullptr;
    Str errorMessage;
    if (!loadDynamicLibrary(filename, handle, errorMessage)) {
        return { DynamicLookupStatus::OpenFailure, nullptr, errorMessage, false };
    }

    if (functionName == "*") {
        return { DynamicLookupStatus::Success, nullptr, Str(), true };
    }

    void* symbol = loadDynamicSymbol(handle, functionName, errorMessage);
    if (!symbol) {
        return { DynamicLookupStatus::InitFailure, nullptr, errorMessage, false };
    }

    return {
        DynamicLookupStatus::Success,
        createDynamicCFunction(L, symbol),
        Str(),
        false
    };
}

static Str moduleNameToOpenFunction(const Str& modname) {
    Str name = modname;
    usize mark = name.find(LUA_IGMARK);
    if (mark != Str::npos) {
        name = name.substr(mark + std::strlen(LUA_IGMARK));
    }

    return "luaopen_" + replaceAll(name, ".", LUA_OFSEP);
}

[[noreturn]] static void dynamicLoadError(LuaState* L,
                                          const Str& modname,
                                          const Str& filename,
                                          const Str& message) {
    Str error = "error loading module '" + modname + "' from file '" +
                filename + "':\n\t" + message;
    L->error(error.c_str());
}

static void pushSearchError(LuaState* L, const Str& message) {
    L->setTop(0);
    L->pushString(L->getGlobalState().getStringPool().intern(message.c_str()));
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
// package.loaders[3] — C library searcher
// =====================================================================

i32 loader_clib(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1 || !L->at(1).isString()) {
        pushSearchError(L, "\n\tno C module name");
        return 1;
    }

    Str modname = L->at(1).asString()->c_str();

    Str pathStr = getPackageStringField(L, "cpath");
    if (pathStr.empty()) {
        pushSearchError(L, "\n\tno field package.cpath");
        return 1;
    }

    Str errorBuf;
    Str filename = searchPath(modname, pathStr, errorBuf);
    if (filename.empty()) {
        pushSearchError(L, errorBuf);
        return 1;
    }

    Str functionName = moduleNameToOpenFunction(modname);
    DynamicLookupResult lookup = lookForDynamicFunction(L, filename, functionName);
    if (lookup.status == DynamicLookupStatus::Success && lookup.function) {
        L->setTop(0);
        L->pushValue(Value(lookup.function));
        return 1;
    }

    dynamicLoadError(L, modname, filename, lookup.message);
}

// =====================================================================
// package.loaders[4] — all-in-one C library searcher
// =====================================================================

i32 loader_clib_allinone(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1 || !L->at(1).isString()) {
        L->setTop(0);
        return 0;
    }

    Str modname = L->at(1).asString()->c_str();
    usize dot = modname.find('.');
    if (dot == Str::npos || dot == 0) {
        L->setTop(0);
        return 0;
    }

    Str rootname = modname.substr(0, dot);
    Str pathStr = getPackageStringField(L, "cpath");
    if (pathStr.empty()) {
        pushSearchError(L, "\n\tno field package.cpath");
        return 1;
    }

    Str errorBuf;
    Str filename = searchPath(rootname, pathStr, errorBuf);
    if (filename.empty()) {
        pushSearchError(L, errorBuf);
        return 1;
    }

    Str functionName = moduleNameToOpenFunction(modname);
    DynamicLookupResult lookup = lookForDynamicFunction(L, filename, functionName);
    if (lookup.status == DynamicLookupStatus::Success && lookup.function) {
        L->setTop(0);
        L->pushValue(Value(lookup.function));
        return 1;
    }

    if (lookup.status == DynamicLookupStatus::InitFailure) {
        Str msg = "\n\tno module '" + modname + "' in file '" + filename + "'";
        pushSearchError(L, msg);
        return 1;
    }

    dynamicLoadError(L, modname, filename, lookup.message);
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
    Vec<Value> options;
    options.reserve(nargs > 1 ? static_cast<usize>(nargs - 1) : 0);
    for (i32 i = 2; i <= nargs; ++i) {
        options.push_back(L->at(i));
    }

    // 1. Check package.loaded[modname]; create/reuse global module path if absent
    Table* loaded = getPackageSubTable(L, "loaded");
    if (!loaded) {
        L->error("'package.loaded' table is missing");
    }

    GCString* modKey = pool.intern(modname.c_str());
    Value existingVal = loaded->get(Value(modKey));

    Table* modTable = nullptr;
    if (existingVal.isTable()) {
        modTable = existingVal.asTable();
        setGlobalModulePath(L, modname, modTable);
    } else {
        modTable = findOrCreateGlobalModuleTable(L, modname);

        // Store it in package.loaded
        loaded->set(Value(modKey), Value(modTable));
    }

    // 2. Set _NAME, _M, and _PACKAGE fields
    GCString* nameKey = pool.intern("_NAME");
    modTable->set(Value(nameKey), Value(modKey));

    GCString* mKey = pool.intern("_M");
    modTable->set(Value(mKey), Value(modTable));

    Str packagePrefix = modulePackagePrefix(modname);
    GCString* packageKey = pool.intern("_PACKAGE");
    GCString* packageVal = pool.intern(packagePrefix.c_str());
    modTable->set(Value(packageKey), Value(packageVal));

    // 3. Switch the calling Lua function to the module environment
    setCallingLuaFunctionEnv(L, modTable);

    // 4. Apply option functions (e.g., package.seeall)
    for (const Value& optVal : options) {
        callModuleOption(L, optVal, modTable);
    }

    L->setTop(0);
    return 0;
}

// =====================================================================
// package.loadlib(libname, funcname) — dynamic C library loading
// =====================================================================

i32 luaP_loadlib(LuaState* L) {
    auto& pool = L->getGlobalState().getStringPool();

    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("bad argument #1 to 'package.loadlib' (string expected, got no value)");
    }
    if (!L->at(1).isString()) {
        L->error("bad argument #1 to 'package.loadlib' (string expected)");
    }
    if (nargs < 2) {
        L->error("bad argument #2 to 'package.loadlib' (string expected, got no value)");
    }
    if (!L->at(2).isString()) {
        L->error("bad argument #2 to 'package.loadlib' (string expected)");
    }

    Str filename = L->at(1).asString()->c_str();
    Str functionName = L->at(2).asString()->c_str();

    DynamicLookupResult lookup = lookForDynamicFunction(L, filename, functionName);
    if (lookup.status == DynamicLookupStatus::Success) {
        L->setTop(0);
        if (lookup.linkedOnly) {
            L->pushBoolean(true);
        } else {
            L->pushValue(Value(lookup.function));
        }
        return 1;
    }

    L->setTop(0);
    L->pushNil();
    L->pushString(pool.intern(lookup.message.c_str()));
    L->pushString(pool.intern(
        lookup.status == DynamicLookupStatus::OpenFailure ? "open" : "init"));
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

    // loader[3] = C library searcher
    Function* clibSearcher = new Function(loader_clib);
    gc.registerObject(clibSearcher);
    loadersTable->set(Value(3.0), Value(clibSearcher));

    // loader[4] = all-in-one C library searcher
    Function* allInOneSearcher = new Function(loader_clib_allinone);
    gc.registerObject(allInOneSearcher);
    loadersTable->set(Value(4.0), Value(allInOneSearcher));
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
