#pragma once

#include "common/types.hpp"

namespace Lua {

class LuaState;

/// 标准库C函数签名
using LibCFunction = i32 (*)(LuaState*);

struct LibFunctionEntry {
    const char* name;
    LibCFunction func;
};

class LibModule {
public:
    virtual ~LibModule() = default;

    virtual const char* getName() const = 0;

    virtual void registerFunctions(LuaState* L) = 0;

    virtual void initialize(LuaState* L) {
        (void)L;
    }
};

} // namespace Lua
