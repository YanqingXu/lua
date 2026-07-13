#ifndef LUA_H
#define LUA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LUA_VERSION "Lua 5.1"
#define LUA_RELEASE "Lua 5.1.5"
#define LUA_VERSION_NUM 501

#define LUA_MULTRET (-1)

#define LUA_REGISTRYINDEX (-10000)
#define LUA_ENVIRONINDEX (-10001)
#define LUA_GLOBALSINDEX (-10002)

#define lua_upvalueindex(i) (LUA_GLOBALSINDEX - (i))

enum { LUA_OK = 0, LUA_YIELD = 1, LUA_ERRRUN = 2, LUA_ERRSYNTAX = 3, LUA_ERRMEM = 4, LUA_ERRERR = 5 };

enum {
    LUA_TNONE = -1,
    LUA_TNIL = 0,
    LUA_TBOOLEAN = 1,
    LUA_TLIGHTUSERDATA = 2,
    LUA_TNUMBER = 3,
    LUA_TSTRING = 4,
    LUA_TTABLE = 5,
    LUA_TFUNCTION = 6,
    LUA_TUSERDATA = 7,
    LUA_TTHREAD = 8
};

typedef struct lua_State lua_State;
typedef double lua_Number;
typedef ptrdiff_t lua_Integer;
typedef int (*lua_CFunction)(lua_State* L);
typedef void* (*lua_Alloc)(void* ud, void* ptr, size_t osize, size_t nsize);

lua_State* lua_newstate(lua_Alloc f, void* ud);
lua_State* lua_open(void);
void lua_close(lua_State* L);
lua_Alloc lua_getallocf(lua_State* L, void** ud);
void lua_setallocf(lua_State* L, lua_Alloc f, void* ud);

int lua_gettop(lua_State* L);
void lua_settop(lua_State* L, int idx);
int lua_checkstack(lua_State* L, int extra);
void lua_xmove(lua_State* from, lua_State* to, int n);
void lua_pushvalue(lua_State* L, int idx);
void lua_remove(lua_State* L, int idx);
void lua_insert(lua_State* L, int idx);
void lua_replace(lua_State* L, int idx);

int lua_type(lua_State* L, int idx);
const char* lua_typename(lua_State* L, int tp);
int lua_isnumber(lua_State* L, int idx);
int lua_isstring(lua_State* L, int idx);
int lua_iscfunction(lua_State* L, int idx);
int lua_isuserdata(lua_State* L, int idx);
int lua_toboolean(lua_State* L, int idx);
lua_Number lua_tonumber(lua_State* L, int idx);
const char* lua_tolstring(lua_State* L, int idx, size_t* len);
void* lua_touserdata(lua_State* L, int idx);
size_t lua_objlen(lua_State* L, int idx);

void lua_pushnil(lua_State* L);
void lua_pushnumber(lua_State* L, lua_Number n);
void lua_pushinteger(lua_State* L, lua_Integer n);
void lua_pushboolean(lua_State* L, int b);
void lua_pushlstring(lua_State* L, const char* s, size_t len);
void lua_pushstring(lua_State* L, const char* s);
void lua_pushlightuserdata(lua_State* L, void* p);
void lua_pushcclosure(lua_State* L, lua_CFunction fn, int n);

const char* lua_getupvalue(lua_State* L, int funcindex, int n);
const char* lua_setupvalue(lua_State* L, int funcindex, int n);

void lua_createtable(lua_State* L, int narr, int nrec);
void* lua_newuserdata(lua_State* L, size_t size);
void lua_gettable(lua_State* L, int idx);
void lua_settable(lua_State* L, int idx);
void lua_rawgeti(lua_State* L, int idx, int n);
void lua_rawseti(lua_State* L, int idx, int n);
void lua_getglobal(lua_State* L, const char* name);
void lua_setglobal(lua_State* L, const char* name);
int lua_getmetatable(lua_State* L, int objindex);
int lua_setmetatable(lua_State* L, int objindex);

void lua_call(lua_State* L, int nargs, int nresults);
int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc);
int lua_error(lua_State* L);
lua_State* lua_newthread(lua_State* L);
int lua_resume(lua_State* L, int nargs);
int lua_yield(lua_State* L, int nresults);
int lua_status(lua_State* L);

#define lua_pop(L, n) lua_settop((L), -(n) - 1)
#define lua_newtable(L) lua_createtable((L), 0, 0)
#define lua_tostring(L, i) lua_tolstring((L), (i), 0)
#define lua_isfunction(L, n) (lua_type((L), (n)) == LUA_TFUNCTION)
#define lua_istable(L, n) (lua_type((L), (n)) == LUA_TTABLE)
#define lua_isnil(L, n) (lua_type((L), (n)) == LUA_TNIL)
#define lua_isboolean(L, n) (lua_type((L), (n)) == LUA_TBOOLEAN)
#define lua_isthread(L, n) (lua_type((L), (n)) == LUA_TTHREAD)
#define lua_isnone(L, n) (lua_type((L), (n)) == LUA_TNONE)
#define lua_isnoneornil(L, n) (lua_type((L), (n)) <= 0)

#ifdef __cplusplus
}
#endif

#endif
