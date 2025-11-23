/**
 * @file test_binary_unary_expr.cpp
 * @brief 测试二元和一元表达式的代码生成
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <iostream>
#include <cassert>

using namespace Lua;
using namespace LuaTest;

void testBinaryArithmetic(TestSuite& suite) {
    // 使用单例获取StringPool
    StringPool& pool = StringPool::getInstance();

    // 测试: local x = 1 + 2
    const char* code = "local x = 1 + 2";
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    // 验证生成的字节码
    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    delete proto;
}

void testBinaryComparison(TestSuite& suite) {
    // 使用单例获取StringPool
    StringPool& pool = StringPool::getInstance();

    // 测试: local x = 1 < 2
    const char* code = "local x = 1 < 2";
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    delete proto;
}

void testBinaryLogical(TestSuite& suite) {
    // 使用单例获取StringPool
    StringPool& pool = StringPool::getInstance();

    // 测试: local x = true and false
    const char* code = "local x = true and false";
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    delete proto;
}

void testUnaryExpressions(TestSuite& suite) {
    // 使用单例获取StringPool
    StringPool& pool = StringPool::getInstance();

    // 测试: local x = -42
    const char* code = "local x = -42";
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    delete proto;
}

void testComplexExpression(TestSuite& suite) {
    // 使用单例获取StringPool
    StringPool& pool = StringPool::getInstance();

    // 测试: local x = (1 + 2) * 3 - 4
    const char* code = "local x = (1 + 2) * 3 - 4";
    Parser parser(code);
    Chunk chunk = parser.parse();

    CodeGenerator codegen(&pool);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");

    delete proto;
}

void registerBinaryUnaryExprTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Binary/Unary Expressions", "Binary Arithmetic", testBinaryArithmetic);
    registry.registerTest("Binary/Unary Expressions", "Binary Comparison", testBinaryComparison);
    registry.registerTest("Binary/Unary Expressions", "Binary Logical", testBinaryLogical);
    registry.registerTest("Binary/Unary Expressions", "Unary Expressions", testUnaryExpressions);
    registry.registerTest("Binary/Unary Expressions", "Complex Expression", testComplexExpression);
}

