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

void registerLexerTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest("Lexer", "Valid number", testValidNumber);
    registry.registerTest("Lexer", "Malformed number trailing id", testMalformedNumberTrailingId);
}
