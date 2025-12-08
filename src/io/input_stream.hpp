/**
 * @file input_stream.hpp
 * @brief 输入流抽象 - 替代 lzio 的 ZIO
 * 
 * 提供统一的字符流接口，支持多种输入源：
 * - 字符串（零拷贝，使用 std::string_view）
 * - 文件流（std::ifstream）
 * - 标准输入流（std::istream）
 * 
 * 设计原则：
 * - 使用 std::istream 替代 lua_Reader 回调
 * - RAII 资源管理
 * - 零拷贝优化（字符串模式）
 * - 类型安全
 * 
 * 参考实现：
 * - lua_c_analysis/src/lzio.h - ZIO 结构
 * - lua_c_analysis/src/lzio.c - ZIO 操作函数
 * 
 * @author Lua C++ Implementation Team
 * @version 0.1.0
 * @date 2025-12-08
 * @since C++17
 */

#pragma once

#include "common/types.hpp"
#include <string_view>
#include <istream>
#include <vector>

namespace Lua {
namespace IO {

/**
 * @brief 输入流抽象类
 * 
 * 提供统一的字符流接口，支持多种输入源。
 * 
 * 特性：
 * - 字符串模式：零拷贝，使用 std::string_view
 * - 流模式：批量缓冲，减少系统调用
 * - RAII 资源管理
 * - 类型安全
 * 
 * 使用示例：
 * @code
 * // 字符串模式（零拷贝）
 * Str source = "local x = 42";
 * InputStream input(source);
 * 
 * // 流模式
 * std::ifstream file("script.lua");
 * InputStream input(file);
 * 
 * // 读取字符
 * while (true) {
 *     i32 c = input.getChar();
 *     if (c == -1) break;
 *     // 处理字符
 * }
 * @endcode
 */
class InputStream {
public:
    /**
     * @brief 从字符串创建输入流（零拷贝）
     * @param source 源字符串（必须在 InputStream 生命周期内有效）
     * 
     * 注意：使用 std::string_view，不拷贝数据。
     * 调用者必须确保源字符串在 InputStream 使用期间保持有效。
     */
    explicit InputStream(std::string_view source);
    
    /**
     * @brief 从 std::istream 创建输入流
     * @param stream 输入流引用（文件流、字符串流等）
     * @param bufferSize 内部缓冲区大小（默认 4KB）
     * 
     * 注意：stream 必须在 InputStream 生命周期内有效。
     */
    explicit InputStream(std::istream& stream, usize bufferSize = 4096);
    
    /**
     * @brief 禁止拷贝构造
     */
    InputStream(const InputStream&) = delete;
    
    /**
     * @brief 禁止拷贝赋值
     */
    InputStream& operator=(const InputStream&) = delete;
    
    /**
     * @brief 移动构造函数
     */
    InputStream(InputStream&&) noexcept = default;
    
    /**
     * @brief 移动赋值运算符
     */
    InputStream& operator=(InputStream&&) noexcept = default;
    
    /**
     * @brief 析构函数
     */
    ~InputStream() = default;
    
    /**
     * @brief 读取下一个字符
     * @return 字符值（0-255），或 -1 表示 EOF
     * 
     * 等价于 lzio 的 zgetc() 宏。
     * 使用 unsigned char 转换避免符号扩展。
     */
    i32 getChar();
    
    /**
     * @brief 前瞻下一个字符（不消费）
     * @return 字符值（0-255），或 -1 表示 EOF
     * 
     * 等价于 lzio 的 luaZ_lookahead()。
     */
    i32 peekChar();
    
    /**
     * @brief 批量读取数据
     * @param buffer 目标缓冲区
     * @param size 请求读取的字节数
     * @return 实际读取的字节数
     * 
     * 等价于 lzio 的 luaZ_read()。
     */
    usize read(void* buffer, usize size);
    
    /**
     * @brief 检查是否到达流末尾
     * @return 如果到达 EOF 返回 true
     */
    bool isEof() const noexcept;
    
    /**
     * @brief 获取当前位置（用于错误报告）
     * @return 当前读取位置（字节偏移）
     */
    usize getPosition() const noexcept;
    
    /**
     * @brief 获取源名称（用于错误报告）
     * @return 源名称字符串
     */
    const Str& getSourceName() const noexcept;

    /**
     * @brief 设置源名称
     * @param name 源名称（用于错误报告）
     */
    void setSourceName(const Str& name);

private:
    /**
     * @brief 填充内部缓冲区（流模式）
     *
     * 从 stream_ 读取数据到 buffer_。
     * 仅在流模式下使用。
     */
    void fillBuffer();

    // =====================================================================
    // 输入源
    // =====================================================================

    std::istream* stream_;           ///< 流式输入（可选，流模式使用）
    std::string_view stringView_;    ///< 字符串输入（零拷贝，字符串模式使用）

    // =====================================================================
    // 缓冲区（仅流模式使用）
    // =====================================================================

    Vec<char> buffer_;               ///< 内部缓冲（RAII 管理）
    usize bufferPos_;                ///< 缓冲区当前位置
    usize bufferSize_;               ///< 缓冲区有效数据大小

    // =====================================================================
    // 状态
    // =====================================================================

    bool eof_;                       ///< EOF 标志
    usize position_;                 ///< 全局位置（用于错误报告）
    bool useStringView_;             ///< 是否使用字符串视图模式
    Str sourceName_;                 ///< 源名称（用于错误报告）
};

} // namespace IO
} // namespace Lua


