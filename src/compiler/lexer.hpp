#pragma once

/**
 * @file lexer.hpp
 * @brief Lua词法分析器
 * 
 * 实现Lua 5.1的词法分析器，将源代码文本转换为Token流。
 * 
 * 核心功能：
 * - 识别Lua 5.1的所有关键字、运算符和字面量
 * - 支持单行注释（--）和多行注释（--[[ ]]）
 * - 支持长字符串（[[ ]]和[=[ ]=]）
 * - 精确的行号和列号跟踪
 * - 详细的错误报告
 * - Token预读机制，支持LL(1)语法分析
 * - 哈希表优化的关键字识别
 * 
 * 参考实现：
 * - lua_c_analysis/src/llex.c - Lua 5.1.5 C版本词法分析器
 * - lua_with_cpp/src/lexer/lexer.cpp - C++参考实现
 */

#include "token.hpp"
#include "common/types.hpp"
#include "io/input_stream.hpp"
#include <optional>

namespace Lua {

/**
 * @brief Lua词法分析器类
 * 
 * 使用单遍扫描算法，从源代码字符串中提取Token。
 * 
 * 特性：
 * - 流式处理，支持大文件
 * - Token预读机制（peekToken），支持LL(1)语法分析
 * - 前瞻一个字符的词法分析
 * - 自动跳过空白和注释
 * - 支持所有Lua 5.1词法规则
 * - 使用哈希表优化关键字识别（O(1)时间复杂度）
 */
class Lexer {
public:
    /**
     * @brief 从字符串构造 Lexer（向后兼容）
     * @param source 源代码字符串
     *
     * 内部会创建 StringInputStream，保持向后兼容性。
     */
    explicit Lexer(const Str& source);

    /**
     * @brief 从 InputStream 构造 Lexer
     * @param input 输入流引用
     *
     * 注意：input 必须在 Lexer 生命周期内保持有效。
     */
    explicit Lexer(IO::InputStream& input);
    
    /**
     * @brief 获取下一个Token
     * @return Token对象
     * 
     * 如果存在预读Token，则返回并清除预读状态；
     * 否则从输入流中解析新Token。
     */
    Token nextToken();
    
    /**
     * @brief 预读下一个Token而不消费当前Token
     * @return 预读的Token对象
     * 
     * 支持LL(1)语法分析的前瞻功能。预读的Token会被缓存，
     * 下次调用nextToken()时会返回该Token。
     */
    Token peekToken();
    
    /**
     * @brief 检查是否到达源代码末尾
     */
    bool isAtEnd() const noexcept;
    
    /**
     * @brief 获取当前行号
     */
    i32 getCurrentLine() const noexcept { return line_; }
    
    /**
     * @brief 获取当前列号
     */
    i32 getCurrentColumn() const noexcept { return column_; }

private:
    // =====================================================================
    // 字符操作
    // =====================================================================
    
    /**
     * @brief 前进一个字符并返回
     */
    char advance();
    
    /**
     * @brief 查看当前字符（不前进）
     */
    char peek() const noexcept;
    
    /**
     * @brief 查看下一个字符（不前进）
     */
    char peekNext() const noexcept;
    
    /**
     * @brief 如果当前字符匹配，则前进
     */
    bool match(char expected);
    
    // =====================================================================
    // Token创建
    // =====================================================================
    
    /**
     * @brief 创建Token
     */
    Token makeToken(TokenType type);
    
    /**
     * @brief 创建错误Token
     */
    Token errorToken(const Str& message);
    
    // =====================================================================
    // 跳过空白和注释
    // =====================================================================
    
    /**
     * @brief 跳过空白字符和注释
     */
    void skipWhitespace();
    
    /**
     * @brief 跳过单行注释
     */
    void skipLineComment();
    
    /**
     * @brief 跳过多行注释
     * @param level 分隔符等级（=的数量）
     */
    void skipLongComment(i32 level);
    
    // =====================================================================
    // 识别不同类型的Token
    // =====================================================================
    
    /**
     * @brief 识别标识符或关键字
     */
    Token identifier();
    
    /**
     * @brief 识别数字（十进制或十六进制）
     */
    Token number();
    
    /**
     * @brief 识别十六进制数字
     */
    Token hexNumber();
    
    /**
     * @brief 识别字符串（单引号或双引号）
     */
    Token string(char quote);
    
    /**
     * @brief 识别长字符串
     * @param level 分隔符等级（=的数量）
     */
    Token longString(i32 level);
    
    /**
     * @brief 跳过长字符串/注释的分隔符 [=*[
     * @return 等号数量，如果不是有效分隔符返回-1
     */
    i32 skipSeparator();

private:
    /**
     * @brief 内部Token解析函数
     * @return 解析的Token对象
     * 
     * 实际执行词法分析的核心函数，被nextToken()和peekToken()调用。
     */
    Token scanToken();

private:
    // =====================================================================
    // 输入流管理
    // =====================================================================

    // 当从字符串构造 Lexer 时，保存一份源代码副本，
    // 以保证 InputStream 持有的 std::string_view 在整个 Lexer 生命周期内始终有效。
    // 注意：此成员不会再用于基于下标的字符访问，仅用于管理生命周期。
    Str sourceStorage_;

    IO::InputStream* input_;           ///< 输入流指针（非拥有，用于引用构造）
    UPtr<IO::InputStream> ownedInput_; ///< 拥有的输入流（用于字符串构造）

    // =====================================================================
    // 字符缓存（用于实现 peek 和 peekNext）
    // =====================================================================

    i32 currentChar_;    ///< 当前字符缓存（-1 表示 EOF）
    i32 nextChar_;       ///< 下一个字符缓存（用于 peekNext）
    bool hasNextChar_;   ///< nextChar_ 是否已加载

    // =====================================================================
    // Lexeme 累积缓冲区
    // =====================================================================

    Str lexemeBuffer_;   ///< 累积当前 token 的字符

    // =====================================================================
    // 位置跟踪
    // =====================================================================

    usize start_;        ///< 当前 Token 起始位置（用于错误报告）
    i32 line_;           ///< 当前行号（Lexer 自行维护）
    i32 column_;         ///< 当前列号（Lexer 自行维护）

    // =====================================================================
    // Token 预读机制（支持 LL(1) 语法分析）
    // =====================================================================

    Opt<Token> lookahead_;  ///< 预读 Token 缓存
};

} // namespace Lua

