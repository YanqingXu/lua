#pragma once

#include "../types.hpp"

namespace Lua {
    // Constant definitions
    constexpr int LUAI_MAXSTACK = 1000;      // Maximum stack size
    constexpr int LUAI_MAXCALLS = 200;       // Maximum call depth
    constexpr int LUAI_MAXCCALLS = 200;      // Maximum C call depth
    constexpr int LUAI_MAXVARS = 200;        // Maximum local variable count
    constexpr int LUAI_MAXUPVALUES = 60;     // Maximum upvalue count
    
    // String constants
    constexpr const char* LUA_VERSION = "Lua 5.1.1";
    constexpr const char* LUA_COPYRIGHT = "Copyright (c) 2025 YanqingXu";
}
