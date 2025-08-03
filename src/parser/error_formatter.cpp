#include "error_formatter.hpp"
#include "../localization/localization_manager.hpp"
#include <sstream>
#include <algorithm>

namespace Lua {
    // Lua 5.1 standard error message templates
    const Str Lua51ErrorMessages::UNEXPECTED_SYMBOL_NEAR = "unexpected symbol near '{0}'";
    const Str Lua51ErrorMessages::SYNTAX_ERROR_NEAR = "syntax error near '{0}'";
    const Str Lua51ErrorMessages::UNEXPECTED_EOF = "unexpected end of file";
    const Str Lua51ErrorMessages::MALFORMED_NUMBER = "malformed number near '{0}'";
    const Str Lua51ErrorMessages::UNFINISHED_STRING = "unfinished string near '{0}'";
    const Str Lua51ErrorMessages::INVALID_ESCAPE_SEQUENCE = "invalid escape sequence near '{0}'";
    const Str Lua51ErrorMessages::CHUNK_HAS_TOO_MANY_SYNTAX_LEVELS = "chunk has too many syntax levels";
    const Str Lua51ErrorMessages::FUNCTION_AT_LINE_ENDS_ON_LINE = "function at line {0} ends on line {1}";
    const Str Lua51ErrorMessages::AMBIGUOUS_SYNTAX = "ambiguous syntax (function call x new statement)";
    
    Str Lua51ErrorFormatter::formatError(const ParseError& error, const Str& sourceCode) {
        std::ostringstream oss;
        
        // Format location in Lua 5.1 style: "filename:line:"
        oss << formatLocation(error.getLocation());
        
        // Format the main error message based on error type
        Str mainMessage = formatErrorByType(error);
        oss << " " << mainMessage;
        
        // Add source context if available
        if (!sourceCode.empty()) {
            Str context = getSourceContext(sourceCode, error.getLocation(), 1);
            if (!context.empty()) {
                oss << "\n" << context;
            }
        }
        
        return oss.str();
    }
    
    Str Lua51ErrorFormatter::formatErrors(const Vec<ParseError>& errors, const Str& sourceCode) {
        if (errors.empty()) {
            return "";
        }
        
        // For Lua 5.1 compatibility, typically only show the first error
        return formatError(errors[0], sourceCode);
    }
    
    Str Lua51ErrorFormatter::formatSyntaxError(const SourceLocation& location, 
                                             const Str& message, 
                                             const Str& nearToken) {
        std::ostringstream oss;
        oss << formatLocation(location);
        
        if (!nearToken.empty()) {
            // Use Lua 5.1 standard format: "syntax error near 'token'"
            oss << " syntax error near " << formatToken(nearToken);
        } else {
            oss << " " << message;
        }
        
        return oss.str();
    }
    
    Str Lua51ErrorFormatter::formatUnexpectedToken(const SourceLocation& location,
                                                  const Str& actualToken,
                                                  const Str& expectedToken) {
        std::ostringstream oss;
        oss << formatLocation(location);
        
        if (actualToken == "<eof>") {
            oss << " unexpected end of file";
        } else {
            oss << " unexpected symbol near " << formatToken(actualToken);
        }
        
        return oss.str();
    }
    
    Str Lua51ErrorFormatter::formatMissingToken(const SourceLocation& location,
                                              const Str& expectedToken) {
        std::ostringstream oss;
        oss << formatLocation(location);
        
        if (expectedToken == "end") {
            oss << " 'end' expected";
        } else if (expectedToken == ")") {
            oss << " ')' expected";
        } else if (expectedToken == "}") {
            oss << " '}' expected";
        } else if (expectedToken == "]") {
            oss << " ']' expected";
        } else {
            oss << " " << formatToken(expectedToken) << " expected";
        }
        
        return oss.str();
    }
    
    Str Lua51ErrorFormatter::getSourceContext(const Str& sourceCode, 
                                             const SourceLocation& location,
                                             int contextLines) {
        if (sourceCode.empty() || location.getLine() <= 0) {
            return "";
        }
        
        std::ostringstream oss;
        int targetLine = location.getLine();
        int startLine = std::max(1, targetLine - contextLines);
        int endLine = targetLine + contextLines;
        
        // Split source code into lines
        std::vector<Str> lines;
        std::istringstream iss(sourceCode);
        Str line;
        while (std::getline(iss, line)) {
            lines.push_back(line);
        }
        
        // Show context lines
        for (int i = startLine; i <= endLine && i <= static_cast<int>(lines.size()); ++i) {
            if (i == targetLine) {
                // Highlight the error line
                oss << ">>> " << lines[i-1] << "\n";
                // Add error pointer
                if (location.getColumn() > 0) {
                    oss << "    " << createErrorPointer(location.getColumn()) << "\n";
                }
            } else {
                oss << "    " << lines[i-1] << "\n";
            }
        }
        
        return oss.str();
    }
    
