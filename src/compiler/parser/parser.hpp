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

#include "compiler/ast.hpp"
#include "lexer.hpp"
#include "token.hpp"
#include "common/lua_error.hpp"
#include "common/types.hpp"
#include <expected>
#include <utility>

namespace Lua {

struct RuntimeServices;

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
     * @param services 显式运行时服务集合（保留给后续词法/解析服务注入）
     */
    Parser(const Str& source, RuntimeServices& services);
    
    /**
     * @brief 解析源代码，生成AST
     * @return Chunk对象（程序块）或 ParseError
     */
    [[nodiscard]] std::expected<Chunk, ParseError> parse();

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
     * @brief 前瞻下一个Token（不消费）
     */
    Token peek();

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
     * @brief 报告语法错误（记录错误但不立即抛出）
     * @param message 错误消息
     *
     * 注意：此方法已修改为支持错误恢复。
     * 错误会被记录到 errors_ 向量中，并在 parse() 结束时统一返回。
     */
    void error(const Str& message);

    /**
     * @brief 记录语法错误但不立即抛出异常
     * @param message 错误消息
     *
     * 将错误添加到 errors_ 向量中，并设置 panicMode_ 标志。
     * 这允许解析器继续解析并收集多个错误。
     */
    void reportError(const Str& message);

    /**
     * @brief 错误恢复同步
     *
     * 在遇到语法错误后，跳过 token 直到找到语句边界，
     * 以便继续解析后续代码。同步点包括：
     * - 语句结束符：';'
     * - 块结束符：'end', 'else', 'elseif', 'until'
     * - 语句开始符：'local', 'function', 'if', 'while', 'for', 'repeat', 'return', 'break', 'do'
     */
    void synchronize();

    /**
     * @brief 安全借用 token 的字符串值，供多个 Parser 实现分片共享。
     */
    static StrView tokenString(const Token& token) noexcept {
        if (std::holds_alternative<Str>(token.value)) {
            return std::get<Str>(token.value);
        }
        return token.lexeme;
    }

    /**
     * @brief 生成带有 near token 后缀的 Lua 风格错误消息。
     */
    static Str errorWithNear(const Str& message, const Token& token);

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
    // =====================================================================
    // 错误恢复机制
    // =====================================================================

    /**
     * @brief 错误收集器
     *
     * 存储解析过程中遇到的所有错误。
     * 在 parse() 方法结束时，如果有错误，会返回包含错误信息的 expected。
     */
    Vec<ParseError> errors_;

    /**
     * @brief 错误恢复模式标志
     *
     * 当遇到语法错误时设置为 true，表示解析器正在进行错误恢复。
     * 在成功同步到下一个语句边界后重置为 false。
     */
    bool panicMode_ = false;

    // =====================================================================
    // AST 节点内存池（P0-3 优化）
    // =====================================================================

    /**
     * @brief AST 节点内存池
     *
     * 用于批量分配 AST 节点，减少内存分配开销。
     * 所有节点在 Parser 对象销毁时一起释放。
     *
     * 设计原则：
     * - 使用 std::vector 存储所有分配的节点
     * - 保持 std::unique_ptr 的所有权语义
     * - 节点生命周期与 Parser 对象绑定
     */
    template<typename T>
    class NodePool {
    public:
        /**
         * @brief 分配一个新节点
         * @tparam U 节点的具体类型（必须是 T 的子类型）
         * @tparam Args 构造函数参数类型
         * @param args 构造函数参数
         * @return 指向新分配节点的 unique_ptr
         */
        template<typename U, typename... Args>
        UPtr<T> allocate(Args&&... args) {
            // 创建节点
            auto node = std::make_unique<T>(U(std::forward<Args>(args)...));

            // 保存原始指针用于生命周期管理
            T* rawPtr = node.get();
            nodes_.push_back(rawPtr);

            // 返回 unique_ptr（所有权转移给调用者）
            return node;
        }

        /**
         * @brief 获取已分配节点的数量
         * @return 节点数量
         */
        size_t size() const {
            return nodes_.size();
        }

        /**
         * @brief 清空内存池
         */
        void clear() {
            nodes_.clear();
        }

    private:
        // 存储所有分配的节点指针（用于统计和调试）
        // 注意：实际的内存管理由 unique_ptr 负责
        Vec<T*> nodes_;
    };

    // =====================================================================
    // 递归深度保护
    // =====================================================================

    // 注意：设置为100以避免栈溢出。实际的C++调用栈深度会更深，
    // 因为每次parseExpression()会调用多层解析函数（parseOrExpr → parseAndExpr → ...）
    static constexpr i32 MAX_RECURSION_DEPTH = 100;
    i32 recursionDepth_ = 0;

    /**
     * @brief RAII递归深度守卫
     *
     * 自动管理递归深度计数，防止深度嵌套导致栈溢出。
     * 在递归函数入口创建，离开时自动递减。
     */
    class RecursionGuard {
    public:
        explicit RecursionGuard(Parser& parser) : parser_(parser) {
            if (++parser_.recursionDepth_ > MAX_RECURSION_DEPTH) {
                parser_.error("chunk has too many syntax levels");
            }
        }

        ~RecursionGuard() {
            --parser_.recursionDepth_;
        }

        // 禁止拷贝和赋值
        RecursionGuard(const RecursionGuard&) = delete;
        RecursionGuard& operator=(const RecursionGuard&) = delete;

    private:
        Parser& parser_;
    };

    friend class RecursionGuard;

    // =====================================================================
    // AST 节点工厂方法（P0-3 优化）
    // =====================================================================

    /**
     * @brief 创建表达式节点
     * @tparam T 表达式的具体类型（如 NumberExpr, BinaryExpr 等）
     * @tparam Args 构造函数参数类型
     * @param args 构造函数参数
     * @return 指向新表达式节点的 unique_ptr
     */
    template<typename T, typename... Args>
    ExprPtr makeExpr(Args&&... args) {
        return exprPool_.allocate<T>(std::forward<Args>(args)...);
    }

    /**
     * @brief 创建语句节点
     * @tparam T 语句的具体类型（如 LocalStmt, IfStmt 等）
     * @tparam Args 构造函数参数类型
     * @param args 构造函数参数
     * @return 指向新语句节点的 unique_ptr
     */
    template<typename T, typename... Args>
    StmtPtr makeStmt(Args&&... args) {
        return stmtPool_.allocate<T>(std::forward<Args>(args)...);
    }

private:
    Lexer lexer_;
    Token current_;
    RuntimeServices* services_ = nullptr;

    // AST 节点内存池
    NodePool<Expr> exprPool_;
    NodePool<Stmt> stmtPool_;
};

} // namespace Lua

