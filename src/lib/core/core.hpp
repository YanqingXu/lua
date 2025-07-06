#pragma once

/**
 * Lua Library Core Framework
 * Provides the foundation for all standard library modules
 */

// Core framework components
#include "lib_define.hpp"
#include "lib_module.hpp"
#include "lib_context.hpp"
#include "lib_func_registry.hpp"
#include "lib_manager.hpp"

namespace Lua::Lib::Core {
    // Export core framework classes
    using LibModule = Lua::Lib::LibModule;
    using LibContext = Lua::Lib::LibContext;
    using LibFuncRegistry = Lua::Lib::LibFuncRegistry;
    using LibraryManager = Lua::Lib::LibraryManager;
    using FunctionMetadata = Lua::Lib::FunctionMetadata;
    
    // Export framework macros and definitions
    // (from lib_define.hpp)
}
