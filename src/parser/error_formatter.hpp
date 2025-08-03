#pragma once

#include "ast/parse_error.hpp"
#include "ast/source_location.hpp"
#include "../common/types.hpp"
#include <vector>
#include <string>

namespace Lua {
    /**
     * @brief Lua 5.1 Compatible Error Formatter
     * 
     * This class formats error messages to match Lua 5.1 official error format exactly.
     * It ensures compatibility with standard Lua error reporting conventions.
     */
    class Lua51ErrorFormatter {
    public:
        /**
         * @brief Format error message in Lua 5.1 style
         * @param error The parse error to format
         * @param sourceCode Optional source code for context display
         * @return Formatted error message string
         */
        static Str formatError(const ParseError& error, const Str& sourceCode = "");
        
        /**
         * @brief Format multiple errors in Lua 5.1 style
         * @param errors Vector of parse errors
         * @param sourceCode Optional source code for context display
         * @return Formatted error messages string
         */
        static Str formatErrors(const Vec<ParseError>& errors, const Str& sourceCode = "");
        
        /**
         * @brief Format syntax error in Lua 5.1 style
         * @param location Source location of the error
         * @param message Error message
         * @param nearToken Token near which the error occurred
         * @return Formatted syntax error string
         */
        static Str formatSyntaxError(const SourceLocation& location, 
                                   const Str& message, 
                                   const Str& nearToken = "");
        
        /**
         * @brief Format unexpected token error in Lua 5.1 style
         * @param location Source location of the error
         * @param actualToken The actual token found
         * @param expectedToken The expected token (optional)
         * @return Formatted unexpected token error string
         */
        static Str formatUnexpectedToken(const SourceLocation& location,
                                       const Str& actualToken,
                                       const Str& expectedToken = "");
        
        /**
         * @brief Format missing token error in Lua 5.1 style
         * @param location Source location of the error
         * @param expectedToken The expected token
         * @return Formatted missing token error string
         */
        static Str formatMissingToken(const SourceLocation& location,
                                    const Str& expectedToken);
        
        /**
         * @brief Get source code context around error location
         * @param sourceCode The complete source code
         * @param location Error location
         * @param contextLines Number of context lines to show (default: 2)
         * @return Source code context string
         */
        static Str getSourceContext(const Str& sourceCode, 
                                  const SourceLocation& location,
                                  int contextLines = 2);
        
        /**
         * @brief Format location in Lua 5.1 style (filename:line:)
         * @param location Source location
         * @return Formatted location string
         */
        static Str formatLocation(const SourceLocation& location);
        
        /**
         * @brief Convert error type to Lua 5.1 compatible message
         * @param errorType The error type
         * @return Lua 5.1 compatible error message
         */
        static Str errorTypeToLua51Message(ErrorType errorType);
        
        /**
         * @brief Check if token needs quotes in error message
         * @param token The token to check
         * @return True if token should be quoted
         */
        static bool shouldQuoteToken(const Str& token);
        
        /**
         * @brief Format token for display in error message
         * @param token The token to format
         * @return Formatted token string
         */
        static Str formatToken(const Str& token);
        
    private:
        /**
         * @brief Extract line from source code
         * @param sourceCode The complete source code
         * @param lineNumber Line number (1-based)
         * @return The specified line content
         */
        static Str extractLine(const Str& sourceCode, int lineNumber);
        
        /**
         * @brief Create error pointer line (^^^^ pointing to error location)
         * @param column Column position (1-based)
         * @param length Length of error span
         * @return Error pointer string
         */
        static Str createErrorPointer(int column, int length = 1);
        
        /**
         * @brief Truncate long tokens for display
         * @param token The token to truncate
         * @param maxLength Maximum length (default: 20)
         * @return Truncated token string
         */
        static Str truncateToken(const Str& token, size_t maxLength = 20);

        /**
         * @brief Format error message based on error type
         * @param error The parse error to format
         * @return Formatted error message
         */
        static Str formatErrorByType(const ParseError& error);

        /**
         * @brief Extract token from error message
         * @param message The error message
         * @return Extracted token
         */
        static Str extractTokenFromMessage(const Str& message);
    };
    
    /**
     * @brief Lua 5.1 Error Message Templates
     * 
     * Contains standard Lua 5.1 error message templates for consistency
     */
    class Lua51ErrorMessages {
    public:
        // Standard Lua 5.1 error message templates
        static const Str UNEXPECTED_SYMBOL_NEAR;
        static const Str SYNTAX_ERROR_NEAR;
        static const Str UNEXPECTED_EOF;
        static const Str MALFORMED_NUMBER;
        static const Str UNFINISHED_STRING;
        static const Str INVALID_ESCAPE_SEQUENCE;
        static const Str CHUNK_HAS_TOO_MANY_SYNTAX_LEVELS;
        static const Str FUNCTION_AT_LINE_ENDS_ON_LINE;
        static const Str AMBIGUOUS_SYNTAX;
        
        /**
         * @brief Get localized Lua 5.1 error message
         * @param messageKey Message key
         * @param args Optional arguments for formatting
         * @return Localized error message
         */
        static Str getMessage(const Str& messageKey, const Vec<Str>& args = {});
    };
}
