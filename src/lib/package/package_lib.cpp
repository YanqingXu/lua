#include "package_lib.hpp"
#include "file_utils.hpp"
#include "../core/lib_registry.hpp"
#include "../core/multi_return_helper.hpp"
#include "../../vm/table.hpp"
#include "../../vm/function.hpp"
#include "../../gc/core/gc_string.hpp"
#include "../../parser/parser.hpp"
#include "../../compiler/compiler.hpp"
#include "../../common/defines.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>

namespace Lua {

// ===================================================================
// PackageLib Implementation
// ===================================================================

void PackageLib::registerFunctions(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Register multi-return functions using new mechanism
    LibRegistry::registerGlobalFunction(state, "loadfile", loadfile);

    // Register legacy single-return functions
    LibRegistry::registerGlobalFunctionLegacy(state, "require", require);
    LibRegistry::registerGlobalFunctionLegacy(state, "dofile", dofile);

    // Create package table
    Value packageTable = LibRegistry::createLibTable(state, "package");

    // Register package table functions
    LibRegistry::registerTableFunction(state, packageTable, "searchpath", searchpath);
}

void PackageLib::initialize(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    // Get package table
    auto packageStr = GCString::create("package");
    Value packageTable = state->getGlobal(packageStr);
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
    // 1. Preload searcher (legacy wrapper)
    loadersArray->set(Value(1), Value(Function::createNativeLegacy([](LuaState* s, i32 n) -> Value {
        return searcherPreload(s, n);
    })));

    // 2. Lua file searcher (legacy wrapper)
    loadersArray->set(Value(2), Value(Function::createNativeLegacy([](LuaState* s, i32 n) -> Value {
        return searcherLua(s, n);
    })));

    table->set(Value("loaders"), Value(loadersArray));

    // Set up package.loaded entries for existing standard libraries
    setupStandardLibraryEntries(state, loadedTable);
}

void PackageLib::setupStandardLibraryEntries(LuaState* state, GCRef<Table> loadedTable) {
    if (!state || !loadedTable) {
        return;
    }

    // Mark core libraries as loaded
    auto globalStr = GCString::create("_G");
    Value globalTable = state->getGlobal(globalStr);
    if (!globalTable.isNil()) {
        loadedTable->set(Value("_G"), globalTable);
    }

    // Mark standard libraries as loaded
    const char* stdLibs[] = {"string", "table", "math", "io", "os", "debug"};
    for (const char* libName : stdLibs) {
        auto libStr = GCString::create(libName);
        Value lib = state->getGlobal(libStr);
        if (!lib.isNil()) {
            auto nameStr = GCString::create(libName);
            loadedTable->set(Value(nameStr), lib);
        }
    }
}

// ===================================================================
// Core Package Functions Implementation
// ===================================================================

Value PackageLib::require(LuaState* state, i32 nargs) {
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

// New Lua 5.1 standard loadfile implementation (multi-return)
i32 PackageLib::loadfile(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    int nargs = state->getTop();
    if (nargs < 1) {
        // Clear arguments and push error result
        state->clearStack();
        state->push(Value());  // nil
        state->push(Value("loadfile: filename expected"));  // error message
        return 2;
    }

    // Get filename argument (now in clean stack environment)
    Value filenameVal = state->get(0);
    if (!filenameVal.isString()) {
        // Clear arguments and push error result
        state->clearStack();
        state->push(Value());  // nil
        state->push(Value("loadfile: filename must be a string"));  // error message
        return 2;
    }

    Str filename = filenameVal.asString();

    try {
        // Try to load and compile the file
        if (!FileUtils::fileExists(filename)) {
            // Clear arguments and push error result
            state->clearStack();
            state->push(Value());  // nil
            state->push(Value("loadfile: file not found: " + filename));  // error message
            return 2;
        }

        Str source = FileUtils::readFile(filename);

        // Parse and compile the source
        Parser parser(source);
        auto ast = parser.parse();

        Compiler compiler;
        auto function = compiler.compile(ast);

        // Clear arguments and push success result
        state->clearStack();
        state->push(Value(function));  // compiled function
        return 1;  // Return 1 value on success

    } catch (const std::exception& e) {
        // Clear arguments and push error result
        state->clearStack();
        state->push(Value());  // nil
        state->push(Value(Str("loadfile: ") + e.what()));  // error message
        return 2;
    }
}



Value PackageLib::dofile(LuaState* state, i32 nargs) {
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
    state->clearStack();
    state->push(filenameVal);
    i32 returnCount = loadfile(state);

    if (returnCount != 1) {
        // loadfile returned error (nil, error_message)
        Value errorMsg = state->get(1);
        throw LuaException("dofile: " + errorMsg.asString());
    }

    Value function = state->get(0);
    if (function.isNil()) {
        throw LuaException("dofile: cannot load file '" + filename + "'");
    }

    // Execute the function
    Vec<Value> args; // No arguments for dofile
    return state->callFunction(function, args);
}

// ===================================================================
// Package Table Functions Implementation
// ===================================================================

// New Lua 5.1 standard searchpath implementation (multi-return)
i32 PackageLib::searchpath(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    int nargs = state->getTop();
    if (nargs < 2) {
        // Clear arguments and push error result
        state->clearStack();
        state->push(Value());  // nil
        state->push(Value("package.searchpath: name and path expected"));  // error message
        return 2;
    }

    // Get arguments (now in clean stack environment)
    Value nameVal = state->get(0);      // First argument
    Value pathVal = state->get(1);      // Second argument

    if (!nameVal.isString()) {
        // Clear arguments and push error result
        state->clearStack();
        state->push(Value());  // nil
        state->push(Value("package.searchpath: name must be a string"));  // error message
        return 2;
    }
    if (!pathVal.isString()) {
        // Clear arguments and push error result
        state->clearStack();
        state->push(Value());  // nil
        state->push(Value("package.searchpath: path must be a string"));  // error message
        return 2;
    }

    Str name = nameVal.asString();
    Str path = pathVal.asString();

    // Optional separator and replacement arguments
    Str sep = ".";
    Str rep = "/";

    if (nargs >= 3) {
        Value sepVal = state->get(2);  // Third argument
        if (sepVal.isString()) {
            sep = sepVal.asString();
        }
    }

    if (nargs >= 4) {
        Value repVal = state->get(3);  // Fourth argument
        if (repVal.isString()) {
            rep = repVal.asString();
        }
    }

    try {
        // Convert module name to file path
        Str filename = name;
        // Replace separator with replacement character
        size_t pos = 0;
        while ((pos = filename.find(sep, pos)) != Str::npos) {
            filename.replace(pos, sep.length(), rep);
            pos += rep.length();
        }

        // Search through path patterns
        std::istringstream pathStream(path);
        Str pattern;
        Str errorMessages;

        while (std::getline(pathStream, pattern, ';')) {
            if (pattern.empty()) continue;

            // Replace ? with filename
            Str filepath = pattern;
            pos = filepath.find("?");
            if (pos != Str::npos) {
                filepath.replace(pos, 1, filename);
            }

            // Check if file exists
            if (FileUtils::fileExists(filepath)) {
                // Clear arguments and push success result
                state->clearStack();
                state->push(Value(filepath));  // found file path
                return 1;  // Return 1 value on success
            }

            // Accumulate error messages
            if (!errorMessages.empty()) {
                errorMessages += "\n\t";
            }
            errorMessages += "no file '" + filepath + "'";
        }

        // Clear arguments and push error result
        state->clearStack();
        state->push(Value());  // nil
        state->push(Value(errorMessages));  // error message
        return 2;

    } catch (const std::exception& e) {
        // Clear arguments and push error result
        state->clearStack();
        state->push(Value());  // nil
        state->push(Value(Str("package.searchpath: ") + e.what()));  // error message
        return 2;
    }
}



// ===================================================================
// Internal Helper Functions Implementation
// ===================================================================

Value PackageLib::findModule(LuaState* state, const Str& modname) {
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
        auto modnameStr = GCString::create(modname);
        Vec<Value> args = {Value(modnameStr)};
        Value result = state->callFunction(searcher, args);

        if (!result.isNil()) {
            // Searcher found something
            if (result.isFunction()) {
                // Call the loader function
                auto modnameStr2 = GCString::create(modname);
                Vec<Value> loaderArgs = {Value(modnameStr2)};
                return state->callFunction(result, loaderArgs);
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

Value PackageLib::loadLuaModule(LuaState* state, const Str& filename, const Str& modname) {
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
        Value result = state->callFunction(Value(function), args);

        // If module doesn't return anything, return true
        if (result.isNil()) {
            result = Value(true);
        }

        return result;
    } catch (const std::exception& e) {
        throw LuaException("error loading module '" + modname + "' from '" + filename + "': " + e.what());
    }
}

bool PackageLib::checkCircularDependency(LuaState* state, const Str& modname) {
    if (!state) {
        return false;
    }

    // Check if module is marked as loading
    Value loadedTable = getLoadedTable(state);
    Str marker = LOADING_MARKER_PREFIX + modname;
    Value loadingMarker = loadedTable.asTable()->get(Value(marker));

    return !loadingMarker.isNil();
}

void PackageLib::markModuleLoading(LuaState* state, const Str& modname) {
    if (!state) {
        return;
    }

    Value loadedTable = getLoadedTable(state);
    Str marker = LOADING_MARKER_PREFIX + modname;
    loadedTable.asTable()->set(Value(marker), Value(true));
}

void PackageLib::unmarkModuleLoading(LuaState* state, const Str& modname) {
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

Value PackageLib::searcherPreload(LuaState* state, i32 nargs) {
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

Value PackageLib::searcherLua(LuaState* state, i32 nargs) {
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

    // Create loader function that loads the file (legacy wrapper)
    auto loaderFn = [filename, modname](LuaState* s, i32) -> Value {
        return loadLuaModule(s, filename, modname);
    };

    return Value(Function::createNativeLegacy(loaderFn));
}

// ===================================================================
// Utility Functions Implementation
// ===================================================================

Value PackageLib::getPackageTable(LuaState* state) {
    if (!state) {
        throw LuaException("State cannot be null");
    }

    auto packageStr = GCString::create("package");
    Value packageTable = state->getGlobal(packageStr);
    if (!packageTable.isTable()) {
        throw LuaException("package table not found");
    }

    return packageTable;
}

Value PackageLib::getLoadedTable(LuaState* state) {
    Value packageTable = getPackageTable(state);
    Value loadedTable = packageTable.asTable()->get(Value("loaded"));

    if (!loadedTable.isTable()) {
        throw LuaException("package.loaded table not found");
    }

    return loadedTable;
}

Value PackageLib::getPreloadTable(LuaState* state) {
    Value packageTable = getPackageTable(state);
    Value preloadTable = packageTable.asTable()->get(Value("preload"));

    if (!preloadTable.isTable()) {
        throw LuaException("package.preload table not found");
    }

    return preloadTable;
}

Value PackageLib::getLoadersArray(LuaState* state) {
    Value packageTable = getPackageTable(state);
    Value loadersArray = packageTable.asTable()->get(Value("loaders"));

    if (!loadersArray.isTable()) {
        throw LuaException("package.loaders array not found");
    }

    return loadersArray;
}

Value PackageLib::getPackagePath(LuaState* state) {
    Value packageTable = getPackageTable(state);
    Value packagePath = packageTable.asTable()->get(Value("path"));

    if (!packagePath.isString()) {
        throw LuaException("package.path not found or not a string");
    }

    return packagePath;
}

void initializePackageLib(LuaState* state) {
    if (!state) {
        throw std::invalid_argument("State cannot be null");
    }

    PackageLib packageLib;
    packageLib.registerFunctions(state);
    packageLib.initialize(state);
}

} // namespace Lua
