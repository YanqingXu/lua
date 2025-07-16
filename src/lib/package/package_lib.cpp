#include "package_lib.hpp"
#include "file_utils.hpp"
#include "../core/lib_registry.hpp"
#include "../../vm/table.hpp"
#include "../../vm/function.hpp"
#include "../../parser/parser.hpp"
#include "../../compiler/compiler.hpp"
#include "../../common/defines.hpp"
#include <iostream>
#include <stdexcept>

namespace Lua {

// ===================================================================
// PackageLib Implementation
// ===================================================================

void PackageLib::registerFunctions(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Register global functions
    REGISTER_GLOBAL_FUNCTION(state, require, require);
    REGISTER_GLOBAL_FUNCTION(state, loadfile, loadfile);
    REGISTER_GLOBAL_FUNCTION(state, dofile, dofile);

    // Create package table
    Value packageTable = LibRegistry::createLibTable(state, "package");

    // Register package table functions
    REGISTER_TABLE_FUNCTION(state, packageTable, searchpath, searchpath);
}

void PackageLib::initialize(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Get package table
    Value packageTable = state->getGlobal("package");
    if (!packageTable.isTable()) {
        throw LuaException("package table not found during initialization");
    }

    auto table = packageTable.asTable();

    // Initialize package.path with default search paths
    table->set(Value("path"), Value(DEFAULT_PACKAGE_PATH));

    // Initialize package.cpath (for C modules, empty for now)
    table->set(Value("cpath"), Value(""));

    // Initialize package.loaded table
    auto loadedTable = GCRef<Table>(new Table());
    table->set(Value("loaded"), Value(loadedTable));

    // Initialize package.preload table
    auto preloadTable = GCRef<Table>(new Table());
    table->set(Value("preload"), Value(preloadTable));

    // Initialize package.loaders array (Lua 5.1 name)
    auto loadersArray = GCRef<Table>(new Table());
    
    // Add default searchers to package.loaders
    // 1. Preload searcher
    loadersArray->set(Value(1), Value(Function::createNative([](State* s, int n) -> Value {
        return searcherPreload(s, static_cast<i32>(n));
    })));
    
    // 2. Lua file searcher
    loadersArray->set(Value(2), Value(Function::createNative([](State* s, int n) -> Value {
        return searcherLua(s, static_cast<i32>(n));
    })));

    table->set(Value("loaders"), Value(loadersArray));

    // Set up package.loaded entries for existing standard libraries
    setupStandardLibraryEntries(state, loadedTable);
}

void PackageLib::setupStandardLibraryEntries(State* state, GCRef<Table> loadedTable) {
    if (!state || !loadedTable) {
        return;
    }

    // Mark core libraries as loaded
    Value globalTable = state->getGlobal("_G");
    if (!globalTable.isNil()) {
        loadedTable->set(Value("_G"), globalTable);
    }

    // Mark standard libraries as loaded
    const char* stdLibs[] = {"string", "table", "math", "io", "os", "debug"};
    for (const char* libName : stdLibs) {
        Value lib = state->getGlobal(libName);
        if (!lib.isNil()) {
            loadedTable->set(Value(libName), lib);
        }
    }
}

// ===================================================================
// Core Package Functions Implementation
// ===================================================================

Value PackageLib::require(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        throw LuaException("require: module name expected");
    }

    // Get first argument using correct stack indexing
    // Arguments are at positions [top-nargs, top-nargs+1, ..., top-1]
    int stackBase = state->getTop() - nargs;
    Value modnameVal = state->get(stackBase);
    if (!modnameVal.isString()) {
        throw LuaException("require: module name must be a string");
    }

    Str modname = modnameVal.toString();

    // Check for circular dependency FIRST
    if (checkCircularDependency(state, modname)) {
        throw LuaException("require: circular dependency detected for module '" + modname + "'");
    }

    // Check if module is already loaded (but not loading markers)
    Value loadedTable = getLoadedTable(state);
    Value cached = loadedTable.asTable()->get(Value(modname));

    // Only return cached value if it's not a loading marker
    if (!cached.isNil() && !cached.isBoolean()) {
        return cached; // Return cached module
    }

    // Mark module as loading
    markModuleLoading(state, modname);

    try {
        // Find and load module
        Value result = findModule(state, modname);

        // Cache the result
        loadedTable.asTable()->set(Value(modname), result);

        // Unmark module as loading
        unmarkModuleLoading(state, modname);

        return result;
    } catch (...) {
        // Unmark module as loading on error
        unmarkModuleLoading(state, modname);
        throw; // Re-throw the exception
    }
}

