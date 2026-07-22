#include <lua.h>
#include <lauxlib.h>
#include <lua_cpp_version.h>

#include <string.h>

int main(void) {
    lua_State* state = luaL_newstate();
    if (state == NULL) {
        return 1;
    }

    lua_pushstring(state, LUA_CPP_VERSION);
    if (strcmp(lua_tostring(state, -1), "0.1.0") != 0 || LUA_CPP_ABI_VERSION != 0) {
        lua_close(state);
        return 2;
    }

    lua_close(state);
    return 0;
}
