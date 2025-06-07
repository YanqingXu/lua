#pragma once

#include "../types.hpp"
#include "../lexer/lexer.hpp"
#include "../compiler/compiler.hpp"
#include "../vm/value.hpp"

// Include all AST node definitions
#include "ast/ast_base.hpp"
#include "ast/expressions.hpp"
#include "ast/statements.hpp"

namespace Lua {
    // Parser class
    class Parser {
    private:
        Lexer lexer;
        Token current;
        Token previous;
        bool hadError;

        // Helper methods
        void advance();
        bool check(TokenType type) const;
        bool match(TokenType type);
        bool match(std::initializer_list<TokenType> types);
        Token consume(TokenType type, const Str& message);
        void error(const Str& message);
        void synchronize();
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
        UPtr<Stmt> blockStatement();
        UPtr<Stmt> returnStatement();
        UPtr<Stmt> breakStatement();

        // Helper for assignment target validation
        bool isValidAssignmentTarget(const Expr* expr) const;

    public:
        explicit Parser(const Str& source);

        // Parse entire program
        Vec<UPtr<Stmt>> parse();

        // Check if there are errors
        bool hasError() const { return hadError; }
    };
}