Value PackageLib::loadfile(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    // Get first argument using correct stack indexing
    int stackBase = state->getTop() - nargs;
    Value filenameVal = state->get(stackBase);
    if (!filenameVal.isString()) {
        return Value(); // nil - invalid filename
    }

    Str filename = filenameVal.toString();

    try {
        // Check if file exists
        if (!FileUtils::fileExists(filename)) {
            return Value(); // nil - file not found
        }

        // Read file content
        Str source = FileUtils::readFile(filename);

        // Parse the source code
        Parser parser(source);
        auto statements = parser.parse();

        if (parser.hasError()) {
            return Value(); // nil - parse error
        }

        // Compile to bytecode
        Compiler compiler;
        GCRef<Function> function = compiler.compile(statements);

        if (!function) {
            return Value(); // nil - compilation error
        }

        return Value(function);
    } catch (const std::exception& e) {
        // Return nil on any error (loadfile doesn't throw)

        std::cerr << "Error loading file '" << filename << "': " << e.what() << std::endl;
        return Value();
    }
}

Value PackageLib::dofile(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        throw LuaException("dofile: filename expected");
    }

    // Get first argument using correct stack indexing
    int stackBase = state->getTop() - nargs;
    Value filenameVal = state->get(stackBase);
    if (!filenameVal.isString()) {
        throw LuaException("dofile: filename must be a string");
    }

    Str filename = filenameVal.toString();

    // Use loadfile to compile the file
    state->push(filenameVal);
    Value function = loadfile(state, 1);
    state->pop(); // Remove filename from stack

    if (function.isNil()) {
        throw LuaException("dofile: cannot load file '" + filename + "'");
    }

    // Execute the function
    Vec<Value> args; // No arguments for dofile
    return state->call(function, args);
}

// ===================================================================
// Package Table Functions Implementation
// ===================================================================

Value PackageLib::searchpath(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 2) {
        throw LuaException("package.searchpath: name and path expected");
    }

    // Get arguments using correct stack indexing
    int stackBase = state->getTop() - nargs;
    Value nameVal = state->get(stackBase);
    Value pathVal = state->get(stackBase + 1);

    if (!nameVal.isString()) {
        throw LuaException("package.searchpath: name must be a string");
    }
    if (!pathVal.isString()) {
        throw LuaException("package.searchpath: path must be a string");
    }

    Str name = nameVal.toString();
    Str path = pathVal.toString();

    // Optional separator and replacement arguments
    Str sep = ".";
    Str rep = "/";

    if (nargs >= 3) {
        Value sepVal = state->get(stackBase + 2);
        if (sepVal.isString()) {
            sep = sepVal.toString();
        }
    }

    if (nargs >= 4) {
        Value repVal = state->get(stackBase + 3);
        if (repVal.isString()) {
            rep = repVal.toString();
        }
    }

    // Search for the file and collect attempted paths
    Vec<Str> attemptedPaths;
    Str result = FileUtils::findModuleFileWithPaths(name, path, sep, rep, attemptedPaths);

    if (result.empty()) {
        // Return nil for now (Lua 5.1 should return nil, error_message)
        // TODO: Implement proper multiple return values
        return Value(); // nil
    }

    return Value(result);
}

// ===================================================================
// Internal Helper Functions Implementation
// ===================================================================

Value PackageLib::findModule(State* state, const Str& modname) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Get package.loaders array
    Value loadersArray = getLoadersArray(state);
    auto loaders = loadersArray.asTable();

    // Try each searcher in order
    for (int i = 1; ; ++i) {
        Value searcher = loaders->get(Value(i));
        if (searcher.isNil()) {
            break; // No more searchers
        }

        if (!searcher.isFunction()) {
            continue; // Skip invalid searchers
        }

        // Call searcher with module name
        Vec<Value> args = {Value(modname)};
        Value result = state->call(searcher, args);

        if (!result.isNil()) {
            // Searcher found something
            if (result.isFunction()) {
                // Call the loader function
                Vec<Value> loaderArgs = {Value(modname)};
                return state->call(result, loaderArgs);
            } else {
                // Direct result
                return result;
            }
        }
    }

    // Module not found - build detailed error message
    Str errorMsg = "module '" + modname + "' not found:";

    // Add information about attempted search paths
    Value packagePath = getPackagePath(state);
    if (packagePath.isString()) {
        Vec<Str> attemptedPaths;
        FileUtils::findModuleFileWithPaths(modname, packagePath.toString(), ".", "/", attemptedPaths);

        for (const Str& path : attemptedPaths) {
            errorMsg += "\n\tno file '" + path + "'";
        }
    }

    throw LuaException(errorMsg);
}

