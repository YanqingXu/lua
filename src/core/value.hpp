#pragma once

/**
 * @file value.hpp
 * @brief Lua动态类型系统的核心实现 - Value类
 * 
 * 设计说明：
 * Value类是Lua解释器中所有值的统一表示，对应C版本的TValue结构。
 * 使用C++17的std::variant替代C的union，提供类型安全的动态类型系统。
 * 
 * 核心特性：
 * - 类型安全：使用std::variant避免未定义行为
 * - 零开销：编译器优化后性能与C版本相当
 * - 现代C++：支持移动语义、RAII等特性
 * - 易于调试：提供丰富的类型检查和转换方法
 * 
 * 参考实现：
 * - lua_c_analysis/src/lobject.h 中的 TValue 结构
 * - lua/docs/ARCHITECTURE.md 中的设计文档
 */

#include "common/types.hpp"
#include <variant>
#include <optional>
#include <string>

namespace Lua {

// 前向声明（GC相关类型暂时使用void*代替，后续实现GC系统时替换）
class GCString;
class Table;
class Function;
class Userdata;
class Thread;

/**
 * @brief Value类 - Lua动态类型系统的核心
 * 
 * 详细说明：
 * Value类使用std::variant实现tagged union，每个Value对象可以存储
 * Lua支持的任意一种类型的值。variant会自动管理类型标签和值的生命周期。
 * 
 * 支持的类型：
 * 1. Nil         - 空值（使用std::monostate表示）
 * 2. Boolean     - 布尔值（bool）
 * 3. Number      - 数值（LuaNumber，即double）
 * 4. LightUserdata - 轻量级用户数据（void*，不受GC管理）
 * 5. String      - 字符串（GCString*，受GC管理）
 * 6. Table       - 表（Table*，受GC管理）
 * 7. Function    - 函数（Function*，受GC管理）
 * 8. Userdata    - 完整用户数据（Userdata*，受GC管理）
 * 9. Thread      - 线程/协程（Thread*，受GC管理）
 * 
 * 内存布局：
 * std::variant会选择最大类型的大小，并添加一个类型标签（通常1字节）。
 * 在64位系统上，大小约为16字节（8字节指针 + 8字节double + 类型标签）。
 */
class Value {
public:
    // =====================================================================
    // 类型定义
    // =====================================================================
    
    /**
     * @brief 值的内部表示类型
     * 
     * 使用std::variant实现类型安全的tagged union。
     * variant的索引顺序对应ValueType枚举的值。
     */
    using ValueVariant = std::variant<
        std::monostate,     // 0: Nil - 空值类型
        bool,               // 1: Boolean - 布尔值
        void*,              // 2: LightUserdata - 轻量级用户数据（C指针）
        LuaNumber,          // 3: Number - 数值（double）
        GCString*,          // 4: String - 字符串（GC对象）
        Table*,             // 5: Table - 表（GC对象）
        Function*,          // 6: Function - 函数（GC对象）
        Userdata*,          // 7: Userdata - 完整用户数据（GC对象）
        Thread*             // 8: Thread - 线程/协程（GC对象）
    >;

    // =====================================================================
    // 构造函数和析构函数
    // =====================================================================
    
    /**
     * @brief 默认构造函数 - 创建Nil值
     */
    Value() : value_(std::monostate{}) {}
    
    /**
     * @brief 布尔值构造函数
     */
    explicit Value(bool b) : value_(b) {}
    
    /**
     * @brief 数值构造函数
     */
    explicit Value(LuaNumber n) : value_(n) {}
    
    /**
     * @brief 整数构造函数（转换为LuaNumber）
     */
    explicit Value(LuaInteger i) : value_(static_cast<LuaNumber>(i)) {}
    
    /**
     * @brief 轻量级用户数据构造函数
     */
    explicit Value(void* p) : value_(p) {}
    
    /**
     * @brief GC对象构造函数（字符串）
     */
    explicit Value(GCString* s) : value_(s) {}
    
    /**
     * @brief GC对象构造函数（表）
     */
    explicit Value(Table* t) : value_(t) {}
    
    /**
     * @brief GC对象构造函数（函数）
     */
    explicit Value(Function* f) : value_(f) {}
    
    /**
     * @brief GC对象构造函数（用户数据）
     */
    explicit Value(Userdata* u) : value_(u) {}
    
    /**
     * @brief GC对象构造函数（线程）
     */
    explicit Value(Thread* th) : value_(th) {}
    
    // 使用默认的拷贝和移动构造/赋值
    Value(const Value&) = default;
    Value(Value&&) noexcept = default;
    Value& operator=(const Value&) = default;
    Value& operator=(Value&&) noexcept = default;
    
    ~Value() = default;

    // =====================================================================
    // 类型检查方法
    // =====================================================================
    
    /**
     * @brief 获取值的类型
     * @return ValueType枚举值
     */
    ValueType getType() const {
        return static_cast<ValueType>(value_.index());
    }
    
    /**
     * @brief 检查是否为Nil
     */
    bool isNil() const {
        return std::holds_alternative<std::monostate>(value_);
    }
    
    /**
     * @brief 检查是否为布尔值
     */
    bool isBoolean() const {
        return std::holds_alternative<bool>(value_);
    }
    
    /**
     * @brief 检查是否为数值
     */
    bool isNumber() const {
        return std::holds_alternative<LuaNumber>(value_);
    }
    
    /**
     * @brief 检查是否为轻量级用户数据
     */
    bool isLightUserdata() const {
        return std::holds_alternative<void*>(value_);
    }
    
