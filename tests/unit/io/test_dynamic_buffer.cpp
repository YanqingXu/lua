/**
 * @file test_dynamic_buffer.cpp
 * @brief DynamicBuffer 类单元测试
 * 
 * 测试 DynamicBuffer 的所有功能，包括：
 * - 构造和析构
 * - 字符和字符串追加
 * - 零拷贝视图
 * - 移动语义
 * - 内存管理（clear, reset, reserve）
 * - 边界条件和大数据量
 */

#include "../framework/test_framework.hpp"
#include "io/dynamic_buffer.hpp"
#include <string>

using namespace Lua::IO;
using namespace LuaTest;

// =====================================================================
// 基础功能测试
// =====================================================================

/**
 * @brief 测试默认构造函数
 */
static void testDefaultConstruction(TestSuite& suite) {
    DynamicBuffer buf;
    ASSERT_TRUE(suite, buf.empty(), "Default constructed buffer should be empty");
    ASSERT_EQ(suite, static_cast<size_t>(0), buf.size(), "Default constructed buffer size should be 0");
}

/**
 * @brief 测试追加单个字符
 */
static void testAppendChar(TestSuite& suite) {
    DynamicBuffer buf;
    buf.append('a');
    buf.append('b');
    buf.append('c');
    
    ASSERT_EQ(suite, static_cast<size_t>(3), buf.size(), "Size should be 3 after appending 3 chars");
    ASSERT_FALSE(suite, buf.empty(), "Buffer should not be empty");
    
    std::string_view view = buf.view();
    ASSERT_EQ(suite, std::string("abc"), std::string(view), "Content should be 'abc'");
}

/**
 * @brief 测试追加字符串
 */
static void testAppendString(TestSuite& suite) {
    DynamicBuffer buf;
    buf.append("hello");
    buf.append(" ");
    buf.append("world");
    
    ASSERT_EQ(suite, static_cast<size_t>(11), buf.size(), "Size should be 11");
    
    std::string_view view = buf.view();
    ASSERT_EQ(suite, std::string("hello world"), std::string(view), "Content should be 'hello world'");
}

/**
 * @brief 测试混合追加（字符和字符串）
 */
static void testMixedAppend(TestSuite& suite) {
    DynamicBuffer buf;
    buf.append('L');
    buf.append("ua");
    buf.append(' ');
    buf.append("5.1");
    
    std::string_view view = buf.view();
    ASSERT_EQ(suite, std::string("Lua 5.1"), std::string(view), "Content should be 'Lua 5.1'");
}

// =====================================================================
// 视图和转换测试
// =====================================================================

/**
 * @brief 测试零拷贝视图
 */
static void testView(TestSuite& suite) {
    DynamicBuffer buf;
    buf.append("test");
    
    std::string_view view1 = buf.view();
    std::string_view view2 = buf.view();
    
    // 视图应该指向相同的数据
    ASSERT_EQ(suite, view1.data(), view2.data(), "Multiple views should point to same data");
    ASSERT_EQ(suite, std::string("test"), std::string(view1), "View content should be 'test'");
}

/**
 * @brief 测试 toString 移动语义
 */
static void testToString(TestSuite& suite) {
    DynamicBuffer buf;
    buf.append("hello");
    
    std::string str = std::move(buf).toString();
    ASSERT_EQ(suite, std::string("hello"), str, "toString should return 'hello'");
}

// =====================================================================
// 内存管理测试
// =====================================================================

/**
 * @brief 测试 clear（保留容量）
 */
static void testClear(TestSuite& suite) {
    DynamicBuffer buf;
    buf.append("test data");
    
    size_t cap = buf.capacity();
    buf.clear();
    
    ASSERT_TRUE(suite, buf.empty(), "Buffer should be empty after clear");
    ASSERT_EQ(suite, static_cast<size_t>(0), buf.size(), "Size should be 0 after clear");
    ASSERT_EQ(suite, cap, buf.capacity(), "Capacity should be preserved after clear");
}

/**
 * @brief 测试 reset（释放内存）
 */
static void testReset(TestSuite& suite) {
    DynamicBuffer buf;
    buf.append("test data");
    
    buf.reset();
    
    ASSERT_TRUE(suite, buf.empty(), "Buffer should be empty after reset");
    ASSERT_EQ(suite, static_cast<size_t>(0), buf.size(), "Size should be 0 after reset");
    ASSERT_EQ(suite, static_cast<size_t>(0), buf.capacity(), "Capacity should be 0 after reset");
}

/**
 * @brief 测试 reserve（预留空间）
 */
