#pragma once

/**
 * @file token.hpp
 * @brief Lua词法标记定义
 * 
 * 定义Lua 5.1词法分析器使用的所有标记类型和相关数据结构。
 * 
 * 词法约定遵循 Lua 5.1 Reference Manual。
 */

#include "common/types.hpp"
#include <variant>

namespace Lua {

/**
 * @brief Token类型枚举
 * 
 * 包含Lua 5.1的所有标记类型：
 * - 关键字（21个）
 * - 运算符和分隔符
 * - 字面量（数字、字符串、标识符）
 * - 特殊标记（EOF、错误）
 * 
 * 注意：单字符标记（如+、-、*等）直接使用ASCII值，
 * 多字符标记从257开始编号（避免与ASCII冲突）
 */
enum class TokenType : i32 {
    // ===== 关键字 (21个，按字母顺序) =====
    // 起始值257，避免与ASCII字符冲突
    And = 257,      // and
    Break,          // break
    Do,             // do
    Else,           // else
    Elseif,         // elseif
    End,            // end
    False,          // false
    For,            // for
    Function,       // function
    If,             // if
    In,             // in
    Local,          // local
    Nil,            // nil
    Not,            // not
    Or,             // or
    Repeat,         // repeat
    Return,         // return
    Then,           // then
    True,           // true
    Until,          // until
    While,          // while
    
    // ===== 多字符运算符 =====
    Concat,         // ..  (字符串连接)
    Dots,           // ... (可变参数)
    Eq,             // ==  (等于)
    Ge,             // >=  (大于等于)
    Le,             // <=  (小于等于)
    Ne,             // ~=  (不等于)
    
    // ===== 字面量和标识符 =====
    Number,         // 数字字面量
    String,         // 字符串字面量
    Name,           // 标识符
    
    // ===== 特殊标记 =====
    Eos,            // 文件结束 (End of Stream)
    Error           // 词法错误
};

/**
 * @brief Token语义信息
 * 
 * 使用std::variant存储不同类型的Token值：
 * - f64: 数字字面量的值
 * - Str: 字符串字面量或标识符的值
 * - std::monostate: 无值（关键字、运算符等）
 */
using TokenValue = Var<std::monostate, f64, Str>;

/**
 * @brief Token结构体
 * 
 * 表示词法分析过程中的一个标记，包含：
 * - 标记类型
 * - 语义值（数字、字符串等）
 * - 源代码位置信息（行号、列号）
 * - 原始词素（lexeme）
 */
struct Token {
    TokenType type;         ///< 标记类型
    TokenValue value;       ///< 语义值
    Str lexeme;             ///< 原始词素（源代码中的文本）
    Str errorMessage;       ///< 词法错误消息（仅 Error token 使用）
    i32 line;               ///< 行号（从1开始）
    i32 column;             ///< 列号（从1开始）
    
    /**
     * @brief 默认构造函数 - 创建EOF标记
     */
    Token() noexcept
        : type(TokenType::Eos)
        , value(std::monostate{})
        , lexeme()
        , errorMessage()
        , line(1)
        , column(1)
    {
    }
    
    /**
     * @brief 构造函数 - 创建指定类型的标记
     */
    Token(TokenType t, const Str& lex, i32 ln, i32 col) noexcept
        : type(t)
        , value(std::monostate{})
        , lexeme(lex)
        , errorMessage()
        , line(ln)
        , column(col)
    {
    }
    
    /**
     * @brief 检查是否为数字标记
     */
    bool isNumber() const noexcept {
        return type == TokenType::Number;
    }
    
    /**
     * @brief 检查是否为字符串标记
     */
    bool isString() const noexcept {
        return type == TokenType::String;
    }
    
    /**
     * @brief 检查是否为标识符
     */
    bool isName() const noexcept {
        return type == TokenType::Name;
    }
    
    /**
     * @brief 检查是否为关键字
     */
    bool isKeyword() const noexcept {
        return type >= TokenType::And && type <= TokenType::While;
    }
};

/**
 * @brief 将TokenType转换为字符串（用于调试和错误报告）
 */
const char* tokenTypeToString(TokenType type);

} // namespace Lua

