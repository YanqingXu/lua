#pragma once

#include "source_location.hpp"
#include "../../common/types.hpp"
#include "../../localization/localization_manager.hpp"
#include <vector>
#include <string>
#include <memory>

namespace Lua {
    // Error type enumeration
    enum class ErrorType {
        // Lexical errors
        UnexpectedCharacter,    // Unexpected character
        UnterminatedString,     // Unterminated string
        InvalidNumber,          // Invalid number format
        
        // Syntax errors
        UnexpectedToken,        // Unexpected token
        MissingToken,          // Missing expected token
        InvalidExpression,      // Invalid expression
        InvalidStatement,       // Invalid statement
        
        // Structural errors
        MismatchedParentheses, // Mismatched parentheses
        MismatchedBraces,      // Mismatched braces
        MismatchedBrackets,    // Mismatched brackets
        
        // Semantic errors
        UndefinedVariable,     // Undefined variable
        RedefinedVariable,     // Redefined variable
        InvalidAssignment,     // Invalid assignment
        
        // Function-related errors
        InvalidFunctionCall,   // Invalid function call
        WrongArgumentCount,    // Wrong argument count
        InvalidReturn,         // Invalid return statement
        
        // Control flow errors
        InvalidBreak,          // Invalid break statement
        InvalidContinue,       // Invalid continue statement
        
        // Other errors
        InternalError,         // Internal error
        Unknown                // Unknown error
    };
    
    // Fix suggestion type
    enum class FixType {
        Insert,     // Insert
        Delete,     // Delete
        Replace,    // Replace
        Move        // Move
    };
    
    // Fix suggestion structure
    struct FixSuggestion {
        FixType type;              // Fix type
        SourceLocation location;   // Fix location
        Str description;          // Fix description
        Str newText;              // New text (for insert/replace)
        
        FixSuggestion(FixType t, const SourceLocation& loc, const Str& desc, const Str& text = "")
            : type(t), location(loc), description(desc), newText(text) {}
    };
    
    // Error severity
    enum class ErrorSeverity {
        Info,       // Information
        Warning,    // Warning
        Error,      // Error
        Fatal       // Fatal error
    };
    
    // Parse error class
    class ParseError {
    private:
        ErrorType type_;                           // Error type
        ErrorSeverity severity_;                   // Error severity
        SourceLocation location_;                  // Error location
        Str message_;                             // Error message
        Str details_;                             // Detailed information
        Vec<FixSuggestion> suggestions_;          // Fix suggestion list
        UPtr<ParseError> relatedError_;           // Related error (for error chaining)
        
    public:
        // Constructor
        ParseError(ErrorType type, const SourceLocation& location, const Str& message,
                  ErrorSeverity severity = ErrorSeverity::Error)
            : type_(type), severity_(severity), location_(location), message_(message) {}
        
        // Copy constructor (deleted due to unique_ptr)
        ParseError(const ParseError&) = delete;
        ParseError& operator=(const ParseError&) = delete;
        
        // Move constructor and move assignment operator
        ParseError(ParseError&&) = default;
        ParseError& operator=(ParseError&&) = default;
        
        // Constructor with detailed information
        ParseError(ErrorType type, const SourceLocation& location, const Str& message,
                  const Str& details, ErrorSeverity severity = ErrorSeverity::Error)
            : type_(type), severity_(severity), location_(location), message_(message), details_(details) {}
        
        // Getter methods
        ErrorType getType() const { return type_; }
        ErrorSeverity getSeverity() const { return severity_; }
        const SourceLocation& getLocation() const { return location_; }
        const Str& getMessage() const { return message_; }
        const Str& getDetails() const { return details_; }
        const Vec<FixSuggestion>& getSuggestions() const { return suggestions_; }
        const ParseError* getRelatedError() const { return relatedError_.get(); }
        
        // Setter methods
        void setDetails(const Str& details) { details_ = details; }
        void setSeverity(ErrorSeverity severity) { severity_ = severity; }
        
        // Add fix suggestion
        void addSuggestion(const FixSuggestion& suggestion) {
            suggestions_.push_back(suggestion);
        }
        
