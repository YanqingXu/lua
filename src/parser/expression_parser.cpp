#include "parser.hpp"

namespace Lua {
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
            // Use the GCString from token instead of creating a new string from lexeme
            return std::make_unique<LiteralExpr>(Value(previous.value.string->getString()));
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