#pragma once

#include "../types.hpp"

namespace Lua {
    // 常量定义
    constexpr int LUAI_MAXSTACK = 1000;      // 最大栈大小
    constexpr int LUAI_MAXCALLS = 200;       // 最大调用深度
    constexpr int LUAI_MAXCCALLS = 200;      // 最大C调用深度
    constexpr int LUAI_MAXVARS = 200;        // 最大局部变量数量
    constexpr int LUAI_MAXUPVALUES = 60;     // 最大上值数量
    
    // 字符串常量
    constexpr const char* LUA_VERSION = "Lua 5.1.1";
    constexpr const char* LUA_COPYRIGHT = "Copyright (C) 1994-2006 Lua.org, PUC-Rio";
}
