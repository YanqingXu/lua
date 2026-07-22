/**
 * @file dynamic_buffer.hpp
 * @brief 动态缓冲区 - 替代 lzio 的 Mbuffer
 * 
 * 使用标准动态数组实现，提供资源自动管理和自动扩展。
 * 主要用于词法分析器累积词法单元内容。
 * 
 * 设计原则：
 * - 资源获取即初始化的自动资源管理
 * - 零拷贝视图（StrView）
 * - 移动语义优化
 * - 预留容量避免频繁重分配
 * @author Lua C++ 实现团队
 * @version 0.1.0
 * @date 2025-12-08
 * @since C++17
 */

#pragma once

#include "common/types.hpp"
#include <vector>
#include <string_view>

namespace Lua {
namespace IO {

/**
 * @brief 动态缓冲区类
 * 
 * 提供高效的字符累积功能，用于词法分析器构建词法单元内容。
 * 
 * 特性：
 * - 资源获取即初始化的自动资源管理
 * - 零拷贝视图（StrView）
 * - 移动语义优化
 * - 预留容量避免频繁重分配
 * 
 * 使用示例：
 * @code
 * DynamicBuffer buf;
 * buf.append('h');
 * buf.append("ello");
 * StrView view = buf.view();  // "hello"
 * Str str = std::move(buf).toString(); // 移动语义
 * @endcode
 */
class DynamicBuffer {
public:
    /**
     * @brief 默认构造函数
     */
    DynamicBuffer() = default;
    
    /**
     * @brief 禁止拷贝构造
     */
    DynamicBuffer(const DynamicBuffer&) = delete;
    
    /**
     * @brief 禁止拷贝赋值
     */
    DynamicBuffer& operator=(const DynamicBuffer&) = delete;
    
    /**
     * @brief 移动构造函数
     */
    DynamicBuffer(DynamicBuffer&&) noexcept = default;
    
    /**
     * @brief 移动赋值运算符
     */
    DynamicBuffer& operator=(DynamicBuffer&&) noexcept = default;
    
    /**
     * @brief 析构函数
     */
    ~DynamicBuffer() = default;
    
    /**
     * @brief 追加单个字符
     * @param c 要追加的字符
     */
    void append(char c);
    
    /**
     * @brief 追加字符串
     * @param str 要追加的字符串视图
     */
    void append(StrView str);
    
    /**
     * @brief 获取内容视图（零拷贝）
     * @return 缓冲区内容的字符串视图
     */
    StrView view() const noexcept;
    
    /**
     * @brief 转换为字符串（移动语义）
     * @return 缓冲区内容的字符串（移动）
     * 
     * 注意：此方法使用移动语义，调用后缓冲区将被清空。
     * 必须使用 std::move 调用：std::move(buf).toString()
     */
    Str toString() &&;
    
    /**
     * @brief 清空缓冲区（保留容量）
     * 
     * 清空内容但保留已分配的内存，避免下次使用时重新分配。
     */
    void clear() noexcept;
    
    /**
     * @brief 重置缓冲区（释放内存）
     * 
     * 清空内容并释放所有已分配的内存。
     */
    void reset() noexcept;
    
    /**
     * @brief 预留空间
     * @param capacity 预留容量
     * 
     * 预先分配内存，避免频繁重分配。
     */
    void reserve(usize capacity);
    
    /**
     * @brief 获取当前大小
     * @return 缓冲区中的字符数
     */
    usize size() const noexcept;
    
    /**
     * @brief 获取当前容量
     * @return 缓冲区的总容量
     */
    usize capacity() const noexcept;
    
    /**
     * @brief 检查是否为空
     * @return 如果缓冲区为空返回 true
     */
    bool empty() const noexcept;

    /**
     * @brief 获取原始数据指针
     * @return 指向缓冲区数据的指针
     */
    const char* data() const noexcept;

private:
    /** @brief 内部缓冲区 */
    Vec<char> buffer_;
};

} // namespace IO
} // namespace Lua


