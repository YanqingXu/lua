/**
 * @file types.hpp
 * @brief Lua解释器基础类型定义
 * 
 * 详细说明：
 * 本文件定义了Lua解释器中使用的所有基础类型、类型别名和前向声明。
 * 采用现代C++17标准，使用标准库类型和智能指针，确保类型安全和内存安全。
 * 
 * 设计理念：
 * - 类型安全：使用强类型和类型别名，避免隐式转换
 * - 现代C++：充分利用C++17/20特性（variant、optional、string_view等）
 * - 可读性：清晰的命名和完整的注释
 * - 可维护性：统一的类型定义，便于修改和扩展
 * 
 * 参考资源：
 * - lua_c_analysis/src/lua.h - Lua 5.1.5 C API类型定义
 * - lua_c_analysis/src/lobject.h - Lua对象类型系统
 * - lua_with_cpp/src/common/types.hpp - C++实现参考
 * 
 * @author Lua C++ Implementation Team
 * @version 0.1.0
 * @date 2025-11-11
 * @since C++17
 */

#pragma once

// =====================================================================
// 标准库头文件
// =====================================================================

// 基础类型
#include <cstdint>      // 固定宽度整数类型
#include <cstddef>      // size_t, ptrdiff_t
#include <climits>      // 整数限制

// 字符串和容器
#include <string>       // std::string
#include <string_view>  // std::string_view (C++17)
#include <vector>       // std::vector
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_set

// 智能指针和内存管理
#include <memory>       // std::shared_ptr, std::unique_ptr

// 现代C++特性
#include <variant>      // std::variant (C++17)
#include <optional>     // std::optional (C++17)
#include <functional>   // std::function

// 并发支持（可选）
#include <atomic>       // std::atomic
#include <mutex>        // std::mutex

// 异常处理
#include <stdexcept>    // std::runtime_error

// 工具
#include <utility>      // std::forward, std::move

// =====================================================================
// 命名空间定义
// =====================================================================

/**
 * @namespace Lua
 * @brief Lua解释器的顶层命名空间
 * 
 * 所有Lua解释器相关的类、函数、常量都定义在此命名空间中，
 * 避免与其他库的命名冲突。
 */
