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

void luaL_openlibs(lua_State* L);
int luaL_error(lua_State* L, const char* fmt, ...);
int luaL_argerror(lua_State* L, int narg, const char* extramsg);
void luaL_argcheck(lua_State* L, int cond, int narg, const char* extramsg);
lua_Number luaL_checknumber(lua_State* L, int narg);
int luaL_checkint(lua_State* L, int narg);
const char* luaL_checklstring(lua_State* L, int narg, size_t* len);
const char* luaL_checkstring(lua_State* L, int narg);

#ifdef __cplusplus
}
#endif

#endif
