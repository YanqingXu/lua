#ifndef LUA_RUNTIME_H
#define LUA_RUNTIME_H

/**
 * @file lua_runtime.h
 * @brief Lua C++ 运行时的稳定生产配置扩展
 *
 * 本头文件只包含 C ABI。调用方应先使用初始化函数填充结构，再按需覆盖字段；
 * `struct_size` 与 `api_version` 使错误版本在创建 State 前被确定性拒绝。
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
 * @brief 单个 Lua 运行时的创建期配置
 *
 * `execution_timeout_ms` 从 State 创建时开始计时；使用
 * `lua_runtime_begin_execution` 可在每个任务前启动新的执行窗口。
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

/** @brief 单次宿主任务共享的执行窗口限制。 */
typedef struct lua_RuntimeExecutionLimits {
    size_t struct_size;
    uint32_t api_version;
    uint64_t instruction_budget;
    uint64_t native_work_budget;
    uint64_t finalizer_budget_per_drain;
    uint64_t timeout_ms;
} lua_RuntimeExecutionLimits;

/** @brief 最近一个已完成执行窗口的只读治理指标快照。 */
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

/** @brief 生命周期安全的非 owning 取消句柄。 */
typedef struct lua_CancellationHandle lua_CancellationHandle;

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 初始化为 Lua 5.1 兼容的 unrestricted/unlimited 配置。 */
void lua_runtime_config_init(lua_RuntimeConfig* config) LUA_CXX_NOEXCEPT;

/**
 * @brief 初始化为面向不可信游戏逻辑的有限 game-server 配置。
 *
 * 该预置禁用文件系统、进程、原生模块、运行时编译、二进制 chunk 与脚本 GC 控制；
 * 只暴露 base/math/string/table/coroutine/package，并设置有限执行与资源预算。
 */
void lua_runtime_config_init_gameserver(lua_RuntimeConfig* config) LUA_CXX_NOEXCEPT;

/** @brief 初始化为 unlimited 且无 deadline 的单次执行窗口。 */
void lua_runtime_execution_limits_init(lua_RuntimeExecutionLimits* limits) LUA_CXX_NOEXCEPT;

/** @brief 初始化运行时指标结构的 ABI/version 字段。 */
void lua_runtime_metrics_init(lua_RuntimeMetrics* metrics) LUA_CXX_NOEXCEPT;

/**
 * @brief 使用创建期配置构造独立 State。
 *
 * 本函数关闭 C++ 异常边界。失败返回 NULL，并在非空 `runtime_status` 中写入
 * `LUA_RUNTIME_ERR_*`。
 */
lua_State* lua_newstate_configured(lua_Alloc allocator, void* allocator_user_data, const lua_RuntimeConfig* config,
                                   int* runtime_status) LUA_CXX_NOEXCEPT;

/** @brief 使用默认 allocator 和创建期配置构造独立 State。 */
lua_State* luaL_newstate_configured(const lua_RuntimeConfig* config, int* runtime_status) LUA_CXX_NOEXCEPT;

/**
 * @brief 在 owner thread 为下一次宿主任务启动新的执行窗口。
 *
 * 调用会清除上一个窗口的取消请求并重置 instruction/native-work 余量。
 * State 正在执行或从其他线程调用时会被拒绝。
 */
int lua_runtime_begin_execution(lua_State* L, const lua_RuntimeExecutionLimits* limits) LUA_CXX_NOEXCEPT;

/**
 * @brief 在 owner thread 且 State 空闲时读取当前执行窗口指标。
 *
 * 调用方必须先调用 `lua_runtime_metrics_init`。指标用于低成本请求日志；
 * 运行中的 State 返回 `LUA_RUNTIME_ERR_BUSY`，避免发布非一致快照。
 */
int lua_runtime_get_metrics(lua_State* L, lua_RuntimeMetrics* metrics) LUA_CXX_NOEXCEPT;

/** @brief 获取可越过 State 生命周期安全失效的跨线程取消句柄。 */
lua_CancellationHandle* lua_runtime_get_cancellation_handle(lua_State* L, int* runtime_status) LUA_CXX_NOEXCEPT;

/** @brief 从任意线程请求取消；空或已失效句柄是安全无操作。 */
void lua_runtime_request_cancellation(lua_CancellationHandle* handle) LUA_CXX_NOEXCEPT;

/** @brief 释放取消句柄；可从任意线程调用。 */
void lua_runtime_release_cancellation_handle(lua_CancellationHandle* handle) LUA_CXX_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif
