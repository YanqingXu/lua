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

static int return_first_upvalue(lua_State* L) {
    lua_pushvalue(L, lua_upvalueindex(1));
    return 1;
}

static int return_aux_meta(lua_State* L) {
    lua_pushstring(L, "meta-value");
    return 1;
}

static int reject_aux_type(lua_State* L) {
    return luaL_typerror(L, 1, "widget");
}

static int reject_aux_argument(lua_State* L) {
    return luaL_argerror(L, 1, "argument failure");
}

static int reject_aux_error(lua_State* L) {
    return luaL_error(L, "auxiliary failure");
}

static int reject_aux_option(lua_State* L) {
    static const char* const options[] = {"alpha", "beta", NULL};
    return luaL_checkoption(L, 1, NULL, options);
}

static int reject_aux_userdata(lua_State* L) {
    (void)luaL_checkudata(L, 1, "aux.widget");
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
    int aux_checks;
    int aux_meta;
    int aux_register;
    int aux_table;
    int aux_buffer;
    int aux_errors;
    int aux_newstate;
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

    lua_settop(L, 0);
    lua_pushnumber(L, 18);
    lua_pushstring(L, "beta");
    lua_pushnil(L);
    aux_checks = luaL_checknumber(L, 1) == 18 && luaL_checkinteger(L, 1) == 18;
    aux_checks = aux_checks && strcmp(luaL_checklstring(L, 2, &length), "beta") == 0 && length == 4;
    aux_checks = aux_checks && luaL_optinteger(L, 3, 41) == 41 && luaL_optnumber(L, 4, 12.5) == 12.5;
    aux_checks = aux_checks && strcmp(luaL_optlstring(L, 3, "fallback", &length), "fallback") == 0 && length == 8;
    luaL_checktype(L, 2, LUA_TSTRING);
    luaL_checkany(L, 3);
    luaL_checkstack(L, 8, "probe");
    {
        static const char* const options[] = {"alpha", "beta", NULL};
        aux_checks = aux_checks && luaL_checkoption(L, 2, NULL, options) == 1;
        aux_checks = aux_checks && luaL_checkoption(L, 4, "alpha", options) == 0;
    }
    luaL_where(L, 0);
    aux_checks = aux_checks && strcmp(lua_tostring(L, -1), "") == 0;
    lua_pop(L, 1);

    lua_settop(L, 0);
    aux_meta = luaL_newmetatable(L, "aux.widget") == 1;
    lua_pushcclosure(L, return_aux_meta, 0);
    lua_setfield(L, -2, "__probe");
    lua_pop(L, 1);
    aux_meta = aux_meta && luaL_newmetatable(L, "aux.widget") == 0;
    lua_pop(L, 1);
    {
        void* payload = lua_newuserdata(L, sizeof(int));
        luaL_getmetatable(L, "aux.widget");
        aux_meta = aux_meta && lua_setmetatable(L, -2) == 1;
        aux_meta = aux_meta && luaL_checkudata(L, -1, "aux.widget") == payload;
        aux_meta = aux_meta && luaL_getmetafield(L, -1, "__probe") == 1;
        lua_pop(L, 1);
        aux_meta = aux_meta && luaL_callmeta(L, -1, "__probe") == 1;
        aux_meta = aux_meta && strcmp(lua_tostring(L, -1), "meta-value") == 0;
    }

    lua_settop(L, 0);
    {
        static const luaL_Reg captured[] = {{"value", return_first_upvalue}, {NULL, NULL}};
        static const luaL_Reg plain[] = {{"probe", return_aux_meta}, {NULL, NULL}};
        lua_pushinteger(L, 73);
        luaL_openlib(L, "aux.probe", captured, 1);
        lua_getfield(L, 1, "value");
        aux_register = lua_pcall(L, 0, 1, 0) == 0 && lua_tonumber(L, -1) == 73;
        lua_pop(L, 1);
        lua_getglobal(L, "aux");
        lua_getfield(L, -1, "probe");
        aux_register = aux_register && lua_rawequal(L, 1, -1);
        lua_settop(L, 0);
        lua_newtable(L);
        luaL_register(L, NULL, plain);
        lua_getfield(L, -1, "probe");
        aux_register = aux_register && lua_pcall(L, 0, 1, 0) == 0;
        aux_register = aux_register && strcmp(lua_tostring(L, -1), "meta-value") == 0;
    }

    lua_settop(L, 0);
    lua_newtable(L);
    aux_table = luaL_findtable(L, -1, "one.two", 2) == NULL && lua_istable(L, -1);
    lua_pop(L, 1);
    lua_pushnumber(L, 5);
    lua_setfield(L, -2, "blocked");
    aux_table = aux_table && strcmp(luaL_findtable(L, -1, "blocked.child", 1), "blocked.child") == 0;
    lua_settop(L, 0);
    aux_table = aux_table && strcmp(luaL_gsub(L, "a-b-a", "a", "xy"), "xy-b-xy") == 0;

    lua_settop(L, 0);
    {
        static const char embedded[] = {'\0', 'x'};
        static const char expected[] = {'p', 'r', 'e', 'f', 'i', 'x', '\0', 'x', 'A', 't', 'a', 'i', 'l', '!'};
        luaL_Buffer buffer;
        char* prepared;
        const char* result;
        luaL_buffinit(L, &buffer);
        luaL_addstring(&buffer, "prefix");
        luaL_addlstring(&buffer, embedded, sizeof(embedded));
        prepared = luaL_prepbuffer(&buffer);
        aux_buffer = prepared == buffer.buffer;
        prepared[0] = 'A';
        luaL_addsize(&buffer, 1);
        lua_pushstring(L, "tail");
        luaL_addvalue(&buffer);
        luaL_addchar(&buffer, '!');
        luaL_pushresult(&buffer);
        result = lua_tolstring(L, -1, &length);
        aux_buffer = aux_buffer && length == sizeof(expected) && memcmp(result, expected, sizeof(expected)) == 0;
    }

    lua_settop(L, 0);
    lua_pushcclosure(L, reject_aux_type, 0);
    aux_errors = lua_pcall(L, 0, 0, 0) == LUA_ERRRUN;
    lua_pop(L, 1);
    lua_pushcclosure(L, reject_aux_argument, 0);
    aux_errors = aux_errors && lua_pcall(L, 0, 0, 0) == LUA_ERRRUN;
    lua_pop(L, 1);
    lua_pushcclosure(L, reject_aux_error, 0);
    aux_errors = aux_errors && lua_pcall(L, 0, 0, 0) == LUA_ERRRUN;
    lua_pop(L, 1);
    lua_pushcclosure(L, reject_aux_option, 0);
    lua_pushstring(L, "gamma");
    aux_errors = aux_errors && lua_pcall(L, 1, 0, 0) == LUA_ERRRUN;
    lua_pop(L, 1);
    lua_pushcclosure(L, reject_aux_userdata, 0);
    lua_pushnumber(L, 1);
    aux_errors = aux_errors && lua_pcall(L, 1, 0, 0) == LUA_ERRRUN;
    lua_pop(L, 1);

    {
        lua_State* auxiliary = luaL_newstate();
        aux_newstate = auxiliary != NULL;
        if (auxiliary != NULL) {
            lua_close(auxiliary);
        }
    }

    printf("table=%d,%d,%d,%d,%d,%d;next=%d,%d;compare=%d,%d,%d;concat=%d,%d,%d,%d,%d;"
           "convert=%d,%d,%d;pointer=%d;thread=%d,%d;gc=%d;aux=%d,%d,%d,%d,%d,%d,%d\n",
           set_top, field_value, raw_value, meta_value, raw_missing, captured_value, iteration_count, iteration_sum,
           equal_with_meta, equal_without_meta, less_with_meta, concat_zero_length, concat_plain, concat_meta,
           concat_error_status, concat_error_top, integer_number, integer_string, cfunction_identity, pointer_identity,
           main_thread_identity, child_thread_identity, gc_contract, aux_checks, aux_meta, aux_register, aux_table,
           aux_buffer, aux_errors, aux_newstate);

    lua_close(L);
    return 0;
}
