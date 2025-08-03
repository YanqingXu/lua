#pragma once

#include "ast/parse_error.hpp"
#include "error_formatter.hpp"
#include "../lexer/lexer.hpp"
#include "../common/types.hpp"

namespace Lua {
    /**
     * @brief Enhanced Error Reporter for Lua 5.1 Compatibility
     * 
     * This class extends the basic ErrorReporter with Lua 5.1 specific
     * error formatting and reporting capabilities.
     */
    class EnhancedErrorReporter {
    private:
        ErrorReporter baseReporter_;
        Str sourceCode_;
        bool lua51Compatible_;
        bool showSourceContext_;
        
    public:
        /**
         * @brief Constructor
         * @param sourceCode Source code for context display
         * @param lua51Compatible Enable Lua 5.1 compatible formatting
         * @param showSourceContext Show source code context in errors
         */
        explicit EnhancedErrorReporter(const Str& sourceCode = "",
                                     bool lua51Compatible = true,
                                     bool showSourceContext = false);
        
        /**
         * @brief Report syntax error with Lua 5.1 formatting
         * @param location Error location
         * @param nearToken Token near which error occurred
         * @param expectedToken Expected token (optional)
         */
        void reportSyntaxError(const SourceLocation& location,
                             const Str& nearToken,
                             const Str& expectedToken = "");
        
        /**
         * @brief Report unexpected token error
         * @param location Error location
         * @param actualToken Actual token found
         * @param expectedToken Expected token (optional)
         */
        void reportUnexpectedToken(const SourceLocation& location,
                                 const Str& actualToken,
                                 const Str& expectedToken = "");
        
        /**
         * @brief Report missing token error
         * @param location Error location
         * @param expectedToken Expected token
         */
        void reportMissingToken(const SourceLocation& location,
                              const Str& expectedToken);
        
        /**
         * @brief Report unfinished string error
         * @param location Error location
         * @param stringStart Beginning of the unfinished string
         */
        void reportUnfinishedString(const SourceLocation& location,
                                  const Str& stringStart);
        
        /**
         * @brief Report malformed number error
         * @param location Error location
         * @param numberText The malformed number text
         */
        void reportMalformedNumber(const SourceLocation& location,
                                 const Str& numberText);
        
        /**
         * @brief Report unexpected end of file
         * @param location Error location
         * @param expectedToken What was expected before EOF
         */
        void reportUnexpectedEOF(const SourceLocation& location,
                               const Str& expectedToken = "");
        
        /**
         * @brief Report ambiguous syntax error
         * @param location Error location
         * @param description Description of the ambiguity
         */
        void reportAmbiguousSyntax(const SourceLocation& location,
                                 const Str& description = "");
        
        /**
         * @brief Report function definition span error
         * @param startLocation Function start location
         * @param endLocation Function end location
         */
        void reportFunctionSpan(const SourceLocation& startLocation,
                              const SourceLocation& endLocation);
        
        /**
         * @brief Get formatted error output
         * @return Formatted error string in Lua 5.1 style
         */
        Str getFormattedOutput() const;
        
        /**
         * @brief Get all errors in Lua 5.1 format
         * @return Vector of formatted error strings
         */
        Vec<Str> getFormattedErrors() const;
        
        /**
         * @brief Check if there are any errors
         * @return True if errors exist
         */
        bool hasErrors() const { return baseReporter_.hasErrors(); }
        
        /**
         * @brief Get error count
         * @return Number of errors
         */
        size_t getErrorCount() const { return baseReporter_.getErrorCount(); }
        
        /**
         * @brief Clear all errors
         */
        void clear() { baseReporter_.clear(); }
        
        /**
         * @brief Set source code for context display
         * @param sourceCode The source code
         */
        void setSourceCode(const Str& sourceCode) { sourceCode_ = sourceCode; }
        
        /**
         * @brief Enable/disable Lua 5.1 compatible formatting
         * @param enabled True to enable Lua 5.1 compatibility
         */
        void setLua51Compatible(bool enabled) { lua51Compatible_ = enabled; }
        
        /**
         * @brief Enable/disable source context display
         * @param enabled True to show source context
         */
        void setShowSourceContext(bool enabled) { showSourceContext_ = enabled; }
        
        /**
         * @brief Get underlying ErrorReporter
         * @return Reference to the base ErrorReporter
         */
        const ErrorReporter& getBaseReporter() const { return baseReporter_; }
        
        /**
         * @brief Create error from token information
         * @param location Error location
         * @param errorType Type of error
         * @param token Token information
         * @param additionalInfo Additional error information
         * @return Created ParseError
         */
        ParseError createTokenError(const SourceLocation& location,
                                  ErrorType errorType,
                                  const Token& token,
                                  const Str& additionalInfo = "");
        
        /**
         * @brief Format error for console output
         * @param error The error to format
         * @return Console-formatted error string
         */
        Str formatForConsole(const ParseError& error) const;
        
        /**
         * @brief Format error for IDE integration
         * @param error The error to format
         * @return IDE-formatted error string (with JSON-like structure)
         */
        Str formatForIDE(const ParseError& error) const;
        
    private:
        /**
         * @brief Add error to base reporter
         * @param error The error to add
         */
        void addError(ParseError&& error);
        
        /**
         * @brief Create standard Lua 5.1 error message
         * @param location Error location
         * @param messageTemplate Message template
         * @param args Template arguments
         * @return Formatted error message
         */
        Str createLua51Message(const SourceLocation& location,
                             const Str& messageTemplate,
                             const Vec<Str>& args = {}) const;
    };
    
    /**
     * @brief Error Reporter Factory
     * 
     * Factory class for creating different types of error reporters
     */
    class ErrorReporterFactory {
    public:
        /**
         * @brief Create Lua 5.1 compatible error reporter
         * @param sourceCode Source code for context
         * @return Enhanced error reporter configured for Lua 5.1
         */
        static EnhancedErrorReporter createLua51Reporter(const Str& sourceCode = "");
        
        /**
         * @brief Create development error reporter (with detailed info)
         * @param sourceCode Source code for context
         * @return Enhanced error reporter configured for development
         */
        static EnhancedErrorReporter createDevelopmentReporter(const Str& sourceCode = "");
        
        /**
         * @brief Create production error reporter (minimal info)
         * @param sourceCode Source code for context
         * @return Enhanced error reporter configured for production
         */
        static EnhancedErrorReporter createProductionReporter(const Str& sourceCode = "");
    };
}
