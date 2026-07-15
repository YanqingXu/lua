#include "lua.h"

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

    lua_createtable(state, 0, 4);

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

    return 1;
}
