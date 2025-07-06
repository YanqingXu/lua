#pragma once

/**
 * Lua Standard Library Framework
 * 
 * This is the main header file for the Lua standard library framework.
 * It provides a unified interface to all standard library modules.
 * 
 * Usage:
 *   #include "lib/lua_lib.hpp"
 *   
 * Organization:
 *   - core/    : Core framework (LibContext, LibManager, etc.)
 *   - base/    : Base library (print, type, assert, etc.)
 *   - string/  : String library (string manipulation functions)
 *   - math/    : Math library (mathematical functions and constants)
 *   - table/   : Table library (table manipulation functions)
 *   - utils/   : Utilities (error handling, type conversion, etc.)
 * 
 * Architecture:
 *   Application Layer
 *   ↓
 *   Management Layer    (LibraryManager)
 *   ↓
 *   Framework Layer     (LibContext, LibFuncRegistry)
 *   ↓
 *   Module Layer        (BaseLib, StringLib, MathLib, TableLib)
 *   ↓
 *   Utility Layer       (Utils, ErrorHandling, TypeConversion)
 */

// Core framework
#include "core/core.hpp"

// Standard library modules
#include "base/base_lib.hpp"
#include "string/string.hpp"
#include "math/math.hpp"
#include "table/table.hpp"

// Utilities
#include "utils/utils.hpp"

namespace Lua::Lib {
    /**
     * Initialize the entire standard library framework
     * This is the recommended way to set up the library system
     */
    void initializeStandardLibrary(State* state, const LibContext& context = LibContext{});
    
    /**
     * Initialize only core modules (base library)
     * Useful for embedded or resource-constrained environments
     */
    void initializeCoreLibrary(State* state, const LibContext& context = LibContext{});
    
    /**
     * Clean up all library resources
     */
    void cleanupStandardLibrary(State* state);
    
    /**
     * Get version information for the standard library framework
     */
    struct VersionInfo {
        const char* version = "1.0.0";
        const char* buildDate = __DATE__ " " __TIME__;
        const char* luaVersion = "5.1.1";
        const char* framework = "Modern C++ Implementation";
    };
    
    VersionInfo getVersionInfo();
}
