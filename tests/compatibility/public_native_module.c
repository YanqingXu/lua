#include "lua.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#define LUA_PUBLIC_MODULE_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define LUA_PUBLIC_MODULE_EXPORT __attribute__((visibility("default")))
#else
#define LUA_PUBLIC_MODULE_EXPORT
#endif

static int module_calls;

static int protected_probe(lua_State* state) {
    lua_pushnumber(state, 42.5);
    return 1;
}

static int next_state_call(lua_State* state) {
    int calls = 0;

    lua_pushstring(state, "lua.publicfixture.calls");
    lua_gettable(state, LUA_REGISTRYINDEX);
    if (lua_isnumber(state, -1)) {
        calls = (int)lua_tonumber(state, -1);
    }
    lua_pop(state, 1);

    ++calls;
    lua_pushstring(state, "lua.publicfixture.calls");
    lua_pushinteger(state, calls);
    lua_settable(state, LUA_REGISTRYINDEX);
    return calls;
}

static int write_close_marker(lua_State* state) {
    const char* path = (const char*)lua_touserdata(state, 1);
    FILE* marker;
    if (path == NULL) {
        return 0;
    }

#if defined(_WIN32)
    if (fopen_s(&marker, path, "wb") != 0) {
        return 0;
    }
#else
    marker = fopen(path, "wb");
    if (marker == NULL) {
        return 0;
    }
#endif
    (void)fputs("module-finalizer-ran", marker);
    (void)fclose(marker);
    return 0;
}

static int install_close_finalizer(lua_State* state) {
    size_t path_length = 0;
    const char* path = lua_tolstring(state, 1, &path_length);
    char* payload;
    if (path == NULL) {
        lua_pushstring(state, "close marker path must be a string");
        return lua_error(state);
    }

    payload = (char*)lua_newuserdata(state, path_length + 1);
    memcpy(payload, path, path_length);
    payload[path_length] = '\0';

    lua_createtable(state, 0, 1);
    lua_pushstring(state, "__gc");
    lua_pushcclosure(state, write_close_marker, 0);
    lua_settable(state, -3);
    (void)lua_setmetatable(state, -2);
    return 1;
}

LUA_PUBLIC_MODULE_EXPORT int luaopen_publicfixture(lua_State* state) {
    const int state_calls = next_state_call(state);
    double protected_value;
    ++module_calls;

    /* Exercise public call-boundary imports, not only table primitives. */
    lua_pushcclosure(state, protected_probe, 0);
    if (lua_pcall(state, 0, 1, 0) != 0) {
        return lua_error(state);
    }
    protected_value = lua_tonumber(state, -1);
    lua_pop(state, 1);

    lua_createtable(state, 0, 5);

    lua_pushstring(state, "source");
    lua_pushstring(state, "public-lua-h-only");
    lua_settable(state, -3);

    lua_pushstring(state, "state_calls");
    lua_pushinteger(state, state_calls);
    lua_settable(state, -3);

    lua_pushstring(state, "module_calls");
    lua_pushinteger(state, module_calls);
    lua_settable(state, -3);

    lua_pushstring(state, "protected_value");
    lua_pushnumber(state, protected_value);
    lua_settable(state, -3);

    lua_pushstring(state, "install_close_finalizer");
    lua_pushcclosure(state, install_close_finalizer, 0);
    lua_settable(state, -3);

    return 1;
}
