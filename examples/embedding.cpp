#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <iostream>

namespace {

int hostDouble(lua_State* state) {
    const lua_Number value = luaL_checknumber(state, 1);
    lua_pushnumber(state, value * 2);
    return 1;
}

} // namespace

int main() {
    lua_State* state = lua_open();
    if (state == nullptr) {
        std::cerr << "could not create Lua state\n";
        return 1;
    }

    luaL_openlibs(state);
    lua_pushcclosure(state, hostDouble, 0);
    lua_setglobal(state, "host_double");

    constexpr const char* script = "return host_double(21)";
    int status = luaL_loadbuffer(state, script, sizeof("return host_double(21)") - 1, "=embedding");
    if (status == LUA_OK) {
        status = lua_pcall(state, 0, 1, 0);
    }

    if (status != LUA_OK) {
        std::cerr << (lua_tostring(state, -1) != nullptr ? lua_tostring(state, -1) : "unknown Lua error") << '\n';
        lua_close(state);
        return 1;
    }

    const lua_Number result = lua_tonumber(state, -1);
    lua_close(state);
    if (result != 42) {
        std::cerr << "unexpected embedding result\n";
        return 1;
    }

    std::cout << "embedding result: 42\n";
    return 0;
}
