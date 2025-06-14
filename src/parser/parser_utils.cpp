#include "parser.hpp"

namespace Lua {
    Parser::Parser(const Str& source) : lexer(source), hadError(false), errorReporter_(ErrorReporter::createDefault()) {
        // Initialize and immediately get the first token
        advance();
    }

    void Parser::advance() {
        previous = current;
        current = lexer.nextToken();

        // Skip error tokens
        while (current.type == TokenType::Error) {
            SourceLocation location = SourceLocation::fromToken(current);
            error(ErrorType::UnexpectedCharacter, location, "Lexical error: " + current.lexeme);
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

        // Use a more specific error type and details
        SourceLocation location = SourceLocation::fromToken(current);
        Str details = "Expected '" + tokenTypeToString(type) + "' but found '" + current.lexeme + "'";
        error(ErrorType::MissingToken, location, message, details);
        return previous; // Error recovery
    }

    void Parser::error(const Str& message) {
        hadError = true;

        // Create a detailed error report using the current token's location
        SourceLocation location = SourceLocation::fromToken(current);
        errorReporter_.reportError(ErrorType::Unknown, location, message);
    }
    
    void Parser::error(ErrorType type, const Str& message) {
        hadError = true;
        SourceLocation location = SourceLocation::fromToken(current);
        errorReporter_.reportError(type, location, message);
    }
    
    void Parser::error(ErrorType type, const Str& message, const Str& details) {
        hadError = true;
        SourceLocation location = SourceLocation::fromToken(current);
        errorReporter_.reportError(type, location, message, details);
    }
    
    void Parser::error(ErrorType type, const SourceLocation& location, const Str& message) {
        hadError = true;
        errorReporter_.reportError(type, location, message);
    }
    
    void Parser::error(ErrorType type, const SourceLocation& location, const Str& message, const Str& details) {
        hadError = true;
        errorReporter_.reportError(type, location, message, details);
    }

    void Parser::synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous.type == TokenType::Semicolon) return;

            switch (current.type) {
                case TokenType::Function:
                case TokenType::Local:
                case TokenType::If:
                case TokenType::While:
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

    Vec<UPtr<Stmt>> Parser::parse() {
        Vec<UPtr<Stmt>> statements;

        while (!isAtEnd()) {
            auto stmt = statement();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        }

        return statements;
    }

    bool Parser::isValidAssignmentTarget(const Expr* expr) const {
        if (!expr) return false;

        ExprType type = expr->getType();
        return type == ExprType::Variable ||
               type == ExprType::Member ||
               type == ExprType::Index;
    }
}