/**
 * @file test_lexer_number.cpp
 * @brief 验证词法分析器对非法数字的处理
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer.hpp"
#include <string>
#include <variant>

using namespace Lua;
using namespace LuaTest;

// 合法数字应成功解析
static void testValidNumber(TestSuite& suite) {
    Lexer lexer("123.45e-1");
    Token t = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Number), static_cast<int>(t.type), "Valid number token type");
    ASSERT_TRUE(suite, std::holds_alternative<f64>(t.value), "Valid number has numeric value");
}

// 非法数字 123abc 应返回错误
static void testMalformedNumberTrailingId(TestSuite& suite) {
    Lexer lexer("123abc");
    Token t = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Error), static_cast<int>(t.type), "Malformed number returns error token");
    ASSERT_EQ(suite, std::string("123abc"), t.lexeme, "Error token carries offending lexeme");
}

// 非法十六进制 0x1G 应返回错误
static void testMalformedHexTrailingLetter(TestSuite& suite) {
    Lexer lexer("0x1G");
    Token t = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Error), static_cast<int>(t.type), "Malformed hex returns error token");
}

// 长字符串应丢弃起始分隔符后的首行换行
static void testLongStringSkipFirstNewline(TestSuite& suite) {
    Lexer lexer("[[\nabc]]");
    Token t = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::String), static_cast<int>(t.type), "Long string token type");
    auto strPtr = std::get_if<Str>(&t.value);
    ASSERT_TRUE(suite, strPtr != nullptr, "Long string has string value");
    if (strPtr) {
        ASSERT_EQ(suite, std::string("abc"), *strPtr, "Leading newline is stripped");
    }
}

// 长注释起始换行应被忽略，后续第一个token行号应为2
static void testLongCommentSkipFirstNewline(TestSuite& suite) {
    Lexer lexer("--[[\ncomment]]123");
    Token t = lexer.nextToken();
    std::string msg = std::string("Number after long comment, got ") + tokenTypeToString(t.type);
    ASSERT_EQ(suite, static_cast<int>(TokenType::Number), static_cast<int>(t.type), msg);
    ASSERT_EQ(suite, 2, t.line, "Line number accounts for stripped first newline");
}

void registerLexerNumberTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest("Lexer Number", "Valid number", testValidNumber);
    registry.registerTest("Lexer Number", "Malformed number trailing id", testMalformedNumberTrailingId);
    registry.registerTest("Lexer Number", "Malformed hex trailing letter", testMalformedHexTrailingLetter);
    registry.registerTest("Lexer Number", "Long string skip first newline", testLongStringSkipFirstNewline);
    registry.registerTest("Lexer Number", "Long comment skip first newline", testLongCommentSkipFirstNewline);
}
