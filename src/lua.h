#ifndef LUA_H
#define LUA_H

#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
#define LUA_CXX_MAY_THROW noexcept(false)
#define LUA_CXX_NOEXCEPT noexcept
extern "C" {
#else
#define LUA_CXX_MAY_THROW
#define LUA_CXX_NOEXCEPT
#endif

#define LUA_VERSION "Lua 5.1"
#define LUA_RELEASE "Lua 5.1.5"
#define LUA_VERSION_NUM 501

#define LUA_MULTRET (-1)
#define LUA_MINSTACK 20
#define LUA_IDSIZE 60

#define LUA_REGISTRYINDEX (-10000)
#define LUA_ENVIRONINDEX (-10001)
#define LUA_GLOBALSINDEX (-10002)

#define lua_upvalueindex(i) (LUA_GLOBALSINDEX - (i))

enum {
    LUA_OK = 0,
    LUA_YIELD = 1,
    LUA_ERRRUN = 2,
    LUA_ERRSYNTAX = 3,
    LUA_ERRMEM = 4,
    LUA_ERRERR = 5,
    /* Project extension statuses returned by lua_tryclose. */
    LUA_ERRTHREAD = 6,
    LUA_ERRBUSY = 7
};

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

enum {
    LUA_GCSTOP = 0,
    LUA_GCRESTART = 1,
    LUA_GCCOLLECT = 2,
    LUA_GCCOUNT = 3,
    LUA_GCCOUNTB = 4,
    LUA_GCSTEP = 5,
    LUA_GCSETPAUSE = 6,
    LUA_GCSETSTEPMUL = 7
};

typedef struct lua_State lua_State;
typedef double lua_Number;
typedef ptrdiff_t lua_Integer;
typedef int (*lua_CFunction)(lua_State* L);
typedef void* (*lua_Alloc)(void* ud, void* ptr, size_t osize, size_t nsize);
typedef const char* (*lua_Reader)(lua_State* L, void* data, size_t* size);
typedef int (*lua_Writer)(lua_State* L, const void* data, size_t size, void* userData);

#define LUA_HOOKCALL 0
#define LUA_HOOKRET 1
#define LUA_HOOKLINE 2
#define LUA_HOOKCOUNT 3
#define LUA_HOOKTAILRET 4

#define LUA_MASKCALL (1 << LUA_HOOKCALL)
#define LUA_MASKRET (1 << LUA_HOOKRET)
#define LUA_MASKLINE (1 << LUA_HOOKLINE)
#define LUA_MASKCOUNT (1 << LUA_HOOKCOUNT)

typedef struct lua_Debug lua_Debug;
typedef void (*lua_Hook)(lua_State* L, lua_Debug* ar);

struct lua_Debug {
    int event;
    const char* name;
    const char* namewhat;
    const char* what;
    const char* source;
    int currentline;
    int nups;
    int linedefined;
    int lastlinedefined;
    char short_src[LUA_IDSIZE];
    int i_ci;
};

lua_State* lua_newstate(lua_Alloc f, void* ud) LUA_CXX_MAY_THROW;
lua_State* lua_open(void) LUA_CXX_MAY_THROW;
void lua_close(lua_State* L) LUA_CXX_NOEXCEPT;
/* Project extension: observable owner-thread runtime teardown. */
int lua_tryclose(lua_State* L) LUA_CXX_NOEXCEPT;
lua_CFunction lua_atpanic(lua_State* L, lua_CFunction panicf) LUA_CXX_MAY_THROW;
lua_Alloc lua_getallocf(lua_State* L, void** ud) LUA_CXX_MAY_THROW;
void lua_setallocf(lua_State* L, lua_Alloc f, void* ud) LUA_CXX_MAY_THROW;

int lua_gettop(lua_State* L) LUA_CXX_MAY_THROW;
void lua_settop(lua_State* L, int idx) LUA_CXX_MAY_THROW;
int lua_checkstack(lua_State* L, int extra) LUA_CXX_NOEXCEPT;
/* Project extension: cooperative cancellation/deadline poll for native callbacks. */
void lua_checkexecution(lua_State* L) LUA_CXX_MAY_THROW;
void lua_xmove(lua_State* from, lua_State* to, int n) LUA_CXX_MAY_THROW;
void lua_pushvalue(lua_State* L, int idx) LUA_CXX_MAY_THROW;
void lua_remove(lua_State* L, int idx) LUA_CXX_MAY_THROW;
void lua_insert(lua_State* L, int idx) LUA_CXX_MAY_THROW;
void lua_replace(lua_State* L, int idx) LUA_CXX_MAY_THROW;

int lua_type(lua_State* L, int idx) LUA_CXX_MAY_THROW;
const char* lua_typename(lua_State* L, int tp) LUA_CXX_MAY_THROW;
int lua_isnumber(lua_State* L, int idx) LUA_CXX_MAY_THROW;
int lua_isstring(lua_State* L, int idx) LUA_CXX_MAY_THROW;
int lua_iscfunction(lua_State* L, int idx) LUA_CXX_MAY_THROW;
int lua_isuserdata(lua_State* L, int idx) LUA_CXX_MAY_THROW;
int lua_equal(lua_State* L, int idx1, int idx2) LUA_CXX_MAY_THROW;
int lua_rawequal(lua_State* L, int idx1, int idx2) LUA_CXX_MAY_THROW;
int lua_lessthan(lua_State* L, int idx1, int idx2) LUA_CXX_MAY_THROW;
int lua_toboolean(lua_State* L, int idx) LUA_CXX_MAY_THROW;
lua_Number lua_tonumber(lua_State* L, int idx) LUA_CXX_MAY_THROW;
lua_Integer lua_tointeger(lua_State* L, int idx) LUA_CXX_MAY_THROW;
const char* lua_tolstring(lua_State* L, int idx, size_t* len) LUA_CXX_MAY_THROW;
lua_CFunction lua_tocfunction(lua_State* L, int idx) LUA_CXX_MAY_THROW;
void* lua_touserdata(lua_State* L, int idx) LUA_CXX_MAY_THROW;
lua_State* lua_tothread(lua_State* L, int idx) LUA_CXX_MAY_THROW;
const void* lua_topointer(lua_State* L, int idx) LUA_CXX_MAY_THROW;
size_t lua_objlen(lua_State* L, int idx) LUA_CXX_MAY_THROW;

void lua_pushnil(lua_State* L) LUA_CXX_MAY_THROW;
void lua_pushnumber(lua_State* L, lua_Number n) LUA_CXX_MAY_THROW;
void lua_pushinteger(lua_State* L, lua_Integer n) LUA_CXX_MAY_THROW;
void lua_pushboolean(lua_State* L, int b) LUA_CXX_MAY_THROW;
void lua_pushlstring(lua_State* L, const char* s, size_t len) LUA_CXX_MAY_THROW;
void lua_pushstring(lua_State* L, const char* s) LUA_CXX_MAY_THROW;
const char* lua_pushvfstring(lua_State* L, const char* fmt, va_list argp) LUA_CXX_MAY_THROW;
const char* lua_pushfstring(lua_State* L, const char* fmt, ...) LUA_CXX_MAY_THROW;
void lua_pushlightuserdata(lua_State* L, void* p) LUA_CXX_MAY_THROW;
void lua_pushcclosure(lua_State* L, lua_CFunction fn, int n) LUA_CXX_MAY_THROW;
int lua_pushthread(lua_State* L) LUA_CXX_MAY_THROW;

const char* lua_getupvalue(lua_State* L, int funcindex, int n) LUA_CXX_MAY_THROW;
const char* lua_setupvalue(lua_State* L, int funcindex, int n) LUA_CXX_MAY_THROW;

void lua_createtable(lua_State* L, int narr, int nrec) LUA_CXX_MAY_THROW;
void* lua_newuserdata(lua_State* L, size_t size) LUA_CXX_MAY_THROW;
void lua_gettable(lua_State* L, int idx) LUA_CXX_MAY_THROW;
void lua_getfield(lua_State* L, int idx, const char* key) LUA_CXX_MAY_THROW;
void lua_rawget(lua_State* L, int idx) LUA_CXX_MAY_THROW;
void lua_settable(lua_State* L, int idx) LUA_CXX_MAY_THROW;
void lua_setfield(lua_State* L, int idx, const char* key) LUA_CXX_MAY_THROW;
void lua_rawset(lua_State* L, int idx) LUA_CXX_MAY_THROW;
void lua_rawgeti(lua_State* L, int idx, int n) LUA_CXX_MAY_THROW;
void lua_rawseti(lua_State* L, int idx, int n) LUA_CXX_MAY_THROW;
void lua_getglobal(lua_State* L, const char* name) LUA_CXX_MAY_THROW;
void lua_setglobal(lua_State* L, const char* name) LUA_CXX_MAY_THROW;
int lua_getmetatable(lua_State* L, int objindex) LUA_CXX_MAY_THROW;
int lua_setmetatable(lua_State* L, int objindex) LUA_CXX_MAY_THROW;
void lua_getfenv(lua_State* L, int idx) LUA_CXX_MAY_THROW;
int lua_setfenv(lua_State* L, int idx) LUA_CXX_MAY_THROW;

void lua_call(lua_State* L, int nargs, int nresults) LUA_CXX_MAY_THROW;
int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc) LUA_CXX_NOEXCEPT;
int lua_cpcall(lua_State* L, lua_CFunction func, void* ud) LUA_CXX_NOEXCEPT;
int lua_error(lua_State* L) LUA_CXX_MAY_THROW;
int lua_next(lua_State* L, int idx) LUA_CXX_MAY_THROW;
void lua_concat(lua_State* L, int n) LUA_CXX_MAY_THROW;
lua_State* lua_newthread(lua_State* L) LUA_CXX_MAY_THROW;
/* Project extension: transactional thread creation without exception propagation. */
lua_State* lua_trynewthread(lua_State* L) LUA_CXX_NOEXCEPT;
int lua_resume(lua_State* L, int nargs) LUA_CXX_NOEXCEPT;
int lua_yield(lua_State* L, int nresults) LUA_CXX_MAY_THROW;
int lua_status(lua_State* L) LUA_CXX_MAY_THROW;
int lua_gc(lua_State* L, int what, int data) LUA_CXX_MAY_THROW;
int lua_load(lua_State* L, lua_Reader reader, void* data, const char* chunkname) LUA_CXX_NOEXCEPT;
int lua_dump(lua_State* L, lua_Writer writer, void* data) LUA_CXX_NOEXCEPT;
void lua_setlevel(lua_State* from, lua_State* to) LUA_CXX_MAY_THROW;

int lua_getstack(lua_State* L, int level, lua_Debug* ar) LUA_CXX_MAY_THROW;
int lua_getinfo(lua_State* L, const char* what, lua_Debug* ar) LUA_CXX_MAY_THROW;
const char* lua_getlocal(lua_State* L, const lua_Debug* ar, int n) LUA_CXX_MAY_THROW;
const char* lua_setlocal(lua_State* L, const lua_Debug* ar, int n) LUA_CXX_MAY_THROW;
int lua_sethook(lua_State* L, lua_Hook func, int mask, int count) LUA_CXX_MAY_THROW;
lua_Hook lua_gethook(lua_State* L) LUA_CXX_MAY_THROW;
int lua_gethookmask(lua_State* L) LUA_CXX_MAY_THROW;
int lua_gethookcount(lua_State* L) LUA_CXX_MAY_THROW;

#define lua_pop(L, n) lua_settop((L), -(n) - 1)
#define lua_newtable(L) lua_createtable((L), 0, 0)
#define lua_tostring(L, i) lua_tolstring((L), (i), 0)
#define lua_pushliteral(L, s) lua_pushlstring((L), "" s, (sizeof(s) / sizeof(char)) - 1)
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
