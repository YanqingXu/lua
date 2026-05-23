/**
 * @file test_lexer_number.cpp
 * @brief 验证词法分析器对非法数字的处理
 */

#include "../framework/test_framework.hpp"
#include "compiler/parser/lexer.hpp"
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
    ASSERT_EQ(suite, std::string("0x1G"), t.lexeme, "Malformed hex carries full offending lexeme");
    ASSERT_EQ(suite, std::string("Malformed hexadecimal number"), t.errorMessage, "Malformed hex carries diagnostic");
}

// 有效十六进制整数应按 Lua 5.1 规则解析
static void testValidHexNumbers(TestSuite& suite) {
    Lexer lexer("0x1 0xA 0xAB");

    Token one = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Number), static_cast<int>(one.type), "0x1 token type");
    ASSERT_EQ(suite, 1.0, std::get<f64>(one.value), "0x1 value");

    Token ten = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Number), static_cast<int>(ten.type), "0xA token type");
    ASSERT_EQ(suite, 10.0, std::get<f64>(ten.value), "0xA value");

    Token ab = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Number), static_cast<int>(ab.type), "0xAB token type");
    ASSERT_EQ(suite, 171.0, std::get<f64>(ab.value), "0xAB value");
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

// 单行注释应在 LF、CR、CRLF 或 LFCR 处结束
static void testLineCommentStopsAtCarriageReturn(TestSuite& suite) {
    Lexer lexer("--comment\rb");
    Token t = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), static_cast<int>(t.type), "Name after CR line comment");
    ASSERT_EQ(suite, std::string("b"), t.lexeme, "Name after CR line comment lexeme");
    ASSERT_EQ(suite, 2, t.line, "Name line after CR line comment");
    ASSERT_EQ(suite, 1, t.column, "Name column after CR line comment");
}

// 失败的长字符串探测不能吞掉 [ 后面的 =
static void testFailedLongStringProbeRestoresInput(TestSuite& suite) {
    Lexer lexer("[==x");

    Token left = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(static_cast<TokenType>('[')), static_cast<int>(left.type), "First token is '['");

    Token eq = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Eq), static_cast<int>(eq.type), "'==' is preserved after failed probe");
    ASSERT_EQ(suite, std::string("=="), eq.lexeme, "'==' lexeme after failed probe");

    Token name = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), static_cast<int>(name.type), "Name after failed probe is preserved");
    ASSERT_EQ(suite, std::string("x"), name.lexeme, "Name lexeme after failed probe");
}

// 长字符串中的不匹配结束符应作为内容保留
static void testLongStringMismatchedClosePreservesContent(TestSuite& suite) {
    Lexer lexer("[=[a]==]b]=]");
    Token t = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::String), static_cast<int>(t.type), "Long string token type");
    auto strPtr = std::get_if<Str>(&t.value);
    ASSERT_TRUE(suite, strPtr != nullptr, "Long string has string value");
    if (strPtr) {
        ASSERT_EQ(suite, std::string("a]==]b"), *strPtr, "Mismatched close delimiter is content");
    }
}

// 长字符串正文中的 CRLF/LFCR 应按 Lua 5.1 规范归一为 '\n'
static void testLongStringNormalizesNewlines(TestSuite& suite) {
    Lexer lexer("[[a\r\nb\n\rc]]");
    Token t = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::String), static_cast<int>(t.type), "Long string token type");
    auto strPtr = std::get_if<Str>(&t.value);
    ASSERT_TRUE(suite, strPtr != nullptr, "Long string has string value");
    if (strPtr) {
        ASSERT_EQ(suite, std::string("a\nb\nc"), *strPtr, "Long string newlines are normalized");
    }
}

// 未闭合长注释应返回词法错误，而不是静默 EOF
static void testUnterminatedLongComment(TestSuite& suite) {
    Lexer lexer("--[[abc");
    Token t = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Error), static_cast<int>(t.type), "Unterminated long comment returns error");
    ASSERT_EQ(suite, std::string("Unterminated long comment"), t.errorMessage, "Long comment diagnostic");
    ASSERT_EQ(suite, 1, t.line, "Long comment error start line");
    ASSERT_EQ(suite, 1, t.column, "Long comment error start column");
}

