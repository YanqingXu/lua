#pragma once

/**
 * @file parser_impl.hpp
 * @brief Lua语法分析器内部实现声明
 *
 * 声明Parser::Impl的内部状态、AST构造、Token流、诊断恢复和各语法分片接口。
 */

#include "parser.hpp"
#include "compiler/lexer/lexer.hpp"
#include "token.hpp"

#include <utility>

namespace Lua {

class Parser::Impl {
public:
    Impl(const Str& source, ParserOptions options);
    Impl(const Str& source, RuntimeServices& services, ParserOptions options);

    [[nodiscard]] std::expected<Chunk, ParseError> parse();
    [[nodiscard]] const Vec<ParseError>& diagnostics() const noexcept;

private:
    class AstFactory {
    public:
        template<typename T, typename... Args>
        ExprPtr makeExpr(Args&&... args) {
            return makeUnique<Expr>(T(std::forward<Args>(args)...));
        }

        template<typename T, typename... Args>
        StmtPtr makeStmt(Args&&... args) {
            return makeUnique<Stmt>(T(std::forward<Args>(args)...));
        }

        ExprPtr makeBinaryExpr(BinaryExpr::Op op, const Token& opToken, ExprPtr left, ExprPtr right) {
            BinaryExpr expr;
            expr.op = op;
            expr.left = std::move(left);
            expr.right = std::move(right);
            expr.line = opToken.line;
            expr.column = opToken.column;
            return makeExpr<BinaryExpr>(std::move(expr));
        }

        ExprPtr makeUnaryExpr(UnaryExpr::Op op, const Token& opToken, ExprPtr operand) {
            UnaryExpr expr;
            expr.op = op;
            expr.operand = std::move(operand);
            expr.line = opToken.line;
            expr.column = opToken.column;
            return makeExpr<UnaryExpr>(std::move(expr));
        }
    };

    class TokenStream {
    public:
        explicit TokenStream(const Str& source)
            : lexer_(source)
            , current_(lexer_.nextToken()) {
        }

        [[nodiscard]] const Token& current() const noexcept {
            return current_;
        }

        void advance() {
            current_ = lexer_.nextToken();
        }

        [[nodiscard]] Token peek() {
            return lexer_.peekToken();
        }

        [[nodiscard]] bool check(TokenType type) const noexcept {
            return current_.type == type;
        }

    private:
        Lexer lexer_;
        Token current_;
    };

    class ParseState {
    public:
        i32 enterSyntaxLevel() noexcept {
            return ++recursionDepth_;
        }

        void leaveSyntaxLevel() noexcept {
            --recursionDepth_;
        }

    private:
        i32 recursionDepth_ = 0;
    };

    class ParseDiagnosticObserver {
    public:
        virtual ~ParseDiagnosticObserver() = default;
        virtual void onParseDiagnostic(const ParseError& error) = 0;
    };

    class ParseDiagnosticCollector final : public ParseDiagnosticObserver {
    public:
        void onParseDiagnostic(const ParseError& error) override {
            diagnostics_.push_back(error);
        }

        void clear() {
            diagnostics_.clear();
        }

        [[nodiscard]] bool empty() const noexcept {
            return diagnostics_.empty();
        }

        [[nodiscard]] const ParseError& first() const {
            return diagnostics_.front();
        }

        [[nodiscard]] const Vec<ParseError>& diagnostics() const noexcept {
            return diagnostics_;
        }

    private:
        Vec<ParseError> diagnostics_;
    };

    class ErrorRecoveryStrategy {
    public:
        virtual ~ErrorRecoveryStrategy() = default;
        [[nodiscard]] virtual bool canRecover(const Impl& parser, const ParseError& error) const = 0;
        virtual void recover(Impl& parser) = 0;
    };

    class FailFastRecoveryStrategy final : public ErrorRecoveryStrategy {
    public:
        [[nodiscard]] bool canRecover(const Impl&, const ParseError&) const override {
            return false;
        }

        void recover(Impl&) override {}
    };

    class StatementBoundaryRecoveryStrategy final : public ErrorRecoveryStrategy {
    public:
        [[nodiscard]] bool canRecover(const Impl&, const ParseError&) const override {
            return true;
        }

        void recover(Impl& parser) override {
            parser.synchronize();
        }
    };

private:
    const Token& current() const;
    void advance();
    Token peek();
    bool check(TokenType type) const;
    bool match(TokenType type);
    void expect(TokenType type, const Str& message);

    void error(const Str& message);
    void errorAt(const Token& token, const Str& message);
    void reportError(const Str& message);
    void reportErrorAt(const Token& token, const Str& message);
    void publishDiagnostic(const ParseError& error);
    void synchronize();
    [[nodiscard]] bool canRecoverFrom(const ParseError& error) const;
    void recoverAfterError();
    [[nodiscard]] static Str errorWithNear(const Str& message, const Token& token);
    [[nodiscard]] static UPtr<ErrorRecoveryStrategy> makeRecoveryStrategy(ParseRecoveryMode mode);

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
    StmtPtr parseExprStmt();

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
    ExprPtr parsePostfixExpr(ExprPtr base);

    Vec<Str> parseParamList();
    Vec<StmtPtr> parseBlock();
    Vec<ExprPtr> parseExprList();

    template<typename T, typename... Args>
    ExprPtr makeExpr(Args&&... args) {
        return astFactory_.makeExpr<T>(std::forward<Args>(args)...);
    }

    template<typename T, typename... Args>
    StmtPtr makeStmt(Args&&... args) {
        return astFactory_.makeStmt<T>(std::forward<Args>(args)...);
    }

    ExprPtr makeBinaryExpr(BinaryExpr::Op op, const Token& opToken, ExprPtr left, ExprPtr right) {
        return astFactory_.makeBinaryExpr(op, opToken, std::move(left), std::move(right));
    }

    ExprPtr makeUnaryExpr(UnaryExpr::Op op, const Token& opToken, ExprPtr operand) {
        return astFactory_.makeUnaryExpr(op, opToken, std::move(operand));
    }

private:
    static constexpr i32 MAX_RECURSION_DEPTH = 92;
    static constexpr i32 MAX_BLOCK_RECURSION_DEPTH = 80;

    class RecursionGuard {
    public:
        explicit RecursionGuard(Impl& parser, i32 maxDepth = MAX_RECURSION_DEPTH)
            : parser_(parser)
            , maxDepth_(maxDepth) {
            entered_ = true;
            if (parser_.parseState_.enterSyntaxLevel() > maxDepth_) {
                parser_.parseState_.leaveSyntaxLevel();
                entered_ = false;
                const Token& token = parser_.current();
                throw ParseError("chunk has too many syntax levels", token.line, token.column);
            }
        }

        ~RecursionGuard() {
            if (entered_) {
                parser_.parseState_.leaveSyntaxLevel();
            }
        }

        RecursionGuard(const RecursionGuard&) = delete;
        RecursionGuard& operator=(const RecursionGuard&) = delete;

    private:
        Impl& parser_;
        i32 maxDepth_;
        bool entered_ = false;
    };

private:
    TokenStream tokenStream_;
    RuntimeServices* services_ = nullptr;
    ParseState parseState_;
    AstFactory astFactory_;
    ParseDiagnosticCollector diagnosticCollector_;
    Vec<ParseDiagnosticObserver*> diagnosticObservers_;
    UPtr<ErrorRecoveryStrategy> recoveryStrategy_;
};

}
