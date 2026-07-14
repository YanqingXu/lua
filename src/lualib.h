#ifndef LUALIB_H
#define LUALIB_H

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

void luaL_openlibs(lua_State* L) LUA_CXX_MAY_THROW;

#ifdef __cplusplus
}
#endif

#endif
