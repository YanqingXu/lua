#pragma once

#include "../common/types.hpp"

// Debug configuration - disabled for production
#define DEBUG_OUTPUT 0
#define DEBUG_LIB_REGISTRATION 0
#define DEBUG_VM_EXECUTION 0
#define DEBUG_STATE_OPERATIONS 0
#define DEBUG_COMPILER 0
#define DEBUG_VM_INSTRUCTIONS 0
#define DEBUG_REGISTER_ALLOCATION 0

// Debug macros - disabled for production
#define DEBUG_PRINT(msg) do { } while(0)
#define DEBUG_PRINT_VAR(name, value) do { } while(0)

namespace Lua {
    // Constant definitions
    constexpr int LUAI_MAXSTACK = 1000;      // Maximum stack size
    constexpr int LUAI_MAXCALLS = 200;       // Maximum call depth
    constexpr int LUAI_MAXCCALLS = 200;      // Maximum C call depth
    constexpr int LUAI_MAXVARS = 200;        // Maximum local variable count
    constexpr int LUAI_MAXUPVALUES = 60;     // Maximum upvalue count
    
    // Enhanced boundary limits for closure handling
    constexpr u8 MAX_UPVALUES_PER_CLOSURE = 255;    // Maximum upvalues per closure
    constexpr u8 MAX_FUNCTION_NESTING_DEPTH = 200;  // Maximum function nesting depth
    constexpr u32 MAX_CLOSURE_MEMORY_SIZE = 1024 * 1024; // 1MB limit per closure
    
    // Error messages for boundary violations
    constexpr const char* ERR_TOO_MANY_UPVALUES = "Too many upvalues in closure";
    constexpr const char* ERR_NESTING_TOO_DEEP = "Function nesting too deep";
    constexpr const char* ERR_MEMORY_EXHAUSTED = "Memory exhausted during closure creation";
    constexpr const char* ERR_INVALID_UPVALUE_INDEX = "Invalid upvalue index";
    constexpr const char* ERR_DESTROYED_UPVALUE = "Accessing destroyed upvalue";
    
    // String constants
    constexpr const char* LUA_VERSION = "Lua 5.1.1";
    constexpr const char* LUA_COPYRIGHT = "Copyright (c) 2025 YanqingXu";
}
