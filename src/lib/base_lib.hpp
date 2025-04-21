#pragma once

#include "../types.hpp"
#include "../vm/state.hpp"

namespace Lua {
    // 注册基础库
    void registerBaseLib(State* state);
    
    // 内置函数
    Value print(State* state, int nargs);
    Value tonumber(State* state, int nargs);
    Value tostring(State* state, int nargs);
    Value type(State* state, int nargs);
}