static void testReserve(TestSuite& suite) {
    DynamicBuffer buf;
    buf.reserve(1000);
    
    ASSERT_TRUE(suite, buf.capacity() >= 1000, "Capacity should be at least 1000");
    ASSERT_EQ(suite, static_cast<size_t>(0), buf.size(), "Size should still be 0 after reserve");
    ASSERT_TRUE(suite, buf.empty(), "Buffer should still be empty after reserve");
}

// =====================================================================
// 移动语义测试
// =====================================================================

/**
 * @brief 测试移动构造函数
 */
static void testMoveConstruction(TestSuite& suite) {
    DynamicBuffer buf1;
    buf1.append("hello");
    
    DynamicBuffer buf2 = std::move(buf1);
    
    ASSERT_EQ(suite, std::string("hello"), std::string(buf2.view()), "Moved buffer should contain 'hello'");
    ASSERT_TRUE(suite, buf1.empty(), "Source buffer should be empty after move");
}

/**
 * @brief 测试移动赋值运算符
 */
static void testMoveAssignment(TestSuite& suite) {
    DynamicBuffer buf1;
    buf1.append("hello");
    
    DynamicBuffer buf2;
    buf2.append("world");
    
    buf2 = std::move(buf1);
    
    ASSERT_EQ(suite, std::string("hello"), std::string(buf2.view()), "Moved buffer should contain 'hello'");
}

// =====================================================================
// 边界条件和特殊情况测试
// =====================================================================

/**
 * @brief 测试空字符串追加
 */
static void testAppendEmptyString(TestSuite& suite) {
    DynamicBuffer buf;
    buf.append("");

    ASSERT_TRUE(suite, buf.empty(), "Appending empty string should keep buffer empty");
    ASSERT_EQ(suite, static_cast<size_t>(0), buf.size(), "Size should be 0");
}

/**
 * @brief 测试空缓冲区的 view
 */
static void testEmptyView(TestSuite& suite) {
    DynamicBuffer buf;
    std::string_view view = buf.view();

    ASSERT_EQ(suite, static_cast<size_t>(0), view.size(), "Empty buffer view size should be 0");
    ASSERT_TRUE(suite, view.empty(), "Empty buffer view should be empty");
}

/**
 * @brief 测试空缓冲区的 toString
 */
static void testEmptyToString(TestSuite& suite) {
    DynamicBuffer buf;
    std::string str = std::move(buf).toString();

    ASSERT_TRUE(suite, str.empty(), "Empty buffer toString should return empty string");
}

/**
 * @brief 测试包含空字符的数据
 */
static void testNullCharacter(TestSuite& suite) {
    DynamicBuffer buf;
    buf.append('a');
    buf.append('\0');
    buf.append('b');

    ASSERT_EQ(suite, static_cast<size_t>(3), buf.size(), "Size should be 3 including null char");

    std::string_view view = buf.view();
    ASSERT_EQ(suite, static_cast<size_t>(3), view.size(), "View size should be 3");
    ASSERT_EQ(suite, 'a', view[0], "First char should be 'a'");
    ASSERT_EQ(suite, '\0', view[1], "Second char should be null");
    ASSERT_EQ(suite, 'b', view[2], "Third char should be 'b'");
}

// =====================================================================
// 性能和大数据量测试
// =====================================================================

/**
 * @brief 测试大量字符追加
 */
static void testLargeData(TestSuite& suite) {
    DynamicBuffer buf;

    // 追加 10000 个字符
    for (int i = 0; i < 10000; ++i) {
        buf.append('x');
    }

    ASSERT_EQ(suite, static_cast<size_t>(10000), buf.size(), "Size should be 10000");

    std::string_view view = buf.view();
    ASSERT_EQ(suite, static_cast<size_t>(10000), view.size(), "View size should be 10000");

    // 验证所有字符都是 'x'
    bool allX = true;
    for (size_t i = 0; i < view.size(); ++i) {
        if (view[i] != 'x') {
            allX = false;
            break;
        }
    }
    ASSERT_TRUE(suite, allX, "All characters should be 'x'");
}

/**
 * @brief 测试大字符串追加
 */
static void testLargeStringAppend(TestSuite& suite) {
    DynamicBuffer buf;

    std::string largeStr(5000, 'y');
    buf.append(largeStr);
    buf.append(largeStr);

    ASSERT_EQ(suite, static_cast<size_t>(10000), buf.size(), "Size should be 10000");

    std::string_view view = buf.view();
    ASSERT_EQ(suite, 'y', view[0], "First char should be 'y'");
    ASSERT_EQ(suite, 'y', view[9999], "Last char should be 'y'");
}

