#pragma once

/**
 * @file parser.hpp
 * @brief Lua语法分析器
 * 
 * 实现Lua 5.1的语法分析器，将Token流转换为抽象语法树（AST）。
 * 
 * 核心功能：
 * - 递归下降解析（Recursive Descent Parsing）
 * - 支持Lua 5.1的完整语法
 * - 运算符优先级处理
 * - 详细的语法错误报告
 * - 生成完整的AST
 * 
 * 参考实现：
 * - lua_c_analysis/src/lparser.c - Lua 5.1.5 C版本语法分析器
 * 
 * 设计原则：
 * - 清晰的递归下降结构
 * - 每个语法规则对应一个解析函数
 * - 使用智能指针管理AST节点
 * - 完整的错误恢复机制
 */

#include "ast.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "common/types.hpp"
#include <stdexcept>

namespace Lua {

/**
 * @brief 语法错误异常
 */
class ParseError : public std::runtime_error {
public:
    ParseError(const Str& message, i32 line, i32 column)
        : std::runtime_error(message)
        , line_(line)
        , column_(column) {}
    
    i32 getLine() const { return line_; }
    i32 getColumn() const { return column_; }

private:
    i32 line_;
    i32 column_;
};

/**
 * @brief Lua语法分析器类
 * 
 * 使用递归下降算法解析Lua源代码，生成抽象语法树。
 * 
 * 特性：
 * - 完整的Lua 5.1语法支持
 * - 正确的运算符优先级和结合性
 * - 详细的错误位置信息
 * - 支持所有Lua语句和表达式
 */
class Parser {
public:
    /**
     * @brief 构造函数
     * @param source 源代码字符串
     */
    explicit Parser(const Str& source);
    
    /**
     * @brief 解析源代码，生成AST
     * @return Chunk对象（程序块）
     */
    Chunk parse();

private:
    // =====================================================================
    // Token管理
    // =====================================================================
    
    /**
     * @brief 获取当前Token
     */
    const Token& current() const;
    
    /**
     * @brief 前进到下一个Token
     */
    void advance();
    
    /**
     * @brief 检查当前Token类型
     */
    bool check(TokenType type) const;
    
    /**
     * @brief 匹配并消费Token
     */
    bool match(TokenType type);
    
    /**
     * @brief 期望特定Token，否则报错
     */
    void expect(TokenType type, const Str& message);
    
    /**
     * @brief 报告语法错误
     */
    [[noreturn]] void error(const Str& message);
    
    // =====================================================================
    // 语句解析
    // =====================================================================
    
    StmtPtr parseStatement();
    StmtPtr parseIfStmt();
    StmtPtr parseWhileStmt();
    StmtPtr parseDoStmt();
    StmtPtr parseForStmt();
    StmtPtr parseRepeatStmt();
    StmtPtr parseFunctionStmt();
    StmtPtr parseLocalStmt();
    StmtPtr parseReturnStmt();
    StmtPtr parseBreakStmt();
    StmtPtr parseExprStmt();  // 赋值或函数调用
    
    // =====================================================================
    // 表达式解析
    // =====================================================================
    
    ExprPtr parseExpression();
    ExprPtr parseOrExpr();
    ExprPtr parseAndExpr();
    ExprPtr parseRelationalExpr();
    ExprPtr parseConcatExpr();
    ExprPtr parseAdditiveExpr();
    ExprPtr parseMultiplicativeExpr();
    ExprPtr parseUnaryExpr();
    ExprPtr parsePowerExpr();
    ExprPtr parsePrimaryExpr();
    ExprPtr parseTableConstructor();
    ExprPtr parseFunctionExpr();
    
    /**
     * @brief 解析后缀表达式（函数调用、索引访问等）
     */
    ExprPtr parsePostfixExpr(ExprPtr base);
    
    // =====================================================================
    // 辅助函数
    // =====================================================================
    
    /**
     * @brief 解析参数列表
     */
    Vec<Str> parseParamList();
    
    /**
     * @brief 解析代码块
     */
    Vec<StmtPtr> parseBlock();
    
    /**
     * @brief 解析表达式列表
     */
    Vec<ExprPtr> parseExprList();

private:
    Lexer lexer_;
    Token current_;
};

} // namespace Lua

