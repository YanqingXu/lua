/**
 * @file test_parser_error_recovery.cpp
 * @brief 测试Parser错误报告机制
 *
 * 验证P0-2优化基础：错误信息包含行号和列号
 * 注意：完整的错误恢复机制（支持一次性报告多个错误）需要更复杂的实现，
 * 目前暂时使用简化版本（遇到第一个错误就抛出异常）
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
        std::string errorMsg = e.what();
        // 验证错误信息包含行号和列号
        bool hasLineInfo = errorMsg.find("line") != std::string::npos;
        bool hasColumnInfo = errorMsg.find("column") != std::string::npos;
        ASSERT_TRUE(suite, hasLineInfo && hasColumnInfo, "Error has location info");
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

        // 验证错误信息格式
        bool hasSyntaxError = errorMsg.find("Syntax error") != std::string::npos;
        bool hasLine = errorMsg.find("line") != std::string::npos;
        bool hasColumn = errorMsg.find("column") != std::string::npos;

        ASSERT_TRUE(suite, hasSyntaxError, "Error message contains 'Syntax error'");
        ASSERT_TRUE(suite, hasLine, "Error message contains line number");
        ASSERT_TRUE(suite, hasColumn, "Error message contains column number");

        // 验证行号和列号
        i32 line = e.getLine();
        i32 column = e.getColumn();
        ASSERT_TRUE(suite, line >= 1, "Line number is valid");
        ASSERT_TRUE(suite, column >= 1, "Column number is valid");
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
        std::string errorMsg = e.what();
        bool hasError = errorMsg.find("Syntax error") != std::string::npos;
        ASSERT_TRUE(suite, hasError, "Unclosed parenthesis throws error");
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
        bool hasError = errorMsg.find("Syntax error") != std::string::npos ||
                        errorMsg.find("Expected") != std::string::npos;
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