/**
 * @brief 测试预留空间后追加
 */
static void testReserveAndAppend(TestSuite& suite) {
    DynamicBuffer buf;
    buf.reserve(100);

    for (int i = 0; i < 50; ++i) {
        buf.append('a');
    }

    ASSERT_EQ(suite, static_cast<size_t>(50), buf.size(), "Size should be 50");
    ASSERT_TRUE(suite, buf.capacity() >= 100, "Capacity should still be at least 100");
}

// =====================================================================
// 实际使用场景测试
// =====================================================================

/**
 * @brief 测试词法分析器场景：累积标识符
 */
static void testLexerIdentifierScenario(TestSuite& suite) {
    DynamicBuffer buf;

    // 模拟读取标识符 "variable_name_123"
    const char* identifier = "variable_name_123";
    for (const char* p = identifier; *p != '\0'; ++p) {
        buf.append(*p);
    }

    std::string_view view = buf.view();
    ASSERT_EQ(suite, std::string("variable_name_123"), std::string(view), "Identifier should match");

    // 清空并重用
    buf.clear();
    buf.append("another_var");

    ASSERT_EQ(suite, std::string("another_var"), std::string(buf.view()), "Reused buffer should work");
}

/**
 * @brief 测试词法分析器场景：累积字符串字面量
 */
static void testLexerStringLiteralScenario(TestSuite& suite) {
    DynamicBuffer buf;

    // 模拟读取字符串 "hello\nworld"
    buf.append("hello");
    buf.append('\n');
    buf.append("world");

    std::string str = std::move(buf).toString();
    ASSERT_EQ(suite, std::string("hello\nworld"), str, "String literal should include newline");
}

/**
 * @brief 测试多次 clear 和重用
 */
static void testMultipleClearAndReuse(TestSuite& suite) {
    DynamicBuffer buf;

    for (int i = 0; i < 5; ++i) {
        buf.append("test");
        ASSERT_EQ(suite, static_cast<size_t>(4), buf.size(), "Size should be 4");
        buf.clear();
        ASSERT_TRUE(suite, buf.empty(), "Buffer should be empty after clear");
    }
}

// =====================================================================
// 测试注册函数
// =====================================================================

/**
 * @brief 注册所有 DynamicBuffer 测试
 */
void registerDynamicBufferTests() {
    auto& registry = TestRegistry::getInstance();

    // 基础功能测试
    registry.registerTest("DynamicBuffer", "Default construction", testDefaultConstruction);
    registry.registerTest("DynamicBuffer", "Append char", testAppendChar);
    registry.registerTest("DynamicBuffer", "Append string", testAppendString);
    registry.registerTest("DynamicBuffer", "Mixed append", testMixedAppend);

    // 视图和转换测试
    registry.registerTest("DynamicBuffer", "View (zero-copy)", testView);
    registry.registerTest("DynamicBuffer", "toString (move semantics)", testToString);

    // 内存管理测试
    registry.registerTest("DynamicBuffer", "Clear (preserve capacity)", testClear);
    registry.registerTest("DynamicBuffer", "Reset (release memory)", testReset);
    registry.registerTest("DynamicBuffer", "Reserve", testReserve);

    // 移动语义测试
    registry.registerTest("DynamicBuffer", "Move construction", testMoveConstruction);
    registry.registerTest("DynamicBuffer", "Move assignment", testMoveAssignment);

    // 边界条件测试
    registry.registerTest("DynamicBuffer", "Append empty string", testAppendEmptyString);
    registry.registerTest("DynamicBuffer", "Empty view", testEmptyView);
    registry.registerTest("DynamicBuffer", "Empty toString", testEmptyToString);
    registry.registerTest("DynamicBuffer", "Null character", testNullCharacter);

    // 性能和大数据量测试
    registry.registerTest("DynamicBuffer", "Large data (10000 chars)", testLargeData);
    registry.registerTest("DynamicBuffer", "Large string append", testLargeStringAppend);
    registry.registerTest("DynamicBuffer", "Reserve and append", testReserveAndAppend);

    // 实际使用场景测试
    registry.registerTest("DynamicBuffer", "Lexer identifier scenario", testLexerIdentifierScenario);
    registry.registerTest("DynamicBuffer", "Lexer string literal scenario", testLexerStringLiteralScenario);
    registry.registerTest("DynamicBuffer", "Multiple clear and reuse", testMultipleClearAndReuse);
}



