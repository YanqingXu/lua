#pragma once

#include "lib/lib_module.hpp"

namespace Lua {

class TestLibModule : public LibModule {
public:
    const char* getName() const override { return "T"; }

    void registerFunctions(LuaState* L) override;
    void initialize(LuaState* L) override;
};

void openTestLib(LuaState* L);

} // namespace Lua