        void addSuggestion(FixType type, const SourceLocation& location, 
                          const Str& description, const Str& newText = "") {
            suggestions_.emplace_back(type, location, description, newText);
        }
        
        // Set related error
        void setRelatedError(UPtr<ParseError> error) {
            relatedError_ = std::move(error);
        }
        
        // Formatting methods
        Str toString() const {
            return formatError(false);
        }
        
        Str toDetailedString() const {
            return formatError(true);
        }
        
        Str toShortString() const {
            return location_.toString() + ": " + severityToString() + ": " + message_;
        }
        
        // Static factory methods - common error types (with localization support)
        static ParseError unexpectedToken(const SourceLocation& location, const Str& expected, const Str& actual) {
            Str message = getLocalizedMessage(MessageCategory::ErrorMessage, "ExpectedButFound", {expected, actual});
            auto error = ParseError(ErrorType::UnexpectedToken, location, message);
            Str suggestion = getLocalizedMessage(MessageCategory::FixSuggestion, "ReplaceWith", {expected});
            error.addSuggestion(FixType::Replace, location, suggestion, expected);
            return error;
        }
        
        static ParseError missingToken(const SourceLocation& location, const Str& expected) {
            Str message = getLocalizedMessage(MessageCategory::ErrorMessage, "Missing", {expected});
            auto error = ParseError(ErrorType::MissingToken, location, message);
            Str suggestion = getLocalizedMessage(MessageCategory::FixSuggestion, "Insert", {expected});
            error.addSuggestion(FixType::Insert, location, suggestion, expected);
            return error;
        }
        
        static ParseError invalidExpression(const SourceLocation& location, const Str& reason = "") {
            Str message;
            if (!reason.empty()) {
                message = getLocalizedMessage(MessageCategory::ErrorMessage, "InvalidExpressionReason", {reason});
            } else {
                message = getLocalizedMessage(MessageCategory::ErrorType, "InvalidExpression");
            }
            return ParseError(ErrorType::InvalidExpression, location, message);
        }
        
        static ParseError undefinedVariable(const SourceLocation& location, const Str& varName) {
            Str message = getLocalizedMessage(MessageCategory::ErrorMessage, "UndefinedVar", {varName});
            auto error = ParseError(ErrorType::UndefinedVariable, location, message);
            Str suggestion = getLocalizedMessage(MessageCategory::FixSuggestion, "DeclareVariable");
            error.addSuggestion(FixType::Insert, location, suggestion, "local " + varName + " = ");
            return error;
        }
        
        static ParseError mismatchedParentheses(const SourceLocation& location, const Str& expected) {
            Str message = getLocalizedMessage(MessageCategory::ErrorMessage, "MismatchedParen");
            auto error = ParseError(ErrorType::MismatchedParentheses, location, message);
            Str suggestion = getLocalizedMessage(MessageCategory::FixSuggestion, "AddMissing", {expected});
            error.addSuggestion(FixType::Insert, location, suggestion, expected);
            return error;
        }
        
        // Severity to string (public static method, with localization support)
        static Str severityToString(ErrorSeverity severity) {
            switch (severity) {
                case ErrorSeverity::Info: return getLocalizedMessage(MessageCategory::Severity, "Info");
                case ErrorSeverity::Warning: return getLocalizedMessage(MessageCategory::Severity, "Warning");
                case ErrorSeverity::Error: return getLocalizedMessage(MessageCategory::Severity, "Error");
                case ErrorSeverity::Fatal: return getLocalizedMessage(MessageCategory::Severity, "Fatal");
                default: return "unknown";
            }
        }
        
