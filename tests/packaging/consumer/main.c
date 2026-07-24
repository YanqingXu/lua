#include <lua.h>
#include <lauxlib.h>
#include <lua_cpp_version.h>
#include <lua_runtime.h>
#include <lualib.h>

#include <stdio.h>
#include <string.h>

static int global_is_nil(lua_State* state, const char* name) {
    int result;
    lua_getglobal(state, name);
    result = lua_isnil(state, -1);
    lua_pop(state, 1);
    return result;
}

static int error_contains(lua_State* state, const char* fragment) {
    const char* message = lua_tostring(state, -1);
    return message != NULL && strstr(message, fragment) != NULL;
}

int main(void) {
    lua_RuntimeConfig config;
    lua_RuntimeExecutionLimits limits;
    lua_RuntimeMetrics metrics;
    lua_CancellationHandle* cancellation;
    lua_State* state;
    int runtime_status = -1;
    int status;

    lua_runtime_config_init_gameserver(&config);
    if (config.struct_size != sizeof(config) || config.api_version != LUA_RUNTIME_API_VERSION ||
        config.standard_libraries != (LUA_RUNTIME_LIB_BASE | LUA_RUNTIME_LIB_MATH | LUA_RUNTIME_LIB_STRING |
                                      LUA_RUNTIME_LIB_TABLE | LUA_RUNTIME_LIB_COROUTINE | LUA_RUNTIME_LIB_PACKAGE) ||
        config.capabilities != 0) {
        return 1;
    }

    config.max_string_bytes = 256;
    config.max_output_bytes = 64;
    state = luaL_newstate_configured(&config, &runtime_status);
    if (state == NULL) {
        return 2;
    }
    if (runtime_status != LUA_RUNTIME_OK) {
        lua_close(state);
        return 3;
    }

    lua_pushstring(state, LUA_CPP_VERSION);
    if (strcmp(lua_tostring(state, -1), "0.1.0") != 0 || LUA_CPP_ABI_VERSION != 0) {
        lua_close(state);
        return 4;
    }
    lua_pop(state, 1);

    luaL_openlibs(state);
    if (!global_is_nil(state, "io") || !global_is_nil(state, "os") || !global_is_nil(state, "debug") ||
        !global_is_nil(state, "loadstring") || !global_is_nil(state, "loadfile") || !global_is_nil(state, "dofile") ||
        !global_is_nil(state, "collectgarbage")) {
        lua_close(state);
        return 5;
    }
    lua_getglobal(state, "package");
    if (!lua_istable(state, -1)) {
        lua_close(state);
        return 6;
    }
    lua_pop(state, 1);

    status = luaL_loadstring(state, "return string.rep('x', 128)");
    if (status == LUA_OK) {
        status = lua_pcall(state, 0, 1, 0);
    }
    if (status != LUA_ERRRUN || !error_contains(state, "string.rep: result exceeds resource limit")) {
        fprintf(stderr, "resource contract failed: status=%d error=%s\n", status,
                lua_tostring(state, -1) != NULL ? lua_tostring(state, -1) : "<none>");
        lua_close(state);
        return 7;
    }
    lua_settop(state, 0);

    lua_runtime_execution_limits_init(&limits);
    limits.instruction_budget = 32;
    limits.finalizer_budget_per_drain = 8;
    if (lua_runtime_begin_execution(state, &limits) != LUA_RUNTIME_OK) {
        lua_close(state);
        return 8;
    }
    status = luaL_loadstring(state, "while true do end");
    if (status != LUA_OK || lua_pcall(state, 0, 0, 0) != LUA_ERRRUN ||
        !error_contains(state, "execution instruction budget exceeded")) {
        lua_close(state);
        return 9;
    }
    lua_settop(state, 0);
    lua_runtime_metrics_init(&metrics);
    if (lua_runtime_get_metrics(state, &metrics) != LUA_RUNTIME_OK || metrics.initial_instruction_budget != 32 ||
        metrics.remaining_instruction_budget != 0 || metrics.consumed_instructions != 32 ||
        metrics.last_stop_reason != LUA_RUNTIME_STOP_INSTRUCTION_BUDGET) {
        lua_close(state);
        return 14;
    }

    status = luaL_loadstring(state, "return 42");
    if (status != LUA_OK) {
        lua_close(state);
        return 10;
    }
    cancellation = lua_runtime_get_cancellation_handle(state, &runtime_status);
    if (cancellation == NULL || runtime_status != LUA_RUNTIME_OK) {
        lua_close(state);
        return 11;
    }
    lua_runtime_execution_limits_init(&limits);
    limits.instruction_budget = 1000;
    if (lua_runtime_begin_execution(state, &limits) != LUA_RUNTIME_OK) {
        lua_runtime_release_cancellation_handle(cancellation);
        lua_close(state);
        return 12;
    }
    lua_runtime_request_cancellation(cancellation);
    if (lua_pcall(state, 0, 1, 0) != LUA_ERRRUN || !error_contains(state, "execution cancelled")) {
        lua_runtime_release_cancellation_handle(cancellation);
        lua_close(state);
        return 13;
    }
    if (lua_runtime_get_metrics(state, &metrics) != LUA_RUNTIME_OK || metrics.cancellation_requested != 1 ||
        metrics.last_stop_reason != LUA_RUNTIME_STOP_CANCELLED) {
        lua_runtime_release_cancellation_handle(cancellation);
        lua_close(state);
        return 15;
    }

    lua_close(state);
    lua_runtime_request_cancellation(cancellation);
    lua_runtime_release_cancellation_handle(cancellation);
    return 0;
}
