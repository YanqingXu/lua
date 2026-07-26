#ifndef LUA_RUNTIME_H
#define LUA_RUNTIME_H

/**
 * @file lua_runtime.h
 * @brief Stable production configuration extensions for the Lua C++ runtime.
 * @details This header exposes a C ABI only.
 * Callers initialize each structure with its initializer, then override selected fields.
 * `struct_size` and `api_version` let the runtime reject incompatible layouts before creating a state.
 */

#include "lua.h"

#include <stdint.h>

#define LUA_RUNTIME_API_VERSION 1U
#define LUA_RUNTIME_UNLIMITED UINT64_MAX
#define LUA_RUNTIME_NO_TIMEOUT UINT64_MAX

enum {
    LUA_RUNTIME_OK = 0,
    LUA_RUNTIME_ERR_ARGUMENT = 1,
    LUA_RUNTIME_ERR_VERSION = 2,
    LUA_RUNTIME_ERR_THREAD = 3,
    LUA_RUNTIME_ERR_BUSY = 4,
    LUA_RUNTIME_ERR_CREATE = 5,
    LUA_RUNTIME_ERR_MEMORY = 6
};

enum {
    LUA_RUNTIME_STOP_NONE = 0,
    LUA_RUNTIME_STOP_INSTRUCTION_BUDGET = 1,
    LUA_RUNTIME_STOP_NATIVE_WORK_BUDGET = 2,
    LUA_RUNTIME_STOP_DEADLINE = 3,
    LUA_RUNTIME_STOP_CANCELLED = 4
};

enum {
    LUA_RUNTIME_LIB_BASE = 1U << 0U,
    LUA_RUNTIME_LIB_MATH = 1U << 1U,
    LUA_RUNTIME_LIB_IO = 1U << 2U,
    LUA_RUNTIME_LIB_STRING = 1U << 3U,
    LUA_RUNTIME_LIB_TABLE = 1U << 4U,
    LUA_RUNTIME_LIB_OS = 1U << 5U,
    LUA_RUNTIME_LIB_COROUTINE = 1U << 6U,
    LUA_RUNTIME_LIB_DEBUG = 1U << 7U,
    LUA_RUNTIME_LIB_PACKAGE = 1U << 8U,
    LUA_RUNTIME_LIB_ALL = (1U << 9U) - 1U
};

enum {
    LUA_RUNTIME_CAP_FILESYSTEM = 1U << 0U,
    LUA_RUNTIME_CAP_PROCESS = 1U << 1U,
    LUA_RUNTIME_CAP_NATIVE_MODULES = 1U << 2U,
    LUA_RUNTIME_CAP_RUNTIME_COMPILATION = 1U << 3U,
    LUA_RUNTIME_CAP_BINARY_CHUNKS = 1U << 4U,
    LUA_RUNTIME_CAP_GC_CONTROL = 1U << 5U,
    LUA_RUNTIME_CAP_ALL = (1U << 6U) - 1U
};

/**
 * @brief Creation-time configuration for one Lua runtime.
 * @details `execution_timeout_ms` starts when the state is created.
 * Call `lua_runtime_begin_execution` before each task to start a fresh execution window.
 */
typedef struct lua_RuntimeConfig {
    size_t struct_size;
    uint32_t api_version;

    uint32_t standard_libraries;
    uint32_t capabilities;

    uint64_t instruction_budget;
    uint64_t native_work_budget;
    uint64_t finalizer_budget_per_drain;
    uint64_t execution_timeout_ms;

    size_t max_string_bytes;
    size_t max_output_bytes;
    size_t max_source_bytes;
    size_t max_proto_bytes;
    size_t max_table_array_slots;
    size_t max_table_hash_entries;
    size_t max_stack_slots;
    size_t max_return_values;
    size_t max_sort_elements;
    size_t max_sort_comparisons;
    size_t max_pattern_steps;
    size_t max_reader_pieces;

    size_t max_compilation_tokens;
    size_t max_compilation_ast_nodes;
    size_t max_compilation_functions;
    size_t max_compilation_constants;
    size_t max_compilation_instructions;
    size_t max_compilation_nesting;
} lua_RuntimeConfig;

