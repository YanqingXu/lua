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
        // Enhanced error recovery with context-aware synchronization
        SourceLocation errorStart = SourceLocation::fromToken(current);
        
        // Skip the current erroneous token
        advance();
        
        // Track recovery context and statistics
        int tokensSkipped = 0;
        int nestingLevel = 0;
        const int maxTokensToSkip = 100; // Increased limit for better recovery
        bool foundGoodSyncPoint = false;
        
        while (!isAtEnd() && tokensSkipped < maxTokensToSkip && !foundGoodSyncPoint) {
            // Primary synchronization point: after semicolon (statement boundary)
            if (previous.type == TokenType::Semicolon) {
                foundGoodSyncPoint = true;
                break;
            }
            
            // Track nesting level for better context awareness
            if (current.type == TokenType::LeftBrace || 
                current.type == TokenType::LeftBracket || 
                current.type == TokenType::LeftParen) {
                nestingLevel++;
            } else if (current.type == TokenType::RightBrace || 
                       current.type == TokenType::RightBracket || 
                       current.type == TokenType::RightParen) {
                nestingLevel--;
                // If we're back to top level, this is a good sync point
                if (nestingLevel <= 0) {
                    advance();
                    foundGoodSyncPoint = true;
                    break;
                }
            }
            
            // Statement-level synchronization points (only at top level)
            if (nestingLevel <= 0) {
                switch (current.type) {
                    // Control flow statements - excellent sync points
                    case TokenType::If:
                    case TokenType::While:
                    case TokenType::For:
                    case TokenType::Repeat:
                    case TokenType::Do:
                        foundGoodSyncPoint = true;
                        break;
                        
                    // Declaration statements - very good sync points
                    case TokenType::Function:
                    case TokenType::Local:
                        foundGoodSyncPoint = true;
                        break;
                        
                    // Flow control statements - good sync points
                    case TokenType::Return:
                    case TokenType::Break:
                        foundGoodSyncPoint = true;
                        break;
                        
                    // Block terminators - context-dependent sync points
                    case TokenType::End:
                    case TokenType::Until:
                    case TokenType::Else:
                    case TokenType::Elseif:
                        foundGoodSyncPoint = true;
                        break;
                        
                    // Expression terminators in control flow contexts
                    case TokenType::Then:
                        // Good sync point for if-then constructs
                        foundGoodSyncPoint = true;
                        break;
                        
                    default:
                        break;
                }
            }
            
            if (foundGoodSyncPoint) break;
            
            // Handle balanced delimiters more intelligently
            if (current.type == TokenType::LeftBrace || 
                current.type == TokenType::LeftBracket || 
                current.type == TokenType::LeftParen) {
                // Try to skip balanced delimiters, but be more careful
                if (!trySkipBalancedDelimiters()) {
                    // If we can't skip balanced delimiters, just advance
                    advance();
                }
                tokensSkipped++;
                continue;
            }
            
            advance();
            tokensSkipped++;
        }
        
        // Report recovery statistics and suggestions
        if (tokensSkipped > 0) {
            SourceLocation errorEnd = SourceLocation::fromToken(previous);
            reportRecoveryInfo(errorStart, errorEnd, tokensSkipped, foundGoodSyncPoint);
        }
        
        // If we've skipped too many tokens without finding a good sync point
        if (tokensSkipped >= maxTokensToSkip) {
            error(ErrorType::InternalError, 
                  "Error recovery failed: too many tokens skipped without finding synchronization point.");
        }
    }

    bool Parser::trySkipBalancedDelimiters() {
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
                return false;
        }
        
        SourceLocation startLocation = SourceLocation::fromToken(current);
        int depth = 0;
        int tokensInDelimiters = 0;
        const int maxDepth = 50; // Increased limit for complex expressions
        const int maxTokensInDelimiters = 200; // Prevent runaway parsing
        
        while (!isAtEnd() && depth < maxDepth && tokensInDelimiters < maxTokensInDelimiters) {
            if (current.type == openType) {
                depth++;
            } else if (current.type == closeType) {
                depth--;
                if (depth == 0) {
                    advance(); // Consume the closing delimiter
                    return true; // Successfully skipped balanced delimiters
                }
            }
            
            advance();
            tokensInDelimiters++;
        }
        
        // If we reach here, we have unmatched or overly complex delimiters
        if (depth >= maxDepth) {
            auto error = ParseError::mismatchedParentheses(startLocation, 
                tokenTypeToString(closeType));
            error.setDetails("Deeply nested delimiters detected during error recovery (depth: " + 
                           std::to_string(depth) + ")");
            errorReporter_.addError(std::move(error));
        } else if (tokensInDelimiters >= maxTokensInDelimiters) {
            error(ErrorType::MismatchedParentheses, startLocation,
                  "Extremely long delimiter block detected during error recovery",
                  "Consider breaking down complex expressions into smaller parts");
        }
        
        return false; // Failed to skip balanced delimiters
    }
    
    void Parser::skipBalancedDelimiters() {
        // Legacy method - now uses the enhanced version
        trySkipBalancedDelimiters();
    }
    
    void Parser::reportRecoveryInfo(const SourceLocation& start, const SourceLocation& end, 
                                   int tokensSkipped, bool foundSyncPoint) {
        if (tokensSkipped <= 3) {
            // Minor recovery - just a note
            return;
        }
        
        Str message;
        ErrorSeverity severity;
        
        if (foundSyncPoint) {
            if (tokensSkipped <= 10) {
                message = "Recovered from syntax error by skipping " + std::to_string(tokensSkipped) + " tokens";
                severity = ErrorSeverity::Info;
            } else {
                message = "Recovered from syntax error by skipping " + std::to_string(tokensSkipped) + " tokens";
                severity = ErrorSeverity::Warning;
            }
        } else {
            message = "Partial recovery: skipped " + std::to_string(tokensSkipped) + " tokens without finding clear synchronization point";
            severity = ErrorSeverity::Warning;
        }
        
        auto recoveryError = ParseError(ErrorType::InternalError, start, message, severity);
        
        // Add helpful suggestions based on recovery context
        if (tokensSkipped > 20) {
            recoveryError.addSuggestion(FixType::Insert, start, 
                "Consider adding missing delimiters or keywords", "");
        }
        
        if (!foundSyncPoint) {
            recoveryError.addSuggestion(FixType::Insert, end, 
                "Add a statement terminator (semicolon) or block delimiter", ";");
        }
        
        recoveryError.setDetails("Recovery span: " + start.toString() + " to " + end.toString());
        errorReporter_.addError(std::move(recoveryError));
    }
    
    bool Parser::isLogicalOperator(TokenType op) const {
        return op == TokenType::And || op == TokenType::Or;
    }
    
    bool Parser::isValidLogicalOperand(const Expr* expr) const {
        if (!expr) return false;
        
        // For now, accept all expressions as potentially valid logical operands
        // In a more sophisticated implementation, we could check for:
        // - Boolean literals
        // - Comparison expressions
        // - Function calls that return booleans
        // - Variables that might contain booleans
        return true;
    }
    
    bool Parser::isValidLengthOperand(const Expr* expr) const {
        if (!expr) return false;
        
        // Check if the expression is likely to be a string or table
        if (auto literal = dynamic_cast<const LiteralExpr*>(expr)) {
            // Check if it's a string literal
            return literal->getValue().isString();
        }
        
        if (auto table = dynamic_cast<const TableExpr*>(expr)) {
            return true;
        }
        
        if (auto variable = dynamic_cast<const VariableExpr*>(expr)) {
            // Variables could potentially be strings or tables
            return true;
        }
        
        // For other expressions (function calls, etc.), assume they might be valid
        return true;
    }

    bool Parser::isAtEnd() const {
        return current.type == TokenType::Eof;
    }

    Vec<UPtr<Stmt>> Parser::parse() {
        Vec<UPtr<Stmt>> statements;
        Token lastToken = current;
        int stuckCount = 0;
        const int maxStuckIterations = 3;

        while (!isAtEnd()) {
            auto stmt = statement();
            if (stmt) {
                statements.push_back(std::move(stmt));
                stuckCount = 0; // Reset stuck counter on successful parsing
            }
            
            // Check if we're stuck in an infinite loop
            if (current.type == lastToken.type && 
                current.line == lastToken.line && 
                current.column == lastToken.column) {
                stuckCount++;
                if (stuckCount >= maxStuckIterations) {
                    error("Parser stuck in infinite loop, forcing advance.");
                    advance(); // Force advance to break the loop
                    stuckCount = 0;
                }
            } else {
                stuckCount = 0;
            }
            
            lastToken = current;
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