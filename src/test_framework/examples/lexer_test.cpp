/**
 * @file lexer_test.cpp
 * @brief 简单的TokenType测试
 */

#include "../../lexer/lexer.hpp"
#include <iostream>
#include <string>

using namespace Lua;

// 独立的tokenTypeToString实现，避免依赖GC系统
std::string simpleTokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::And: return "and";
        case TokenType::Break: return "break";
        case TokenType::Do: return "do";
        case TokenType::Else: return "else";
        case TokenType::Elseif: return "elseif";
        case TokenType::End: return "end";
        case TokenType::False: return "false";
        case TokenType::For: return "for";
        case TokenType::Function: return "function";
        case TokenType::If: return "if";
        case TokenType::In: return "in";
        case TokenType::Local: return "local";
        case TokenType::Nil: return "nil";
        case TokenType::Not: return "not";
        case TokenType::Or: return "or";
        case TokenType::Repeat: return "repeat";
        case TokenType::Return: return "return";
        case TokenType::Then: return "then";
        case TokenType::True: return "true";
        case TokenType::Until: return "until";
        case TokenType::While: return "while";
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Percent: return "%";
        case TokenType::Caret: return "^";
        case TokenType::Hash: return "#";
        case TokenType::Equal: return "==";
        case TokenType::LessEqual: return "<=";
        case TokenType::GreaterEqual: return ">=";
        case TokenType::Less: return "<";
        case TokenType::Greater: return ">";
        case TokenType::NotEqual: return "~=";
        case TokenType::Assign: return "=";
        case TokenType::LeftParen: return "(";
        case TokenType::RightParen: return ")";
        case TokenType::LeftBrace: return "{";
        case TokenType::RightBrace: return "}";
        case TokenType::LeftBracket: return "[";
        case TokenType::RightBracket: return "]";
        case TokenType::Semicolon: return ";";
        case TokenType::Colon: return ":";
        case TokenType::Comma: return ",";
        case TokenType::Dot: return ".";
        case TokenType::DotDot: return "..";
        case TokenType::DotDotDot: return "...";
        case TokenType::Number: return "number";
        case TokenType::String: return "string";
        case TokenType::Name: return "identifier";
        case TokenType::Eof: return "end of file";
        case TokenType::Error: return "error";
        default: return "unknown";
    }
}

void testTokenTypeToString() {
    std::cout << "=== TokenType字符串转换测试 ===" << std::endl;
    
    // 测试关键字
    std::cout << "TokenType::If -> " << simpleTokenTypeToString(TokenType::If) << std::endl;
    std::cout << "TokenType::Else -> " << simpleTokenTypeToString(TokenType::Else) << std::endl;
    std::cout << "TokenType::Function -> " << simpleTokenTypeToString(TokenType::Function) << std::endl;
    std::cout << "TokenType::Local -> " << simpleTokenTypeToString(TokenType::Local) << std::endl;
    std::cout << "TokenType::Return -> " << simpleTokenTypeToString(TokenType::Return) << std::endl;
    
    // 测试操作符
    std::cout << "TokenType::Plus -> " << simpleTokenTypeToString(TokenType::Plus) << std::endl;
    std::cout << "TokenType::Minus -> " << simpleTokenTypeToString(TokenType::Minus) << std::endl;
    std::cout << "TokenType::Star -> " << simpleTokenTypeToString(TokenType::Star) << std::endl;
    std::cout << "TokenType::Slash -> " << simpleTokenTypeToString(TokenType::Slash) << std::endl;
    std::cout << "TokenType::Assign -> " << simpleTokenTypeToString(TokenType::Assign) << std::endl;
    
    // 测试其他类型
    std::cout << "TokenType::Number -> " << simpleTokenTypeToString(TokenType::Number) << std::endl;
    std::cout << "TokenType::String -> " << simpleTokenTypeToString(TokenType::String) << std::endl;
    std::cout << "TokenType::Name -> " << simpleTokenTypeToString(TokenType::Name) << std::endl;
    std::cout << "TokenType::Eof -> " << simpleTokenTypeToString(TokenType::Eof) << std::endl;
    
    std::cout << "TokenType字符串转换测试完成!" << std::endl;
}

void testTokenCreation() {
    std::cout << "\n=== Token创建测试 ===" << std::endl;
    
    // 创建一个简单的token
    Token token1(TokenType::If, "if", 1, 1);
    std::cout << "Token1: type=" << static_cast<int>(token1.type) 
              << ", lexeme='" << token1.lexeme 
              << "', line=" << token1.line 
              << ", column=" << token1.column << std::endl;
    
    // 创建另一个token
    Token token2(TokenType::Number, "42", 1, 5);
    std::cout << "Token2: type=" << static_cast<int>(token2.type) 
              << ", lexeme='" << token2.lexeme 
              << ", line=" << token2.line 
              << ", column=" << token2.column << std::endl;
    
    std::cout << "Token创建测试完成!" << std::endl;
}

int main() {
    try {
        std::cout << "=== Lexer基础组件测试 ===" << std::endl;
        std::cout << "测试TokenType和Token的基本功能" << std::endl << std::endl;
        
        testTokenTypeToString();
        testTokenCreation();
        
        std::cout << "\n=== 所有测试完成 ===" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cout << "测试失败: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "未知错误发生" << std::endl;
        return 1;
    }
}