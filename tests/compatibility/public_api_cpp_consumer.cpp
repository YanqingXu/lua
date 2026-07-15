#include "lua.h"
#include "lauxlib.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

extern "C" int lua_public_c_header_probe(void);

/*
 * These markers are consumed by check_lua51_public_api_contract.py. The
 * compile-time assertions make the header's exact C++ ABI part of the test,
 * while the checker makes omissions fail whenever the header grows.
 */
#define REQUIRE_SIGNATURE(name, ...)                                                                                   \
    static_assert(std::is_same_v<decltype(&name), __VA_ARGS__>, #name " signature drifted")
#define REQUIRE_PUBLIC_MACRO(name) static_assert(sizeof(#name) > 1, #name " macro is required")
#define REQUIRE_PUBLIC_CONSTANT(name) static_assert(sizeof(#name) > 1, #name " constant is required")
#define REQUIRE_PUBLIC_TYPE(name) static_assert(std::is_same_v<name*, name*>, #name " type is required")

REQUIRE_SIGNATURE(lua_newstate, lua_State* (*)(lua_Alloc, void*) noexcept(false));
REQUIRE_SIGNATURE(lua_open, lua_State* (*)() noexcept(false));
REQUIRE_SIGNATURE(lua_close, void (*)(lua_State*) noexcept);
REQUIRE_SIGNATURE(lua_getallocf, lua_Alloc (*)(lua_State*, void**) noexcept(false));
REQUIRE_SIGNATURE(lua_setallocf, void (*)(lua_State*, lua_Alloc, void*) noexcept(false));
REQUIRE_SIGNATURE(lua_gettop, int (*)(lua_State*) noexcept(false));
REQUIRE_SIGNATURE(lua_settop, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_checkstack, int (*)(lua_State*, int) noexcept);
REQUIRE_SIGNATURE(lua_xmove, void (*)(lua_State*, lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_pushvalue, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_remove, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_insert, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_replace, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_type, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_typename, const char* (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_isnumber, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_isstring, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_iscfunction, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_isuserdata, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_equal, int (*)(lua_State*, int, int) noexcept(false));
REQUIRE_SIGNATURE(lua_rawequal, int (*)(lua_State*, int, int) noexcept(false));
REQUIRE_SIGNATURE(lua_lessthan, int (*)(lua_State*, int, int) noexcept(false));
REQUIRE_SIGNATURE(lua_toboolean, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_tonumber, lua_Number (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_tolstring, const char* (*)(lua_State*, int, size_t*) noexcept(false));
REQUIRE_SIGNATURE(lua_touserdata, void* (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_objlen, size_t (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_pushnil, void (*)(lua_State*) noexcept(false));
REQUIRE_SIGNATURE(lua_pushnumber, void (*)(lua_State*, lua_Number) noexcept(false));
REQUIRE_SIGNATURE(lua_pushinteger, void (*)(lua_State*, lua_Integer) noexcept(false));
REQUIRE_SIGNATURE(lua_pushboolean, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_pushlstring, void (*)(lua_State*, const char*, size_t) noexcept(false));
REQUIRE_SIGNATURE(lua_pushstring, void (*)(lua_State*, const char*) noexcept(false));
REQUIRE_SIGNATURE(lua_pushlightuserdata, void (*)(lua_State*, void*) noexcept(false));
REQUIRE_SIGNATURE(lua_pushcclosure, void (*)(lua_State*, lua_CFunction, int) noexcept(false));
REQUIRE_SIGNATURE(lua_getupvalue, const char* (*)(lua_State*, int, int) noexcept(false));
REQUIRE_SIGNATURE(lua_setupvalue, const char* (*)(lua_State*, int, int) noexcept(false));
REQUIRE_SIGNATURE(lua_createtable, void (*)(lua_State*, int, int) noexcept(false));
REQUIRE_SIGNATURE(lua_newuserdata, void* (*)(lua_State*, size_t) noexcept(false));
REQUIRE_SIGNATURE(lua_gettable, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_getfield, void (*)(lua_State*, int, const char*) noexcept(false));
REQUIRE_SIGNATURE(lua_rawget, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_settable, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_setfield, void (*)(lua_State*, int, const char*) noexcept(false));
REQUIRE_SIGNATURE(lua_rawset, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_rawgeti, void (*)(lua_State*, int, int) noexcept(false));
REQUIRE_SIGNATURE(lua_rawseti, void (*)(lua_State*, int, int) noexcept(false));
REQUIRE_SIGNATURE(lua_getglobal, void (*)(lua_State*, const char*) noexcept(false));
REQUIRE_SIGNATURE(lua_setglobal, void (*)(lua_State*, const char*) noexcept(false));
REQUIRE_SIGNATURE(lua_getmetatable, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_setmetatable, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_call, void (*)(lua_State*, int, int) noexcept(false));
REQUIRE_SIGNATURE(lua_pcall, int (*)(lua_State*, int, int, int) noexcept);
REQUIRE_SIGNATURE(lua_error, int (*)(lua_State*) noexcept(false));
REQUIRE_SIGNATURE(lua_next, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_concat, void (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_newthread, lua_State* (*)(lua_State*) noexcept(false));
REQUIRE_SIGNATURE(lua_trynewthread, lua_State* (*)(lua_State*) noexcept);
REQUIRE_SIGNATURE(lua_resume, int (*)(lua_State*, int) noexcept);
REQUIRE_SIGNATURE(lua_yield, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(lua_status, int (*)(lua_State*) noexcept(false));
REQUIRE_SIGNATURE(lua_load, int (*)(lua_State*, lua_Reader, void*, const char*) noexcept);
REQUIRE_SIGNATURE(lua_dump, int (*)(lua_State*, lua_Writer, void*) noexcept);
REQUIRE_SIGNATURE(luaL_openlibs, void (*)(lua_State*) noexcept(false));
REQUIRE_SIGNATURE(luaL_error, int (*)(lua_State*, const char*, ...) noexcept(false));
REQUIRE_SIGNATURE(luaL_argerror, int (*)(lua_State*, int, const char*) noexcept(false));
REQUIRE_SIGNATURE(luaL_argcheck, void (*)(lua_State*, int, int, const char*) noexcept(false));
REQUIRE_SIGNATURE(luaL_checknumber, lua_Number (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(luaL_checkint, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(luaL_checklstring, const char* (*)(lua_State*, int, size_t*) noexcept(false));
REQUIRE_SIGNATURE(luaL_checkstring, const char* (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(luaL_loadbuffer, int (*)(lua_State*, const char*, size_t, const char*) noexcept);
REQUIRE_SIGNATURE(luaL_loadstring, int (*)(lua_State*, const char*) noexcept);
REQUIRE_SIGNATURE(luaL_loadfile, int (*)(lua_State*, const char*) noexcept);
REQUIRE_SIGNATURE(luaL_ref, int (*)(lua_State*, int) noexcept(false));
REQUIRE_SIGNATURE(luaL_unref, void (*)(lua_State*, int, int) noexcept(false));

REQUIRE_PUBLIC_MACRO(LUA_VERSION);
REQUIRE_PUBLIC_MACRO(LUA_RELEASE);
REQUIRE_PUBLIC_MACRO(LUA_VERSION_NUM);
REQUIRE_PUBLIC_MACRO(LUA_MULTRET);
REQUIRE_PUBLIC_MACRO(LUA_REGISTRYINDEX);
REQUIRE_PUBLIC_MACRO(LUA_ENVIRONINDEX);
REQUIRE_PUBLIC_MACRO(LUA_GLOBALSINDEX);
REQUIRE_PUBLIC_MACRO(lua_upvalueindex);
REQUIRE_PUBLIC_MACRO(lua_pop);
REQUIRE_PUBLIC_MACRO(lua_newtable);
REQUIRE_PUBLIC_MACRO(lua_tostring);
REQUIRE_PUBLIC_MACRO(lua_isfunction);
REQUIRE_PUBLIC_MACRO(lua_istable);
REQUIRE_PUBLIC_MACRO(lua_isnil);
REQUIRE_PUBLIC_MACRO(lua_isboolean);
REQUIRE_PUBLIC_MACRO(lua_isthread);
REQUIRE_PUBLIC_MACRO(lua_isnone);
REQUIRE_PUBLIC_MACRO(lua_isnoneornil);
REQUIRE_PUBLIC_MACRO(LUA_NOREF);
REQUIRE_PUBLIC_MACRO(LUA_REFNIL);
REQUIRE_PUBLIC_MACRO(LUA_ERRFILE);
REQUIRE_PUBLIC_MACRO(luaL_getref);

REQUIRE_PUBLIC_CONSTANT(LUA_OK);
REQUIRE_PUBLIC_CONSTANT(LUA_YIELD);
REQUIRE_PUBLIC_CONSTANT(LUA_ERRRUN);
REQUIRE_PUBLIC_CONSTANT(LUA_ERRSYNTAX);
REQUIRE_PUBLIC_CONSTANT(LUA_ERRMEM);
REQUIRE_PUBLIC_CONSTANT(LUA_ERRERR);
REQUIRE_PUBLIC_CONSTANT(LUA_TNONE);
REQUIRE_PUBLIC_CONSTANT(LUA_TNIL);
REQUIRE_PUBLIC_CONSTANT(LUA_TBOOLEAN);
REQUIRE_PUBLIC_CONSTANT(LUA_TLIGHTUSERDATA);
REQUIRE_PUBLIC_CONSTANT(LUA_TNUMBER);
REQUIRE_PUBLIC_CONSTANT(LUA_TSTRING);
REQUIRE_PUBLIC_CONSTANT(LUA_TTABLE);
REQUIRE_PUBLIC_CONSTANT(LUA_TFUNCTION);
REQUIRE_PUBLIC_CONSTANT(LUA_TUSERDATA);
REQUIRE_PUBLIC_CONSTANT(LUA_TTHREAD);

REQUIRE_PUBLIC_TYPE(lua_State);
REQUIRE_PUBLIC_TYPE(lua_Number);
REQUIRE_PUBLIC_TYPE(lua_Integer);
REQUIRE_PUBLIC_TYPE(lua_CFunction);
REQUIRE_PUBLIC_TYPE(lua_Alloc);
REQUIRE_PUBLIC_TYPE(lua_Reader);
REQUIRE_PUBLIC_TYPE(lua_Writer);
REQUIRE_PUBLIC_TYPE(luaL_Reg);

static_assert(std::string_view(LUA_VERSION) == "Lua 5.1");
static_assert(std::string_view(LUA_RELEASE) == "Lua 5.1.5");
static_assert(LUA_VERSION_NUM == 501);
static_assert(LUA_MULTRET == -1);
static_assert(LUA_REGISTRYINDEX == -10000);
static_assert(LUA_ENVIRONINDEX == -10001);
static_assert(LUA_GLOBALSINDEX == -10002);
static_assert(lua_upvalueindex(1) == LUA_GLOBALSINDEX - 1);
static_assert(LUA_OK == 0 && LUA_YIELD == 1 && LUA_ERRRUN == 2 && LUA_ERRSYNTAX == 3 && LUA_ERRMEM == 4 &&
              LUA_ERRERR == 5 && LUA_ERRFILE == 6);
static_assert(LUA_TNONE == -1 && LUA_TNIL == 0 && LUA_TBOOLEAN == 1 && LUA_TLIGHTUSERDATA == 2 && LUA_TNUMBER == 3 &&
              LUA_TSTRING == 4 && LUA_TTABLE == 5 && LUA_TFUNCTION == 6 && LUA_TUSERDATA == 7 && LUA_TTHREAD == 8);
static_assert(LUA_NOREF == -2 && LUA_REFNIL == -1);

static_assert(std::is_same_v<lua_Number, double>);
static_assert(std::is_same_v<lua_Integer, std::ptrdiff_t>);
static_assert(std::is_same_v<lua_CFunction, int (*)(lua_State*)>);
static_assert(std::is_same_v<lua_Alloc, void* (*)(void*, void*, size_t, size_t)>);
static_assert(std::is_same_v<lua_Reader, const char* (*)(lua_State*, void*, size_t*)>);
static_assert(std::is_same_v<lua_Writer, int (*)(lua_State*, const void*, size_t, void*)>);
static_assert(std::is_standard_layout_v<luaL_Reg>);
static_assert(std::is_same_v<decltype(luaL_Reg::name), const char*>);
static_assert(std::is_same_v<decltype(luaL_Reg::func), lua_CFunction>);
static_assert(offsetof(luaL_Reg, name) == 0);
static_assert(offsetof(luaL_Reg, func) == sizeof(const char*));
static_assert(sizeof(luaL_Reg) == sizeof(const char*) + sizeof(lua_CFunction));

#undef REQUIRE_PUBLIC_TYPE
#undef REQUIRE_PUBLIC_CONSTANT
#undef REQUIRE_PUBLIC_MACRO
#undef REQUIRE_SIGNATURE

namespace {

const char* throwingReader(lua_State*, void*, size_t*) {
    throw std::runtime_error("consumer reader failure");
}

int publicNoop(lua_State*) {
    return 0;
}

bool exercisePublicMacros(lua_State* state) {
    const int originalTop = lua_gettop(state);
    bool valid = lua_upvalueindex(1) == LUA_GLOBALSINDEX - 1;

    lua_newtable(state);
    valid = valid && lua_istable(state, -1);
    lua_pop(state, 1);

    lua_pushcclosure(state, publicNoop, 0);
    valid = valid && lua_isfunction(state, -1);
    lua_pop(state, 1);

    lua_pushboolean(state, 1);
    valid = valid && lua_isboolean(state, -1);
    lua_pop(state, 1);

    lua_pushnil(state);
    valid = valid && lua_isnil(state, -1) && lua_isnoneornil(state, -1);
    lua_pop(state, 1);

    valid = valid && lua_isnone(state, lua_gettop(state) + 1) && lua_isnoneornil(state, lua_gettop(state) + 1);

    lua_State* thread = lua_newthread(state);
    valid = valid && thread != nullptr && lua_isthread(state, -1);
    lua_pop(state, 1);

    lua_State* safeThread = lua_trynewthread(state);
    valid = valid && safeThread != nullptr && lua_isthread(state, -1);
    lua_pop(state, 1);

    lua_pushstring(state, "macro-ref");
    const int reference = luaL_ref(state, LUA_REGISTRYINDEX);
    luaL_getref(state, reference);
    const char* referencedText = lua_tostring(state, -1);
    valid = valid && referencedText != nullptr && std::string_view(referencedText) == "macro-ref";
    lua_pop(state, 1);
    luaL_unref(state, LUA_REGISTRYINDEX, reference);

    lua_settop(state, originalTop);
    return valid;
}

} // namespace

int main() {
    static_assert(noexcept(lua_checkstack(nullptr, 0)));
    static_assert(noexcept(lua_pcall(nullptr, 0, 0, 0)));
    static_assert(!noexcept(lua_newthread(nullptr)));
    static_assert(noexcept(lua_trynewthread(nullptr)));
    static_assert(noexcept(lua_close(nullptr)));
    static_assert(noexcept(lua_resume(nullptr, 0)));
    static_assert(noexcept(lua_load(nullptr, nullptr, nullptr, nullptr)));
    static_assert(noexcept(lua_dump(nullptr, nullptr, nullptr)));
    static_assert(noexcept(luaL_loadbuffer(nullptr, nullptr, 0, nullptr)));
    static_assert(noexcept(luaL_loadstring(nullptr, nullptr)));
    static_assert(noexcept(luaL_loadfile(nullptr, nullptr)));
    static_assert(!noexcept(lua_call(nullptr, 0, 0)));
    static_assert(!noexcept(lua_error(nullptr)));

    if (lua_public_c_header_probe() != 76) {
        return 1;
    }

    lua_State* state = lua_open();
    if (state == nullptr) {
        return 2;
    }

    if (!exercisePublicMacros(state)) {
        lua_close(state);
        return 3;
    }

    lua_pushinteger(state, 42);
    lua_setglobal(state, "public_contract_value");
    lua_getglobal(state, "public_contract_value");
    luaL_argcheck(state, lua_isnumber(state, -1), 1, "global number expected");
    if (luaL_checkint(state, -1) != 42) {
        lua_close(state);
        return 4;
    }
    lua_pop(state, 1);

    lua_pushstring(state, "public-contract");
    if (std::string_view(luaL_checkstring(state, -1)) != "public-contract") {
        lua_close(state);
        return 5;
    }
    lua_pop(state, 1);

    lua_pushstring(state, nullptr);
    if (!lua_isnil(state, -1)) {
        lua_close(state);
        return 6;
    }
    lua_pop(state, 1);

    lua_pushnumber(state, 12345);
    if (lua_objlen(state, -1) != 5 || lua_type(state, -1) != LUA_TSTRING) {
        lua_close(state);
        return 7;
    }
    lua_pop(state, 1);

    lua_pushnumber(state, 73);
    int loadStatus = LUA_OK;
    try {
        loadStatus = lua_load(state, throwingReader, nullptr, "=consumer-reader");
    } catch (...) {
        lua_close(state);
        return 8;
    }
    if (loadStatus != LUA_ERRRUN || lua_gettop(state) != 2 || lua_tonumber(state, 1) != 73 ||
        std::string(lua_tostring(state, -1)) != "consumer reader failure") {
        lua_close(state);
        return 9;
    }

    lua_settop(state, 0);
    lua_pushnumber(state, 1);
    bool unprotectedCallThrew = false;
    try {
        lua_call(state, 0, 0);
    } catch (...) {
        unprotectedCallThrew = true;
    }
    lua_close(state);
    return unprotectedCallThrew ? 0 : 10;
}
