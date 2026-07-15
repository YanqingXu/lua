#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"

#include <iostream>
#include <string>

namespace {

int hostDouble(lua_State* state) {
    const lua_Number value = luaL_checknumber(state, 1);
    lua_pushnumber(state, value * 2);
    return 1;
}

bool loadPublicNativeModule(lua_State* state, const char* modulePath) {
    lua_settop(state, 0);
    lua_getglobal(state, "package");
    lua_pushstring(state, "loadlib");
    lua_gettable(state, -2);
    lua_remove(state, -2);
    lua_pushstring(state, modulePath);
    lua_pushstring(state, "luaopen_publicfixture");
    if (lua_pcall(state, 2, 1, 0) != LUA_OK || !lua_isfunction(state, -1)) {
        return false;
    }

    lua_pushstring(state, "publicfixture");
    if (lua_pcall(state, 1, 1, 0) != LUA_OK || !lua_istable(state, -1)) {
        return false;
    }

    lua_pushstring(state, "source");
    lua_gettable(state, -2);
    const char* source = lua_tostring(state, -1);
    const bool valid = source != nullptr && std::string(source) == "public-lua-h-only";
    lua_settop(state, 0);
    return valid;
}

} // namespace

int main(int argc, char** argv) {
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
    if (result != 42) {
        std::cerr << "unexpected embedding result\n";
        lua_close(state);
        return 1;
    }

    if (argc == 2 && !loadPublicNativeModule(state, argv[1])) {
        std::cerr << "could not load the public native module\n";
        lua_close(state);
        return 1;
    }

    lua_close(state);
    std::cout << "embedding result: 42\n";
    return 0;
}
