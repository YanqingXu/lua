#pragma once

// Generated build systems define this explicitly. Hand-maintained Visual
// Studio projects retain the full developer feature set by default.
#ifndef LUA_CPP_ENABLE_DEBUGGER
#define LUA_CPP_ENABLE_DEBUGGER 1
#endif

#if LUA_CPP_ENABLE_DEBUGGER != 0 && LUA_CPP_ENABLE_DEBUGGER != 1
#error "LUA_CPP_ENABLE_DEBUGGER must be 0 or 1"
#endif
