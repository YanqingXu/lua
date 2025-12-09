/**
 * @file test_parser_error_recovery.cpp
 * @brief 测试Parser错误报告机制
 *
 * 验证P0-2优化基础：ParseError 包含行号和列号信息
 * 注意：完整的错误恢复机制（支持一次性报告多个错误）需要更复杂的实现，
 * 目前暂时使用简化版本（遇到第一个错误就抛出异常）
 *
 * 错误消息格式已简化为与官方 Lua 5.1.5 保持一致：
 * - 错误消息本身只包含简洁的描述（如 "syntax error"、"Expected 'then'"）
 * - 位置信息通过 ParseError::getLine() 和 getColumn() 获取
 * - 完整格式由调用者组装：progname: source:line: message
 */

#include "../framework/test_framework.hpp"
#include "compiler/parser.hpp"
#include <iostream>
#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Parser Error Reporting";

/**
 * @brief 测试语法错误能被正确捕获和报告
 */
void testSyntaxErrorReporting(TestSuite& suite) {
    // 使用无效的表达式
    std::string code = "local x = +";  // 缺少操作数

    try {
        Parser parser(code);
        Chunk chunk = parser.parse();
        ASSERT_TRUE(suite, false, "Should throw ParseError");
    } catch (const ParseError& e) {
        // 验证 ParseError 包含位置信息（通过 getLine/getColumn 方法）
        bool hasValidLine = e.getLine() >= 1;
        bool hasValidColumn = e.getColumn() >= 1;
        ASSERT_TRUE(suite, hasValidLine && hasValidColumn, "Error has location info");
    } catch (...) {
        ASSERT_TRUE(suite, false, "Unexpected exception type");
    }
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

    try {
        Parser parser(code);
        Chunk chunk = parser.parse();
        ASSERT_TRUE(suite, false, "Should throw ParseError");
    } catch (const ParseError& e) {
        std::string errorMsg = e.what();

        // 验证错误消息不为空
        bool hasMessage = !errorMsg.empty();
        ASSERT_TRUE(suite, hasMessage, "Error message is not empty");

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

    try {
        Parser parser(code);
        Chunk chunk = parser.parse();
        // 正常代码应该成功解析
        ASSERT_TRUE(suite, true, "Normal code parses successfully");
    } catch (const ParseError& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        ASSERT_TRUE(suite, false, "Normal code should not throw");
    }
}

/**
 * @brief 测试未闭合的括号错误
 */
void testUnclosedParenthesis(TestSuite& suite) {
    std::string code = "local x = (1 + 2";  // 缺少右括号

    try {
        Parser parser(code);
        Chunk chunk = parser.parse();
        ASSERT_TRUE(suite, false, "Should throw ParseError");
    } catch (const ParseError& e) {
        // 验证抛出了 ParseError 并包含有效的位置信息
        bool hasValidLocation = e.getLine() >= 1 && e.getColumn() >= 1;
        ASSERT_TRUE(suite, hasValidLocation, "Unclosed parenthesis throws error");
    }
}

/**
 * @brief 测试缺少 end 关键字的错误
 */
void testMissingEnd(TestSuite& suite) {
    std::string code = "if true then\n  print(1)";  // 缺少 end

    try {
        Parser parser(code);
        Chunk chunk = parser.parse();
        ASSERT_TRUE(suite, false, "Should throw ParseError");
    } catch (const ParseError& e) {
        std::string errorMsg = e.what();
        // 验证错误消息包含有意义的描述
        bool hasError = !errorMsg.empty() &&
                        (errorMsg.find("Expected") != std::string::npos ||
                         errorMsg.find("expected") != std::string::npos ||
                         errorMsg.find("syntax") != std::string::npos ||
                         errorMsg.find("<eof>") != std::string::npos);
        ASSERT_TRUE(suite, hasError, "Missing end throws error");
    }
}

} // namespace

/**
 * @brief 注册错误报告测试
 */
void registerParserErrorRecoveryTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "syntax error reporting", testSyntaxErrorReporting);
    registry.registerTest(kSuiteName, "error message format", testErrorMessageFormat);
    registry.registerTest(kSuiteName, "normal code parsing", testNormalCodeParsing);
    registry.registerTest(kSuiteName, "unclosed parenthesis", testUnclosedParenthesis);
    registry.registerTest(kSuiteName, "missing end", testMissingEnd);
}