        // Error type to string (with localization support)
        static Str errorTypeToString(ErrorType type) {
            switch (type) {
                case ErrorType::UnexpectedCharacter: return getLocalizedMessage(MessageCategory::ErrorType, "UnexpectedCharacter");
                case ErrorType::UnterminatedString: return getLocalizedMessage(MessageCategory::ErrorType, "UnterminatedString");
                case ErrorType::InvalidNumber: return getLocalizedMessage(MessageCategory::ErrorType, "InvalidNumber");
                case ErrorType::UnexpectedToken: return getLocalizedMessage(MessageCategory::ErrorType, "UnexpectedToken");
                case ErrorType::MissingToken: return getLocalizedMessage(MessageCategory::ErrorType, "MissingToken");
                case ErrorType::InvalidExpression: return getLocalizedMessage(MessageCategory::ErrorType, "InvalidExpression");
                case ErrorType::InvalidStatement: return getLocalizedMessage(MessageCategory::ErrorType, "InvalidStatement");
                case ErrorType::MismatchedParentheses: return getLocalizedMessage(MessageCategory::ErrorType, "MismatchedParentheses");
                case ErrorType::MismatchedBraces: return getLocalizedMessage(MessageCategory::ErrorType, "MismatchedBraces");
                case ErrorType::MismatchedBrackets: return getLocalizedMessage(MessageCategory::ErrorType, "MismatchedBrackets");
                case ErrorType::UndefinedVariable: return getLocalizedMessage(MessageCategory::ErrorType, "UndefinedVariable");
                case ErrorType::RedefinedVariable: return getLocalizedMessage(MessageCategory::ErrorType, "RedefinedVariable");
                case ErrorType::InvalidAssignment: return getLocalizedMessage(MessageCategory::ErrorType, "InvalidAssignment");
                case ErrorType::InvalidFunctionCall: return getLocalizedMessage(MessageCategory::ErrorType, "InvalidFunctionCall");
                case ErrorType::WrongArgumentCount: return getLocalizedMessage(MessageCategory::ErrorType, "WrongArgumentCount");
                case ErrorType::InvalidReturn: return getLocalizedMessage(MessageCategory::ErrorType, "InvalidReturn");
                case ErrorType::InvalidBreak: return getLocalizedMessage(MessageCategory::ErrorType, "InvalidBreak");
                case ErrorType::InvalidContinue: return getLocalizedMessage(MessageCategory::ErrorType, "InvalidContinue");
                case ErrorType::InternalError: return getLocalizedMessage(MessageCategory::ErrorType, "InternalError");
                case ErrorType::Unknown: return getLocalizedMessage(MessageCategory::ErrorType, "Unknown");
                default: return "Unknown";
            }
        }
        
    private:
        // Format error information (with localization support)
        Str formatError(bool includeDetails) const {
            Str result;
            
            // Basic error information
            result += location_.toString() + ": " + severityToString() + ": " + message_;
            
            // Detailed information
            if (includeDetails && !details_.empty()) {
                Str detailsLabel = getLocalizedMessage(MessageCategory::General, "Details");
                result += "\n  " + detailsLabel + ": " + details_;
            }
            
            // Fix suggestions
            if (includeDetails && !suggestions_.empty()) {
                Str suggestionsLabel = getLocalizedMessage(MessageCategory::General, "Suggestions");
                result += "\n  " + suggestionsLabel + ":";
                for (const auto& suggestion : suggestions_) {
                    result += "\n    - " + suggestion.description;
                    if (!suggestion.newText.empty()) {
                        result += " (" + suggestion.newText + ")";
                    }
                }
            }
            
            // Related error
            if (includeDetails && relatedError_) {
                Str relatedLabel = getLocalizedMessage(MessageCategory::General, "Related");
                result += "\n  " + relatedLabel + ": " + relatedError_->toShortString();
            }
            
            return result;
        }
        
        // Severity to string
        Str severityToString() const {
            switch (severity_) {
                case ErrorSeverity::Info: return "info";
                case ErrorSeverity::Warning: return "warning";
                case ErrorSeverity::Error: return "error";
                case ErrorSeverity::Fatal: return "fatal";
                default: return "unknown";
            }
        }
    };
    
    // Error collector class - for collecting multiple errors
    class ErrorCollector {
    private:
        Vec<ParseError> errors_;
        size_t maxErrors_;
        
    public:
        explicit ErrorCollector(size_t maxErrors = 100) : maxErrors_(maxErrors) {}
        
        // Add error (move semantics only)
        void addError(ParseError&& error) {
            if (errors_.size() < maxErrors_) {
                errors_.push_back(std::move(error));
            }
        }
        
