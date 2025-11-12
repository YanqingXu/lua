/**
 * @file macros.hpp
 * @brief Lua解释器工具宏定义
 * 
 * 详细说明：
 * 本文件定义了Lua解释器中使用的各种工具宏，包括断言、日志、
 * 调试、性能统计等功能。
 * 
 * 设计理念：
 * - 调试友好：提供丰富的调试宏
 * - 性能优化：在发布版本中禁用调试代码
 * - 可读性：清晰的宏命名和注释
 * - 安全性：使用do-while(0)包装多语句宏
 * 
 * @author Lua C++ Implementation Team
 * @version 0.1.0
 * @date 2025-11-11
 * @since C++17
 */

#pragma once

#include "types.hpp"
#include "config.hpp"
#include <cassert>
#include <iostream>
#include <sstream>

namespace Lua {

// =====================================================================
// 断言宏
// =====================================================================

/**
 * @name 断言宏
 * @brief 用于运行时检查的断言宏
 * @{
 */

/// 标准断言（仅在调试模式下有效）
#ifdef NDEBUG
    #define LUA_ASSERT(condition) ((void)0)
#else
    #define LUA_ASSERT(condition) assert(condition)
#endif

/// 带消息的断言
#ifdef NDEBUG
    #define LUA_ASSERT_MSG(condition, message) ((void)0)
#else
    #define LUA_ASSERT_MSG(condition, message) \
        do { \
            if (!(condition)) { \
                std::cerr << "Assertion failed: " << #condition << "\n" \
                          << "Message: " << message << "\n" \
                          << "File: " << __FILE__ << "\n" \
                          << "Line: " << __LINE__ << std::endl; \
                assert(condition); \
            } \
        } while (0)
#endif

/// 始终有效的断言（即使在发布版本中）
#define LUA_VERIFY(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "Verification failed: " << #condition << "\n" \
                      << "File: " << __FILE__ << "\n" \
                      << "Line: " << __LINE__ << std::endl; \
            std::abort(); \
        } \
    } while (0)

/** @} */

// =====================================================================
// 日志宏
// =====================================================================

/**
 * @name 日志宏
 * @brief 用于输出日志信息的宏
 * @{
 */

/// 调试日志（仅在调试模式下输出）
#if DEBUG_MODE
    #define LUA_LOG_DEBUG(message) \
        do { \
            std::cout << "[DEBUG] " << message << " (" \
                      << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
        } while (0)
#else
    #define LUA_LOG_DEBUG(message) ((void)0)
#endif

/// 信息日志
#define LUA_LOG_INFO(message) \
    do { \
        std::cout << "[INFO] " << message << std::endl; \
    } while (0)

/// 警告日志
#define LUA_LOG_WARNING(message) \
    do { \
        std::cerr << "[WARNING] " << message << " (" \
                  << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
    } while (0)

/// 错误日志
#define LUA_LOG_ERROR(message) \
    do { \
        std::cerr << "[ERROR] " << message << " (" \
                  << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
    } while (0)

/// 详细日志（仅在启用详细日志时输出）
#if VERBOSE_LOG
    #define LUA_LOG_VERBOSE(message) \
        do { \
            std::cout << "[VERBOSE] " << message << std::endl; \
        } while (0)
#else
    #define LUA_LOG_VERBOSE(message) ((void)0)
#endif

/// GC日志（仅在启用GC日志时输出）
#if GC_LOG
    #define LUA_LOG_GC(message) \
        do { \
            std::cout << "[GC] " << message << std::endl; \
        } while (0)
#else
    #define LUA_LOG_GC(message) ((void)0)
#endif

/** @} */

// =====================================================================
// 未使用参数宏
// =====================================================================

/**
 * @name 未使用参数宏
 * @brief 标记未使用的参数，避免编译器警告
 * @{
 */

/// 标记未使用的参数
#define LUA_UNUSED(x) ((void)(x))

/// 标记未使用的多个参数
#define LUA_UNUSED2(x, y) ((void)(x), (void)(y))
#define LUA_UNUSED3(x, y, z) ((void)(x), (void)(y), (void)(z))

/** @} */

// =====================================================================
// 内联和优化提示宏
// =====================================================================

/**
 * @name 内联和优化提示宏
 * @brief 编译器优化提示
 * @{
 */

/// 强制内联
#if defined(_MSC_VER)
    #define LUA_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define LUA_FORCE_INLINE inline __attribute__((always_inline))
#else
    #define LUA_FORCE_INLINE inline
#endif

/// 禁止内联
#if defined(_MSC_VER)
    #define LUA_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
    #define LUA_NO_INLINE __attribute__((noinline))
#else
    #define LUA_NO_INLINE
#endif

/// 分支预测提示（likely）
#if defined(__GNUC__) || defined(__clang__)
    #define LUA_LIKELY(x) __builtin_expect(!!(x), 1)
#else
    #define LUA_LIKELY(x) (x)
#endif

