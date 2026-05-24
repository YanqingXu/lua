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
 * 设计原则：
 * - 清晰的递归下降结构
 * - 每个语法规则对应一个解析函数
 * - 使用智能指针管理AST节点
 * - 完整的错误恢复机制
 */

#include "common/lua_error.hpp"
#include "common/types.hpp"
#include "compiler/ast.hpp"
#include <expected>

namespace Lua {

struct RuntimeServices;

enum class ParseRecoveryMode {
    FailFast,
    StatementBoundary
};

struct ParserOptions {
    ParseRecoveryMode recoveryMode = ParseRecoveryMode::FailFast;
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
     * @brief 构造函数
     * @param source 源代码字符串
     * @param options 解析器配置
     */
    Parser(const Str& source, ParserOptions options);

    /**
     * @brief 构造函数
     * @param source 源代码字符串
     * @param services 显式运行时服务集合（保留给后续词法/解析服务注入）
     */
    Parser(const Str& source, RuntimeServices& services);

    /**
     * @brief 构造函数
     * @param source 源代码字符串
     * @param services 显式运行时服务集合（保留给后续词法/解析服务注入）
     * @param options 解析器配置
     */
    Parser(const Str& source, RuntimeServices& services, ParserOptions options);

    ~Parser();

    Parser(const Parser&) = delete;
    Parser& operator=(const Parser&) = delete;
    Parser(Parser&&) noexcept;
    Parser& operator=(Parser&&) noexcept;
    
    /**
     * @brief 解析源代码，生成AST
     * @return Chunk对象（程序块）或 ParseError
     */
    [[nodiscard]] std::expected<Chunk, ParseError> parse();

    /**
     * @brief 返回本次解析中收集到的诊断信息。
     *
     * 默认 FailFast 模式下通常只包含第一个错误；StatementBoundary 模式会尝试继续
     * 解析后续语句并收集更多错误。
     */
    [[nodiscard]] const Vec<ParseError>& diagnostics() const noexcept;

private:
    class Impl;
    UPtr<Impl> impl_;
};

} // namespace Lua