        // Get errors
        const Vec<ParseError>& getErrors() const { return errors_; }
        size_t getErrorCount() const { return errors_.size(); }
        bool hasErrors() const { return !errors_.empty(); }
        bool hasMaxErrors() const { return errors_.size() >= maxErrors_; }
        
        // Filter by severity
        Vec<const ParseError*> getErrorsBySeverity(ErrorSeverity severity) const {
            Vec<const ParseError*> result;
            for (const auto& error : errors_) {
                if (error.getSeverity() == severity) {
                    result.push_back(&error);
                }
            }
            return result;
        }
        
        // Clear errors
        void clear() { errors_.clear(); }
        
        // Format all errors
        Str toString() const {
            if (errors_.empty()) {
                return "No errors";
            }
            
            Str result;
            for (size_t i = 0; i < errors_.size(); ++i) {
                if (i > 0) result += "\n";
                result += errors_[i].toString();
            }
            return result;
        }
        
        Str toDetailedString() const {
            if (errors_.empty()) {
                return "No errors";
            }
            
            Str result;
            for (size_t i = 0; i < errors_.size(); ++i) {
                if (i > 0) result += "\n\n";
                result += errors_[i].toDetailedString();
            }
            return result;
        }
    };
    
    // Error reporter configuration
    struct ErrorReporterConfig {
        size_t maxErrors = 100;          // Maximum error count
        bool stopOnFirstError = false;   // Stop on first error
        bool includeWarnings = true;     // Include warnings
        bool includeInfo = false;        // Include information
        
        ErrorReporterConfig() = default;
        ErrorReporterConfig(size_t max, bool stopFirst = false, bool warnings = true, bool info = false)
            : maxErrors(max), stopOnFirstError(stopFirst), includeWarnings(warnings), includeInfo(info) {}
    };
    
    // Error reporter class - unified error collection and reporting
    class ErrorReporter {
    private:
        ErrorCollector collector_;
        ErrorReporterConfig config_;
        
    public:
        explicit ErrorReporter(const ErrorReporterConfig& config = ErrorReporterConfig())
            : collector_(config.maxErrors), config_(config) {}
            
        // Basic error reporting methods
        void reportError(ErrorType type, const SourceLocation& location, const Str& message,
                        ErrorSeverity severity = ErrorSeverity::Error) {
            if (!shouldReportError(severity)) return;
            
            ParseError error(type, location, message, severity);
            collector_.addError(std::move(error));
        }
        
        void reportError(ErrorType type, const SourceLocation& location, const Str& message,
                        const Str& details, ErrorSeverity severity = ErrorSeverity::Error) {
            if (!shouldReportError(severity)) return;
            
            ParseError error(type, location, message, details, severity);
            collector_.addError(std::move(error));
        }
        
        // Directly add ParseError object
        void addError(ParseError&& error) {
            if (!shouldReportError(error.getSeverity())) return;
            collector_.addError(std::move(error));
        }
        
        // Convenient error reporting methods
        void reportSyntaxError(const SourceLocation& location, const Str& message) {
            reportError(ErrorType::InvalidExpression, location, message, ErrorSeverity::Error);
        }
        
        void reportUnexpectedToken(const SourceLocation& location, const Str& expected, const Str& actual) {
            Str message = "Expected '" + expected + "', but got '" + actual + "'";
            auto error = ParseError::unexpectedToken(location, expected, actual);
            addError(std::move(error));
        }
        
        void reportMissingToken(const SourceLocation& location, const Str& expected) {
            auto error = ParseError::missingToken(location, expected);
            addError(std::move(error));
        }
        
        void reportSemanticError(const SourceLocation& location, const Str& message) {
            reportError(ErrorType::UndefinedVariable, location, message, ErrorSeverity::Error);
        }
        
        void reportWarning(const SourceLocation& location, const Str& message) {
            reportError(ErrorType::Unknown, location, message, ErrorSeverity::Warning);
        }
        
        void reportInfo(const SourceLocation& location, const Str& message) {
            reportError(ErrorType::Unknown, location, message, ErrorSeverity::Info);
        }
        
