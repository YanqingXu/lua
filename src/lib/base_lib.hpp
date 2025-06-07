#pragma once

#include "../types.hpp"
#include "../vm/state.hpp"

namespace Lua {
    // Register base library
    void registerBaseLib(State* state);
    
    // Built-in functions
    Value print(State* state, int nargs);
    Value tonumber(State* state, int nargs);
    Value tostring(State* state, int nargs);
    Value type(State* state, int nargs);
    Value ipairs(State* state, int nargs);
    Value pairs(State* state, int nargs);
}
