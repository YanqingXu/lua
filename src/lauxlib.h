#ifndef LAUXLIB_H
#define LAUXLIB_H

#include <stdio.h>

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct luaL_Reg {
    const char* name;
    lua_CFunction func;
} luaL_Reg;

#define LUAL_BUFFERSIZE BUFSIZ

typedef struct luaL_Buffer {
    char* p;
    int lvl;
    lua_State* L;
    char buffer[LUAL_BUFFERSIZE];
} luaL_Buffer;

#define LUA_NOREF (-2)
#define LUA_REFNIL (-1)
#define LUA_ERRFILE (LUA_ERRERR + 1)

void luaL_openlibs(lua_State* L) LUA_CXX_MAY_THROW;
void luaL_openlib(lua_State* L, const char* libname, const luaL_Reg* functions, int upvalueCount) LUA_CXX_MAY_THROW;
void luaL_register(lua_State* L, const char* libname, const luaL_Reg* functions) LUA_CXX_MAY_THROW;
int luaL_getmetafield(lua_State* L, int objectIndex, const char* event) LUA_CXX_MAY_THROW;
int luaL_callmeta(lua_State* L, int objectIndex, const char* event) LUA_CXX_MAY_THROW;
int luaL_typerror(lua_State* L, int narg, const char* typeName) LUA_CXX_MAY_THROW;
int luaL_error(lua_State* L, const char* fmt, ...) LUA_CXX_MAY_THROW;
int luaL_argerror(lua_State* L, int narg, const char* extramsg) LUA_CXX_MAY_THROW;
void luaL_argcheck(lua_State* L, int cond, int narg, const char* extramsg) LUA_CXX_MAY_THROW;
lua_Number luaL_checknumber(lua_State* L, int narg) LUA_CXX_MAY_THROW;
lua_Number luaL_optnumber(lua_State* L, int narg, lua_Number defaultValue) LUA_CXX_MAY_THROW;
lua_Integer luaL_checkinteger(lua_State* L, int narg) LUA_CXX_MAY_THROW;
lua_Integer luaL_optinteger(lua_State* L, int narg, lua_Integer defaultValue) LUA_CXX_MAY_THROW;
int luaL_checkint(lua_State* L, int narg) LUA_CXX_MAY_THROW;
const char* luaL_checklstring(lua_State* L, int narg, size_t* len) LUA_CXX_MAY_THROW;
const char* luaL_optlstring(lua_State* L, int narg, const char* defaultValue, size_t* len) LUA_CXX_MAY_THROW;
const char* luaL_checkstring(lua_State* L, int narg) LUA_CXX_MAY_THROW;
void luaL_checkstack(lua_State* L, int size, const char* message) LUA_CXX_MAY_THROW;
void luaL_checktype(lua_State* L, int narg, int type) LUA_CXX_MAY_THROW;
void luaL_checkany(lua_State* L, int narg) LUA_CXX_MAY_THROW;
int luaL_newmetatable(lua_State* L, const char* typeName) LUA_CXX_MAY_THROW;
void* luaL_checkudata(lua_State* L, int narg, const char* typeName) LUA_CXX_MAY_THROW;
void luaL_where(lua_State* L, int level) LUA_CXX_MAY_THROW;
int luaL_checkoption(lua_State* L, int narg, const char* defaultValue, const char* const options[]) LUA_CXX_MAY_THROW;
int luaL_loadbuffer(lua_State* L, const char* buffer, size_t size, const char* name) LUA_CXX_NOEXCEPT;
int luaL_loadstring(lua_State* L, const char* source) LUA_CXX_NOEXCEPT;
int luaL_loadfile(lua_State* L, const char* filename) LUA_CXX_NOEXCEPT;
int luaL_ref(lua_State* L, int tableIndex) LUA_CXX_MAY_THROW;
void luaL_unref(lua_State* L, int tableIndex, int reference) LUA_CXX_MAY_THROW;
lua_State* luaL_newstate(void) LUA_CXX_MAY_THROW;
const char* luaL_gsub(lua_State* L, const char* source, const char* pattern, const char* replacement) LUA_CXX_MAY_THROW;
const char* luaL_findtable(lua_State* L, int tableIndex, const char* fieldName, int sizeHint) LUA_CXX_MAY_THROW;

void luaL_buffinit(lua_State* L, luaL_Buffer* buffer) LUA_CXX_MAY_THROW;
char* luaL_prepbuffer(luaL_Buffer* buffer) LUA_CXX_MAY_THROW;
void luaL_addlstring(luaL_Buffer* buffer, const char* text, size_t length) LUA_CXX_MAY_THROW;
void luaL_addstring(luaL_Buffer* buffer, const char* text) LUA_CXX_MAY_THROW;
void luaL_addvalue(luaL_Buffer* buffer) LUA_CXX_MAY_THROW;
void luaL_pushresult(luaL_Buffer* buffer) LUA_CXX_MAY_THROW;

#define luaL_optstring(L, n, d) luaL_optlstring((L), (n), (d), NULL)
#define luaL_optint(L, n, d) ((int)luaL_optinteger((L), (n), (d)))
#define luaL_checklong(L, n) ((long)luaL_checkinteger((L), (n)))
#define luaL_optlong(L, n, d) ((long)luaL_optinteger((L), (n), (d)))
#define luaL_typename(L, i) lua_typename((L), lua_type((L), (i)))
#define luaL_dofile(L, filename) (luaL_loadfile((L), (filename)) || lua_pcall((L), 0, LUA_MULTRET, 0))
#define luaL_dostring(L, source) (luaL_loadstring((L), (source)) || lua_pcall((L), 0, LUA_MULTRET, 0))
#define luaL_getmetatable(L, name) lua_getfield((L), LUA_REGISTRYINDEX, (name))
#define luaL_opt(L, function, n, d) (lua_isnoneornil((L), (n)) ? (d) : function((L), (n)))

#define luaL_addchar(B, c)                                                                                             \
    ((void)((B)->p < ((B)->buffer + LUAL_BUFFERSIZE) || luaL_prepbuffer((B))), (*(B)->p++ = (char)(c)))
#define luaL_putchar(B, c) luaL_addchar((B), (c))
#define luaL_addsize(B, n) ((B)->p += (n))

#define luaL_getref(L, ref) lua_rawgeti((L), LUA_REGISTRYINDEX, (ref))
#define luaL_reg luaL_Reg

#ifdef __cplusplus
}
#endif

#endif