namespace Lua {

// =====================================================================
// 基础整数类型别名
// =====================================================================

/**
 * @name 有符号整数类型
 * @brief 固定宽度的有符号整数类型别名
 * @{
 */
using i8  = int8_t;     ///< 8位有符号整数 (-128 到 127)
using i16 = int16_t;    ///< 16位有符号整数 (-32,768 到 32,767)
using i32 = int32_t;    ///< 32位有符号整数 (-2^31 到 2^31-1)
using i64 = int64_t;    ///< 64位有符号整数 (-2^63 到 2^63-1)
/** @} */

/**
 * @name 无符号整数类型
 * @brief 固定宽度的无符号整数类型别名
 * @{
 */
using u8  = uint8_t;    ///< 8位无符号整数 (0 到 255)
using u16 = uint16_t;   ///< 16位无符号整数 (0 到 65,535)
using u32 = uint32_t;   ///< 32位无符号整数 (0 到 2^32-1)
using u64 = uint64_t;   ///< 64位无符号整数 (0 到 2^64-1)
/** @} */

// =====================================================================
// 浮点类型别名
// =====================================================================

/**
 * @name 浮点数类型
 * @brief 浮点数类型别名
 * @{
 */
using f32 = float;      ///< 32位单精度浮点数 (IEEE 754)
using f64 = double;     ///< 64位双精度浮点数 (IEEE 754)
/** @} */

// =====================================================================
// 大小和差值类型
// =====================================================================

/**
 * @name 大小和差值类型
 * @brief 用于表示大小和指针差值的类型
 * @{
 */
using usize = size_t;       ///< 无符号大小类型，用于数组索引和大小
using isize = ptrdiff_t;    ///< 有符号差值类型，用于指针运算
/** @} */

// =====================================================================
// 字符串类型别名
// =====================================================================

/**
 * @name 字符串类型
 * @brief 字符串和字符串视图类型别名
 * @{
 */
using CharPtr = const char*;
using Str = std::string;            ///< 标准字符串类型
using StrView = std::string_view;   ///< 字符串视图类型 (C++17)，高效的只读字符串引用
/** @} */

// =====================================================================
// 容器模板别名
// =====================================================================

/**
 * @name 容器模板
 * @brief 标准容器的模板别名
 * @{
 */

/// 动态数组容器
template<typename T>
using Vec = std::vector<T>;

/// 哈希映射容器
template<typename K, typename V>
using HashMap = std::unordered_map<K, V>;

/// 哈希集合容器
template<typename T>
using HashSet = std::unordered_set<T>;

/** @} */

// =====================================================================
// 现代C++类型工具
// =====================================================================

/**
 * @name 现代C++类型工具
 * @brief C++17/20特性的类型别名
 * @{
 */

/// 变体类型（类型安全的union）
template<typename... Types>
using Var = std::variant<Types...>;

/// 可选类型（可能不存在的值）
template<typename T>
using Opt = std::optional<T>;

/// 函数对象类型
template<typename Signature>
using Func = std::function<Signature>;

/** @} */

// =====================================================================
// 智能指针别名
// =====================================================================

/**
 * @name 智能指针类型
 * @brief 智能指针的类型别名
 * @{
 */

/// 共享指针（引用计数）
template<typename T>
using Ptr = std::shared_ptr<T>;

/// 弱引用指针
template<typename T>
using WPtr = std::weak_ptr<T>;

/// 独占指针（唯一所有权）
template<typename T>
using UPtr = std::unique_ptr<T>;

/** @} */

// =====================================================================
// 智能指针工厂函数
// =====================================================================

/**
 * @name 智能指针工厂函数
 * @brief 创建智能指针的便捷函数
 * @{
 */

/**
 * @brief 创建共享指针
 * @tparam T 对象类型
 * @tparam Args 构造函数参数类型
 * @param args 构造函数参数
 * @return 指向新对象的共享指针
 */
template<typename T, typename... Args>
inline Ptr<T> makePtr(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/**
 * @brief 创建独占指针
 * @tparam T 对象类型
 * @tparam Args 构造函数参数类型
 * @param args 构造函数参数
 * @return 指向新对象的独占指针
 */
template<typename T, typename... Args>
inline UPtr<T> makeUnique(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

/** @} */

// =====================================================================
// 并发类型别名（可选）
// =====================================================================

/**
 * @name 并发类型
 * @brief 线程安全相关的类型别名
 * @{
 */

/// 原子类型
template<typename T>
using Atom = std::atomic<T>;

/// 互斥锁
using Mtx = std::mutex;

/// 作用域锁
using ScopedLock = std::scoped_lock<Mtx>;

/** @} */

// =====================================================================
// Lua特定类型定义
// =====================================================================

/**
 * @name Lua基本类型
 * @brief Lua语言的基本数据类型
 * @{
 */

/// Lua整数类型（64位有符号整数）
using LuaInteger = i64;

/// Lua数字类型（64位双精度浮点数）
using LuaNumber = f64;

/// Lua布尔类型
using LuaBoolean = bool;

/** @} */

// =====================================================================
// Lua类型标签枚举
// =====================================================================

/**
 * @enum ValueType
 * @brief Lua值的类型标签
 * 
 * 定义了Lua中所有可能的值类型。这些类型对应Lua 5.1.5中的类型系统。
 * 
 * 对应关系：
 * - Nil          -> LUA_TNIL
 * - Boolean      -> LUA_TBOOLEAN
 * - LightUserdata -> LUA_TLIGHTUSERDATA
 * - Number       -> LUA_TNUMBER
 * - String       -> LUA_TSTRING
 * - Table        -> LUA_TTABLE
 * - Function     -> LUA_TFUNCTION
 * - Userdata     -> LUA_TUSERDATA
 * - Thread       -> LUA_TTHREAD
 */
enum class ValueType : u8 {
    Nil = 0,            ///< 空值类型
    Boolean = 1,        ///< 布尔类型
    LightUserdata = 2,  ///< 轻量用户数据（C指针）
    Number = 3,         ///< 数字类型
    String = 4,         ///< 字符串类型
    Table = 5,          ///< 表类型
    Function = 6,       ///< 函数类型
    Userdata = 7,       ///< 完整用户数据
    Thread = 8          ///< 线程（协程）类型
};

/**
 * @enum GCObjectType
 * @brief 垃圾回收对象的类型标签
 *
 * 定义了所有需要垃圾回收的对象类型，包括用户可见类型和内部类型。
 */
enum class GCObjectType : u8 {
    String = 4,         ///< 字符串对象
    Table = 5,          ///< 表对象
    Function = 6,       ///< 函数对象
    Userdata = 7,       ///< 用户数据对象
    Thread = 8,         ///< 线程对象
    Proto = 9,          ///< 函数原型（内部类型）
    Upval = 10,         ///< 上值（内部类型）
};

/**
 * @enum GCColor
 * @brief 垃圾回收对象的颜色标记
 *
 * 三色标记算法中的颜色定义：
 * - White（白色）：未访问的对象，可能被回收
 * - Gray（灰色）：已访问但未扫描其引用的对象
 * - Black（黑色）：已访问且已扫描所有引用的对象
 */
enum class GCColor : u8 {
    White = 0,          ///< 白色 - 未访问
    Gray = 1,           ///< 灰色 - 已访问未扫描
    Black = 2           ///< 黑色 - 已完全扫描
};

// =====================================================================
// 前向声明
// =====================================================================

/**
 * @name 核心类前向声明
 * @brief 核心类的前向声明，避免循环依赖
 * @{
 */

class Value;            ///< Lua值类型
class GCObject;         ///< 垃圾回收对象基类
class GCString;         ///< 字符串对象
class Table;            ///< 表对象
class Function;         ///< 函数对象
class Userdata;         ///< 用户数据对象
class Thread;           ///< 线程对象
class Proto;            ///< 函数原型
class Upval;            ///< 上值

class LuaState;         ///< Lua状态机
class GlobalState;      ///< 全局状态
class GarbageCollector; ///< 垃圾回收器
class StringPool;       ///< 字符串池

/** @} */

} // namespace Lua

