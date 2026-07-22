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
 * - 资源获取即初始化的资源管理
 * - 零拷贝优化（字符串模式）
 * - 类型安全
 * @author Lua C++ 实现团队
 * @version 0.1.0
 * @date 2025-12-08
 * @since C++17
 */

#pragma once

#include "common/types.hpp"
#include <string_view>
#include <istream>
#include <fstream>
#include <optional>
#include <vector>

namespace Lua {
namespace IO {

/**
 * @brief 输入流抽象类
 * 
 * 提供统一的字符流接口，支持多种输入源。
 * 
 * 特性：
 * - 字符串模式：零拷贝，使用 StrView
 * - 流模式：批量缓冲，减少系统调用
 * - 资源获取即初始化的资源管理
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
     * @param source 源字符串（必须在输入流生命周期内有效）
     * 
     * 注意：使用 std::string_view，不拷贝数据。
     * 调用者必须确保源字符串在输入流使用期间保持有效。
     */
    explicit InputStream(StrView source);
    
    /**
     * @brief 从 std::istream 创建输入流
     * @param stream 输入流引用（文件流、字符串流等）
     * @param bufferSize 内部缓冲区大小（默认 4KB）
     * 
     * @note stream 必须在输入流生命周期内有效。
     */
    explicit InputStream(std::istream& stream, usize bufferSize = 4096);
    
    /**
     * @brief 文件路径标签（用于区分文件路径构造函数）
     */
    struct FromFile {};
    static constexpr FromFile fromFile{};

    /**
     * @brief 从文件路径创建输入流
     * @param tag 文件路径标签（使用 InputStream::fromFile）
     * @param filePath 文件路径（相对或绝对路径）
     * @param bufferSize 内部缓冲区大小（默认 4KB）
     *
     * 特性：
     * - 文件以二进制模式打开（避免换行符转换）
     * - 源名称自动设置为文件路径
     * - 文件流由输入流对象自动管理
     *
     * 使用示例：
     * @code
     * InputStream input(InputStream::fromFile, "script.lua");
     * @endcode
     *
     * @throws std::runtime_error 如果文件打开失败
     */
    InputStream(FromFile tag, const Str& filePath, usize bufferSize = 4096);
    
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

    /** @brief 流式输入（可选，流模式使用） */
    std::istream* stream_;
    /** @brief 拥有的文件流（用于文件路径构造） */
    Opt<std::ifstream> ownedFileStream_;
    /** @brief 字符串输入（零拷贝，字符串模式使用） */
    StrView stringView_;

    // =====================================================================
    // 缓冲区（仅流模式使用）
    // =====================================================================

    /** @brief 内部缓冲（自动资源管理）。 */
    Vec<char> buffer_;
    /** @brief 缓冲区当前位置 */
    usize bufferPos_;
    /** @brief 缓冲区有效数据大小 */
    usize bufferSize_;

    // =====================================================================
    // 状态
    // =====================================================================

    /** @brief EOF 标志 */
    bool eof_;
    /** @brief 全局位置（用于错误报告） */
    usize position_;
    /** @brief 是否使用字符串视图模式 */
    bool useStringView_;
    /** @brief 源名称（用于错误报告） */
    Str sourceName_;
};

} // namespace IO
} // namespace Lua


