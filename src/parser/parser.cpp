#include "parser.hpp"

namespace Lua {
    Parser::Parser(const Str& source) : lexer(source), hadError(false) {
        // Initialize and immediately get the first token
        advance();
    }
    
    void Parser::advance() {
        previous = current;
        current = lexer.nextToken();
        
        // Skip error tokens
        while (current.type == TokenType::Error) {
            error(current.lexeme);
            current = lexer.nextToken();
        }
    }
    
    bool Parser::check(TokenType type) const {
        return current.type == type;
    }
    
    bool Parser::match(TokenType type) {
        if (!check(type)) return false;
        advance();
        return true;
    }
    
    bool Parser::match(std::initializer_list<TokenType> types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }
    
    Token Parser::consume(TokenType type, const Str& message) {
        if (check(type)) {
            Token token = current;
            advance();
            return token;
        }
        
        error(message);
        return previous; // Error recovery
    }
    
    void Parser::error(const Str& message) {
        hadError = true;
        // More detailed error handling and reporting can be added here
    }
    
    void Parser::synchronize() {
        // Error recovery: jump to next statement
        advance();
        
        while (!check(TokenType::Eof)) {
            if (previous.type == TokenType::Semicolon) return;
            
            switch (current.type) {
                case TokenType::Function:
                case TokenType::Local:
                case TokenType::If:
                case TokenType::While:
                case TokenType::For:
                case TokenType::Return:
                    return;
                default:
                    break;
            }
            
            advance();
        }
    }
    
    UPtr<Expr> Parser::expression() {
        return logicalOr();
    }
    
    UPtr<Expr> Parser::logicalOr() {
        auto expr = logicalAnd();
        
        while (match(TokenType::Or)) {
            TokenType op = previous.type;
            auto right = logicalAnd();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        
        return expr;
    }
    
    UPtr<Expr> Parser::logicalAnd() {
        auto expr = equality();
        
        while (match(TokenType::And)) {
            TokenType op = previous.type;
            auto right = equality();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        
        return expr;
    }
    
    UPtr<Expr> Parser::equality() {
        auto expr = comparison();
        
        while (match({TokenType::NotEqual, TokenType::Equal})) {
            TokenType op = previous.type;
            auto right = comparison();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        
        return expr;
    }
    
    UPtr<Expr> Parser::comparison() {
        auto expr = concatenation();
        
        while (match({TokenType::Greater, TokenType::GreaterEqual, 
                     TokenType::Less, TokenType::LessEqual})) {
            TokenType op = previous.type;
            auto right = concatenation();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        
        return expr;
    }
    
    UPtr<Expr> Parser::concatenation() {
        auto expr = simpleExpression();
        
        // String concatenation is right-associative
        if (match(TokenType::DotDot)) {
            TokenType op = previous.type;
            auto right = concatenation(); // Right-associative recursion
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        
        return expr;
    }
    
    UPtr<Expr> Parser::simpleExpression() {
        auto expr = term();
        
        while (match({TokenType::Plus, TokenType::Minus})) {
            TokenType op = previous.type;
            auto right = term();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        
        return expr;
    }
    
    UPtr<Expr> Parser::term() {
        auto expr = unary();
        
        while (match({TokenType::Star, TokenType::Slash, TokenType::Percent})) {
            TokenType op = previous.type;
            auto right = unary();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        
        return expr;
    }
    
    UPtr<Expr> Parser::unary() {
        if (match({TokenType::Not, TokenType::Minus, TokenType::Hash})) {
            TokenType op = previous.type;
            auto right = unary();
            return std::make_unique<UnaryExpr>(op, std::move(right));
        }
        
        return power();
    }
    
    UPtr<Expr> Parser::power() {
        auto expr = primary();
        
        // Handle member access and function calls
        while (true) {
            if (match(TokenType::Dot)) {
                Token name = consume(TokenType::Name, "Expect property name after '.'.");
                expr = std::make_unique<MemberExpr>(std::move(expr), name.lexeme);
            } else if (check(TokenType::LeftParen)) {
                expr = finishCall(std::move(expr));
            } else {
                break;
            }
        }
        
        // Power operator is right-associative
        if (match(TokenType::Caret)) {
            TokenType op = previous.type;
            auto right = unary(); // Right-associative, but unary has higher precedence
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        
        return expr;
    }
    
    UPtr<Expr> Parser::primary() {
        if (match(TokenType::False)) {
            return std::make_unique<LiteralExpr>(Value(false));
        }
        
        if (match(TokenType::True)) {
            return std::make_unique<LiteralExpr>(Value(true));
        }
        
        if (match(TokenType::Nil)) {
            return std::make_unique<LiteralExpr>(Value(nullptr));
        }
        
        if (match(TokenType::Number)) {
            return std::make_unique<LiteralExpr>(Value(previous.value.number));
        }
        
        if (match(TokenType::String)) {
            return std::make_unique<LiteralExpr>(Value(*previous.value.string));
        }
        
        if (match(TokenType::Name)) {
            return std::make_unique<VariableExpr>(previous.lexeme);
        }
        
        if (match(TokenType::LeftParen)) {
            auto expr = expression();
            consume(TokenType::RightParen, "Expect ')' after expression.");
            return expr;
        }
        
        error("Expect expression.");
        return nullptr;
    }
    
    // Add new method to handle function calls
    UPtr<Expr> Parser::finishCall(UPtr<Expr> callee) {
        consume(TokenType::LeftParen, "Expect '(' for function call.");
        
        Vec<UPtr<Expr>> arguments;
        
        if (!check(TokenType::RightParen)) {
            do {
                arguments.push_back(expression());
            } while (match(TokenType::Comma));
        }
        
        consume(TokenType::RightParen, "Expect ')' after arguments.");
        
        return std::make_unique<CallExpr>(std::move(callee), std::move(arguments));
    }
    
    UPtr<Stmt> Parser::statement() {
        if (match(TokenType::Local)) {
            return localDeclaration();
        }
        
        return expressionStatement();
    }
    
    UPtr<Stmt> Parser::expressionStatement() {
        auto expr = expression();
        // Semicolon is optional in Lua
        match(TokenType::Semicolon);
        return std::make_unique<ExprStmt>(std::move(expr));
    }
    
    UPtr<Stmt> Parser::localDeclaration() {
        Token name = consume(TokenType::Name, "Expect variable name after 'local'.");
        
        UPtr<Expr> initializer = nullptr;
        if (match(TokenType::Assign)) {
            initializer = expression();
        }
        
        // Semicolon is optional in Lua
        match(TokenType::Semicolon);
        
        return std::make_unique<LocalStmt>(name.lexeme, std::move(initializer));
    }
    
    Vec<UPtr<Stmt>> Parser::parse() {
        Vec<UPtr<Stmt>> statements;
        
        while (!check(TokenType::Eof)) {
            try {
                statements.push_back(statement());
            } catch (...) {
                synchronize();
            }
        }
        
        return statements;
    }
}
