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

    bool Parser::isAtEnd() const {
        return current.type == TokenType::Eof;
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

    UPtr<Expr> Parser::primary() {
        if (match(TokenType::True)) {
            return std::make_unique<LiteralExpr>(Value(true));
        }

        if (match(TokenType::False)) {
            return std::make_unique<LiteralExpr>(Value(false));
        }

        if (match(TokenType::Nil)) {
            return std::make_unique<LiteralExpr>(Value());
        }

        if (match(TokenType::Number)) {
            return std::make_unique<LiteralExpr>(Value(std::stod(previous.lexeme)));
        }

        if (match(TokenType::String)) {
            return std::make_unique<LiteralExpr>(Value(previous.lexeme));
        }

        if (match(TokenType::Name)) {
            return std::make_unique<VariableExpr>(previous.lexeme);
        }

        if (match(TokenType::LeftParen)) {
            auto expr = expression();
            consume(TokenType::RightParen, "Expect ')' after expression.");
            return expr;
        }

        // Table constructor
        if (match(TokenType::LeftBrace)) {
            return tableConstructor();
        }

        // Function expression
        if (match(TokenType::Function)) {
            return functionExpression();
        }

        error("Expect expression.");
        return nullptr;
    }

    UPtr<Expr> Parser::power() {
        auto expr = primary();

        // Handle member access, index access and function calls
        while (true) {
            if (match(TokenType::Dot)) {
                Token name = consume(TokenType::Name, "Expect property name after '.'.");
                expr = std::make_unique<MemberExpr>(std::move(expr), name.lexeme);
            } else if (match(TokenType::LeftBracket)) {
                auto index = expression();
                consume(TokenType::RightBracket, "Expect ']' after index.");
                expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
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

    UPtr<Expr> Parser::tableConstructor() {
        Vec<TableField> fields;

        if (!check(TokenType::RightBrace)) {
            do {
                UPtr<Expr> key = nullptr;
                UPtr<Expr> value = nullptr;

                if (match(TokenType::LeftBracket)) {
                    // [expr] = value
                    key = expression();
                    consume(TokenType::RightBracket, "Expect ']' after table key.");
                    consume(TokenType::Assign, "Expect '=' after table key.");
                    value = expression();
                } else if (check(TokenType::Name)) {
                    Token nameToken = current;
                    advance();

                    if (match(TokenType::Assign)) {
                        // name = value
                        key = std::make_unique<LiteralExpr>(Value(nameToken.lexeme));
                        value = expression();
                    } else {
                        // Just a value (array-style)
                        // Put the name back as a variable expression
                        auto nameExpr = std::make_unique<VariableExpr>(nameToken.lexeme);
                        value = std::move(nameExpr);
                    }
                } else {
                    // Just a value (array-style)
                    value = expression();
                }

                fields.emplace_back(std::move(key), std::move(value));

            } while (match({TokenType::Comma, TokenType::Semicolon}));
        }

        consume(TokenType::RightBrace, "Expect '}' after table fields.");
        return std::make_unique<TableExpr>(std::move(fields));
    }

    UPtr<Stmt> Parser::statement() {
        if (match(TokenType::Local)) {
            return localDeclaration();
        }
    
        if (match(TokenType::If)) {
            return ifStatement();
        }
    
        if (match(TokenType::Return)) {
            return returnStatement();
        }
    
        return assignmentStatement();
    }

    UPtr<Stmt> Parser::assignmentStatement() {
        auto expr = expression();

        // Check if this is an assignment
        if (match(TokenType::Assign)) {
            if (!isValidAssignmentTarget(expr.get())) {
                error("Invalid assignment target.");
                return std::make_unique<ExprStmt>(std::move(expr));
            }

            auto value = expression();
            match(TokenType::Semicolon); // Optional semicolon
            return std::make_unique<AssignStmt>(std::move(expr), std::move(value));
        }

        // It's just an expression statement
        match(TokenType::Semicolon); // Optional semicolon
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    UPtr<Stmt> Parser::ifStatement() {
        auto condition = expression();
        consume(TokenType::Then, "Expect 'then' after if condition.");

        auto thenBranch = blockStatement();

        UPtr<Stmt> elseBranch = nullptr;
        if (match(TokenType::Else)) {
            elseBranch = blockStatement();
        }

        consume(TokenType::End, "Expect 'end' after if statement.");

        return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }

    UPtr<Stmt> Parser::blockStatement() {
        Vec<UPtr<Stmt>> statements;

        while (!check(TokenType::End) && !check(TokenType::Else) && !check(TokenType::Eof)) {
            statements.push_back(statement());
        }

        return std::make_unique<BlockStmt>(std::move(statements));
    }

    UPtr<Stmt> Parser::returnStatement() {
        UPtr<Expr> value = nullptr;

        // check if there is a value to return
        if (!check(TokenType::End) && !check(TokenType::Else) &&
            !check(TokenType::Semicolon) && !isAtEnd()) {
            value = expression();
        }

        match(TokenType::Semicolon);
        return std::make_unique<ReturnStmt>(std::move(value));
    }

    bool Parser::isValidAssignmentTarget(const Expr* expr) const {
        if (!expr) return false;

        ExprType type = expr->getType();
        return type == ExprType::Variable || 
               type == ExprType::Member || 
               type == ExprType::Index;
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

    UPtr<Stmt> Parser::expressionStatement() {
        auto expr = expression();
        match(TokenType::Semicolon); // Optional semicolon
        return std::make_unique<ExprStmt>(std::move(expr));
    }

    UPtr<Stmt> Parser::localDeclaration() {
        // Parse local variable declaration: local name = value
        Token name = consume(TokenType::Name, "Expect variable name.");

        UPtr<Expr> initializer = nullptr;
        if (match(TokenType::Assign)) {
            initializer = expression();
        }

        match(TokenType::Semicolon); // Optional semicolon
        return std::make_unique<LocalStmt>(name.lexeme, std::move(initializer));
    }

    UPtr<Expr> Parser::functionExpression() {
        consume(TokenType::LeftParen, "Expect '(' after 'function'.");

        Vec<Str> parameters;
        if (!check(TokenType::RightParen)) {
            do {
                Token param = consume(TokenType::Name, "Expect parameter name.");
                parameters.push_back(param.lexeme);
            } while (match(TokenType::Comma));
        }

        consume(TokenType::RightParen, "Expect ')' after parameters.");

        // Parse function body as a block statement
        auto body = blockStatement();

        consume(TokenType::End, "Expect 'end' after function body.");

        return std::make_unique<FunctionExpr>(std::move(parameters), std::move(body));
    }
}


