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

        // String concatenation is right-associative (lower precedence than +/-)
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
            SourceLocation opLocation = SourceLocation::fromToken(previous);
            
            UPtr<Expr> expr;
            try {
                expr = unary();
            } catch (...) {
                auto unaryOpError = ParseError(ErrorType::InvalidExpression, 
                    opLocation, 
                    "Invalid operand for unary operator");
                unaryOpError.addSuggestion(FixType::Insert, 
                    SourceLocation::fromToken(current), 
                    "Add valid expression after unary operator", "nil");
                errorReporter_.addError(std::move(unaryOpError));
                
                synchronize();
                expr = std::make_unique<LiteralExpr>(Value()); // Recovery value
            }
            
            // Validate unary operator usage
            if (op == TokenType::Hash && !isValidLengthOperand(expr.get())) {
                auto lengthOpError = ParseError(ErrorType::InvalidExpression, 
                    opLocation, 
                    "Length operator (#) can only be applied to strings and tables");
                lengthOpError.addSuggestion(FixType::Replace, 
                    opLocation, 
                    "Use string or table expression", "\"\"" );
                errorReporter_.addError(std::move(lengthOpError));
            }
            
            return std::make_unique<UnaryExpr>(op, std::move(expr));
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
        SourceLocation startLocation = SourceLocation::fromToken(current);
        
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
            try {
                double value = std::stod(previous.lexeme);
                return std::make_unique<LiteralExpr>(Value(value));
            } catch (const std::exception& e) {
                error(ErrorType::InvalidNumber, "Invalid number format: " + previous.lexeme + 
                    " (" + e.what() + ")");
                auto invalidNumberError = ParseError(ErrorType::InvalidNumber, 
                    SourceLocation::fromToken(previous), 
                    "Invalid number format: " + previous.lexeme);
                invalidNumberError.addSuggestion(FixType::Replace, 
                    SourceLocation::fromToken(previous), 
                    "Use valid number format", "0");
                errorReporter_.addError(std::move(invalidNumberError));
                return std::make_unique<LiteralExpr>(Value(0.0)); // Recovery value
            }
        }

        if (match(TokenType::String)) {
            // Use the GCString from token instead of creating a new string from lexeme
            return std::make_unique<LiteralExpr>(Value(previous.value.string->getString()));
        }

        if (match(TokenType::Name)) {
            return std::make_unique<VariableExpr>(previous.lexeme);
        }

        if (match(TokenType::LeftParen)) {
            UPtr<Expr> expr;
            try {
                expr = expression();
            } catch (...) {
                error(ErrorType::InvalidExpression, "Invalid expression in parentheses");
                synchronize();
                expr = std::make_unique<LiteralExpr>(Value()); // nil as recovery
            }
            
            if (consume(TokenType::RightParen, "Expect ')' after expression.").type != TokenType::RightParen) {
                auto mismatchedParenError = ParseError::mismatchedParentheses(
                    SourceLocation::fromToken(current), ")");
                mismatchedParenError.setDetails("Opening parenthesis at " + startLocation.toString());
                errorReporter_.addError(std::move(mismatchedParenError));
            }
            return expr;
        }

        // Table constructor
        if (match(TokenType::LeftBrace)) {
            try {
                return tableConstructor();
            } catch (...) {
                error(ErrorType::InvalidExpression, "Invalid table constructor");
                synchronize();
                return std::make_unique<TableExpr>(Vec<TableField>());
            }
        }

        // Function expression
        if (match(TokenType::Function)) {
            try {
                return functionExpression();
            } catch (...) {
                error(ErrorType::InvalidExpression, "Invalid function expression");
                synchronize();
                // Return a dummy function expression
                return std::make_unique<FunctionExpr>(Vec<Str>(), 
                    std::make_unique<BlockStmt>(Vec<UPtr<Stmt>>()));
            }
        }

        // Enhanced error reporting for unexpected tokens
        auto unexpectedError = ParseError::unexpectedToken(startLocation, 
            "expression", current.lexeme);
        
        // Provide context-specific suggestions
        if (current.type == TokenType::RightParen || 
            current.type == TokenType::RightBrace || 
            current.type == TokenType::RightBracket) {
            unexpectedError.addSuggestion(FixType::Delete, startLocation, 
                "Remove unmatched closing delimiter", "");
        } else if (current.type == TokenType::Assign) {
            unexpectedError.addSuggestion(FixType::Insert, startLocation, 
                "Add variable name before assignment", "variable");
        } else {
            unexpectedError.addSuggestion(FixType::Replace, startLocation, 
                "Replace with valid expression", "nil");
        }
        
        errorReporter_.addError(std::move(unexpectedError));
        
        // Return a recovery expression
        return std::make_unique<LiteralExpr>(Value());
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
        bool isVariadic = false;
        
        if (!check(TokenType::RightParen)) {
            do {
                if (check(TokenType::DotDotDot)) {
                    advance(); // consume '...'
                    isVariadic = true;
                    break; // '...' must be the last parameter
                } else {
                    Token param = consume(TokenType::Name, "Expect parameter name.");
                    parameters.push_back(param.lexeme);
                }
            } while (match(TokenType::Comma));
        }

        consume(TokenType::RightParen, "Expect ')' after parameters.");

        // Parse function body as a block statement
        auto body = blockStatement();

        consume(TokenType::End, "Expect 'end' after function body.");

        return std::make_unique<FunctionExpr>(std::move(parameters), std::move(body), isVariadic);
    }
}