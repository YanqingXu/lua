#pragma once

/**
 * @file gc_string.hpp
 * @brief GC管理的字符串对象 - 字符串驻留和哈希缓存
 * 
 * 设计说明：
 * GCString是Lua字符串系统的核心，实现了字符串驻留（string interning）机制。
 * 相同内容的字符串在内存中只存储一份，通过StringPool统一管理。
 * 
 * 核心特性：
 * - 字符串驻留：相同内容的字符串共享同一个对象
 * - 哈希缓存：创建时预计算哈希值，后续O(1)访问
 * - 不可变性：字符串创建后内容不可修改
 * - 指针比较：驻留字符串可以直接比较地址（O(1)）
 * - GC集成：继承自GCObject，由垃圾回收器管理
 * 
 * 参考实现：
 * - lua_c_analysis/src/lstring.h 中的 TString 结构
 * - lua_c_analysis/src/lstring.c 中的字符串管理函数
 * - lua/docs/architecture/overview.md 中的设计文档
 */

#include "core/gc_object.hpp"
#include <string>
#include <string_view>

namespace Lua {

/**
 * @brief GCString类 - GC管理的字符串对象
 * 
 * 详细说明：
 * GCString实现了Lua的字符串类型，对应C版本的TString结构。
 * 每个字符串对象包含预计算的哈希值、长度信息和实际的字符串数据。
 * 
 * 内存布局：
 * - GCObject基类：24字节（next, type, marked, vtable）
 * - hash_: 8字节（usize）
 * - length_: 8字节（usize）
 * - data_: 32字节（Str，包含SSO优化）
 * 总计：约72字节（基类部分）+ 字符串数据
 * 
 * 字符串驻留：
 * 所有GCString对象通过StringPool创建和管理，确保相同内容的字符串
 * 只有一个实例。这使得字符串比较可以简化为指针比较。
 * 
 * 哈希算法：
 * 使用与Lua 5.1.5相同的哈希算法，在字符串创建时计算并缓存。
 * 哈希值用于快速查找和表键比较。
 * 
 * 不可变性：
 * 字符串一旦创建，内容不可修改。这是字符串驻留的前提条件。
 */
class GCString : public GCObject {
public:
    // =====================================================================
    // 构造函数和析构函数
    // =====================================================================
    
    /**
     * @brief 构造函数 - 从字符串视图创建GCString
     * @param str 字符串内容
     *
     * 注意：此构造函数应该只被StringPool调用，不应直接使用。
     */
    explicit GCString(StrView str);
    
    /**
     * @brief 析构函数
     */
    ~GCString() override = default;
    
    // 禁止拷贝和移动（字符串由StringPool管理）
    GCString(const GCString&) = delete;
    GCString(GCString&&) = delete;
    GCString& operator=(const GCString&) = delete;
    GCString& operator=(GCString&&) = delete;

    // =====================================================================
    // 访问器
    // =====================================================================
    
    /**
     * @brief 获取字符串的哈希值
     * @return 预计算的哈希值
     */
    usize getHash() const noexcept {
        return hash_;
    }
    
    /**
     * @brief 获取字符串长度
     * @return 字符串的字节长度
     */
    usize getLength() const noexcept {
        return length_;
    }
    
    /**
     * @brief 获取字符串数据（Str引用）
     * @return 字符串数据的常量引用
     */
    const Str& getData() const noexcept {
        return data_;
    }

    /**
     * @brief 获取C风格字符串
     * @return 指向以null结尾的字符串的指针
     */
    const char* c_str() const noexcept {
        return data_.c_str();
    }

    /**
     * @brief 获取字符串视图
     * @return 字符串视图
     */
    StrView view() const noexcept {
        return StrView(data_);
    }

    // =====================================================================
    // GCObject接口实现
    // =====================================================================
    
    /**
     * @brief 标记对象引用的其他对象
     *
     * 字符串对象不引用其他GC对象，所以这个方法为空实现。
     */
    void mark(GarbageCollector& /*gc*/) override {
        // 字符串对象不引用其他GC对象
    }

    /**
     * @brief 获取对象占用的内存大小
     * @return 对象占用的字节数
     */
    usize getSize() const override;

    // =====================================================================
    // 固定字符串（防止GC回收）
    // =====================================================================

    /**
     * @brief 标记字符串为固定（防止GC回收）
     *
     * 固定的字符串永远不会被垃圾回收器回收，主要用于：
     * - Lua关键字（if, then, else等）
     * - 元方法名称（__index, __add等）
     * - 系统常量字符串
     *
     * @note 对应C实现的 luaS_fix() 宏
     * @see lua_c_analysis/src/lstring.h 第296行
     */
    void markFixed() noexcept {
        setMarked(getMarked() | GCBits::FIXED);
    }

    /**
     * @brief 检查字符串是否为固定字符串
     * @return true 如果字符串被标记为固定
     */
    bool isFixed() const noexcept {
        return (getMarked() & GCBits::FIXED) != 0;
    }

    // =====================================================================
    // 比较运算符
    // =====================================================================
    
    /**
     * @brief 相等性比较（指针比较）
     * 
     * 由于字符串驻留，相同内容的字符串必定是同一个对象，
     * 因此可以直接比较指针地址。
     * 
     * @param other 要比较的另一个字符串
     * @return true 如果是同一个对象
     */
    bool operator==(const GCString& other) const noexcept {
        return this == &other;
    }
    
    /**
     * @brief 不等性比较
     */
    bool operator!=(const GCString& other) const noexcept {
        return !(*this == other);
    }

    // =====================================================================
    // 哈希函数（静态方法）
    // =====================================================================
    
    /**
     * @brief 计算字符串的哈希值
     *
     * 使用与Lua 5.1.5相同的哈希算法。
     * 对于长字符串，采用采样策略以提高性能。
     *
     * @param str 字符串视图
     * @return 哈希值
     */
    static usize computeHash(StrView str) noexcept;

private:
    usize hash_;            ///< 预计算的哈希值
    usize length_;          ///< 字符串长度（字节数）
    Str data_;              ///< 字符串数据（使用SSO优化）
};

} // namespace Lua