Value PackageLib::loadLuaModule(State* state, const Str& filename, const Str& modname) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    try {
        // Read file content
        Str source = FileUtils::readFile(filename);

        // Parse the source code
        Parser parser(source);
        auto statements = parser.parse();

        if (parser.hasError()) {
            throw LuaException("syntax error in module '" + modname + "' (file: " + filename + ")");
        }

        // Compile to bytecode
        Compiler compiler;
        GCRef<Function> function = compiler.compile(statements);

        if (!function) {
            throw LuaException("compilation error in module '" + modname + "' (file: " + filename + ")");
        }

        // Execute the module
        Vec<Value> args; // No arguments for module execution
        Value result = state->call(Value(function), args);

        // If module doesn't return anything, return true
        if (result.isNil()) {
            result = Value(true);
        }

        return result;
    } catch (const std::exception& e) {
        throw LuaException("error loading module '" + modname + "' from '" + filename + "': " + e.what());
    }
}

bool PackageLib::checkCircularDependency(State* state, const Str& modname) {
    if (!state) {
        return false;
    }

    // Check if module is marked as loading
    Value loadedTable = getLoadedTable(state);
    Str marker = LOADING_MARKER_PREFIX + modname;
    Value loadingMarker = loadedTable.asTable()->get(Value(marker));

    return !loadingMarker.isNil();
}

void PackageLib::markModuleLoading(State* state, const Str& modname) {
    if (!state) {
        return;
    }

    Value loadedTable = getLoadedTable(state);
    Str marker = LOADING_MARKER_PREFIX + modname;
    loadedTable.asTable()->set(Value(marker), Value(true));
}

void PackageLib::unmarkModuleLoading(State* state, const Str& modname) {
    if (!state) {
        return;
    }

    Value loadedTable = getLoadedTable(state);
    Str marker = LOADING_MARKER_PREFIX + modname;
    loadedTable.asTable()->set(Value(marker), Value());
}

// ===================================================================
// Package Searcher Functions Implementation
// ===================================================================

Value PackageLib::searcherPreload(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    // Get first argument using correct stack indexing
    int stackBase = state->getTop() - nargs;
    Value modnameVal = state->get(stackBase);
    if (!modnameVal.isString()) {
        return Value(); // nil - invalid module name
    }

    Str modname = modnameVal.toString();

    // Check package.preload table
    Value preloadTable = getPreloadTable(state);
    Value loader = preloadTable.asTable()->get(Value(modname));

    if (!loader.isNil() && loader.isFunction()) {
        return loader; // Return preloaded function
    }

    return Value(); // nil - not found in preload
}

Value PackageLib::searcherLua(State* state, i32 nargs) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    if (nargs < 1) {
        return Value(); // nil - insufficient arguments
    }

    // Get first argument using correct stack indexing
    int stackBase = state->getTop() - nargs;
    Value modnameVal = state->get(stackBase);
    if (!modnameVal.isString()) {
        return Value(); // nil - invalid module name
    }

    Str modname = modnameVal.toString();

    // Get package.path
    Value packagePath = getPackagePath(state);
    if (!packagePath.isString()) {
        return Value(); // nil - no package.path
    }

    // Search for module file
    Str filename = FileUtils::findModuleFile(modname, packagePath.toString());

    if (filename.empty()) {
        return Value(); // nil - file not found
    }

    // Create loader function that loads the file
    auto loaderFn = [filename, modname](State* s, int) -> Value {
        return loadLuaModule(s, filename, modname);
    };

    return Value(Function::createNative(loaderFn));
}

// ===================================================================
// Utility Functions Implementation
// ===================================================================

Value PackageLib::getPackageTable(State* state) {
    if (!state) {
        throw LuaException("State cannot be null");
    }

    Value packageTable = state->getGlobal("package");
    if (!packageTable.isTable()) {
        throw LuaException("package table not found");
    }

    return packageTable;
}

Value PackageLib::getLoadedTable(State* state) {
    Value packageTable = getPackageTable(state);
    Value loadedTable = packageTable.asTable()->get(Value("loaded"));

    if (!loadedTable.isTable()) {
        throw LuaException("package.loaded table not found");
    }

    return loadedTable;
}

Value PackageLib::getPreloadTable(State* state) {
    Value packageTable = getPackageTable(state);
    Value preloadTable = packageTable.asTable()->get(Value("preload"));

    if (!preloadTable.isTable()) {
        throw LuaException("package.preload table not found");
    }

    return preloadTable;
}

Value PackageLib::getLoadersArray(State* state) {
    Value packageTable = getPackageTable(state);
    Value loadersArray = packageTable.asTable()->get(Value("loaders"));

    if (!loadersArray.isTable()) {
        throw LuaException("package.loaders array not found");
    }

    return loadersArray;
}

Value PackageLib::getPackagePath(State* state) {
    Value packageTable = getPackageTable(state);
    Value packagePath = packageTable.asTable()->get(Value("path"));

    if (!packagePath.isString()) {
        throw LuaException("package.path not found or not a string");
    }

    return packagePath;
}

void initializePackageLib(State* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    PackageLib packageLib;
    packageLib.registerFunctions(state);
    packageLib.initialize(state);
}

} // namespace Lua
