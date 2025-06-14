#pragma once

#include "source_location.hpp"
#include "../../common/types.hpp"
#include "../../localization/localization_manager.hpp"
#include <vector>
#include <string>
#include <memory>

namespace Lua {
    // 错误类型枚举
    enum class ErrorType {
        // 词法错误
        UnexpectedCharacter,    // 意外字符
        UnterminatedString,     // 未终止的字符串
        InvalidNumber,          // 无效数字格式
        
        // 语法错误
        UnexpectedToken,        // 意外的token
        MissingToken,          // 缺少期望的token
        InvalidExpression,      // 无效表达式
        InvalidStatement,       // 无效语句
        
        // 结构错误
        MismatchedParentheses, // 括号不匹配
        MismatchedBraces,      // 大括号不匹配
        MismatchedBrackets,    // 方括号不匹配
        
        // 语义错误
        UndefinedVariable,     // 未定义变量
        RedefinedVariable,     // 重复定义变量
        InvalidAssignment,     // 无效赋值
        
        // 函数相关错误
        InvalidFunctionCall,   // 无效函数调用
        WrongArgumentCount,    // 参数数量错误
        InvalidReturn,         // 无效返回语句
        
        // 控制流错误
        InvalidBreak,          // 无效break语句
        InvalidContinue,       // 无效continue语句
        
        // 其他错误
        InternalError,         // 内部错误
        Unknown                // 未知错误
    };
    
    // 修复建议类型
    enum class FixType {
        Insert,     // 插入
        Delete,     // 删除
        Replace,    // 替换
        Move        // 移动
    };
    
    // 修复建议结构体
    struct FixSuggestion {
        FixType type;              // 修复类型
        SourceLocation location;   // 修复位置
        Str description;          // 修复描述
        Str newText;              // 新文本（用于插入/替换）
        
        FixSuggestion(FixType t, const SourceLocation& loc, const Str& desc, const Str& text = "")
            : type(t), location(loc), description(desc), newText(text) {}
    };
    
    // 错误严重程度
    enum class ErrorSeverity {
        Info,       // 信息
        Warning,    // 警告
        Error,      // 错误
        Fatal       // 致命错误
    };
    
    // 解析错误类
    class ParseError {
    private:
        ErrorType type_;                           // 错误类型
        ErrorSeverity severity_;                   // 错误严重程度
        SourceLocation location_;                  // 错误位置
        Str message_;                             // 错误消息
        Str details_;                             // 详细信息
        Vec<FixSuggestion> suggestions_;          // 修复建议列表
        UPtr<ParseError> relatedError_;           // 相关错误（用于错误链）
        
    public:
        // 构造函数
        ParseError(ErrorType type, const SourceLocation& location, const Str& message,
                  ErrorSeverity severity = ErrorSeverity::Error)
            : type_(type), severity_(severity), location_(location), message_(message) {}
        
        // 拷贝构造函数（删除，因为包含unique_ptr）
        ParseError(const ParseError&) = delete;
        ParseError& operator=(const ParseError&) = delete;
        
        // 移动构造函数和移动赋值运算符
        ParseError(ParseError&&) = default;
        ParseError& operator=(ParseError&&) = default;
        
        // 带详细信息的构造函数
        ParseError(ErrorType type, const SourceLocation& location, const Str& message,
                  const Str& details, ErrorSeverity severity = ErrorSeverity::Error)
            : type_(type), severity_(severity), location_(location), message_(message), details_(details) {}
        
        // Getter方法
        ErrorType getType() const { return type_; }
        ErrorSeverity getSeverity() const { return severity_; }
        const SourceLocation& getLocation() const { return location_; }
        const Str& getMessage() const { return message_; }
        const Str& getDetails() const { return details_; }
        const Vec<FixSuggestion>& getSuggestions() const { return suggestions_; }
        const ParseError* getRelatedError() const { return relatedError_.get(); }
        
        // Setter方法
        void setDetails(const Str& details) { details_ = details; }
        void setSeverity(ErrorSeverity severity) { severity_ = severity; }
        
        // 添加修复建议
        void addSuggestion(const FixSuggestion& suggestion) {
            suggestions_.push_back(suggestion);
        }
        
        void addSuggestion(FixType type, const SourceLocation& location, 
                          const Str& description, const Str& newText = "") {
            suggestions_.emplace_back(type, location, description, newText);
        }
        
        // 设置相关错误
        void setRelatedError(UPtr<ParseError> error) {
            relatedError_ = std::move(error);
        }
        
        // 格式化方法
        Str toString() const {
            return formatError(false);
        }
        
