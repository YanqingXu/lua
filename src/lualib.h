#ifndef LUALIB_H
#define LUALIB_H

#include "lua.h"

#define LUA_FILEHANDLE "FILE*"

#define LUA_COLIBNAME "coroutine"
#define LUA_TABLIBNAME "table"
#define LUA_IOLIBNAME "io"
#define LUA_OSLIBNAME "os"
#define LUA_STRLIBNAME "string"
#define LUA_MATHLIBNAME "math"
#define LUA_DBLIBNAME "debug"
#define LUA_LOADLIBNAME "package"

#ifdef __cplusplus
extern "C" {
#endif

int luaopen_base(lua_State* L) LUA_CXX_MAY_THROW;
int luaopen_table(lua_State* L) LUA_CXX_MAY_THROW;
int luaopen_io(lua_State* L) LUA_CXX_MAY_THROW;
int luaopen_os(lua_State* L) LUA_CXX_MAY_THROW;
int luaopen_string(lua_State* L) LUA_CXX_MAY_THROW;
int luaopen_math(lua_State* L) LUA_CXX_MAY_THROW;
int luaopen_debug(lua_State* L) LUA_CXX_MAY_THROW;
int luaopen_package(lua_State* L) LUA_CXX_MAY_THROW;
void luaL_openlibs(lua_State* L) LUA_CXX_MAY_THROW;

#ifdef __cplusplus
}
#endif

#ifndef lua_assert
#define lua_assert(x) ((void)0)
#endif

#endif
