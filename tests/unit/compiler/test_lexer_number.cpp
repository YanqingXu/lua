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

void registerLexerTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest("Lexer", "Valid number", testValidNumber);
    registry.registerTest("Lexer", "Malformed number trailing id", testMalformedNumberTrailingId);
    registry.registerTest("Lexer", "Malformed hex trailing letter", testMalformedHexTrailingLetter);
    registry.registerTest("Lexer", "Long string skip first newline", testLongStringSkipFirstNewline);
}