        // Query methods
        bool hasErrors() const { return collector_.hasErrors(); }
        bool hasErrorsOrWarnings() const {
            const auto& errors = collector_.getErrors();
            for (const auto& error : errors) {
                if (error.getSeverity() == ErrorSeverity::Error || 
                    error.getSeverity() == ErrorSeverity::Fatal ||
                    (config_.includeWarnings && error.getSeverity() == ErrorSeverity::Warning)) {
                    return true;
                }
            }
            return false;
        }
        
        size_t getErrorCount() const { return collector_.getErrorCount(); }
        size_t getErrorCount(ErrorSeverity severity) const {
            return collector_.getErrorsBySeverity(severity).size();
        }
        
        const Vec<ParseError>& getErrors() const { return collector_.getErrors(); }
        Vec<const ParseError*> getErrorsBySeverity(ErrorSeverity severity) const {
            return collector_.getErrorsBySeverity(severity);
        }
        
        bool shouldStopParsing() const {
            if (config_.stopOnFirstError && hasErrors()) return true;
            return collector_.hasMaxErrors();
        }
        
        // Control methods
        void clear() { collector_.clear(); }
        void setMaxErrors(size_t maxErrors) {
            config_.maxErrors = maxErrors;
            // Note: Cannot directly modify collector_'s maxErrors, need to recreate
        }
        
        const ErrorReporterConfig& getConfig() const { return config_; }
        void setConfig(const ErrorReporterConfig& config) { config_ = config; }
        
        // Output methods
        Str toString() const {
            return collector_.toString();
        }
        
        Str toDetailedString() const {
            return collector_.toDetailedString();
        }
        
        Str toShortString() const {
            if (!hasErrors()) return "No errors";
            
            size_t errorCount = getErrorCount(ErrorSeverity::Error) + getErrorCount(ErrorSeverity::Fatal);
            size_t warningCount = getErrorCount(ErrorSeverity::Warning);
            
            Str result = std::to_string(errorCount) + " error(s)";
            if (warningCount > 0) {
                result += ", " + std::to_string(warningCount) + " warning(s)";
            }
            return result;
        }
        
        // JSON format output (simple implementation)
        Str toJson() const {
            Str result = "{\"errors\":[";
            const auto& errors = collector_.getErrors();
            for (size_t i = 0; i < errors.size(); ++i) {
                if (i > 0) result += ",";
                result += "{";
                result += "\"type\":\"" + ParseError::errorTypeToString(errors[i].getType()) + "\",";
                result += "\"severity\":\"" + ParseError::severityToString(errors[i].getSeverity()) + "\",";
                result += "\"line\":" + std::to_string(errors[i].getLocation().getLine()) + ",";
                result += "\"column\":" + std::to_string(errors[i].getLocation().getColumn()) + ",";
                result += "\"message\":\"" + escapeJsonString(errors[i].getMessage()) + "\"";
                result += "}";
            }
            result += "],\"count\":" + std::to_string(errors.size()) + "}";
            return result;
        }
        
        // Static factory methods
        static ErrorReporter createDefault() {
            return ErrorReporter();
        }
        
        static ErrorReporter createStrict() {
            ErrorReporterConfig config;
            config.stopOnFirstError = true;
            config.includeWarnings = true;
            config.includeInfo = false;
            return ErrorReporter(config);
        }
        
        static ErrorReporter createPermissive() {
            ErrorReporterConfig config;
            config.maxErrors = 1000;
            config.stopOnFirstError = false;
            config.includeWarnings = true;
            config.includeInfo = true;
            return ErrorReporter(config);
        }
        
    private:
        bool shouldReportError(ErrorSeverity severity) const {
            switch (severity) {
                case ErrorSeverity::Info:
                    return config_.includeInfo;
                case ErrorSeverity::Warning:
                    return config_.includeWarnings;
                case ErrorSeverity::Error:
                case ErrorSeverity::Fatal:
                    return true;
                default:
                    return true;
            }
        }
        
        Str escapeJsonString(const Str& str) const {
            Str result;
            for (char c : str) {
                switch (c) {
                    case '"': result += "\\\""; break;
                    case '\\': result += "\\\\"; break;
                    case '\n': result += "\\n"; break;
                    case '\r': result += "\\r"; break;
                    case '\t': result += "\\t"; break;
                    default: result += c; break;
                }
            }
            return result;
        }
    };
}