        Str toDetailedString() const {
            return formatError(true);
        }
        
        Str toShortString() const {
            return location_.toString() + ": " + severityToString() + ": " + message_;
        }
        
        // 静态工厂方法 - 常见错误类型（支持本地化）
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
        
        // 严重程度转字符串（公有静态方法，支持本地化）
        static Str severityToString(ErrorSeverity severity) {
            switch (severity) {
                case ErrorSeverity::Info: return getLocalizedMessage(MessageCategory::Severity, "Info");
                case ErrorSeverity::Warning: return getLocalizedMessage(MessageCategory::Severity, "Warning");
                case ErrorSeverity::Error: return getLocalizedMessage(MessageCategory::Severity, "Error");
                case ErrorSeverity::Fatal: return getLocalizedMessage(MessageCategory::Severity, "Fatal");
                default: return "unknown";
            }
        }
        
        // 错误类型转字符串（支持本地化）
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
        // 格式化错误信息（支持本地化）
        Str formatError(bool includeDetails) const {
            Str result;
            
            // 基本错误信息
            result += location_.toString() + ": " + severityToString() + ": " + message_;
            
            // 详细信息
            if (includeDetails && !details_.empty()) {
                Str detailsLabel = getLocalizedMessage(MessageCategory::General, "Details");
                result += "\n  " + detailsLabel + ": " + details_;
            }
            
            // 修复建议
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
            
            // 相关错误
            if (includeDetails && relatedError_) {
                Str relatedLabel = getLocalizedMessage(MessageCategory::General, "Related");
                result += "\n  " + relatedLabel + ": " + relatedError_->toShortString();
            }
            
            return result;
        }
        
        // 严重程度转字符串
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
    
    // 错误收集器类 - 用于收集多个错误
    class ErrorCollector {
    private:
        Vec<ParseError> errors_;
        size_t maxErrors_;
        
    public:
        explicit ErrorCollector(size_t maxErrors = 100) : maxErrors_(maxErrors) {}
        
        // 添加错误（只支持移动语义）
        void addError(ParseError&& error) {
            if (errors_.size() < maxErrors_) {
                errors_.push_back(std::move(error));
            }
        }
        
        // 获取错误
        const Vec<ParseError>& getErrors() const { return errors_; }
        size_t getErrorCount() const { return errors_.size(); }
        bool hasErrors() const { return !errors_.empty(); }
        bool hasMaxErrors() const { return errors_.size() >= maxErrors_; }
        
        // 按严重程度过滤
        Vec<const ParseError*> getErrorsBySeverity(ErrorSeverity severity) const {
            Vec<const ParseError*> result;
            for (const auto& error : errors_) {
                if (error.getSeverity() == severity) {
                    result.push_back(&error);
                }
            }
            return result;
        }
        
        // 清空错误
        void clear() { errors_.clear(); }
        
        // 格式化所有错误
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
    
    // 错误报告器配置
    struct ErrorReporterConfig {
        size_t maxErrors = 100;          // 最大错误数
        bool stopOnFirstError = false;   // 遇到第一个错误时停止
        bool includeWarnings = true;     // 是否包含警告
        bool includeInfo = false;        // 是否包含信息
        
        ErrorReporterConfig() = default;
        ErrorReporterConfig(size_t max, bool stopFirst = false, bool warnings = true, bool info = false)
            : maxErrors(max), stopOnFirstError(stopFirst), includeWarnings(warnings), includeInfo(info) {}
    };
    
    // 错误报告器类 - 统一管理错误收集和报告
    class ErrorReporter {
    private:
        ErrorCollector collector_;
        ErrorReporterConfig config_;
        
    public:
        explicit ErrorReporter(const ErrorReporterConfig& config = ErrorReporterConfig())
            : collector_(config.maxErrors), config_(config) {}
            
        // 基础错误报告方法
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
        
        // 直接添加ParseError对象
        void addError(ParseError&& error) {
            if (!shouldReportError(error.getSeverity())) return;
            collector_.addError(std::move(error));
        }
        
        // 便捷的错误报告方法
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
        
        // 查询方法
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
        
        // 控制方法
        void clear() { collector_.clear(); }
        void setMaxErrors(size_t maxErrors) {
            config_.maxErrors = maxErrors;
            // 注意：这里不能直接修改collector_的maxErrors，需要重新创建
        }
        
        const ErrorReporterConfig& getConfig() const { return config_; }
        void setConfig(const ErrorReporterConfig& config) { config_ = config; }
        
        // 输出方法
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
        
        // JSON格式输出（简单实现）
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
        
        // 静态工厂方法
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