    /**
     * @brief 检查是否为字符串
     */
    bool isString() const {
        return std::holds_alternative<GCString*>(value_);
    }
    
    /**
     * @brief 检查是否为表
     */
    bool isTable() const {
        return std::holds_alternative<Table*>(value_);
    }
    
    /**
     * @brief 检查是否为函数
     */
    bool isFunction() const {
        return std::holds_alternative<Function*>(value_);
    }
    
    /**
     * @brief 检查是否为用户数据
     */
    bool isUserdata() const {
        return std::holds_alternative<Userdata*>(value_);
    }
    
    /**
     * @brief 检查是否为线程
     */
    bool isThread() const {
        return std::holds_alternative<Thread*>(value_);
    }
    
    /**
     * @brief 检查是否为GC对象（需要垃圾回收的对象）
     */
    bool isCollectable() const {
        return isString() || isTable() || isFunction() || 
               isUserdata() || isThread();
    }

    // =====================================================================
    // 值访问方法（带类型检查）
    // =====================================================================
    
    /**
     * @brief 获取布尔值
     * @return 布尔值
     * @throws std::bad_variant_access 如果类型不匹配
     */
    bool asBoolean() const {
        return std::get<bool>(value_);
    }
    
    /**
     * @brief 获取数值
     * @return LuaNumber（double）
     * @throws std::bad_variant_access 如果类型不匹配
     */
    LuaNumber asNumber() const {
        return std::get<LuaNumber>(value_);
    }
    
    /**
     * @brief 获取整数值（从数值转换）
     * @return LuaInteger（int64_t）
     * @throws std::bad_variant_access 如果类型不匹配
     */
    LuaInteger asInteger() const {
        return static_cast<LuaInteger>(std::get<LuaNumber>(value_));
    }
    
    /**
     * @brief 获取轻量级用户数据指针
     * @return void*
     * @throws std::bad_variant_access 如果类型不匹配
     */
    void* asLightUserdata() const {
        return std::get<void*>(value_);
    }
    
    /**
     * @brief 获取字符串对象指针
     * @return GCString*
     * @throws std::bad_variant_access 如果类型不匹配
     */
    GCString* asString() const {
        return std::get<GCString*>(value_);
    }
    
    /**
     * @brief 获取表对象指针
     * @return Table*
     * @throws std::bad_variant_access 如果类型不匹配
     */
    Table* asTable() const {
        return std::get<Table*>(value_);
    }
    
    /**
     * @brief 获取函数对象指针
     * @return Function*
     * @throws std::bad_variant_access 如果类型不匹配
     */
    Function* asFunction() const {
        return std::get<Function*>(value_);
    }

    /**
     * @brief 获取用户数据对象指针
     * @return Userdata*
     * @throws std::bad_variant_access 如果类型不匹配
     */
    Userdata* asUserdata() const {
        return std::get<Userdata*>(value_);
    }

    /**
     * @brief 获取线程对象指针
     * @return Thread*
     * @throws std::bad_variant_access 如果类型不匹配
     */
    Thread* asThread() const {
        return std::get<Thread*>(value_);
    }

    // =====================================================================
    // 安全的值访问方法（返回std::optional）
    // =====================================================================

    /**
     * @brief 安全地尝试获取布尔值
     * @return std::optional<bool> 如果类型匹配返回值，否则返回空
     */
    std::optional<bool> tryGetBoolean() const {
        if (isBoolean()) {
            return asBoolean();
        }
        return std::nullopt;
    }

    /**
     * @brief 安全地尝试获取数值
     * @return std::optional<LuaNumber> 如果类型匹配返回值，否则返回空
     */
    std::optional<LuaNumber> tryGetNumber() const {
        if (isNumber()) {
            return asNumber();
        }
        return std::nullopt;
    }

    /**
     * @brief 安全地尝试获取整数值
     * @return std::optional<LuaInteger> 如果类型匹配返回值，否则返回空
     */
    std::optional<LuaInteger> tryGetInteger() const {
        if (isNumber()) {
            return asInteger();
        }
        return std::nullopt;
    }

    // =====================================================================
    // Lua语义的真值判断
    // =====================================================================

    /**
     * @brief 判断值在Lua语义下是否为假
     *
     * Lua中只有nil和false被认为是假值，其他所有值（包括0和空字符串）都是真值。
     *
     * @return true 如果值为nil或false
     * @return false 其他所有情况
     */
    bool isFalse() const {
        return isNil() || (isBoolean() && !asBoolean());
    }

    /**
     * @brief 判断值在Lua语义下是否为真
     *
     * @return true 如果值不是nil且不是false
     * @return false 如果值是nil或false
     */
    bool isTrue() const {
        return !isFalse();
    }

    // =====================================================================
    // 比较运算符
    // =====================================================================

    /**
     * @brief 相等性比较
     *
     * 比较规则：
     * - 类型必须相同
     * - 对于基础类型（nil, boolean, number），比较值
     * - 对于GC对象，比较指针（引用相等）
     *
     * @param other 要比较的另一个Value
     * @return true 如果两个值相等
     */
    bool operator==(const Value& other) const {
        // 类型不同，直接返回false
        if (value_.index() != other.value_.index()) {
            return false;
        }

        // 使用variant的相等比较
        return value_ == other.value_;
    }

    /**
     * @brief 不等性比较
     */
    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    // =====================================================================
    // 调试和字符串表示
    // =====================================================================

    /**
     * @brief 获取值的字符串表示（用于调试）
     * @return 值的字符串描述
     */
    std::string toString() const;

private:
    ValueVariant value_;  ///< 值的内部存储
};

} // namespace Lua

