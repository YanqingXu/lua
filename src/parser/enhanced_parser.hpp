#pragma once

#include "parser.hpp"
#include "enhanced_error_reporter.hpp"
#include "../common/types.hpp"

namespace Lua {
    /**
     * @brief Enhanced Parser with Lua 5.1 Compatible Error Reporting
     * 
     * This class extends the basic Parser with enhanced error reporting
     * capabilities that match Lua 5.1 official error format.
     */
    class EnhancedParser : public Parser {
    private:
        EnhancedErrorReporter enhancedReporter_;
        bool lua51ErrorFormat_;
        
    public:
        /**
         * @brief Constructor
         * @param source Source code to parse
         * @param lua51ErrorFormat Enable Lua 5.1 compatible error format
         */
        explicit EnhancedParser(const Str& source, bool lua51ErrorFormat = true);
        
        /**
         * @brief Parse with enhanced error reporting
         * @return Vector of parsed statements
         */
        Vec<UPtr<Stmt>> parseWithEnhancedErrors();
        
        /**
         * @brief Parse expression with enhanced error reporting
         * @return Parsed expression
         */
        UPtr<Expr> parseExpressionWithEnhancedErrors();
        
        /**
         * @brief Get enhanced error reporter
         * @return Reference to enhanced error reporter
         */
        const EnhancedErrorReporter& getEnhancedReporter() const { return enhancedReporter_; }
        
        /**
         * @brief Get formatted error output
         * @return Lua 5.1 compatible error output
         */
        Str getFormattedErrors() const;
        
        /**
         * @brief Enable/disable Lua 5.1 error format
         * @param enabled True to enable Lua 5.1 format
         */
        void setLua51ErrorFormat(bool enabled);
        
        /**
         * @brief Enable/disable source context in errors
         * @param enabled True to show source context
         */
        void setShowSourceContext(bool enabled);
        
    protected:
        /**
         * @brief Override error reporting to use enhanced reporter
         * @param message Error message
         */
        void error(const Str& message) override;
        
        /**
         * @brief Override error reporting with type
         * @param type Error type
         * @param message Error message
         */
        void error(ErrorType type, const Str& message) override;
        
        /**
         * @brief Override error reporting with location
         * @param type Error type
         * @param location Error location
         * @param message Error message
         */
        void error(ErrorType type, const SourceLocation& location, const Str& message) override;
        
        /**
         * @brief Report unexpected token with enhanced formatting
         * @param expected Expected token
         * @param actual Actual token
         */
        void reportUnexpectedToken(const Str& expected, const Token& actual);
        
        /**
         * @brief Report missing token with enhanced formatting
         * @param expected Expected token
         * @param location Error location
         */
        void reportMissingToken(const Str& expected, const SourceLocation& location);
        
        /**
         * @brief Report syntax error with enhanced formatting
         * @param nearToken Token near error
         * @param location Error location
         */
        void reportSyntaxError(const Token& nearToken, const SourceLocation& location);
        
        /**
         * @brief Report unfinished string with enhanced formatting
         * @param stringStart Beginning of unfinished string
         * @param location Error location
         */
        void reportUnfinishedString(const Str& stringStart, const SourceLocation& location);
        
        /**
         * @brief Report malformed number with enhanced formatting
         * @param numberText Malformed number text
         * @param location Error location
         */
        void reportMalformedNumber(const Str& numberText, const SourceLocation& location);
        
        /**
         * @brief Report unexpected EOF with enhanced formatting
         * @param expected What was expected
         * @param location Error location
         */
        void reportUnexpectedEOF(const Str& expected, const SourceLocation& location);
    };
    
    /**
     * @brief Parser Factory for different error reporting modes
     */
    class ParserFactory {
    public:
        /**
         * @brief Create Lua 5.1 compatible parser
         * @param source Source code
         * @return Enhanced parser with Lua 5.1 error format
         */
        static EnhancedParser createLua51Parser(const Str& source);
        
        /**
         * @brief Create development parser with detailed errors
         * @param source Source code
         * @return Enhanced parser with detailed error information
         */
        static EnhancedParser createDevelopmentParser(const Str& source);
        
        /**
         * @brief Create production parser with minimal errors
         * @param source Source code
         * @return Enhanced parser with minimal error information
         */
        static EnhancedParser createProductionParser(const Str& source);
    };
    
    /**
     * @brief Error Comparison Utility
     * 
     * Utility for comparing error output with Lua 5.1 reference
     */
    class ErrorComparisonUtil {
    public:
        /**
         * @brief Compare error output with Lua 5.1 reference
         * @param ourOutput Our parser's error output
         * @param lua51Reference Lua 5.1 reference output
         * @return Similarity score (0.0 to 1.0)
         */
        static double compareWithLua51(const Str& ourOutput, const Str& lua51Reference);
        
        /**
         * @brief Extract error components for comparison
         * @param errorMessage Error message to analyze
         * @return Structured error components
         */
        struct ErrorComponents {
            Str filename;
            int line;
            int column;
            Str errorType;
            Str message;
            Str nearToken;
        };
        
        static ErrorComponents extractComponents(const Str& errorMessage);
        
        /**
         * @brief Generate test cases for error format validation
         * @return Vector of test cases with expected Lua 5.1 output
         */
        static Vec<std::pair<Str, Str>> generateTestCases();
    };
}
