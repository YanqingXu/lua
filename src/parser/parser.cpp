#include "parser.hpp"

namespace Lua {
    Parser::Parser(const Str& source) : lexer(source), hadError(false) {
        // 初始化后立即获取第一个标记
        advance();
    }
    
    void Parser::advance() {
        previous = current;
        current = lexer.nextToken();
        
        // 跳过错误标记
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
        return previous; // 错误恢复
    }
    
    void Parser::error(const Str& message) {
        hadError = true;
        // 这里可以添加更详细的错误处理和报告
    }
    
    void Parser::synchronize() {
        // 错误恢复：跳转到下一个语句
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
    
    std::unique_ptr<Expr> Parser::expression() {
        return simpleExpression();
    }
    
    std::unique_ptr<Expr> Parser::simpleExpression() {
        auto expr = term();
        
        while (match({TokenType::Plus, TokenType::Minus})) {
            TokenType op = previous.type;
            auto right = term();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        
        return expr;
    }
    
    std::unique_ptr<Expr> Parser::term() {
        auto expr = factor();
        
        while (match({TokenType::Star, TokenType::Slash, TokenType::Percent})) {
            TokenType op = previous.type;
            auto right = factor();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        
        return expr;
    }
    
    std::unique_ptr<Expr> Parser::factor() {
        if (match({TokenType::Minus, TokenType::Not})) {
            TokenType op = previous.type;
            auto right = factor();
            return std::make_unique<UnaryExpr>(op, std::move(right));
        }
        
        return primary();
    }
    
    std::unique_ptr<Expr> Parser::primary() {
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
        
        // 处理其他基本表达式...
        
        error("Expect expression.");
        return nullptr;
    }
    
    UniquePtr<Stmt> Parser::statement() {
        if (match(TokenType::Local)) {
            return localDeclaration();
        }
        
        return expressionStatement();
    }
    
    UniquePtr<Stmt> Parser::expressionStatement() {
        auto expr = expression();
        // 在Lua中分号是可选的
        match(TokenType::Semicolon);
        return std::make_unique<ExprStmt>(std::move(expr));
    }
    
    UniquePtr<Stmt> Parser::localDeclaration() {
        Token name = consume(TokenType::Name, "Expect variable name after 'local'.");
        
        UniquePtr<Expr> initializer = nullptr;
        if (match(TokenType::Assign)) {
            initializer = expression();
        }
        
        // 在Lua中分号是可选的
        match(TokenType::Semicolon);
        
        return std::make_unique<LocalStmt>(name.lexeme, std::move(initializer));
    }
    
    Vec<UniquePtr<Stmt>> Parser::parse() {
        Vec<UniquePtr<Stmt>> statements;
        
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
