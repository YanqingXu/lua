#ifndef LAUXLIB_H
#define LAUXLIB_H

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct luaL_Reg {
    const char* name;
    lua_CFunction func;
} luaL_Reg;

#define LUA_NOREF (-2)
#define LUA_REFNIL (-1)
#define LUA_ERRFILE (LUA_ERRERR + 1)

void luaL_openlibs(lua_State* L);
int luaL_error(lua_State* L, const char* fmt, ...);
int luaL_argerror(lua_State* L, int narg, const char* extramsg);
void luaL_argcheck(lua_State* L, int cond, int narg, const char* extramsg);
lua_Number luaL_checknumber(lua_State* L, int narg);
int luaL_checkint(lua_State* L, int narg);
const char* luaL_checklstring(lua_State* L, int narg, size_t* len);
const char* luaL_checkstring(lua_State* L, int narg);
int luaL_loadbuffer(lua_State* L, const char* buffer, size_t size, const char* name);
int luaL_loadstring(lua_State* L, const char* source);
int luaL_loadfile(lua_State* L, const char* filename);
int luaL_ref(lua_State* L, int tableIndex);
void luaL_unref(lua_State* L, int tableIndex, int reference);

#define luaL_getref(L, ref) lua_rawgeti((L), LUA_REGISTRYINDEX, (ref))

#ifdef __cplusplus
}
#endif

#endif