/** @brief Execution-window limits shared by one host task. */
typedef struct lua_RuntimeExecutionLimits {
    size_t struct_size;
    uint32_t api_version;
    uint64_t instruction_budget;
    uint64_t native_work_budget;
    uint64_t finalizer_budget_per_drain;
    uint64_t timeout_ms;
} lua_RuntimeExecutionLimits;

/** @brief Read-only metrics snapshot for the most recently completed execution window. */
typedef struct lua_RuntimeMetrics {
    size_t struct_size;
    uint32_t api_version;

    uint64_t initial_instruction_budget;
    uint64_t remaining_instruction_budget;
    uint64_t consumed_instructions;
    uint64_t initial_native_work_budget;
    uint64_t remaining_native_work_budget;
    uint64_t consumed_native_work;
    uint64_t finalizer_budget_per_drain;

    uint32_t deadline_configured;
    uint32_t cancellation_requested;
    uint32_t last_stop_reason;
} lua_RuntimeMetrics;

/** @brief Non-owning cancellation handle that becomes inert when its state is destroyed. */
typedef struct lua_CancellationHandle lua_CancellationHandle;

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize an unrestricted, unlimited Lua 5.1-compatible configuration. */
void lua_runtime_config_init(lua_RuntimeConfig* config) LUA_CXX_NOEXCEPT;

/**
 * @brief Initialize the bounded game-server preset for untrusted scripts.
 * @details The preset disables filesystem, process, native-module, runtime-compilation, binary-chunk, and script
 * GC-control capabilities. It exposes only the base, math, string, table, coroutine, and package libraries.
 * The preset also applies bounded execution and resource budgets.
 */
void lua_runtime_config_init_gameserver(lua_RuntimeConfig* config) LUA_CXX_NOEXCEPT;

/** @brief Initialize an unlimited execution window with no deadline. */
void lua_runtime_execution_limits_init(lua_RuntimeExecutionLimits* limits) LUA_CXX_NOEXCEPT;

/** @brief Initialize the ABI and version fields of a runtime metrics structure. */
void lua_runtime_metrics_init(lua_RuntimeMetrics* metrics) LUA_CXX_NOEXCEPT;

/**
 * @brief Create an independent state from a creation-time configuration.
 * @details C++ exceptions do not cross this boundary.
 * Failure returns NULL and writes a `LUA_RUNTIME_ERR_*` value when `runtime_status` is non-NULL.
 */
lua_State* lua_newstate_configured(lua_Alloc allocator, void* allocator_user_data, const lua_RuntimeConfig* config,
                                   int* runtime_status) LUA_CXX_NOEXCEPT;

/** @brief Create an independent state with the default allocator. */
lua_State* luaL_newstate_configured(const lua_RuntimeConfig* config, int* runtime_status) LUA_CXX_NOEXCEPT;

/**
 * @brief Start a fresh execution window for the next task on the owner thread.
 * @details This clears the previous cancellation request and resets the instruction and native-work budgets.
 * The call is rejected while the state is executing or when invoked from another thread.
 */
int lua_runtime_begin_execution(lua_State* L, const lua_RuntimeExecutionLimits* limits) LUA_CXX_NOEXCEPT;

/**
 * @brief Read execution-window metrics while the state is idle on its owner thread.
 * @details Callers must invoke `lua_runtime_metrics_init` first.
 * These metrics are intended for low-cost request logging.
 * An executing state returns `LUA_RUNTIME_ERR_BUSY` instead of publishing an inconsistent snapshot.
 */
int lua_runtime_get_metrics(lua_State* L, lua_RuntimeMetrics* metrics) LUA_CXX_NOEXCEPT;

/** @brief Acquire a cross-thread cancellation handle that safely outlives the state. */
lua_CancellationHandle* lua_runtime_get_cancellation_handle(lua_State* L, int* runtime_status) LUA_CXX_NOEXCEPT;

/** @brief Request cancellation from any thread; a null or expired handle is a safe no-op. */
void lua_runtime_request_cancellation(lua_CancellationHandle* handle) LUA_CXX_NOEXCEPT;

/** @brief Release a cancellation handle from any thread. */
void lua_runtime_release_cancellation_handle(lua_CancellationHandle* handle) LUA_CXX_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif
