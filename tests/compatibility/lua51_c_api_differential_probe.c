#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static int final_api_token;
static int cpcall_argument_ok;

static int first_panic(lua_State* L) {
    (void)L;
    return 0;
}

static int second_panic(lua_State* L) {
    (void)L;
    return 0;
}

static const char* push_vformat(lua_State* L, const char* format, ...) {
    const char* result;
    va_list arguments;
    va_start(arguments, format);
    result = lua_pushvfstring(L, format, arguments);
    va_end(arguments);
    return result;
}

static int capture_cpcall_argument(lua_State* L) {
    cpcall_argument_ok = lua_gettop(L) == 1 && lua_touserdata(L, 1) == &final_api_token;
    return 0;
}

static int fail_cpcall(lua_State* L) {
    lua_pushlightuserdata(L, &final_api_token);
    return lua_error(L);
}

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

static int call_luaopen_base(lua_State* L) {
    return luaopen_base(L);
}

static int call_luaopen_table(lua_State* L) {
    return luaopen_table(L);
}

static int call_luaopen_io(lua_State* L) {
    return luaopen_io(L);
}

static int call_luaopen_os(lua_State* L) {
    return luaopen_os(L);
}

static int call_luaopen_string(lua_State* L) {
    return luaopen_string(L);
}

static int call_luaopen_math(lua_State* L) {
    return luaopen_math(L);
}

static int call_luaopen_debug(lua_State* L) {
    return luaopen_debug(L);
}

static int call_luaopen_package(lua_State* L) {
    return luaopen_package(L);
}

static int run_library_opener(lua_State* L, lua_CFunction opener, const char* name) {
    lua_settop(L, 0);
    lua_pushcclosure(L, opener, 0);
    lua_pushstring(L, name);
    lua_call(L, 1, LUA_MULTRET);
    return lua_gettop(L);
}

static int debug_stack_info;
static int debug_hook_calls;
static int debug_hook_returns;
static int debug_hook_lines;
static int debug_hook_counts;
static int debug_hook_line_info;

static int inspect_debug_caller(lua_State* L) {
    lua_Debug current;
    lua_Debug caller;
    const char* local_name;
    const char* set_name;
    int current_ok;
    int caller_ok;

    memset(&current, 0, sizeof(current));
    memset(&caller, 0, sizeof(caller));
    current_ok = lua_getstack(L, 0, &current) != 0 && lua_getinfo(L, "Slu", &current) != 0;
    current_ok = current_ok && strcmp(current.what, "C") == 0 && current.currentline == -1;
    caller_ok = lua_getstack(L, 1, &caller) != 0 && lua_getinfo(L, "SlnufL", &caller) != 0;
    caller_ok = caller_ok && strcmp(caller.what, "main") == 0 && caller.currentline > 0;
    caller_ok = caller_ok && lua_isfunction(L, -2) && lua_istable(L, -1);
    lua_pop(L, 2);

    local_name = lua_getlocal(L, &caller, 1);
    caller_ok = caller_ok && local_name != NULL && strcmp(local_name, "debug_value") == 0;
    caller_ok = caller_ok && lua_tonumber(L, -1) == 7;
    lua_pop(L, 1);
    lua_pushinteger(L, 19);
    set_name = lua_setlocal(L, &caller, 1);
    caller_ok = caller_ok && set_name != NULL && strcmp(set_name, "debug_value") == 0;

    debug_stack_info = current_ok && caller_ok;
    return 0;
}

static void capture_debug_hook(lua_State* L, lua_Debug* ar) {
    if (ar->event == LUA_HOOKCALL) {
        ++debug_hook_calls;
    } else if (ar->event == LUA_HOOKRET || ar->event == LUA_HOOKTAILRET) {
        ++debug_hook_returns;
    } else if (ar->event == LUA_HOOKLINE) {
        ++debug_hook_lines;
        debug_hook_line_info = lua_getinfo(L, "l", ar) != 0 && ar->currentline > 0;
    } else if (ar->event == LUA_HOOKCOUNT) {
        ++debug_hook_counts;
    }
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
    int debug_contract;
    int debug_function;
    int debug_stack;
    int debug_mutation;
    int debug_hook_config;
    int debug_hook_run;
    int debug_hook_events;
    int debug_disable;
    int debug_invalid;
    int standard_library_contract;
    int panic_contract;
    int format_contract;
    int environment_contract;
    int cpcall_contract;
    int setlevel_contract;
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

    lua_settop(L, 0);
    {
        lua_Debug function_info;
        int function_query;
        memset(&function_info, 0, sizeof(function_info));
        lua_pushcclosure(L, inspect_debug_caller, 0);
        function_query = lua_getinfo(L, ">Suf", &function_info) == 1;
        function_query = function_query && strcmp(function_info.what, "C") == 0;
        function_query = function_query && strcmp(function_info.source, "=[C]") == 0;
        function_query = function_query && strcmp(function_info.short_src, "[C]") == 0;
        function_query = function_query && function_info.nups == 0 && lua_isfunction(L, -1);
        lua_pop(L, 1);

        lua_pushcclosure(L, inspect_debug_caller, 0);
        lua_setglobal(L, "inspect_debug");
        luaL_loadstring(L, "local debug_value = 7\ninspect_debug()\nreturn debug_value");
        debug_function = function_query;
        debug_mutation = lua_pcall(L, 0, 1, 0) == 0 && lua_tonumber(L, -1) == 19;
        debug_stack = debug_stack_info;
        lua_pop(L, 1);

        debug_hook_calls = 0;
        debug_hook_returns = 0;
        debug_hook_lines = 0;
        debug_hook_counts = 0;
        debug_hook_line_info = 0;
        debug_hook_config =
            lua_sethook(L, capture_debug_hook, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT, 2) == 1;
        debug_hook_config = debug_hook_config && lua_gethook(L) == capture_debug_hook;
        debug_hook_config =
            debug_hook_config && lua_gethookmask(L) == (LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT);
        debug_hook_config = debug_hook_config && lua_gethookcount(L) == 2;
        luaL_loadstring(L, "local total = 0\nfor i = 1, 3 do total = total + i end\nreturn total");
        debug_hook_run = lua_pcall(L, 0, 1, 0) == 0 && lua_tonumber(L, -1) == 6;
        lua_pop(L, 1);
        debug_hook_events = debug_hook_calls > 0 && debug_hook_returns > 0;
        debug_hook_events = debug_hook_events && debug_hook_lines > 0 && debug_hook_line_info && debug_hook_counts > 0;
        debug_disable = lua_sethook(L, capture_debug_hook, 0, 9) == 1;
        debug_disable = debug_disable && lua_gethook(L) == NULL && lua_gethookmask(L) == 0;
        debug_disable = debug_disable && lua_gethookcount(L) == 9;
        lua_sethook(L, NULL, 0, 0);
        debug_disable = debug_disable && lua_gethookcount(L) == 0;

        {
            lua_Debug missing;
            debug_invalid = lua_getstack(L, -1, &missing) == 1;
            debug_invalid = debug_invalid && lua_getstack(L, 99, &missing) == 0;
        }
        debug_contract = debug_function && debug_stack && debug_mutation && debug_hook_config && debug_hook_run &&
                         debug_hook_events && debug_disable && debug_invalid;
    }

    {
        lua_State* libraries = lua_open();
        standard_library_contract = libraries != NULL;
        if (libraries != NULL) {
            int result_count;

            result_count = run_library_opener(libraries, call_luaopen_base, "");
            standard_library_contract = standard_library_contract && result_count == 2 && lua_gettop(libraries) == 2;
            standard_library_contract =
                standard_library_contract && lua_istable(libraries, 1) && lua_istable(libraries, 2);
            lua_getglobal(libraries, "_G");
            standard_library_contract = standard_library_contract && lua_rawequal(libraries, 1, -1);
            lua_pop(libraries, 1);
            lua_getglobal(libraries, LUA_COLIBNAME);
            standard_library_contract = standard_library_contract && lua_rawequal(libraries, 2, -1);

            result_count = run_library_opener(libraries, call_luaopen_table, LUA_TABLIBNAME);
            standard_library_contract = standard_library_contract && result_count == 1 && lua_istable(libraries, -1);
            lua_getglobal(libraries, LUA_TABLIBNAME);
            standard_library_contract = standard_library_contract && lua_rawequal(libraries, -1, -2);

            result_count = run_library_opener(libraries, call_luaopen_io, LUA_IOLIBNAME);
            standard_library_contract = standard_library_contract && result_count == 1 && lua_istable(libraries, -1);
            lua_getglobal(libraries, LUA_IOLIBNAME);
            standard_library_contract = standard_library_contract && lua_rawequal(libraries, -1, -2);

            result_count = run_library_opener(libraries, call_luaopen_os, LUA_OSLIBNAME);
            standard_library_contract = standard_library_contract && result_count == 1 && lua_istable(libraries, -1);
            lua_getglobal(libraries, LUA_OSLIBNAME);
            standard_library_contract = standard_library_contract && lua_rawequal(libraries, -1, -2);

            result_count = run_library_opener(libraries, call_luaopen_string, LUA_STRLIBNAME);
            standard_library_contract = standard_library_contract && result_count == 1 && lua_istable(libraries, -1);
            lua_getglobal(libraries, LUA_STRLIBNAME);
            standard_library_contract = standard_library_contract && lua_rawequal(libraries, -1, -2);

            result_count = run_library_opener(libraries, call_luaopen_math, LUA_MATHLIBNAME);
            standard_library_contract = standard_library_contract && result_count == 1 && lua_istable(libraries, -1);
            lua_getglobal(libraries, LUA_MATHLIBNAME);
            standard_library_contract = standard_library_contract && lua_rawequal(libraries, -1, -2);

            result_count = run_library_opener(libraries, call_luaopen_debug, LUA_DBLIBNAME);
            standard_library_contract = standard_library_contract && result_count == 1 && lua_istable(libraries, -1);
            lua_getglobal(libraries, LUA_DBLIBNAME);
            standard_library_contract = standard_library_contract && lua_rawequal(libraries, -1, -2);

            result_count = run_library_opener(libraries, call_luaopen_package, LUA_LOADLIBNAME);
            standard_library_contract = standard_library_contract && result_count == 1 && lua_istable(libraries, -1);
            lua_getglobal(libraries, LUA_LOADLIBNAME);
            standard_library_contract = standard_library_contract && lua_rawequal(libraries, -1, -2);

            lua_settop(libraries, 0);
            lua_pushinteger(libraries, 29);
            luaL_openlibs(libraries);
            standard_library_contract = standard_library_contract && lua_gettop(libraries) == 1;
            standard_library_contract = standard_library_contract && lua_tointeger(libraries, 1) == 29;
            standard_library_contract = standard_library_contract && strcmp(LUA_FILEHANDLE, "FILE*") == 0;

            lua_close(libraries);
        }
    }

    lua_settop(L, 0);
    (void)lua_atpanic(L, first_panic);
    panic_contract = lua_atpanic(L, second_panic) == first_panic;
    {
        lua_State* child = lua_newthread(L);
        panic_contract = panic_contract && child != NULL && lua_atpanic(child, first_panic) == second_panic;
        lua_setlevel(L, child);
        setlevel_contract = lua_gettop(L) == 1 && lua_gettop(child) == 0;
    }

    lua_settop(L, 0);
    {
        char pointer_text[4 * sizeof(void*) + 8];
        char expected[160];
        const char* formatted;
        snprintf(pointer_text, sizeof(pointer_text), "%p", (void*)&final_api_token);
        snprintf(expected, sizeof(expected), "text|Z|17|1.25|%s|%%|%%q|(null)", pointer_text);
        formatted =
            lua_pushfstring(L, "%s|%c|%d|%f|%p|%%|%q|%s", "text", 'Z', 17, 1.25, (void*)&final_api_token, (char*)NULL);
        format_contract = strcmp(formatted, expected) == 0 && formatted == lua_tostring(L, -1);
        format_contract = format_contract && strcmp(push_vformat(L, "v=%d/%s", 29, "ok"), "v=29/ok") == 0;
    }

    lua_settop(L, 0);
    environment_contract = luaL_loadstring(L, "return environment_value") == 0;
    lua_newtable(L);
    lua_pushinteger(L, 73);
    lua_setfield(L, -2, "environment_value");
    environment_contract = environment_contract && lua_setfenv(L, 1) == 1 && lua_gettop(L) == 1;
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "environment_value");
    environment_contract = environment_contract && lua_tointeger(L, -1) == 73;
    lua_pop(L, 2);
    environment_contract = environment_contract && lua_pcall(L, 0, 1, 0) == 0 && lua_tointeger(L, -1) == 73;

    lua_settop(L, 0);
    (void)lua_newuserdata(L, sizeof(int));
    lua_newtable(L);
    lua_pushinteger(L, 81);
    lua_setfield(L, -2, "userdata_value");
    environment_contract = environment_contract && lua_setfenv(L, 1) == 1;
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "userdata_value");
    environment_contract = environment_contract && lua_tointeger(L, -1) == 81;

    lua_settop(L, 0);
    (void)lua_newthread(L);
    lua_newtable(L);
    lua_pushinteger(L, 91);
    lua_setfield(L, -2, "thread_value");
    environment_contract = environment_contract && lua_setfenv(L, 1) == 1;
    lua_getfenv(L, 1);
    lua_getfield(L, -1, "thread_value");
    environment_contract = environment_contract && lua_tointeger(L, -1) == 91;

    lua_settop(L, 0);
    lua_pushinteger(L, 5);
    lua_newtable(L);
    environment_contract = environment_contract && lua_setfenv(L, 1) == 0 && lua_gettop(L) == 1;
    lua_getfenv(L, 1);
    environment_contract = environment_contract && lua_isnil(L, -1);

    lua_settop(L, 0);
    lua_pushstring(L, "prefix");
    cpcall_argument_ok = 0;
    cpcall_contract = lua_cpcall(L, capture_cpcall_argument, &final_api_token) == 0 && cpcall_argument_ok;
    cpcall_contract = cpcall_contract && lua_gettop(L) == 1 && strcmp(lua_tostring(L, 1), "prefix") == 0;
    cpcall_contract = cpcall_contract && lua_cpcall(L, fail_cpcall, NULL) == LUA_ERRRUN;
    cpcall_contract = cpcall_contract && lua_gettop(L) == 2 && lua_touserdata(L, -1) == &final_api_token;

    printf("table=%d,%d,%d,%d,%d,%d;next=%d,%d;compare=%d,%d,%d;concat=%d,%d,%d,%d,%d;"
           "convert=%d,%d,%d;pointer=%d;thread=%d,%d;gc=%d;aux=%d,%d,%d,%d,%d,%d,%d;"
           "debug=%d,%d,%d,%d,%d,%d,%d,%d,%d;open=%d;final=%d,%d,%d,%d,%d\n",
           set_top, field_value, raw_value, meta_value, raw_missing, captured_value, iteration_count, iteration_sum,
           equal_with_meta, equal_without_meta, less_with_meta, concat_zero_length, concat_plain, concat_meta,
           concat_error_status, concat_error_top, integer_number, integer_string, cfunction_identity, pointer_identity,
           main_thread_identity, child_thread_identity, gc_contract, aux_checks, aux_meta, aux_register, aux_table,
           aux_buffer, aux_errors, aux_newstate, debug_contract, debug_function, debug_stack, debug_mutation,
           debug_hook_config, debug_hook_run, debug_hook_events, debug_disable, debug_invalid,
           standard_library_contract, panic_contract, format_contract, environment_contract, cpcall_contract,
           setlevel_contract);

    lua_close(L);
    return 0;
}
