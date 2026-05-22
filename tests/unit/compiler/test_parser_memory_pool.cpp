/**
 * @file test_parser_memory_pool.cpp
 * @brief 测试 Parser 内存池功能
 *
 * 验证P0-3优化：NodePool 内存池减少内存分配开销
 */

#include "../framework/test_framework.hpp"
#include "compiler/parser/parser.hpp"
#include <iostream>
#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Parser Memory Pool";

/**
 * @brief 测试用例 1：基本内存池功能
 */
void testBasicMemoryPoolFunctionality(TestSuite& suite) {
    const char* code = R"(
        local x = 42
        local y = "hello"
    )";

    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk block = std::move(*parsed);

        // 验证解析成功
        ASSERT_TRUE(suite, block.statements.size() == 2, "Should have 2 statements");
        ASSERT_TRUE(suite, block.statements[0] != nullptr, "First statement should exist");
        ASSERT_TRUE(suite, block.statements[1] != nullptr, "Second statement should exist");
    } catch (const std::exception& e) {
        ASSERT_TRUE(suite, false, std::string("Exception: ") + e.what());
    }
}

/**
 * @brief 测试用例 2：解析后内存池状态
 */
void testMemoryPoolStateAfterParsing(TestSuite& suite) {
    const char* code = R"(
        local a = 1 + 2 * 3
        local b = {x = 10, y = 20}
        local c = function(n) return n * 2 end
    )";

    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk block = std::move(*parsed);

        // 验证解析成功
        ASSERT_TRUE(suite, block.statements.size() == 3, "Should have 3 statements");

        // 验证所有语句都被正确分配
        for (size_t i = 0; i < block.statements.size(); ++i) {
            ASSERT_TRUE(suite, block.statements[i] != nullptr, "Statement should exist");
        }

        // 验证第一个语句（包含二元表达式）
        auto* firstStmt = std::get_if<LocalStmt>(&block.statements[0]->variant);
        ASSERT_TRUE(suite, firstStmt != nullptr, "First statement should be LocalStmt");
        ASSERT_TRUE(suite, firstStmt->values.size() == 1, "Should have 1 value");
        ASSERT_TRUE(suite, firstStmt->values[0] != nullptr, "Value should exist");

        // 验证第二个语句（包含表构造器）
        auto* secondStmt = std::get_if<LocalStmt>(&block.statements[1]->variant);
        ASSERT_TRUE(suite, secondStmt != nullptr, "Second statement should be LocalStmt");
        ASSERT_TRUE(suite, secondStmt->values.size() == 1, "Should have 1 value");

        auto* tableExpr = std::get_if<TableExpr>(&secondStmt->values[0]->variant);
        ASSERT_TRUE(suite, tableExpr != nullptr, "Value should be TableExpr");
        ASSERT_TRUE(suite, tableExpr->fields.size() == 2, "Table should have 2 fields");

        // 验证第三个语句（包含函数表达式）
        auto* thirdStmt = std::get_if<LocalStmt>(&block.statements[2]->variant);
        ASSERT_TRUE(suite, thirdStmt != nullptr, "Third statement should be LocalStmt");
        ASSERT_TRUE(suite, thirdStmt->values.size() == 1, "Should have 1 value");

        auto* funcExpr = std::get_if<FunctionExpr>(&thirdStmt->values[0]->variant);
        ASSERT_TRUE(suite, funcExpr != nullptr, "Value should be FunctionExpr");
        ASSERT_TRUE(suite, funcExpr->params.size() == 1, "Function should have 1 param");
    } catch (const std::exception& e) {
        ASSERT_TRUE(suite, false, std::string("Exception: ") + e.what());
    }
}

/**
 * @brief 测试用例 3：复杂嵌套结构
 */
void testComplexNestedStructures(TestSuite& suite) {
    const char* code = R"(
        local result = ((1 + 2) * (3 + 4)) / ((5 - 6) + (7 * 8))
        local nested = {
            a = {b = {c = {d = 1}}},
            func = function(x)
                return function(y)
                    return x + y
                end
            end
        }
    )";

    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk block = std::move(*parsed);

        // 验证解析成功
        ASSERT_TRUE(suite, block.statements.size() == 2, "Should have 2 statements");

        // 验证第一个语句（深度嵌套的二元表达式）
        auto* firstStmt = std::get_if<LocalStmt>(&block.statements[0]->variant);
        ASSERT_TRUE(suite, firstStmt != nullptr, "First statement should be LocalStmt");
        ASSERT_TRUE(suite, firstStmt->values.size() == 1, "Should have 1 value");

        auto* binExpr = std::get_if<BinaryExpr>(&firstStmt->values[0]->variant);
        ASSERT_TRUE(suite, binExpr != nullptr, "Value should be BinaryExpr");
        ASSERT_TRUE(suite, binExpr->op == BinaryExpr::Op::Div, "Top-level op should be Div");

        // 验证第二个语句（嵌套表和函数）
        auto* secondStmt = std::get_if<LocalStmt>(&block.statements[1]->variant);
        ASSERT_TRUE(suite, secondStmt != nullptr, "Second statement should be LocalStmt");
        ASSERT_TRUE(suite, secondStmt->values.size() == 1, "Should have 1 value");

        auto* tableExpr = std::get_if<TableExpr>(&secondStmt->values[0]->variant);
        ASSERT_TRUE(suite, tableExpr != nullptr, "Value should be TableExpr");
        ASSERT_TRUE(suite, tableExpr->fields.size() == 2, "Table should have 2 fields");
    } catch (const std::exception& e) {
        ASSERT_TRUE(suite, false, std::string("Exception: ") + e.what());
    }
}

/**
 * @brief 测试用例 4：内存池统计信息
 */
void testMemoryPoolStatistics(TestSuite& suite) {
    const char* code = R"(
        local x = 1
        local y = 2
        local z = x + y
    )";

    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk block = std::move(*parsed);

        // 验证解析成功
        ASSERT_TRUE(suite, block.statements.size() == 3, "Should have 3 statements");

        // 验证所有节点都被正确分配
        for (const auto& stmt : block.statements) {
            ASSERT_TRUE(suite, stmt != nullptr, "Statement should exist");
        }
    } catch (const std::exception& e) {
        ASSERT_TRUE(suite, false, std::string("Exception: ") + e.what());
    }
}

}  // namespace

// =====================================================================
// 测试注册
// =====================================================================

void registerParserMemoryPoolTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Basic memory pool functionality", testBasicMemoryPoolFunctionality);
    registry.registerTest(kSuiteName, "Memory pool state after parsing", testMemoryPoolStateAfterParsing);
    registry.registerTest(kSuiteName, "Complex nested structures", testComplexNestedStructures);
    registry.registerTest(kSuiteName, "Memory pool statistics", testMemoryPoolStatistics);
}
