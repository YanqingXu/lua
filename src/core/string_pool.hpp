#pragma once

/**
 * @file string_pool.hpp
 * @brief 字符串池 - 字符串驻留管理
 * 
 * 设计说明：
 * StringPool实现了字符串驻留（string interning）机制，确保相同内容的
 * 字符串在内存中只存储一份。这是Lua字符串系统的核心组件。
 * 
 * 核心特性：
 * - 字符串驻留：相同内容的字符串返回相同指针
 * - 哈希表管理：使用哈希表快速查找字符串
 * - GC集成：与垃圾回收器协作管理字符串生命周期
 * - 单例模式：全局唯一的字符串池实例
 * 
 * 参考实现：
 * - lua_c_analysis/src/lstring.c 中的字符串表管理
 * - lua_c_analysis/src/lstate.h 中的 stringtable 结构
 * - lua/docs/architecture/overview.md 中的设计文档
 */

#include "core/gc_string.hpp"
#include <unordered_map>
#include <string_view>
#include <memory>

namespace Lua {

class GarbageCollector;

/**
 * @brief StringPool类 - 字符串池管理器
 * 
 * 详细说明：
 * StringPool管理所有GCString对象的创建和查找，实现字符串驻留机制。
 * 它使用哈希表存储所有字符串，确保相同内容的字符串只有一个实例。
 * 
 * 字符串驻留流程：
 * 1. 调用intern()方法请求创建字符串
 * 2. 计算字符串的哈希值
 * 3. 在哈希表中查找是否已存在
 * 4. 如果存在，返回已有的GCString指针
 * 5. 如果不存在，创建新的GCString并加入哈希表
 * 
 * 内存管理：
 * - StringPool持有所有GCString对象的原始指针
 * - GCString对象由StringPool创建，由GC系统回收
 * - 当GC回收字符串时，需要从StringPool中移除
 * 
 * 线程安全：
 * 当前实现不考虑多线程，单线程使用。
 * 
 * 性能特点：
 * - 查找：平均O(1)时间复杂度
 * - 插入：平均O(1)时间复杂度
 * - 内存：额外的哈希表开销
 */
class StringPool {
public:
    // =====================================================================
    // 单例模式
    // =====================================================================
    
    /**
     * @brief 获取StringPool的单例实例
     * @return StringPool的引用
     */
    static StringPool& getInstance() {
        static StringPool instance;
        return instance;
    }
    
    // 禁止拷贝和移动
    StringPool(const StringPool&) = delete;
    StringPool(StringPool&&) = delete;
    StringPool& operator=(const StringPool&) = delete;
    StringPool& operator=(StringPool&&) = delete;

    // =====================================================================
    // 字符串驻留接口
    // =====================================================================
    
    /**
     * @brief 驻留字符串 - 获取或创建字符串对象
     *
     * 如果字符串已存在，返回已有的GCString指针；
     * 如果不存在，创建新的GCString并加入池中。
     *
     * @param str 字符串内容
     * @return GCString指针
     */
    GCString* intern(StrView str);

    /**
     * @brief 驻留字符串 - 从C字符串创建（自动计算长度）
     * @param str C风格字符串（以null结尾）
     * @return GCString指针
     *
     * @note 这是一个便利方法，会自动计算字符串长度
     */
    GCString* intern(const char* str) {
        return intern(StrView(str));
    }

    /**
     * @brief 驻留字符串 - 从C字符串创建（指定长度）
     * @param str C风格字符串
     * @param len 字符串长度
     * @return GCString指针
     */
    GCString* intern(const char* str, usize len) {
        return intern(StrView(str, len));
    }

    /**
     * @brief 设置新字符串默认注册到的GC实例
     */
    void setGarbageCollector(GarbageCollector* collector);

    GarbageCollector* getGarbageCollector() const noexcept {
        return collector_;
    }

    // =====================================================================
    // 查找接口
    // =====================================================================
    
    /**
     * @brief 查找字符串 - 不创建新对象
     *
     * 在池中查找指定内容的字符串，如果不存在返回nullptr。
     *
     * @param str 字符串内容
     * @return GCString指针，如果不存在返回nullptr
     */
    GCString* find(StrView str) const;

    /**
     * @brief 查找字符串 - 从C字符串查找（自动计算长度）
     * @param str C风格字符串（以null结尾）
     * @return GCString指针，如果不存在返回nullptr
     */
    GCString* find(const char* str) const {
        return find(StrView(str));
    }

    /**
     * @brief 查找字符串 - 从C字符串查找
     * @param str C风格字符串
     * @param len 字符串长度
     * @return GCString指针，如果不存在返回nullptr
     */
    GCString* find(const char* str, usize len) const {
        return find(StrView(str, len));
    }

    // =====================================================================
    // GC相关接口
    // =====================================================================
    
    /**
     * @brief 从池中移除字符串
     * 
     * 当GC回收字符串对象时调用，从池中移除对应的条目。
     * 
     * @param str 要移除的字符串指针
     */
    void remove(GCString* str);
    
    /**
     * @brief 清空字符串池
     * 
     * 移除所有字符串（不释放内存，由GC负责）。
     * 主要用于测试和调试。
     */
    void clear();
    
    /**
     * @brief 获取池中字符串的数量
     * @return 字符串数量
     */
    usize size() const {
        return pool_.size();
    }
    
    /**
     * @brief 检查池是否为空
     * @return true 如果池为空
     */
    bool empty() const {
        return pool_.empty();
    }

    /**
     * @brief 调整字符串池大小（预分配）
     *
     * 预分配哈希表空间，减少后续插入时的重哈希开销。
     * 这是Lua 5.1.5初始化过程的一部分。
     *
     * @param newSize 新的哈希表大小
     *
     * @note 对应C实现的 luaS_resize()
     * @see lua_c_analysis/src/lstring.c 第93-130行
     */
    void resize(usize newSize);

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    StringPool() = default;
    
    /**
     * @brief 析构函数
     */
    ~StringPool() = default;

    // =====================================================================
    // 内部数据结构
    // =====================================================================
    
    /**
     * @brief 字符串哈希表
     *
     * 使用HashMap存储字符串。
     * Key: 字符串内容（Str）
     * Value: GCString指针
     *
     * 注意：我们使用Str作为key而不是StrView，
     * 因为StrView不拥有数据，可能导致悬空引用。
     */
    HashMap<Str, GCString*> pool_;
    GarbageCollector* collector_ = nullptr;
};

} // namespace Lua

