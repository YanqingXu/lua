#include "parser.hpp"

namespace Lua {
    Parser::Parser(const Str& source) : lexer(source), hadError(false), errorReporter_(ErrorReporter::createDefault()) {
        // Initialize and immediately get the first token
        advance();
    }

    UPtr<Expr> Parser::parseExpression() {
        return expression();
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
        // Skip the current erroneous token
        advance();
        
        // Track how many tokens we've skipped to avoid infinite loops
        int tokensSkipped = 0;
        const int maxTokensToSkip = 50; // Reasonable limit
        
        while (!isAtEnd() && tokensSkipped < maxTokensToSkip) {
            // Primary synchronization point: after semicolon
            if (previous.type == TokenType::Semicolon) {
                return;
            }
            
            // Statement-level synchronization points
            switch (current.type) {
                // Control flow statements
                case TokenType::If:
                case TokenType::While:
                case TokenType::For:
                case TokenType::Repeat:
                case TokenType::Do:
                    return;
                    
                // Declaration statements
                case TokenType::Function:
                case TokenType::Local:
                    return;
                    
                // Flow control statements
                case TokenType::Return:
                case TokenType::Break:
                    return;
                    
                // Block terminators - good recovery points
                case TokenType::End:
                case TokenType::Until:
                case TokenType::Else:
                case TokenType::Elseif:
                    return;
                    
                // Expression terminators in certain contexts
                case TokenType::Then:
                    // Only synchronize on 'then' if we're likely in an if condition
                    return;
                    
                default:
                    break;
            }
            
            // Block-level synchronization: look for balanced braces/brackets
            if (current.type == TokenType::LeftBrace || 
                current.type == TokenType::LeftBracket || 
                current.type == TokenType::LeftParen) {
                // Skip balanced delimiters to avoid getting stuck inside them
                skipBalancedDelimiters();
                tokensSkipped++;
                continue;
            }
            
            // If we encounter a closing delimiter without matching opening,
            // it might be a good synchronization point
            if (current.type == TokenType::RightBrace || 
                current.type == TokenType::RightBracket || 
                current.type == TokenType::RightParen) {
                advance();
                return;
            }
            
            advance();
            tokensSkipped++;
        }
        
        // If we've skipped too many tokens, report a warning
        if (tokensSkipped >= maxTokensToSkip) {
            error(ErrorType::InternalError, 
                  "Error recovery skipped too many tokens. Parser may be confused.");
        }
    }

    void Parser::skipBalancedDelimiters() {
        TokenType openType = current.type;
        TokenType closeType;
        
        // Determine the matching closing delimiter
        switch (openType) {
            case TokenType::LeftParen:
                closeType = TokenType::RightParen;
                break;
            case TokenType::LeftBrace:
                closeType = TokenType::RightBrace;
                break;
            case TokenType::LeftBracket:
                closeType = TokenType::RightBracket;
                break;
            default:
                // Not a delimiter we handle
                return;
        }
        
        int depth = 0;
        int maxDepth = 20; // Prevent infinite loops in malformed code
        
        while (!isAtEnd() && depth < maxDepth) {
            if (current.type == openType) {
                depth++;
            } else if (current.type == closeType) {
                depth--;
                if (depth == 0) {
                    advance(); // Consume the closing delimiter
                    return;
                }
            }
            advance();
        }
        
        // If we exit due to max depth, report an error
        if (depth >= maxDepth) {
            error(ErrorType::MismatchedParentheses, 
                  "Deeply nested or unmatched delimiters detected during error recovery.");
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