#include "localization_manager.hpp"

namespace Lua {
    // Static member initialization
    std::unique_ptr<LocalizationManager> LocalizationManager::instance_ = nullptr;
    
    void LocalizationManager::initializeDefaultCatalogs() {
        // Initialize English message catalog
        auto englishCatalog = std::make_unique<MessageCatalog>(Language::English);
        initializeEnglishMessages(*englishCatalog);
        catalogs_[Language::English] = std::move(englishCatalog);
        
        // Initialize Chinese message catalog
        auto chineseCatalog = std::make_unique<MessageCatalog>(Language::Chinese);
        initializeChineseMessages(*chineseCatalog);
        catalogs_[Language::Chinese] = std::move(chineseCatalog);
    }
    
    void LocalizationManager::initializeEnglishMessages(MessageCatalog& catalog) {
        // Error type messages
        catalog.addMessage(MessageCategory::ErrorType, "UnexpectedCharacter", "Unexpected Character");
        catalog.addMessage(MessageCategory::ErrorType, "UnterminatedString", "Unterminated String");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidNumber", "Invalid Number");
        catalog.addMessage(MessageCategory::ErrorType, "UnexpectedToken", "Unexpected Token");
        catalog.addMessage(MessageCategory::ErrorType, "MissingToken", "Missing Token");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidExpression", "Invalid Expression");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidStatement", "Invalid Statement");
        catalog.addMessage(MessageCategory::ErrorType, "MismatchedParentheses", "Mismatched Parentheses");
        catalog.addMessage(MessageCategory::ErrorType, "MismatchedBraces", "Mismatched Braces");
        catalog.addMessage(MessageCategory::ErrorType, "MismatchedBrackets", "Mismatched Brackets");
        catalog.addMessage(MessageCategory::ErrorType, "UndefinedVariable", "Undefined Variable");
        catalog.addMessage(MessageCategory::ErrorType, "RedefinedVariable", "Redefined Variable");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidAssignment", "Invalid Assignment");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidFunctionCall", "Invalid Function Call");
        catalog.addMessage(MessageCategory::ErrorType, "WrongArgumentCount", "Wrong Argument Count");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidReturn", "Invalid Return");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidBreak", "Invalid Break");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidContinue", "Invalid Continue");
        catalog.addMessage(MessageCategory::ErrorType, "InternalError", "Internal Error");
        catalog.addMessage(MessageCategory::ErrorType, "Unknown", "Unknown Error");
        
        // Severity messages
        catalog.addMessage(MessageCategory::Severity, "Info", "info");
        catalog.addMessage(MessageCategory::Severity, "Warning", "warning");
        catalog.addMessage(MessageCategory::Severity, "Error", "error");
        catalog.addMessage(MessageCategory::Severity, "Fatal", "fatal");
        
        // Error message templates (Lua 5.1 compatible)
        catalog.addMessage(MessageCategory::ErrorMessage, "ExpectedButFound", "Expected '{0}', but found '{1}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "Missing", "Missing '{0}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "InvalidExpressionReason", "Invalid expression: {0}");
        catalog.addMessage(MessageCategory::ErrorMessage, "UndefinedVar", "Undefined variable '{0}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "MismatchedParen", "Mismatched parentheses");

        // Lua 5.1 standard error messages
        catalog.addMessage(MessageCategory::ErrorMessage, "UnexpectedSymbolNear", "unexpected symbol near '{0}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "SyntaxErrorNear", "syntax error near '{0}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "UnexpectedEOF", "unexpected end of file");
        catalog.addMessage(MessageCategory::ErrorMessage, "MalformedNumber", "malformed number near '{0}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "UnfinishedString", "unfinished string near '{0}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "InvalidEscapeSequence", "invalid escape sequence near '{0}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "ChunkTooManySyntaxLevels", "chunk has too many syntax levels");
        catalog.addMessage(MessageCategory::ErrorMessage, "FunctionSpan", "function at line {0} ends on line {1}");
        catalog.addMessage(MessageCategory::ErrorMessage, "AmbiguousSyntax", "ambiguous syntax (function call x new statement)");
        catalog.addMessage(MessageCategory::ErrorMessage, "EndExpected", "'{0}' expected");
        catalog.addMessage(MessageCategory::ErrorMessage, "EndExpectedToClose", "'{0}' expected (to close '{1}' at line {2})");
        
        // Fix suggestion messages
        catalog.addMessage(MessageCategory::FixSuggestion, "ReplaceWith", "Replace with '{0}'");
        catalog.addMessage(MessageCategory::FixSuggestion, "Insert", "Insert '{0}'");
        catalog.addMessage(MessageCategory::FixSuggestion, "DeclareVariable", "Declare variable before use");
        catalog.addMessage(MessageCategory::FixSuggestion, "AddMissing", "Add missing '{0}'");
        
        // General messages
        catalog.addMessage(MessageCategory::General, "Details", "Details");
        catalog.addMessage(MessageCategory::General, "Suggestions", "Suggestions");
        catalog.addMessage(MessageCategory::General, "Related", "Related");
    }
    
    void LocalizationManager::initializeChineseMessages(MessageCatalog& catalog) {
        // Error type messages
        catalog.addMessage(MessageCategory::ErrorType, "UnexpectedCharacter", "意外字符");
        catalog.addMessage(MessageCategory::ErrorType, "UnterminatedString", "未终止的字符串");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidNumber", "无效数字格式");
        catalog.addMessage(MessageCategory::ErrorType, "UnexpectedToken", "意外的标记");
        catalog.addMessage(MessageCategory::ErrorType, "MissingToken", "缺少标记");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidExpression", "无效表达式");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidStatement", "无效语句");
        catalog.addMessage(MessageCategory::ErrorType, "MismatchedParentheses", "括号不匹配");
        catalog.addMessage(MessageCategory::ErrorType, "MismatchedBraces", "大括号不匹配");
        catalog.addMessage(MessageCategory::ErrorType, "MismatchedBrackets", "方括号不匹配");
        catalog.addMessage(MessageCategory::ErrorType, "UndefinedVariable", "未定义变量");
        catalog.addMessage(MessageCategory::ErrorType, "RedefinedVariable", "重复定义变量");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidAssignment", "无效赋值");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidFunctionCall", "无效函数调用");
        catalog.addMessage(MessageCategory::ErrorType, "WrongArgumentCount", "参数数量错误");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidReturn", "无效返回语句");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidBreak", "无效break语句");
        catalog.addMessage(MessageCategory::ErrorType, "InvalidContinue", "无效continue语句");
        catalog.addMessage(MessageCategory::ErrorType, "InternalError", "内部错误");
        catalog.addMessage(MessageCategory::ErrorType, "Unknown", "未知错误");
        
        // Severity messages
        catalog.addMessage(MessageCategory::Severity, "Info", "信息");
        catalog.addMessage(MessageCategory::Severity, "Warning", "警告");
        catalog.addMessage(MessageCategory::Severity, "Error", "错误");
        catalog.addMessage(MessageCategory::Severity, "Fatal", "致命错误");
        
        // Error message templates (Lua 5.1 compatible Chinese)
        catalog.addMessage(MessageCategory::ErrorMessage, "ExpectedButFound", "期望 '{0}'，但发现 '{1}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "Missing", "缺少 '{0}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "InvalidExpressionReason", "无效表达式：{0}");
        catalog.addMessage(MessageCategory::ErrorMessage, "UndefinedVar", "未定义变量 '{0}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "MismatchedParen", "括号不匹配");

        // Lua 5.1 standard error messages (Chinese)
        catalog.addMessage(MessageCategory::ErrorMessage, "UnexpectedSymbolNear", "在 '{0}' 附近出现意外符号");
        catalog.addMessage(MessageCategory::ErrorMessage, "SyntaxErrorNear", "在 '{0}' 附近出现语法错误");
        catalog.addMessage(MessageCategory::ErrorMessage, "UnexpectedEOF", "意外的文件结束");
        catalog.addMessage(MessageCategory::ErrorMessage, "MalformedNumber", "在 '{0}' 附近出现格式错误的数字");
        catalog.addMessage(MessageCategory::ErrorMessage, "UnfinishedString", "在 '{0}' 附近出现未完成的字符串");
        catalog.addMessage(MessageCategory::ErrorMessage, "InvalidEscapeSequence", "在 '{0}' 附近出现无效的转义序列");
        catalog.addMessage(MessageCategory::ErrorMessage, "ChunkTooManySyntaxLevels", "代码块的语法层次过多");
        catalog.addMessage(MessageCategory::ErrorMessage, "FunctionSpan", "第 {0} 行的函数在第 {1} 行结束");
        catalog.addMessage(MessageCategory::ErrorMessage, "AmbiguousSyntax", "语法歧义（函数调用与新语句）");
        catalog.addMessage(MessageCategory::ErrorMessage, "EndExpected", "期望 '{0}'");
        catalog.addMessage(MessageCategory::ErrorMessage, "EndExpectedToClose", "期望 '{0}'（用于关闭第 {2} 行的 '{1}'）");
        
        // Fix suggestion messages
        catalog.addMessage(MessageCategory::FixSuggestion, "ReplaceWith", "替换为 '{0}'");
        catalog.addMessage(MessageCategory::FixSuggestion, "Insert", "插入 '{0}'");
        catalog.addMessage(MessageCategory::FixSuggestion, "DeclareVariable", "在使用前声明变量");
        catalog.addMessage(MessageCategory::FixSuggestion, "AddMissing", "添加缺少的 '{0}'");
        
        // General messages
        catalog.addMessage(MessageCategory::General, "Details", "详细信息");
        catalog.addMessage(MessageCategory::General, "Suggestions", "建议");
        catalog.addMessage(MessageCategory::General, "Related", "相关");
    }
}