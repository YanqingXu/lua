#pragma once

/**
 * @file userdata.hpp
 * @brief Lua用户数据类型实现
 * 
 * 设计说明:
 * Userdata类实现了Lua 5.1中的用户数据类型,允许将任意C++数据包装成Lua对象。
 * Lua支持两种用户数据:
 * 1. 轻量级用户数据(Light Userdata): 简单的void*指针,不受GC管理
 * 2. 完整用户数据(Full Userdata): GC管理的内存块,支持元表和终结器
 * 
 * 核心特性:
 * - 内存管理: 完整用户数据由GC自动管理
 * - 元表支持: 完整用户数据可以设置元表实现自定义行为
 * - 类型安全: 提供类型化访问接口
 * - 对齐保证: 用户数据内存按8字节对齐
 */

#include "common/types.hpp"
#include "gc_object.hpp"
#include <cstring>
#include <cstdlib>

namespace Lua {

// 前向声明
class Table;
class GarbageCollector;

/**
 * @brief Userdata类 - Lua用户数据对象
 * 
 * 详细说明:
 * Userdata允许将C++数据结构包装成Lua对象,是Lua与C++交互的核心机制。
 * 
 * 内存布局(完整用户数据):
 * [Userdata对象头部][用户数据块]
 * 
 * 用户数据块紧跟在Userdata对象之后,保证8字节对齐。
 */
class Userdata : public GCObject {
public:
    // =====================================================================
    // 静态工厂方法
    // =====================================================================
    
    /**
     * @brief 创建完整用户数据
     * @param size 用户数据大小(字节)
     * @return 新创建的Userdata对象指针
     * @throws std::bad_alloc 如果内存分配失败
     */
    static Userdata* createFull(usize size);
    
    /**
     * @brief 创建完整用户数据并初始化为指定值
     * @tparam T 数据类型
     * @param value 初始值
     * @return 新创建的Userdata对象指针
     */
    template<typename T>
    static Userdata* create(const T& value) {
        Userdata* ud = createFull(sizeof(T));
        *static_cast<T*>(ud->getData()) = value;
        return ud;
    }
    
    // =====================================================================
    // 析构函数
    // =====================================================================
    
    /**
     * @brief 析构函数 - 释放用户数据内存
     */
    ~Userdata();
    
    // =====================================================================
    // 数据访问
    // =====================================================================
    
    /**
     * @brief 获取用户数据指针
     * @return void* 指向用户数据的指针
     */
    void* getData() const noexcept {
        return data_;
    }
    
    /**
     * @brief 获取类型化的用户数据指针
     * @tparam T 数据类型
     * @return T* 类型化指针,如果大小不匹配返回nullptr
     */
    template<typename T>
    T* getTypedData() const noexcept {
        if (sizeof(T) > size_) {
            return nullptr;
        }
        return static_cast<T*>(data_);
    }
    
    /**
     * @brief 获取用户数据大小
     * @return usize 用户数据大小(字节)
     */
    usize getDataSize() const noexcept {
        return size_;
    }
    
    // =====================================================================
    // 元表操作
    // =====================================================================
    
    /**
     * @brief 获取元表
     * @return Table* 元表指针,如果没有元表返回nullptr
     */
    Table* getMetatable() const noexcept {
        return metatable_;
    }
    
    /**
     * @brief 设置元表
     * @param mt 元表指针
     */
    void setMetatable(Table* mt) noexcept {
        metatable_ = mt;
    }
    
    /**
     * @brief 检查是否有元表
     * @return bool 如果有元表返回true
     */
    bool hasMetatable() const noexcept {
        return metatable_ != nullptr;
    }
    
    // =====================================================================
    // GCObject接口实现
    // =====================================================================

    /**
     * @brief 标记用户数据引用的对象
     *
     * 标记元表（如果存在）
     */
    void mark(GarbageCollector& gc) override;

    /**
     * @brief 获取用户数据占用的内存大小
     *
     * @return 对象大小 + 用户数据大小
     */
    usize getSize() const override;

private:
    // =====================================================================
    // 私有构造函数
    // =====================================================================
    
    /**
     * @brief 私有构造函数 - 创建完整用户数据
     * @param size 用户数据大小
     */
    explicit Userdata(usize size);
    
    // 禁止拷贝和移动
    Userdata(const Userdata&) = delete;
    Userdata& operator=(const Userdata&) = delete;
    Userdata(Userdata&&) = delete;
    Userdata& operator=(Userdata&&) = delete;
    
    // =====================================================================
    // 成员变量
    // =====================================================================
    
    usize size_;        ///< 用户数据大小(字节)
    void* data_;        ///< 用户数据指针
    Table* metatable_;  ///< 元表指针
};

} // namespace Lua

