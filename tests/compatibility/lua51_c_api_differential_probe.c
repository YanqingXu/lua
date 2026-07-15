#include "lua.h"
#include "lauxlib.h"

#include <stdio.h>
#include <string.h>

static int return_fallback(lua_State* L) {
    lua_pushstring(L, "fallback");
    return 1;
}

static int capture_new_field(lua_State* L) {
    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, lua_upvalueindex(1));
    return 0;
}

static int compare_ids_equal(lua_State* L) {
    lua_getfield(L, 1, "id");
    lua_getfield(L, 2, "id");
    lua_pushboolean(L, lua_tonumber(L, -2) == lua_tonumber(L, -1));
    return 1;
}

static int compare_ids_less(lua_State* L) {
    lua_getfield(L, 1, "id");
    lua_getfield(L, 2, "id");
    lua_pushboolean(L, lua_tonumber(L, -2) < lua_tonumber(L, -1));
    return 1;
}

static int return_joined(lua_State* L) {
    lua_pushstring(L, "joined");
    return 1;
}

static int reject_bad_concat(lua_State* L) {
    lua_pushboolean(L, 1);
    lua_pushboolean(L, 0);
    lua_concat(L, 2);
    return 0;
}

int main(void) {
    lua_State* L = lua_open();
    int set_top;
    int field_value;
    int raw_value;
    int meta_value;
    int raw_missing;
    int captured_value;
    int iteration_count = 0;
    int iteration_sum = 0;
    int equal_with_meta;
    int equal_without_meta;
    int less_with_meta;
    int concat_zero_length;
    int concat_plain;
    int concat_meta;
    int concat_error_status;
    int concat_error_top;
    int integer_number;
    int integer_string;
    int cfunction_identity;
    int pointer_identity;
    int main_thread_identity;
    int child_thread_identity;
    int gc_contract;
    size_t length = 0;

    if (L == NULL) {
        return 2;
    }

    lua_newtable(L);
    lua_pushnumber(L, 7);
    lua_setfield(L, 1, "answer");
    set_top = lua_gettop(L);
    lua_getfield(L, 1, "answer");
    field_value = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);
    lua_pushstring(L, "answer");
    lua_rawget(L, 1);
    raw_value = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_newtable(L);
    lua_pushcclosure(L, return_fallback, 0);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, 1);
    lua_getfield(L, 1, "missing");
    meta_value = strcmp(lua_tostring(L, -1), "fallback") == 0;
    lua_pop(L, 1);
    lua_pushstring(L, "missing");
    lua_rawget(L, 1);
    raw_missing = lua_isnil(L, -1);
    lua_pop(L, 1);

    lua_newtable(L);
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_pushcclosure(L, capture_new_field, 1);
    lua_setfield(L, 3, "__newindex");
    lua_pop(L, 1);
    lua_pushnumber(L, 12);
    lua_setfield(L, 1, "captured");
    lua_getfield(L, 2, "captured");
    captured_value = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushnumber(L, 22);
    lua_rawseti(L, 1, 2);
    lua_pushnumber(L, 11);
    lua_setfield(L, 1, "alpha");
    lua_pushnil(L);
    while (lua_next(L, 1) != 0) {
        ++iteration_count;
        iteration_sum += (int)lua_tonumber(L, -1);
        lua_pop(L, 1);
    }

    lua_settop(L, 0);
    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_setfield(L, 1, "id");
    lua_newtable(L);
    lua_pushnumber(L, 1);
    lua_setfield(L, 2, "id");
    lua_newtable(L);
    lua_pushcclosure(L, compare_ids_equal, 0);
    lua_setfield(L, 3, "__eq");
    lua_pushcclosure(L, compare_ids_less, 0);
    lua_setfield(L, 3, "__lt");
    lua_pushvalue(L, 3);
    lua_setmetatable(L, 1);
    lua_pushvalue(L, 3);
    lua_setmetatable(L, 2);
    equal_with_meta = lua_equal(L, 1, 2);
    equal_without_meta = lua_rawequal(L, 1, 2);
    lua_pushnumber(L, 2);
    lua_setfield(L, 2, "id");
    less_with_meta = lua_lessthan(L, 1, 2);

    lua_settop(L, 0);
    lua_concat(L, 0);
    lua_tolstring(L, -1, &length);
    concat_zero_length = length == 0;
    lua_settop(L, 0);
    lua_pushstring(L, "value=");
    lua_pushnumber(L, 12);
    lua_concat(L, 2);
    concat_plain = strcmp(lua_tostring(L, -1), "value=12") == 0;

    lua_settop(L, 0);
    lua_newtable(L);
    lua_newtable(L);
    lua_newtable(L);
    lua_pushcclosure(L, return_joined, 0);
    lua_setfield(L, 3, "__concat");
    lua_pushvalue(L, 3);
    lua_setmetatable(L, 1);
    lua_pushvalue(L, 3);
    lua_setmetatable(L, 2);
    lua_remove(L, 3);
    lua_concat(L, 2);
    concat_meta = strcmp(lua_tostring(L, -1), "joined") == 0;

    lua_settop(L, 0);
    lua_pushcclosure(L, reject_bad_concat, 0);
    concat_error_status = lua_pcall(L, 0, 0, 0);
    concat_error_top = lua_gettop(L);

    lua_settop(L, 0);
    lua_pushnumber(L, 42.0);
    lua_pushstring(L, "-17");
    integer_number = (int)lua_tointeger(L, 1);
    integer_string = (int)lua_tointeger(L, 2);
    lua_pushcclosure(L, return_fallback, 0);
    cfunction_identity = lua_tocfunction(L, 3) == return_fallback;

    lua_settop(L, 0);
    lua_newtable(L);
    pointer_identity = lua_topointer(L, 1) != NULL;
    lua_pushvalue(L, 1);
    pointer_identity = pointer_identity && lua_topointer(L, 1) == lua_topointer(L, 2);

    lua_settop(L, 0);
    main_thread_identity = lua_pushthread(L) == 1 && lua_tothread(L, -1) == L && lua_topointer(L, -1) == L;
    lua_settop(L, 0);
    {
        lua_State* child = lua_newthread(L);
        child_thread_identity = child != NULL && lua_tothread(L, -1) == child && lua_topointer(L, -1) == child;
        child_thread_identity = child_thread_identity && lua_pushthread(child) == 0 &&
                                lua_tothread(child, -1) == child && lua_topointer(child, -1) == child;
        lua_pop(child, 1);
    }
    lua_settop(L, 0);

    gc_contract = lua_gc(L, LUA_GCSETPAUSE, 201) == 200;
    gc_contract = gc_contract && lua_gc(L, LUA_GCSETPAUSE, 200) == 201;
    gc_contract = gc_contract && lua_gc(L, LUA_GCSETSTEPMUL, 301) == 200;
    gc_contract = gc_contract && lua_gc(L, LUA_GCSETSTEPMUL, 200) == 301;
    gc_contract = gc_contract && lua_gc(L, LUA_GCSTOP, 0) == 0;
    gc_contract = gc_contract && lua_gc(L, LUA_GCRESTART, 0) == 0;
    gc_contract = gc_contract && lua_gc(L, LUA_GCCOLLECT, 0) == 0;
    gc_contract = gc_contract && lua_gc(L, LUA_GCCOUNT, 0) >= 0;
    gc_contract = gc_contract && lua_gc(L, LUA_GCCOUNTB, 0) >= 0 && lua_gc(L, LUA_GCCOUNTB, 0) < 1024;
    gc_contract = gc_contract && lua_gc(L, 999, 0) == -1;

    printf("table=%d,%d,%d,%d,%d,%d;next=%d,%d;compare=%d,%d,%d;concat=%d,%d,%d,%d,%d;"
           "convert=%d,%d,%d;pointer=%d;thread=%d,%d;gc=%d\n",
           set_top, field_value, raw_value, meta_value, raw_missing, captured_value, iteration_count, iteration_sum,
           equal_with_meta, equal_without_meta, less_with_meta, concat_zero_length, concat_plain, concat_meta,
           concat_error_status, concat_error_top, integer_number, integer_string, cfunction_identity, pointer_identity,
           main_thread_identity, child_thread_identity, gc_contract);

    lua_close(L);
    return 0;
}