    Str Lua51ErrorFormatter::formatLocation(const SourceLocation& location) {
        return location.toLua51String();
    }
    
    Str Lua51ErrorFormatter::errorTypeToLua51Message(ErrorType errorType) {
        switch (errorType) {
            case ErrorType::UnexpectedCharacter:
                return "unexpected symbol";
            case ErrorType::UnterminatedString:
                return "unfinished string";
            case ErrorType::InvalidNumber:
                return "malformed number";
            case ErrorType::UnexpectedToken:
                return "unexpected symbol";
            case ErrorType::MissingToken:
                return "expected";
            case ErrorType::InvalidExpression:
                return "syntax error";
            case ErrorType::InvalidStatement:
                return "syntax error";
            case ErrorType::MismatchedParentheses:
            case ErrorType::MismatchedBraces:
            case ErrorType::MismatchedBrackets:
                return "syntax error";
            default:
                return "syntax error";
        }
    }
    
    bool Lua51ErrorFormatter::shouldQuoteToken(const Str& token) {
        if (token.empty()) return false;
        if (token == "<eof>") return false;
        if (token.length() == 1 && !std::isalnum(token[0])) return true;
        return true;
    }
    
    Str Lua51ErrorFormatter::formatToken(const Str& token) {
        if (token.empty()) return "''";
        if (token == "<eof>") return "<eof>";

        // Remove any trailing whitespace or newlines, but preserve string content
        Str cleanToken = token;

        // For Lua 5.1 compatibility, don't truncate tokens for better error messages
        Str truncated = cleanToken;

        if (shouldQuoteToken(truncated)) {
            return "'" + truncated + "'";
        }
        return truncated;
    }
    
    Str Lua51ErrorFormatter::extractLine(const Str& sourceCode, int lineNumber) {
        if (lineNumber <= 0) return "";
        
        std::istringstream iss(sourceCode);
        Str line;
        int currentLine = 1;
        
        while (std::getline(iss, line) && currentLine <= lineNumber) {
            if (currentLine == lineNumber) {
                return line;
            }
            currentLine++;
        }
        
        return "";
    }
    
    Str Lua51ErrorFormatter::createErrorPointer(int column, int length) {
        if (column <= 0) return "";
        
        std::ostringstream oss;
        for (int i = 1; i < column; ++i) {
            oss << " ";
        }
        for (int i = 0; i < length; ++i) {
            oss << "^";
        }
        return oss.str();
    }
    
    Str Lua51ErrorFormatter::truncateToken(const Str& token, size_t maxLength) {
        if (token.length() <= maxLength) {
            return token;
        }
        return token.substr(0, maxLength - 3) + "...";
    }
    
    Str Lua51ErrorFormatter::formatErrorByType(const ParseError& error) {
        switch (error.getType()) {
            case ErrorType::UnexpectedToken:
                return "unexpected symbol near " + formatToken(extractTokenFromMessage(error.getMessage()));
            case ErrorType::UnterminatedString:
                return "unfinished string near " + formatToken(extractTokenFromMessage(error.getMessage()));
            case ErrorType::InvalidNumber:
                return "malformed number near " + formatToken(extractTokenFromMessage(error.getMessage()));
            case ErrorType::MissingToken:
                return extractTokenFromMessage(error.getMessage()) + " expected";
            case ErrorType::UnexpectedCharacter:
                return "unexpected symbol near " + formatToken(extractTokenFromMessage(error.getMessage()));
            default:
                return "syntax error";
        }
    }

    Str Lua51ErrorFormatter::extractTokenFromMessage(const Str& message) {
        // Extract token from error message (simple implementation)
        // Look for patterns like "Expected 'token'" or "Found 'token'"
        size_t start = message.find("'");
        if (start != Str::npos) {
            size_t end = message.find("'", start + 1);
            if (end != Str::npos) {
                return message.substr(start + 1, end - start - 1);
            }
        }
        return "";
    }

    Str Lua51ErrorMessages::getMessage(const Str& messageKey, const Vec<Str>& args) {
        // Use localization manager for consistent message formatting
        return getLocalizedMessage(MessageCategory::ErrorMessage, messageKey, args);
    }
}
