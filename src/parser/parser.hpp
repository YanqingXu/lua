#pragma once

#include "../common/types.hpp"
#include "../lexer/lexer.hpp"
#include "../compiler/compiler.hpp"
#include "../vm/value.hpp"

// Include all AST node definitions
#include "ast/ast_base.hpp"
#include "ast/expressions.hpp"
#include "ast/statements.hpp"
#include "ast/parse_error.hpp"

namespace Lua {
    // Parser class
    class Parser {
    private:
        Lexer lexer;
        Token current;
        Token previous;
        bool hadError;
        ErrorReporter errorReporter_;

        // Helper methods
        void advance();
        bool check(TokenType type) const;
        bool match(TokenType type);
        bool match(std::initializer_list<TokenType> types);
        
    public:
        // Made public for testing purposes
        Token consume(TokenType type, const Str& message);
        void error(const Str& message);
        void error(ErrorType type, const Str& message);
        void error(ErrorType type, const Str& message, const Str& details);
        void error(ErrorType type, const SourceLocation& location, const Str& message);
        void error(ErrorType type, const SourceLocation& location, const Str& message, const Str& details);
        
    private:
        void synchronize();
        void skipBalancedDelimiters();
        bool trySkipBalancedDelimiters();
        void reportRecoveryInfo(const SourceLocation& start, const SourceLocation& end, 
                               int tokensSkipped, bool foundSyncPoint);
        
        // Expression validation helpers
        bool isLogicalOperator(TokenType op) const;
        bool isValidLogicalOperand(const Expr* expr) const;
        bool isValidLengthOperand(const Expr* expr) const;
        bool isAtEnd() const;

        // Parse expressions
        UPtr<Expr> expression();
        UPtr<Expr> logicalOr();
        UPtr<Expr> logicalAnd();
        UPtr<Expr> equality();
        UPtr<Expr> comparison();
        UPtr<Expr> concatenation();
        UPtr<Expr> simpleExpression();
        UPtr<Expr> term();
        UPtr<Expr> unary();
        UPtr<Expr> power();
        UPtr<Expr> primary();
        UPtr<Expr> finishCall(UPtr<Expr> callee);
        UPtr<Expr> tableConstructor();
        UPtr<Expr> functionExpression();

        // Parse statements
        UPtr<Stmt> statement();
        UPtr<Stmt> expressionStatement();
        UPtr<Stmt> localDeclaration();
        UPtr<Stmt> assignmentStatement();
        UPtr<Stmt> ifStatement();
        UPtr<Stmt> whileStatement();
        UPtr<Stmt> forStatement();
        UPtr<Stmt> forInStatement();
        UPtr<Stmt> repeatUntilStatement();
        UPtr<Stmt> blockStatement();
        UPtr<Stmt> returnStatement();
        UPtr<Stmt> breakStatement();
        UPtr<Stmt> functionStatement();
        UPtr<Stmt> doStatement();

        // Helper for assignment target validation
        bool isValidAssignmentTarget(const Expr* expr) const;

    public:
        explicit Parser(const Str& source);

        // Parse entire program
        Vec<UPtr<Stmt>> parse();
        
        // Parse single expression (for testing)
        UPtr<Expr> parseExpression();

        // Check if there are errors
        bool hasError() const { return hadError; }
        
        // Error reporting methods
        const ErrorReporter& getErrorReporter() const { return errorReporter_; }
        const Vec<ParseError>& getErrors() const { return errorReporter_.getErrors(); }
        size_t getErrorCount() const { return errorReporter_.getErrorCount(); }
        bool hasErrorsOrWarnings() const { return errorReporter_.hasErrorsOrWarnings(); }
        
        // Clear errors
        void clearErrors() { errorReporter_.clear(); hadError = false; }
    };
}