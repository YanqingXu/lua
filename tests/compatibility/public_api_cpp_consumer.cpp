#include "lua.h"
#include "lauxlib.h"

#include <stdexcept>
#include <string>

extern "C" int lua_public_c_header_probe(void);

namespace {

const char* throwingReader(lua_State*, void*, size_t*) {
    throw std::runtime_error("consumer reader failure");
}

} // namespace

int main() {
    static_assert(noexcept(lua_checkstack(nullptr, 0)));
    static_assert(noexcept(lua_pcall(nullptr, 0, 0, 0)));
    static_assert(noexcept(lua_newthread(nullptr)));
    static_assert(noexcept(lua_close(nullptr)));
    static_assert(noexcept(lua_resume(nullptr, 0)));
    static_assert(noexcept(lua_load(nullptr, nullptr, nullptr, nullptr)));
    static_assert(noexcept(lua_dump(nullptr, nullptr, nullptr)));
    static_assert(noexcept(luaL_loadbuffer(nullptr, nullptr, 0, nullptr)));
    static_assert(noexcept(luaL_loadstring(nullptr, nullptr)));
    static_assert(noexcept(luaL_loadfile(nullptr, nullptr)));
    static_assert(!noexcept(lua_call(nullptr, 0, 0)));
    static_assert(!noexcept(lua_error(nullptr)));

    if (lua_public_c_header_probe() != 60) {
        return 1;
    }

    lua_State* state = lua_open();
    if (state == nullptr) {
        return 2;
    }

    lua_pushstring(state, nullptr);
    if (!lua_isnil(state, -1)) {
        lua_close(state);
        return 3;
    }
    lua_pop(state, 1);

    lua_pushnumber(state, 12345);
    if (lua_objlen(state, -1) != 5 || lua_type(state, -1) != LUA_TSTRING) {
        lua_close(state);
        return 4;
    }
    lua_pop(state, 1);

    lua_pushnumber(state, 73);
    int loadStatus = LUA_OK;
    try {
        loadStatus = lua_load(state, throwingReader, nullptr, "=consumer-reader");
    } catch (...) {
        lua_close(state);
        return 5;
    }
    if (loadStatus != LUA_ERRRUN || lua_gettop(state) != 2 || lua_tonumber(state, 1) != 73 ||
        std::string(lua_tostring(state, -1)) != "consumer reader failure") {
        lua_close(state);
        return 6;
    }

    lua_settop(state, 0);
    lua_pushnumber(state, 1);
    bool unprotectedCallThrew = false;
    try {
        lua_call(state, 0, 0);
    } catch (...) {
        unprotectedCallThrew = true;
    }
    lua_close(state);
    return unprotectedCallThrew ? 0 : 7;
}
