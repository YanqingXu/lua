#pragma once

#include "source_location.hpp"
#include "../../common/types.hpp"
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
        
        // 静态工厂方法 - 常见错误类型
        static ParseError unexpectedToken(const SourceLocation& location, const Str& expected, const Str& actual) {
            Str message = "Expected '" + expected + "', but found '" + actual + "'";
            auto error = ParseError(ErrorType::UnexpectedToken, location, message);
            error.addSuggestion(FixType::Replace, location, "Replace with '" + expected + "'", expected);
            return error;
        }
        
        static ParseError missingToken(const SourceLocation& location, const Str& expected) {
            Str message = "Missing '" + expected + "'";
            auto error = ParseError(ErrorType::MissingToken, location, message);
            error.addSuggestion(FixType::Insert, location, "Insert '" + expected + "'", expected);
            return error;
        }
        
        static ParseError invalidExpression(const SourceLocation& location, const Str& reason = "") {
            Str message = "Invalid expression";
            if (!reason.empty()) {
                message += ": " + reason;
            }
            return ParseError(ErrorType::InvalidExpression, location, message);
        }
        
        static ParseError undefinedVariable(const SourceLocation& location, const Str& varName) {
            Str message = "Undefined variable '" + varName + "'";
            auto error = ParseError(ErrorType::UndefinedVariable, location, message);
            error.addSuggestion(FixType::Insert, location, "Declare variable before use", "local " + varName + " = ");
            return error;
        }
        
        static ParseError mismatchedParentheses(const SourceLocation& location, const Str& expected) {
            Str message = "Mismatched parentheses";
            auto error = ParseError(ErrorType::MismatchedParentheses, location, message);
            error.addSuggestion(FixType::Insert, location, "Add missing '" + expected + "'", expected);
            return error;
        }
        
        // 错误类型转字符串
        static Str errorTypeToString(ErrorType type) {
            switch (type) {
                case ErrorType::UnexpectedCharacter: return "Unexpected Character";
                case ErrorType::UnterminatedString: return "Unterminated String";
                case ErrorType::InvalidNumber: return "Invalid Number";
                case ErrorType::UnexpectedToken: return "Unexpected Token";
                case ErrorType::MissingToken: return "Missing Token";
                case ErrorType::InvalidExpression: return "Invalid Expression";
                case ErrorType::InvalidStatement: return "Invalid Statement";
                case ErrorType::MismatchedParentheses: return "Mismatched Parentheses";
                case ErrorType::MismatchedBraces: return "Mismatched Braces";
                case ErrorType::MismatchedBrackets: return "Mismatched Brackets";
                case ErrorType::UndefinedVariable: return "Undefined Variable";
                case ErrorType::RedefinedVariable: return "Redefined Variable";
                case ErrorType::InvalidAssignment: return "Invalid Assignment";
                case ErrorType::InvalidFunctionCall: return "Invalid Function Call";
                case ErrorType::WrongArgumentCount: return "Wrong Argument Count";
                case ErrorType::InvalidReturn: return "Invalid Return";
                case ErrorType::InvalidBreak: return "Invalid Break";
                case ErrorType::InvalidContinue: return "Invalid Continue";
                case ErrorType::InternalError: return "Internal Error";
                case ErrorType::Unknown: return "Unknown Error";
                default: return "Unknown";
            }
        }
        
    private:
        // 格式化错误信息
        Str formatError(bool includeDetails) const {
            Str result;
            
            // 基本错误信息
            result += location_.toString() + ": " + severityToString() + ": " + message_;
            
            // 详细信息
            if (includeDetails && !details_.empty()) {
                result += "\n  Details: " + details_;
            }
            
            // 修复建议
            if (includeDetails && !suggestions_.empty()) {
                result += "\n  Suggestions:";
                for (const auto& suggestion : suggestions_) {
                    result += "\n    - " + suggestion.description;
                    if (!suggestion.newText.empty()) {
                        result += " (" + suggestion.newText + ")";
                    }
                }
            }
            
            // 相关错误
            if (includeDetails && relatedError_) {
                result += "\n  Related: " + relatedError_->toShortString();
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
}