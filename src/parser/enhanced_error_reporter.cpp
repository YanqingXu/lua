#include "enhanced_error_reporter.hpp"
#include <sstream>

namespace Lua {
    EnhancedErrorReporter::EnhancedErrorReporter(const Str& sourceCode,
                                               bool lua51Compatible,
                                               bool showSourceContext)
        : sourceCode_(sourceCode)
        , lua51Compatible_(lua51Compatible)
        , showSourceContext_(showSourceContext) {

        // Configure for Lua 5.1 behavior if enabled
        if (lua51Compatible) {
            ErrorReporterConfig config;
            config.maxErrors = 1;
            config.stopOnFirstError = true;
            baseReporter_.setConfig(config);
        }
    }
    
    void EnhancedErrorReporter::reportSyntaxError(const SourceLocation& location,
                                                const Str& nearToken,
                                                const Str& expectedToken) {
        if (lua51Compatible_) {
            Str message = Lua51ErrorFormatter::formatSyntaxError(location, "", nearToken);
            auto error = ParseError(ErrorType::InvalidExpression, location, message);
            addError(std::move(error));
        } else {
            baseReporter_.reportSyntaxError(location, "Syntax error near '" + nearToken + "'");
        }
    }
    
    void EnhancedErrorReporter::reportUnexpectedToken(const SourceLocation& location,
                                                    const Str& actualToken,
                                                    const Str& expectedToken) {
        if (lua51Compatible_) {
            // Use formatUnexpectedToken to ensure proper token formatting with quotes
            Str message = Lua51ErrorFormatter::formatUnexpectedToken(location, actualToken, expectedToken);
            auto error = ParseError(ErrorType::UnexpectedToken, location, message);
            addError(std::move(error));
        } else {
            baseReporter_.reportUnexpectedToken(location, expectedToken, actualToken);
        }
    }
    
    void EnhancedErrorReporter::reportMissingToken(const SourceLocation& location,
                                                 const Str& expectedToken) {
        if (lua51Compatible_) {
            Str message = Lua51ErrorFormatter::formatMissingToken(location, expectedToken);
            auto error = ParseError(ErrorType::MissingToken, location, message);
            addError(std::move(error));
        } else {
            baseReporter_.reportMissingToken(location, expectedToken);
        }
    }
    
    void EnhancedErrorReporter::reportUnfinishedString(const SourceLocation& location,
                                                     const Str& stringStart) {
        // For Lua 5.1 compatibility, don't truncate the string token
        Str tokenToShow = stringStart;

        if (lua51Compatible_) {
            Str message = createLua51Message(location,
                                           Lua51ErrorMessages::UNFINISHED_STRING,
                                           {tokenToShow});
            auto error = ParseError(ErrorType::UnterminatedString, location, message);
            addError(std::move(error));
        } else {
            Str truncatedStart = stringStart.length() > 10 ?
                               stringStart.substr(0, 10) + "..." : stringStart;
            baseReporter_.reportError(ErrorType::UnterminatedString, location,
                                    "Unfinished string starting with: " + truncatedStart);
        }
    }
    
    void EnhancedErrorReporter::reportMalformedNumber(const SourceLocation& location,
                                                    const Str& numberText) {
        Str truncatedNumber = numberText.length() > 15 ? 
                            numberText.substr(0, 15) + "..." : numberText;
        
        if (lua51Compatible_) {
            Str message = createLua51Message(location, 
                                           Lua51ErrorMessages::MALFORMED_NUMBER, 
                                           {truncatedNumber});
            auto error = ParseError(ErrorType::InvalidNumber, location, message);
            addError(std::move(error));
        } else {
            baseReporter_.reportError(ErrorType::InvalidNumber, location, 
                                    "Malformed number: " + truncatedNumber);
        }
    }
    
    void EnhancedErrorReporter::reportUnexpectedEOF(const SourceLocation& location,
                                                  const Str& expectedToken) {
        if (lua51Compatible_) {
            Str message;
            if (!expectedToken.empty()) {
                message = Lua51ErrorFormatter::formatLocation(location) + " " + 
                         expectedToken + " expected (to close at line " + 
                         std::to_string(location.getLine()) + ")";
            } else {
                message = Lua51ErrorFormatter::formatLocation(location) + " " + 
                         Lua51ErrorMessages::UNEXPECTED_EOF;
            }
            auto error = ParseError(ErrorType::UnexpectedToken, location, message);
            addError(std::move(error));
        } else {
            baseReporter_.reportError(ErrorType::UnexpectedToken, location, 
                                    "Unexpected end of file");
        }
    }
    
    void EnhancedErrorReporter::reportAmbiguousSyntax(const SourceLocation& location,
                                                    const Str& description) {
        Str message;
        if (lua51Compatible_) {
            message = Lua51ErrorFormatter::formatLocation(location) + " " + 
                     (description.empty() ? Lua51ErrorMessages::AMBIGUOUS_SYNTAX : description);
        } else {
            message = "Ambiguous syntax" + (description.empty() ? "" : ": " + description);
        }
        
        auto error = ParseError(ErrorType::InvalidExpression, location, message);
        addError(std::move(error));
    }
    