/// 分支预测提示（unlikely）
#if defined(__GNUC__) || defined(__clang__)
    #define LUA_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LUA_UNLIKELY(x) (x)
#endif

/** @} */

// =====================================================================
// 错误处理宏
// =====================================================================

/**
 * @name 错误处理宏
 * @brief 错误处理相关的宏
 * @{
 */

/// 抛出运行时错误
#define LUA_THROW_ERROR(message) \
    do { \
        std::ostringstream oss; \
        oss << message; \
        throw std::runtime_error(oss.str()); \
    } while (0)

/// 检查条件，失败则抛出错误
#define LUA_CHECK(condition, message) \
    do { \
        if (LUA_UNLIKELY(!(condition))) { \
            LUA_THROW_ERROR(message); \
        } \
    } while (0)

/// 检查指针非空
#define LUA_CHECK_NOT_NULL(ptr, name) \
    LUA_CHECK((ptr) != nullptr, name " is null")

/// 检查索引范围
#define LUA_CHECK_RANGE(index, min, max, name) \
    LUA_CHECK((index) >= (min) && (index) < (max), \
              name " out of range: " << (index) << " not in [" << (min) << ", " << (max) << ")")

/** @} */

// =====================================================================
// 类型转换宏
// =====================================================================

/**
 * @name 类型转换宏
 * @brief 安全的类型转换宏
 * @{
 */

/// 静态类型转换（编译期检查）
#define LUA_STATIC_CAST(type, value) static_cast<type>(value)

/// 动态类型转换（运行期检查）
#define LUA_DYNAMIC_CAST(type, value) dynamic_cast<type>(value)

/// 常量转换
#define LUA_CONST_CAST(type, value) const_cast<type>(value)

/// 重新解释转换（谨慎使用）
#define LUA_REINTERPRET_CAST(type, value) reinterpret_cast<type>(value)

/** @} */

// =====================================================================
// 性能统计宏
// =====================================================================

/**
 * @name 性能统计宏
 * @brief 性能统计相关的宏
 * @{
 */

#if PERF_STATS
    #include <chrono>
    
    /// 开始性能计时
    #define LUA_PERF_START(name) \
        auto __perf_start_##name = std::chrono::high_resolution_clock::now()
    
    /// 结束性能计时并输出
    #define LUA_PERF_END(name) \
        do { \
            auto __perf_end_##name = std::chrono::high_resolution_clock::now(); \
            auto __perf_duration_##name = std::chrono::duration_cast<std::chrono::microseconds>( \
                __perf_end_##name - __perf_start_##name).count(); \
            std::cout << "[PERF] " << #name << ": " << __perf_duration_##name << " us" << std::endl; \
        } while (0)
#else
    #define LUA_PERF_START(name) ((void)0)
    #define LUA_PERF_END(name) ((void)0)
#endif

/** @} */

// =====================================================================
// 调试辅助宏
// =====================================================================

/**
 * @name 调试辅助宏
 * @brief 调试辅助功能
 * @{
 */

/// 打印变量值（调试用）
#if DEBUG_MODE
    #define LUA_DEBUG_PRINT(var) \
        std::cout << "[DEBUG] " << #var << " = " << (var) << " (" \
                  << __FILE__ << ":" << __LINE__ << ")" << std::endl
#else
    #define LUA_DEBUG_PRINT(var) ((void)0)
#endif

/// 标记未实现的功能
#define LUA_NOT_IMPLEMENTED() \
    LUA_THROW_ERROR("Not implemented: " << __FUNCTION__ << " at " << __FILE__ << ":" << __LINE__)

/// 标记不应到达的代码
#define LUA_UNREACHABLE() \
    do { \
        LUA_LOG_ERROR("Unreachable code reached in " << __FUNCTION__); \
        std::abort(); \
    } while (0)

/** @} */

// =====================================================================
// 位操作宏
// =====================================================================

/**
 * @name 位操作宏
 * @brief 常用的位操作宏
 * @{
 */

/// 设置位
#define LUA_BIT_SET(value, bit) ((value) |= (1u << (bit)))

/// 清除位
#define LUA_BIT_CLEAR(value, bit) ((value) &= ~(1u << (bit)))

/// 切换位
#define LUA_BIT_TOGGLE(value, bit) ((value) ^= (1u << (bit)))

/// 检查位
#define LUA_BIT_CHECK(value, bit) (((value) & (1u << (bit))) != 0)

/** @} */

// =====================================================================
// 对齐宏
// =====================================================================

/**
 * @name 对齐宏
 * @brief 内存对齐相关的宏
 * @{
 */

/// 向上对齐到指定边界
#define LUA_ALIGN_UP(value, alignment) \
    (((value) + (alignment) - 1) & ~((alignment) - 1))

/// 向下对齐到指定边界
#define LUA_ALIGN_DOWN(value, alignment) \
    ((value) & ~((alignment) - 1))

/// 检查是否对齐
#define LUA_IS_ALIGNED(value, alignment) \
    (((value) & ((alignment) - 1)) == 0)

/** @} */

} // namespace Lua

