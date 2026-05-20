/**
 * @file test_parser_error_recovery.cpp
 * @brief 测试Parser错误报告机制
 *
 * 验证P0-2优化基础：ParseError 包含行号和列号信息
 * 注意：完整的错误恢复机制（支持一次性报告多个错误）需要更复杂的实现，
 * 目前暂时使用简化版本（遇到第一个错误就返回 ParseError）
 *
 * 错误消息格式已简化为与官方 Lua 5.1.5 保持一致：
 * - 错误消息本身只包含简洁的描述（如 "syntax error"、"Expected 'then'"）
 * - 位置信息通过 ParseError::getLine() 和 getColumn() 获取
 * - 完整格式由调用者组装：progname: source:line: message
 */

#include "../framework/test_framework.hpp"
#include "common/lua_error.hpp"
#include "compiler/parser.hpp"
#include <expected>
#include <string>
#include <type_traits>
#include <utility>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Parser Error Reporting";

void testParseReturnsExpectedType(TestSuite& suite) {
    using ParseResult = decltype(std::declval<Parser&>().parse());
    bool hasExpectedSignature = std::is_same_v<ParseResult, std::expected<Chunk, ParseError>>;
    ASSERT_TRUE(suite, hasExpectedSignature, "parse returns expected chunk or parse error");
}

void testParseExpectedFailureValue(TestSuite& suite) {
    Parser parser("local x = +");
    auto parsed = parser.parse();

    ASSERT_TRUE(suite, !parsed.has_value(), "invalid input returns parse error");
    if (parsed.has_value()) {
        return;
    }

    const ParseError& error = parsed.error();
    ASSERT_TRUE(suite, error.getLine() >= 1, "parse error has line info");
    ASSERT_TRUE(suite, error.getColumn() >= 1, "parse error has column info");
}

/**
 * @brief 测试语法错误能被正确捕获和报告
 */
void testSyntaxErrorReporting(TestSuite& suite) {
    // 使用无效的表达式
    std::string code = "local x = +";  // 缺少操作数

    Parser parser(code);
    auto parsed = parser.parse();
    ASSERT_TRUE(suite, !parsed.has_value(), "Expected ParseError");
    if (parsed) {
        return;
    }

    const ParseError& e = parsed.error();
    bool hasValidLine = e.getLine() >= 1;
    bool hasValidColumn = e.getColumn() >= 1;
    ASSERT_TRUE(suite, hasValidLine && hasValidColumn, "Error has location info");
}

/**
 * @brief 测试错误信息格式包含正确的行号、列号和描述
 */
void testErrorMessageFormat(TestSuite& suite) {
    // 缺少 then 关键字
    std::string code =
        "local x = 1\n"
        "if x > 0\n"
        "  print(x)\n"
        "end\n";

    Parser parser(code);
    auto parsed = parser.parse();
    ASSERT_TRUE(suite, !parsed.has_value(), "Expected ParseError");
    if (parsed) {
        return;
    }

    const ParseError& e = parsed.error();
    std::string errorMsg = e.what();

    // 验证错误消息不为空
    ASSERT_TRUE(suite, !errorMsg.empty(), "Error message is not empty");

    // 验证 ParseError 对象包含位置信息
    i32 line = e.getLine();
    i32 column = e.getColumn();
    ASSERT_TRUE(suite, line >= 1, "Line number is valid");
    ASSERT_TRUE(suite, column >= 1, "Column number is valid");

    // 验证错误消息包含有意义的描述
    bool hasExpected = errorMsg.find("Expected") != std::string::npos ||
                       errorMsg.find("expected") != std::string::npos ||
                       errorMsg.find("syntax") != std::string::npos ||
                       errorMsg.find("Unexpected") != std::string::npos;
    ASSERT_TRUE(suite, hasExpected, "Error message has meaningful description");
}

/**
 * @brief 测试正常代码能正确解析
 */
void testNormalCodeParsing(TestSuite& suite) {
    std::string code =
        "local x = 1\n"
        "if x > 0 then\n"
        "  print(x)\n"
        "end\n";

    Parser parser(code);
    auto parsed = parser.parse();
    ASSERT_TRUE(suite, parsed.has_value(), "Normal code parses successfully");
}

/**
 * @brief 测试未闭合的括号错误
 */
void testUnclosedParenthesis(TestSuite& suite) {
    std::string code = "local x = (1 + 2";  // 缺少右括号

    Parser parser(code);
    auto parsed = parser.parse();
    ASSERT_TRUE(suite, !parsed.has_value(), "Expected ParseError");
    if (parsed) {
        return;
    }

    const ParseError& e = parsed.error();
    bool hasValidLocation = e.getLine() >= 1 && e.getColumn() >= 1;
    ASSERT_TRUE(suite, hasValidLocation, "Unclosed parenthesis returns error");
}

/**
 * @brief 测试缺少 end 关键字的错误
 */
void testMissingEnd(TestSuite& suite) {
    std::string code = "if true then\n  print(1)";  // 缺少 end

    Parser parser(code);
    auto parsed = parser.parse();
    ASSERT_TRUE(suite, !parsed.has_value(), "Expected ParseError");
    if (parsed) {
        return;
    }

    const ParseError& e = parsed.error();
    std::string errorMsg = e.what();
    // 验证错误消息包含有意义的描述
    bool hasError = !errorMsg.empty() &&
                    (errorMsg.find("Expected") != std::string::npos ||
                     errorMsg.find("expected") != std::string::npos ||
                     errorMsg.find("syntax") != std::string::npos ||
                     errorMsg.find("<eof>") != std::string::npos);
    ASSERT_TRUE(suite, hasError, "Missing end returns error");
}

/**
 * @brief 测试解析、运行时、内存异常统一继承 LuaError。
 */
void testUnifiedErrorHierarchy(TestSuite& suite) {
    ASSERT_TRUE(suite, (std::is_base_of<LuaError, ParseError>::value),
                "ParseError derives from LuaError");
    ASSERT_TRUE(suite, (std::is_base_of<LuaError, RuntimeError>::value),
                "RuntimeError derives from LuaError");
    ASSERT_TRUE(suite, (std::is_base_of<RuntimeError, MemoryError>::value),
                "MemoryError derives from RuntimeError");
    ASSERT_TRUE(suite, (std::is_base_of<std::runtime_error, LuaError>::value),
                "LuaError remains compatible with std::runtime_error");
}

} // namespace

/**
 * @brief 注册错误报告测试
 */
void registerParserErrorRecoveryTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "parse returns expected type", testParseReturnsExpectedType);
    registry.registerTest(kSuiteName, "parse expected failure value", testParseExpectedFailureValue);
    registry.registerTest(kSuiteName, "syntax error reporting", testSyntaxErrorReporting);
    registry.registerTest(kSuiteName, "error message format", testErrorMessageFormat);
    registry.registerTest(kSuiteName, "normal code parsing", testNormalCodeParsing);
    registry.registerTest(kSuiteName, "unclosed parenthesis", testUnclosedParenthesis);
    registry.registerTest(kSuiteName, "missing end", testMissingEnd);
    registry.registerTest(kSuiteName, "unified error hierarchy", testUnifiedErrorHierarchy);
}

