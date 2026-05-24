/**
 * @file test_parser_recursion.cpp
 * @brief 测试Parser递归深度限制功能
 * 
 * 验证 RecursionGuard RAII 类防止深度嵌套导致栈溢出
 */

#include "../framework/test_framework.hpp"
#include "compiler/parser/parser.hpp"
#include <iostream>
#include <string>
#include <sstream>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Parser Recursion Depth";

/**
 * @brief 生成深度嵌套的括号表达式
 * @param depth 嵌套深度
 * @return 生成的代码字符串
 */
std::string generateNestedParentheses(int depth) {
    std::ostringstream oss;
    oss << "local x = ";
    
    // 生成左括号
    for (int i = 0; i < depth; ++i) {
        oss << "(";
    }
    
    oss << "42";
    
    // 生成右括号
    for (int i = 0; i < depth; ++i) {
        oss << ")";
    }
    
    return oss.str();
}

/**
 * @brief 生成深度嵌套的if语句
 * @param depth 嵌套深度
 * @return 生成的代码字符串
 */
std::string generateNestedIfStatements(int depth) {
    std::ostringstream oss;
    
    // 生成嵌套的if语句
    for (int i = 0; i < depth; ++i) {
        oss << "if true then\n";
    }
    
    oss << "local x = 1\n";
    
    // 生成对应的end
    for (int i = 0; i < depth; ++i) {
        oss << "end\n";
    }
    
    return oss.str();
}

/**
 * @brief 生成深度嵌套的表构造器
 * @param depth 嵌套深度
 * @return 生成的代码字符串
 */
std::string generateNestedTables(int depth) {
    std::ostringstream oss;
    oss << "local x = ";
    
    // 生成嵌套的表
    for (int i = 0; i < depth; ++i) {
        oss << "{";
    }
    
    oss << "42";
    
    // 生成对应的右括号
    for (int i = 0; i < depth; ++i) {
        oss << "}";
    }
    
    return oss.str();
}

/**
 * @brief 测试正常深度的嵌套（应该成功）
 */
void testNormalDepthNesting(TestSuite& suite) {
    // 测试50层嵌套（低于递归限制）
    std::string code = generateNestedParentheses(50);
    
    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
        ASSERT_TRUE(suite, true, "Normal depth (50) parsing succeeded");
    } catch (const ParseError& e) {
		std::cerr << "ParseError: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        ASSERT_TRUE(suite, false, "Normal depth (50) should not throw");
    }
}

/**
 * @brief 测试接近限制的嵌套（应该成功）
 */
void testNearLimitDepthNesting(TestSuite& suite) {
    // 测试90层嵌套（接近表达式递归限制）
    std::string code = generateNestedParentheses(90);

    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
        ASSERT_TRUE(suite, true, "Near limit depth (90) parsing succeeded");
    } catch (const ParseError& e) {
		std::cerr << "ParseError: " << e.what() << " at line " << e.getLine() << ", column " << e.getColumn() << std::endl;
        ASSERT_TRUE(suite, false, "Near limit depth (90) should not throw");
    }
}

/**
 * @brief 测试超过限制的嵌套（应该抛出异常）
 */
void testExceedLimitDepthNesting(TestSuite& suite) {
    // 测试150层嵌套（超过100的限制）
    std::string code = generateNestedParentheses(150);

    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
        ASSERT_TRUE(suite, false, "Exceed limit depth (150) should throw");
    } catch (const ParseError& e) {
        // 验证错误信息包含"too many syntax levels"
        std::string errorMsg = e.what();
        bool hasCorrectMessage = errorMsg.find("too many syntax levels") != std::string::npos;
        ASSERT_TRUE(suite, hasCorrectMessage, "Exceed limit depth (150) throws correct error");
    }
}

/**
 * @brief 测试深度嵌套的if语句
 */
void testNestedIfStatements(TestSuite& suite) {
    // 测试150层嵌套if语句（超过100的限制）
    std::string code = generateNestedIfStatements(150);

    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
        ASSERT_TRUE(suite, false, "Deeply nested if statements should throw");
    } catch (const ParseError& e) {
        std::string errorMsg = e.what();
        bool hasCorrectMessage = errorMsg.find("too many syntax levels") != std::string::npos;
        ASSERT_TRUE(suite, hasCorrectMessage, "Nested if statements throw correct error");
    }
}

} // namespace

/**
 * @brief 注册递归深度测试
 */
void registerParserRecursionTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest(kSuiteName, "normal depth nesting", testNormalDepthNesting);
    registry.registerTest(kSuiteName, "near limit depth nesting", testNearLimitDepthNesting);
    registry.registerTest(kSuiteName, "exceed limit depth nesting", testExceedLimitDepthNesting);
    registry.registerTest(kSuiteName, "nested if statements", testNestedIfStatements);
}