// Lua 5.1 将 CRLF 和 LFCR 都视为单个换行
static void testCrLfAndLfCrLineColumns(TestSuite& suite) {
    Lexer lexer("a\r\nb\n\rc");

    Token a = lexer.nextToken();
    ASSERT_EQ(suite, 1, a.line, "a line");
    ASSERT_EQ(suite, 1, a.column, "a column");

    Token b = lexer.nextToken();
    ASSERT_EQ(suite, 2, b.line, "b line after CRLF");
    ASSERT_EQ(suite, 1, b.column, "b column after CRLF");

    Token c = lexer.nextToken();
    ASSERT_EQ(suite, 3, c.line, "c line after LFCR");
    ASSERT_EQ(suite, 1, c.column, "c column after LFCR");
}

// 普通 token 的列号不应再依赖 lexeme 长度反推
static void testTokenColumnAfterNewlineAndSpaces(TestSuite& suite) {
    Lexer lexer("a\n  b");
    (void)lexer.nextToken();

    Token b = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), static_cast<int>(b.type), "b token type");
    ASSERT_EQ(suite, 2, b.line, "b line");
    ASSERT_EQ(suite, 3, b.column, "b column");
}

// 短字符串中的反斜杠换行支持 CRLF，原始换行仍报错
static void testShortStringEscapedCrLfAndRawCr(TestSuite& suite) {
    Lexer escaped("'a\\\r\nb'");
    Token ok = escaped.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::String), static_cast<int>(ok.type), "Escaped CRLF string token type");
    auto strPtr = std::get_if<Str>(&ok.value);
    ASSERT_TRUE(suite, strPtr != nullptr, "Escaped CRLF string has value");
    if (strPtr) {
        ASSERT_EQ(suite, std::string("a\nb"), *strPtr, "Escaped CRLF is normalized");
    }

    Lexer raw("'a\rb'");
    Token error = raw.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Error), static_cast<int>(error.type), "Raw CR in short string is error");
    ASSERT_EQ(suite, std::string("Unterminated string"), error.errorMessage, "Raw CR diagnostic");
}

// 输入中真实的 NUL 字节不应被游标包装误判为 EOF
static void testEmbeddedNullByteIsNotEof(TestSuite& suite) {
    Str source("a\0b", 3);
    Lexer lexer(source);

    Token a = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), static_cast<int>(a.type), "Name before NUL");
    ASSERT_EQ(suite, std::string("a"), a.lexeme, "Name before NUL lexeme");

    Token nul = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Error), static_cast<int>(nul.type), "NUL byte is an unexpected character");
    ASSERT_EQ(suite, static_cast<usize>(1), nul.lexeme.size(), "NUL lexeme has one byte");
    ASSERT_TRUE(suite, nul.lexeme[0] == '\0', "NUL lexeme preserves byte");

    Token b = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), static_cast<int>(b.type), "Name after NUL");
    ASSERT_EQ(suite, std::string("b"), b.lexeme, "Name after NUL lexeme");
}

void registerLexerNumberTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest("Lexer Number", "Valid number", testValidNumber);
    registry.registerTest("Lexer Number", "Malformed number trailing id", testMalformedNumberTrailingId);
    registry.registerTest("Lexer Number", "Malformed hex trailing letter", testMalformedHexTrailingLetter);
    registry.registerTest("Lexer Number", "Valid hex numbers", testValidHexNumbers);
    registry.registerTest("Lexer Number", "Long string skip first newline", testLongStringSkipFirstNewline);
    registry.registerTest("Lexer Number", "Long comment skip first newline", testLongCommentSkipFirstNewline);
    registry.registerTest("Lexer Number", "Line comment stops at CR", testLineCommentStopsAtCarriageReturn);
    registry.registerTest("Lexer Number", "Failed long string probe restores input", testFailedLongStringProbeRestoresInput);
    registry.registerTest("Lexer Number", "Long string mismatched close preserves content", testLongStringMismatchedClosePreservesContent);
    registry.registerTest("Lexer Number", "Long string normalizes newlines", testLongStringNormalizesNewlines);
    registry.registerTest("Lexer Number", "Unterminated long comment", testUnterminatedLongComment);
    registry.registerTest("Lexer Number", "CRLF and LFCR line columns", testCrLfAndLfCrLineColumns);
    registry.registerTest("Lexer Number", "Token column after newline and spaces", testTokenColumnAfterNewlineAndSpaces);
    registry.registerTest("Lexer Number", "Short string CRLF handling", testShortStringEscapedCrLfAndRawCr);
    registry.registerTest("Lexer Number", "Embedded NUL byte is not EOF", testEmbeddedNullByteIsNotEof);
}
