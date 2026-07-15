#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

/*
 * This translation unit is deliberately C, not C++. Every real function in
 * the project's public headers must appear here. Each evaluated assignment to
 * the volatile function-pointer sink creates a real relocation, so both the C
 * compiler and the static/shared final linker must resolve the full surface.
 */
static volatile lua_CFunction lua_public_link_sink;

int lua_public_c_header_probe(void) {
    int declarations = 0;
#define REQUIRE_SYMBOL(name)                                                                                           \
    do {                                                                                                               \
        lua_public_link_sink = (lua_CFunction)(name);                                                                  \
        if (lua_public_link_sink == NULL) {                                                                            \
            return -1;                                                                                                 \
        }                                                                                                              \
        ++declarations;                                                                                                \
    } while (0)

    REQUIRE_SYMBOL(lua_newstate);
    REQUIRE_SYMBOL(lua_open);
    REQUIRE_SYMBOL(lua_close);
    REQUIRE_SYMBOL(lua_newthread);
    REQUIRE_SYMBOL(lua_trynewthread);
    REQUIRE_SYMBOL(lua_gettop);
    REQUIRE_SYMBOL(lua_settop);
    REQUIRE_SYMBOL(lua_pushvalue);
    REQUIRE_SYMBOL(lua_remove);
    REQUIRE_SYMBOL(lua_insert);
    REQUIRE_SYMBOL(lua_replace);
    REQUIRE_SYMBOL(lua_checkstack);
    REQUIRE_SYMBOL(lua_xmove);
    REQUIRE_SYMBOL(lua_isnumber);
    REQUIRE_SYMBOL(lua_isstring);
    REQUIRE_SYMBOL(lua_iscfunction);
    REQUIRE_SYMBOL(lua_isuserdata);
    REQUIRE_SYMBOL(lua_equal);
    REQUIRE_SYMBOL(lua_rawequal);
    REQUIRE_SYMBOL(lua_lessthan);
    REQUIRE_SYMBOL(lua_type);
    REQUIRE_SYMBOL(lua_typename);
    REQUIRE_SYMBOL(lua_tonumber);
    REQUIRE_SYMBOL(lua_tointeger);
    REQUIRE_SYMBOL(lua_toboolean);
    REQUIRE_SYMBOL(lua_tolstring);
    REQUIRE_SYMBOL(lua_tocfunction);
    REQUIRE_SYMBOL(lua_tothread);
    REQUIRE_SYMBOL(lua_topointer);
    REQUIRE_SYMBOL(lua_objlen);
    REQUIRE_SYMBOL(lua_touserdata);
    REQUIRE_SYMBOL(lua_pushnil);
    REQUIRE_SYMBOL(lua_pushnumber);
    REQUIRE_SYMBOL(lua_pushinteger);
    REQUIRE_SYMBOL(lua_pushlstring);
    REQUIRE_SYMBOL(lua_pushstring);
    REQUIRE_SYMBOL(lua_pushcclosure);
    REQUIRE_SYMBOL(lua_pushboolean);
    REQUIRE_SYMBOL(lua_pushlightuserdata);
    REQUIRE_SYMBOL(lua_pushthread);
    REQUIRE_SYMBOL(lua_gettable);
    REQUIRE_SYMBOL(lua_getfield);
    REQUIRE_SYMBOL(lua_rawget);
    REQUIRE_SYMBOL(lua_getglobal);
    REQUIRE_SYMBOL(lua_rawgeti);
    REQUIRE_SYMBOL(lua_createtable);
    REQUIRE_SYMBOL(lua_newuserdata);
    REQUIRE_SYMBOL(lua_getmetatable);
    REQUIRE_SYMBOL(lua_settable);
    REQUIRE_SYMBOL(lua_setfield);
    REQUIRE_SYMBOL(lua_rawset);
    REQUIRE_SYMBOL(lua_setglobal);
    REQUIRE_SYMBOL(lua_rawseti);
    REQUIRE_SYMBOL(lua_setmetatable);
    REQUIRE_SYMBOL(lua_call);
    REQUIRE_SYMBOL(lua_pcall);
    REQUIRE_SYMBOL(lua_load);
    REQUIRE_SYMBOL(lua_dump);
    REQUIRE_SYMBOL(lua_yield);
    REQUIRE_SYMBOL(lua_resume);
    REQUIRE_SYMBOL(lua_status);
    REQUIRE_SYMBOL(lua_error);
    REQUIRE_SYMBOL(lua_gc);
    REQUIRE_SYMBOL(lua_next);
    REQUIRE_SYMBOL(lua_concat);
    REQUIRE_SYMBOL(lua_getallocf);
    REQUIRE_SYMBOL(lua_setallocf);
    REQUIRE_SYMBOL(lua_getupvalue);
    REQUIRE_SYMBOL(lua_setupvalue);
    REQUIRE_SYMBOL(luaL_argcheck);
    REQUIRE_SYMBOL(luaL_argerror);
    REQUIRE_SYMBOL(luaL_typerror);
    REQUIRE_SYMBOL(luaL_getmetafield);
    REQUIRE_SYMBOL(luaL_callmeta);
    REQUIRE_SYMBOL(luaL_checklstring);
    REQUIRE_SYMBOL(luaL_optlstring);
    REQUIRE_SYMBOL(luaL_checknumber);
    REQUIRE_SYMBOL(luaL_optnumber);
    REQUIRE_SYMBOL(luaL_checkinteger);
    REQUIRE_SYMBOL(luaL_optinteger);
    REQUIRE_SYMBOL(luaL_checkint);
    REQUIRE_SYMBOL(luaL_checkstring);
    REQUIRE_SYMBOL(luaL_checkstack);
    REQUIRE_SYMBOL(luaL_checktype);
    REQUIRE_SYMBOL(luaL_checkany);
    REQUIRE_SYMBOL(luaL_newmetatable);
    REQUIRE_SYMBOL(luaL_checkudata);
    REQUIRE_SYMBOL(luaL_where);
    REQUIRE_SYMBOL(luaL_checkoption);
    REQUIRE_SYMBOL(luaL_error);
    REQUIRE_SYMBOL(luaL_ref);
    REQUIRE_SYMBOL(luaL_unref);
    REQUIRE_SYMBOL(luaL_loadfile);
    REQUIRE_SYMBOL(luaL_loadbuffer);
    REQUIRE_SYMBOL(luaL_loadstring);
    REQUIRE_SYMBOL(luaL_newstate);
    REQUIRE_SYMBOL(luaL_gsub);
    REQUIRE_SYMBOL(luaL_findtable);
    REQUIRE_SYMBOL(luaL_buffinit);
    REQUIRE_SYMBOL(luaL_prepbuffer);
    REQUIRE_SYMBOL(luaL_addlstring);
    REQUIRE_SYMBOL(luaL_addstring);
    REQUIRE_SYMBOL(luaL_addvalue);
    REQUIRE_SYMBOL(luaL_pushresult);
    REQUIRE_SYMBOL(luaL_openlib);
    REQUIRE_SYMBOL(luaL_register);
    REQUIRE_SYMBOL(luaL_openlibs);

#undef REQUIRE_SYMBOL
    return declarations;
}