    void EnhancedErrorReporter::reportFunctionSpan(const SourceLocation& startLocation,
                                                 const SourceLocation& endLocation) {
        if (lua51Compatible_) {
            Str message = createLua51Message(startLocation, 
                                           Lua51ErrorMessages::FUNCTION_AT_LINE_ENDS_ON_LINE,
                                           {std::to_string(startLocation.getLine()),
                                            std::to_string(endLocation.getLine())});
            auto error = ParseError(ErrorType::InvalidStatement, startLocation, message);
            addError(std::move(error));
        } else {
            baseReporter_.reportInfo(startLocation, 
                                   "Function defined at line " + std::to_string(startLocation.getLine()) + 
                                   " ends at line " + std::to_string(endLocation.getLine()));
        }
    }
    
    Str EnhancedErrorReporter::getFormattedOutput() const {
        const auto& errors = baseReporter_.getErrors();
        if (errors.empty()) {
            return "";
        }

        if (lua51Compatible_) {
            // For Lua 5.1 compatibility, only return the first error message
            // The message is already formatted in Lua 5.1 style by the report methods
            return errors[0].getMessage();
        } else {
            return baseReporter_.toString();
        }
    }
    
    Vec<Str> EnhancedErrorReporter::getFormattedErrors() const {
        Vec<Str> formattedErrors;
        const auto& errors = baseReporter_.getErrors();
        
        for (const auto& error : errors) {
            if (lua51Compatible_) {
                formattedErrors.push_back(Lua51ErrorFormatter::formatError(error, sourceCode_));
            } else {
                formattedErrors.push_back(error.toString());
            }
        }
        
        return formattedErrors;
    }
    
    ParseError EnhancedErrorReporter::createTokenError(const SourceLocation& location,
                                                     ErrorType errorType,
                                                     const Token& token,
                                                     const Str& additionalInfo) {
        Str message;
        
        if (lua51Compatible_) {
            switch (errorType) {
                case ErrorType::UnexpectedToken:
                    message = Lua51ErrorFormatter::formatUnexpectedToken(location, token.lexeme);
                    break;
                case ErrorType::UnterminatedString:
                    message = createLua51Message(location, Lua51ErrorMessages::UNFINISHED_STRING, {token.lexeme});
                    break;
                case ErrorType::InvalidNumber:
                    message = createLua51Message(location, Lua51ErrorMessages::MALFORMED_NUMBER, {token.lexeme});
                    break;
                default:
                    message = Lua51ErrorFormatter::formatSyntaxError(location, additionalInfo, token.lexeme);
                    break;
            }
        } else {
            message = "Error with token '" + token.lexeme + "'";
            if (!additionalInfo.empty()) {
                message += ": " + additionalInfo;
            }
        }
        
        return ParseError(errorType, location, message);
    }
    
    Str EnhancedErrorReporter::formatForConsole(const ParseError& error) const {
        if (lua51Compatible_) {
            Str formatted = Lua51ErrorFormatter::formatError(error, sourceCode_);
            if (showSourceContext_ && !sourceCode_.empty()) {
                Str context = Lua51ErrorFormatter::getSourceContext(sourceCode_, error.getLocation(), 1);
                if (!context.empty()) {
                    formatted += "\n" + context;
                }
            }
            return formatted;
        } else {
            return error.toDetailedString();
        }
    }
    
    Str EnhancedErrorReporter::formatForIDE(const ParseError& error) const {
        std::ostringstream oss;
        oss << "{"
            << "\"type\":\"" << static_cast<int>(error.getType()) << "\","
            << "\"severity\":\"" << static_cast<int>(error.getSeverity()) << "\","
            << "\"location\":{"
            << "\"file\":\"" << error.getLocation().getFilename() << "\","
            << "\"line\":" << error.getLocation().getLine() << ","
            << "\"column\":" << error.getLocation().getColumn()
            << "},"
            << "\"message\":\"" << error.getMessage() << "\""
            << "}";
        return oss.str();
    }
    
    void EnhancedErrorReporter::addError(ParseError&& error) {
        baseReporter_.addError(std::move(error));
    }
    
    Str EnhancedErrorReporter::createLua51Message(const SourceLocation& location,
                                                const Str& messageTemplate,
                                                const Vec<Str>& args) const {
        Str locationStr = Lua51ErrorFormatter::formatLocation(location);
        Str message = Lua51ErrorMessages::getMessage(messageTemplate, args);
        return locationStr + " " + message;
    }
    
    // Factory implementations
    EnhancedErrorReporter ErrorReporterFactory::createLua51Reporter(const Str& sourceCode) {
        EnhancedErrorReporter reporter(sourceCode, true, false);
        // Configure for Lua 5.1 behavior: only show first error
        ErrorReporterConfig config;
        config.maxErrors = 1;
        config.stopOnFirstError = true;
        // Note: We need to modify the constructor to accept config
        return reporter;
    }
    
    EnhancedErrorReporter ErrorReporterFactory::createDevelopmentReporter(const Str& sourceCode) {
        return EnhancedErrorReporter(sourceCode, false, true);
    }
    
    EnhancedErrorReporter ErrorReporterFactory::createProductionReporter(const Str& sourceCode) {
        return EnhancedErrorReporter(sourceCode, true, false);
    }
}
