/**
 * @file config.hpp
 * @brief Lua解释器配置选项
 * 
 * 详细说明：
 * 本文件定义了Lua解释器的各种配置选项和编译时常量。
 * 这些配置影响解释器的行为、性能和内存使用。
 * 
 * 设计理念：
 * - 可配置性：通过宏定义控制编译时行为
 * - 性能优化：提供性能相关的配置选项
 * - 平台适配：支持不同平台的特定配置
 * - 调试支持：提供调试模式的配置
 * 
 * 参考资源：
 * - lua_c_analysis/src/luaconf.h - Lua 5.1.5配置文件
 * 
 * @author Lua C++ Implementation Team
 * @version 0.1.0
 * @date 2025-11-11
 * @since C++17
 */

#pragma once

#include "types.hpp"

namespace Lua {

// =====================================================================
// 版本信息
// =====================================================================

/**
 * @name 版本信息
 * @brief Lua解释器版本标识
 * @{
 */

/// Lua版本号（兼容Lua 5.1.5）
constexpr const char* LUA_VERSION = "Lua 5.1 (C++ Implementation)";

/// Lua发布版本
constexpr const char* LUA_RELEASE = "Lua 5.1.5 (C++ Implementation)";

/// Lua版本号（数值形式）
constexpr i32 LUA_VERSION_NUM = 501;

/// 版权信息
constexpr const char* LUA_COPYRIGHT = "Copyright (C) 2025 Lua C++ Implementation Team";

/** @} */

// =====================================================================
// 数值类型配置
// =====================================================================

/**
 * @name 数值类型配置
 * @brief 配置Lua的数值类型
 * @{
 */

/// Lua数字类型的最小值
constexpr LuaNumber LUA_NUMBER_MIN = -1.7976931348623157e+308;

/// Lua数字类型的最大值
constexpr LuaNumber LUA_NUMBER_MAX = 1.7976931348623157e+308;

/// Lua整数类型的最小值
constexpr LuaInteger LUA_INTEGER_MIN = -9223372036854775807LL - 1LL;

/// Lua整数类型的最大值
constexpr LuaInteger LUA_INTEGER_MAX = 9223372036854775807LL;

/** @} */

// =====================================================================
// 栈和调用配置
// =====================================================================

/**
 * @name 栈和调用配置
 * @brief 配置虚拟机栈和函数调用相关参数
 * @{
 */

/// 最小栈大小（保证C函数可用的栈空间）
constexpr i32 LUA_MINSTACK = 20;

/// 初始栈大小
constexpr i32 LUA_INITIAL_STACK_SIZE = 40;

/// 最大栈大小
constexpr i32 LUA_MAX_STACK_SIZE = 1000000;

/// 最大C调用深度（防止栈溢出）
constexpr i32 LUA_MAX_CCALLS = 200;

/// 最大参数数量
constexpr i32 LUA_MAX_PARAMS = 250;

/// 最大上值数量
constexpr i32 LUA_MAX_UPVALUES = 60;

/** @} */

// =====================================================================
// 表配置
// =====================================================================

/**
 * @name 表配置
 * @brief 配置表数据结构的参数
 * @{
 */

/// 表的初始数组大小
constexpr usize TABLE_INITIAL_ARRAY_SIZE = 0;

/// 表的初始哈希大小
constexpr usize TABLE_INITIAL_HASH_SIZE = 0;

/// 表的最大数组部分大小
constexpr usize TABLE_MAX_ARRAY_SIZE = (1u << 26);  // 64M

/// 表的负载因子（用于哈希表扩容）
constexpr f64 TABLE_LOAD_FACTOR = 0.75;

/** @} */

// =====================================================================
// 字符串配置
// =====================================================================

/**
 * @name 字符串配置
 * @brief 配置字符串系统的参数
 * @{
 */

/// 字符串池初始大小
constexpr usize STRING_POOL_INITIAL_SIZE = 128;

/// 短字符串最大长度（使用驻留机制）
constexpr usize STRING_SHORT_MAX_LENGTH = 40;

/// 字符串哈希种子（用于哈希计算）
constexpr u32 STRING_HASH_SEED = 0x9e3779b9u;

/** @} */

// =====================================================================
// 垃圾回收配置
// =====================================================================

/**
 * @name 垃圾回收配置
 * @brief 配置垃圾回收器的参数
 * @{
 */

/// GC暂停比例（百分比）
/// 当内存使用量达到上次GC后的(100 + GC_PAUSE)%时触发GC
constexpr i32 GC_PAUSE = 200;

/// GC步进倍率（百分比）
/// 控制增量GC每次执行的工作量
constexpr i32 GC_STEP_MUL = 200;

/// GC最小步长（字节）
constexpr usize GC_MIN_STEP = 1024;

/// GC最大步长（字节）
constexpr usize GC_MAX_STEP = 1024 * 1024;  // 1MB

/// 初始内存阈值（字节）
constexpr usize GC_INITIAL_THRESHOLD = 1024 * 1024;  // 1MB

/** @} */

// =====================================================================
// 调试和日志配置
// =====================================================================

/**
 * @name 调试配置
 * @brief 调试和日志相关的配置
 * @{
 */

/// 是否启用调试模式
#ifdef NDEBUG
    constexpr bool DEBUG_MODE = false;
#else
    constexpr bool DEBUG_MODE = true;
#endif

/// 是否启用详细日志
#ifdef LUA_VERBOSE_LOG
    constexpr bool VERBOSE_LOG = true;
#else
    constexpr bool VERBOSE_LOG = false;
#endif

/// 是否启用GC日志
#ifdef LUA_GC_LOG
    constexpr bool GC_LOG = true;
#else
    constexpr bool GC_LOG = false;
#endif

/// 是否启用性能统计
#ifdef LUA_PERF_STATS
    constexpr bool PERF_STATS = true;
#else
    constexpr bool PERF_STATS = false;
#endif

/** @} */

// =====================================================================
// 内存配置
// =====================================================================

/**
 * @name 内存配置
 * @brief 内存管理相关的配置
 * @{
 */

/// 默认内存限制（字节，0表示无限制）
constexpr usize DEFAULT_MEMORY_LIMIT = 0;

/// 内存对齐大小
constexpr usize MEMORY_ALIGNMENT = 8;

/// 是否启用内存池
#ifdef LUA_USE_MEMORY_POOL
    constexpr bool USE_MEMORY_POOL = true;
#else
    constexpr bool USE_MEMORY_POOL = false;
#endif

/** @} */

// =====================================================================
// 编译器配置
// =====================================================================

/**
 * @name 编译器配置
 * @brief 编译器相关的配置
 * @{
 */

/// 最大局部变量数量
constexpr i32 MAX_LOCAL_VARS = 200;

/// 最大寄存器数量
constexpr i32 MAX_REGISTERS = 250;

/// 最大常量数量
constexpr i32 MAX_CONSTANTS = 262144;  // 2^18

/// 最大代码长度
constexpr i32 MAX_CODE_SIZE = 524288;  // 2^19

/** @} */

// =====================================================================
// 优化配置
// =====================================================================

/**
 * @name 优化配置
 * @brief 性能优化相关的配置
 * @{
 */

/// 是否启用尾调用优化
#ifdef LUA_DISABLE_TAIL_CALL
    constexpr bool ENABLE_TAIL_CALL = false;
#else
    constexpr bool ENABLE_TAIL_CALL = true;
#endif

/// 是否启用常量折叠
#ifdef LUA_DISABLE_CONST_FOLDING
    constexpr bool ENABLE_CONST_FOLDING = false;
#else
    constexpr bool ENABLE_CONST_FOLDING = true;
#endif

/// 是否启用内联缓存
#ifdef LUA_ENABLE_INLINE_CACHE
    constexpr bool ENABLE_INLINE_CACHE = true;
#else
    constexpr bool ENABLE_INLINE_CACHE = false;
#endif

/** @} */

// =====================================================================
// 平台相关配置
// =====================================================================

/**
 * @name 平台配置
 * @brief 平台相关的配置
 * @{
 */

/// 是否为Windows平台
#ifdef _WIN32
    constexpr bool IS_WINDOWS = true;
#else
    constexpr bool IS_WINDOWS = false;
#endif

/// 是否为Linux平台
#ifdef __linux__
    constexpr bool IS_LINUX = true;
#else
    constexpr bool IS_LINUX = false;
#endif

/// 是否为macOS平台
#ifdef __APPLE__
    constexpr bool IS_MACOS = true;
#else
    constexpr bool IS_MACOS = false;
#endif

/// 是否为64位平台
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(_M_ARM64)
    constexpr bool IS_64BIT = true;
#else
    constexpr bool IS_64BIT = false;
#endif

/** @} */

// =====================================================================
// 特性开关
// =====================================================================

/**
 * @name 特性开关
 * @brief 可选特性的开关
 * @{
 */

/// 是否启用协程支持
#ifdef LUA_DISABLE_COROUTINE
    constexpr bool ENABLE_COROUTINE = false;
#else
    constexpr bool ENABLE_COROUTINE = true;
#endif

/// 是否启用调试钩子
#ifdef LUA_DISABLE_DEBUG_HOOK
    constexpr bool ENABLE_DEBUG_HOOK = false;
#else
    constexpr bool ENABLE_DEBUG_HOOK = true;
#endif

/// 是否启用弱引用表
#ifdef LUA_DISABLE_WEAK_TABLE
    constexpr bool ENABLE_WEAK_TABLE = false;
#else
    constexpr bool ENABLE_WEAK_TABLE = true;
#endif

/** @} */

} // namespace Lua

