/**
 * @file test_input_stream_string.cpp
 * @brief InputStream 类（字符串模式）单元测试
 * 
 * 测试 InputStream 的字符串模式功能，包括：
 * - 字符串模式构造（std::string_view）
 * - 字符读取（getChar, peekChar）
 * - 批量读取（read）
 * - 位置和状态管理
 * - 零拷贝验证
 * - 边界条件
 * - 实际使用场景
 */

#include "../framework/test_framework.hpp"
#include "common/types.hpp"
#include "io/input_stream.hpp"
#include <string>
#include <cstring>

using namespace Lua;
using namespace Lua::IO;
using namespace LuaTest;

// =====================================================================
// 基础功能测试
// =====================================================================

/**
 * @brief 测试字符串模式构造函数
 */
static void testStringConstruction(TestSuite& suite) {
    Str source = "hello";
    InputStream input(source);
    
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF initially");
    ASSERT_EQ(suite, static_cast<size_t>(0), input.getPosition(), "Initial position should be 0");
}

/**
 * @brief 测试空字符串构造
 */
static void testEmptyStringConstruction(TestSuite& suite) {
    Str source = "";
    InputStream input(source);
    
    ASSERT_TRUE(suite, input.isEof(), "Empty string should be EOF immediately");
    ASSERT_EQ(suite, static_cast<size_t>(0), input.getPosition(), "Position should be 0");
}

/**
 * @brief 测试默认源名称
 */
static void testDefaultSourceName(TestSuite& suite) {
    Str source = "test";
    InputStream input(source);
    
    ASSERT_EQ(suite, Str("string"), input.getSourceName(), "Default source name should be 'string'");
}

// =====================================================================
// 字符读取测试
// =====================================================================

/**
 * @brief 测试 getChar 逐字符读取
 */
static void testGetChar(TestSuite& suite) {
    Str source = "abc";
    InputStream input(source);
    
    i32 c1 = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('a'), c1, "First char should be 'a'");
    ASSERT_EQ(suite, static_cast<size_t>(1), input.getPosition(), "Position should be 1");
    
    i32 c2 = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('b'), c2, "Second char should be 'b'");
    ASSERT_EQ(suite, static_cast<size_t>(2), input.getPosition(), "Position should be 2");
    
    i32 c3 = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('c'), c3, "Third char should be 'c'");
    ASSERT_EQ(suite, static_cast<size_t>(3), input.getPosition(), "Position should be 3");
    
    i32 c4 = input.getChar();
    ASSERT_EQ(suite, -1, c4, "Should return -1 at EOF");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all chars");
}

/**
 * @brief 测试 peekChar 前瞻字符
 */
static void testPeekChar(TestSuite& suite) {
    Str source = "xyz";
    InputStream input(source);
    
    i32 peek1 = input.peekChar();
    ASSERT_EQ(suite, static_cast<i32>('x'), peek1, "Peek should return 'x'");
    ASSERT_EQ(suite, static_cast<size_t>(0), input.getPosition(), "Peek should not advance position");
    
    i32 peek2 = input.peekChar();
    ASSERT_EQ(suite, static_cast<i32>('x'), peek2, "Multiple peeks should return same char");
    ASSERT_EQ(suite, static_cast<size_t>(0), input.getPosition(), "Position should still be 0");
    
    i32 c1 = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('x'), c1, "getChar should return peeked char");
    ASSERT_EQ(suite, static_cast<size_t>(1), input.getPosition(), "Position should advance after getChar");
    
    i32 peek3 = input.peekChar();
    ASSERT_EQ(suite, static_cast<i32>('y'), peek3, "Peek should now return 'y'");
}

/**
 * @brief 测试 peekChar 在 EOF
 */
static void testPeekCharAtEof(TestSuite& suite) {
    Str source = "a";
    InputStream input(source);
    
    input.getChar(); // 读取 'a'
    
    i32 peek = input.peekChar();
    ASSERT_EQ(suite, -1, peek, "Peek at EOF should return -1");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF");
}

/**
 * @brief 测试读取到 EOF
 */
static void testReadToEof(TestSuite& suite) {
    Str source = "ab";
    InputStream input(source);
    
    input.getChar(); // 'a'
    input.getChar(); // 'b'
    
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all chars");
    
    i32 c = input.getChar();
    ASSERT_EQ(suite, -1, c, "getChar at EOF should return -1");
    
    i32 c2 = input.getChar();
    ASSERT_EQ(suite, -1, c2, "Multiple getChar at EOF should return -1");
}

// =====================================================================
// 批量读取测试
// =====================================================================

/**
 * @brief 测试 read 批量读取
 */
static void testRead(TestSuite& suite) {
    Str source = "hello world";
    InputStream input(source);
    
    char buffer[6];
    usize bytesRead = input.read(buffer, 5);
    buffer[5] = '\0';
    
    ASSERT_EQ(suite, static_cast<size_t>(5), bytesRead, "Should read 5 bytes");
    ASSERT_EQ(suite, Str("hello"), Str(buffer), "Buffer should contain 'hello'");
    ASSERT_EQ(suite, static_cast<size_t>(5), input.getPosition(), "Position should be 5");
}

/**
 * @brief 测试 read 读取全部数据
 */
static void testReadAll(TestSuite& suite) {
    Str source = "test";
    InputStream input(source);

    char buffer[10];
    usize bytesRead = input.read(buffer, 10);

    ASSERT_EQ(suite, static_cast<size_t>(4), bytesRead, "Should read 4 bytes (all available)");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all");
    ASSERT_EQ(suite, static_cast<size_t>(4), input.getPosition(), "Position should be 4");
}

/**
 * @brief 测试 read 部分读取
 */
static void testReadPartial(TestSuite& suite) {
    Str source = "abcdefgh";
    InputStream input(source);

    char buffer1[4];
    usize bytes1 = input.read(buffer1, 3);
    ASSERT_EQ(suite, static_cast<size_t>(3), bytes1, "First read should get 3 bytes");

    char buffer2[4];
    usize bytes2 = input.read(buffer2, 3);
    ASSERT_EQ(suite, static_cast<size_t>(3), bytes2, "Second read should get 3 bytes");

    ASSERT_EQ(suite, static_cast<size_t>(6), input.getPosition(), "Position should be 6");
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF yet");
}

/**
 * @brief 测试 read 超过可用数据
 */
static void testReadBeyondAvailable(TestSuite& suite) {
    Str source = "abc";
    InputStream input(source);

    char buffer[10];
    usize bytesRead = input.read(buffer, 10);

    ASSERT_EQ(suite, static_cast<size_t>(3), bytesRead, "Should only read 3 bytes (all available)");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF");
}

/**
 * @brief 测试 read 零字节
 */
static void testReadZeroBytes(TestSuite& suite) {
    Str source = "test";
    InputStream input(source);

    char buffer[10];
    usize bytesRead = input.read(buffer, 0);

    ASSERT_EQ(suite, static_cast<size_t>(0), bytesRead, "Should read 0 bytes");
    ASSERT_EQ(suite, static_cast<size_t>(0), input.getPosition(), "Position should not change");
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF");
}

// =====================================================================
// 位置和状态测试
// =====================================================================

/**
 * @brief 测试位置跟踪
 */
static void testPositionTracking(TestSuite& suite) {
    Str source = "12345";
    InputStream input(source);

    ASSERT_EQ(suite, static_cast<size_t>(0), input.getPosition(), "Initial position should be 0");

    input.getChar();
    ASSERT_EQ(suite, static_cast<size_t>(1), input.getPosition(), "Position should be 1");

    input.getChar();
    ASSERT_EQ(suite, static_cast<size_t>(2), input.getPosition(), "Position should be 2");

    char buffer[2];
    input.read(buffer, 2);
    ASSERT_EQ(suite, static_cast<size_t>(4), input.getPosition(), "Position should be 4 after read");
}

/**
 * @brief 测试 EOF 状态
 */
static void testEofState(TestSuite& suite) {
    Str source = "x";
    InputStream input(source);

    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF initially");

    input.getChar();
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all");

    input.getChar(); // 再次读取
    ASSERT_TRUE(suite, input.isEof(), "Should still be EOF");
}

/**
 * @brief 测试源名称设置
 */
static void testSetSourceName(TestSuite& suite) {
    Str source = "test";
    InputStream input(source);

    input.setSourceName("my_script.lua");
    ASSERT_EQ(suite, Str("my_script.lua"), input.getSourceName(), "Source name should be updated");

    input.setSourceName("another.lua");
    ASSERT_EQ(suite, Str("another.lua"), input.getSourceName(), "Source name should be updated again");
}

// =====================================================================
// 边界条件测试
// =====================================================================

/**
 * @brief 测试单字符字符串
 */
static void testSingleChar(TestSuite& suite) {
    Str source = "x";
    InputStream input(source);

    i32 c = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('x'), c, "Should read 'x'");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after single char");
}

/**
 * @brief 测试包含空字符的字符串
 */
static void testNullCharacter(TestSuite& suite) {
    Str source("a\0b", 3); // 包含空字符的字符串
    InputStream input(source);

    i32 c1 = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('a'), c1, "First char should be 'a'");

    i32 c2 = input.getChar();
    ASSERT_EQ(suite, 0, c2, "Second char should be null (0)");

    i32 c3 = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('b'), c3, "Third char should be 'b'");

    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after all chars");
}

/**
 * @brief 测试 UTF-8 多字节字符（按字节读取）
 */
static void testUtf8Bytes(TestSuite& suite) {
    //Str source = "中"; // UTF-8: E4 B8 AD (3 bytes)
    const char utf8Bytes[] = { static_cast<char>(0xE4), static_cast<char>(0xB8), static_cast<char>(0xAD) };
	Str source(utf8Bytes, 3);
    InputStream input(source);

    i32 b1 = input.getChar();
    i32 b2 = input.getChar();
    i32 b3 = input.getChar();

    // 验证读取了 3 个字节
    ASSERT_EQ(suite, static_cast<size_t>(3), input.getPosition(), "Should read 3 bytes for UTF-8 char");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all bytes");

    // 验证字节值（UTF-8 编码）
    ASSERT_TRUE(suite, b1 >= 0 && b1 <= 255, "Byte 1 should be in valid range");
    ASSERT_TRUE(suite, b2 >= 0 && b2 <= 255, "Byte 2 should be in valid range");
    ASSERT_TRUE(suite, b3 >= 0 && b3 <= 255, "Byte 3 should be in valid range");
}

/**
 * @brief 测试大字符串
 */
static void testLargeString(TestSuite& suite) {
    Str source(10000, 'x'); // 10000 个 'x'
    InputStream input(source);

    // 逐字符读取前 100 个
    for (int i = 0; i < 100; ++i) {
        i32 c = input.getChar();
        // ASSERT_EQ(suite, static_cast<i32>('x'), c, "Each char should be 'x'");
    }

    ASSERT_EQ(suite, static_cast<size_t>(100), input.getPosition(), "Position should be 100");
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF yet");

    // 批量读取剩余部分
    char buffer[10000];
    usize bytesRead = input.read(buffer, 9900);
    ASSERT_EQ(suite, static_cast<size_t>(9900), bytesRead, "Should read remaining 9900 bytes");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all");
}

/**
 * @brief 测试特殊字符
 */
static void testSpecialCharacters(TestSuite& suite) {
    Str source = "\n\r\t\\ \"\'";
    InputStream input(source);

    ASSERT_EQ(suite, static_cast<i32>('\n'), input.getChar(), "Should read newline");
    ASSERT_EQ(suite, static_cast<i32>('\r'), input.getChar(), "Should read carriage return");
    ASSERT_EQ(suite, static_cast<i32>('\t'), input.getChar(), "Should read tab");
    ASSERT_EQ(suite, static_cast<i32>('\\'), input.getChar(), "Should read backslash");
    ASSERT_EQ(suite, static_cast<i32>(' '), input.getChar(), "Should read space");
    ASSERT_EQ(suite, static_cast<i32>('"'), input.getChar(), "Should read double quote");
    ASSERT_EQ(suite, static_cast<i32>('\''), input.getChar(), "Should read single quote");
}

// =====================================================================
// 零拷贝验证测试
// =====================================================================

/**
 * @brief 测试零拷贝（字符串模式不拷贝数据）
 */
static void testZeroCopy(TestSuite& suite) {
    Str source = "zero copy test";
    InputStream input(source);

    // 字符串模式应该直接使用 string_view，不拷贝数据
    // 我们通过读取验证功能正常
    char buffer[15];
    usize bytesRead = input.read(buffer, 14);
    buffer[14] = '\0';

    ASSERT_EQ(suite, static_cast<size_t>(14), bytesRead, "Should read 14 bytes");
    ASSERT_EQ(suite, Str("zero copy test"), Str(buffer), "Content should match");
}

// =====================================================================
// 实际使用场景测试
// =====================================================================

/**
 * @brief 测试词法分析器场景：读取标识符
 */
static void testLexerIdentifierScenario(TestSuite& suite) {
    Str source = "local variable = 42";
    InputStream input(source);
    input.setSourceName("test.lua");

    // 读取 "local"
    char token1[6];
    for (int i = 0; i < 5; ++i) {
        token1[i] = static_cast<char>(input.getChar());
    }
    token1[5] = '\0';
    ASSERT_EQ(suite, Str("local"), Str(token1), "First token should be 'local'");

    // 跳过空格
    i32 space = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>(' '), space, "Should read space");

    // 读取 "variable"
    char token2[9];
    for (int i = 0; i < 8; ++i) {
        token2[i] = static_cast<char>(input.getChar());
    }
    token2[8] = '\0';
    ASSERT_EQ(suite, Str("variable"), Str(token2), "Second token should be 'variable'");
}

/**
 * @brief 测试词法分析器场景：使用 peekChar 前瞻
 */
static void testLexerLookaheadScenario(TestSuite& suite) {
    Str source = "123abc";
    InputStream input(source);

    // 读取数字
    Str number = "";
    while (true) {
        i32 c = input.peekChar();
        if (c >= '0' && c <= '9') {
            number += static_cast<char>(input.getChar());
        } else {
            break;
        }
    }

    ASSERT_EQ(suite, Str("123"), number, "Should read number '123'");
    ASSERT_EQ(suite, static_cast<size_t>(3), input.getPosition(), "Position should be 3");

    // 验证下一个字符是 'a'
    i32 next = input.peekChar();
    ASSERT_EQ(suite, static_cast<i32>('a'), next, "Next char should be 'a'");
}

/**
 * @brief 测试词法分析器场景：读取字符串字面量
 */
static void testLexerStringLiteralScenario(TestSuite& suite) {
    Str source = "\"hello\\nworld\"";
    InputStream input(source);

    // 跳过开始的引号
    i32 quote1 = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('"'), quote1, "Should read opening quote");

    // 读取字符串内容（简化版，不处理转义）
    Str content = "";
    while (true) {
        i32 c = input.peekChar();
        if (c == '"' || c == -1) {
            break;
        }
        content += static_cast<char>(input.getChar());
    }

    ASSERT_EQ(suite, Str("hello\\nworld"), content, "Should read string content");

    // 读取结束引号
    i32 quote2 = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('"'), quote2, "Should read closing quote");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF");
}

/**
 * @brief 测试混合使用 getChar, peekChar, read
 */
static void testMixedOperations(TestSuite& suite) {
    Str source = "abcdefghij";
    InputStream input(source);

    // getChar
    i32 c1 = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('a'), c1, "Should read 'a'");

    // peekChar
    i32 p1 = input.peekChar();
    ASSERT_EQ(suite, static_cast<i32>('b'), p1, "Should peek 'b'");

    // read
    char buffer[3];
    usize bytes = input.read(buffer, 3);
    ASSERT_EQ(suite, static_cast<size_t>(3), bytes, "Should read 3 bytes");
    ASSERT_EQ(suite, 'b', buffer[0], "Buffer[0] should be 'b'");
    ASSERT_EQ(suite, 'c', buffer[1], "Buffer[1] should be 'c'");
    ASSERT_EQ(suite, 'd', buffer[2], "Buffer[2] should be 'd'");

    // getChar again
    i32 c2 = input.getChar();
    ASSERT_EQ(suite, static_cast<i32>('e'), c2, "Should read 'e'");

    ASSERT_EQ(suite, static_cast<size_t>(5), input.getPosition(), "Position should be 5");
}

/**
 * @brief 测试 Lua 代码片段
 */
static void testLuaCodeSnippet(TestSuite& suite) {
    Str source = "local x = 42\nreturn x";
    InputStream input(source);
    input.setSourceName("snippet.lua");

    // 验证可以读取完整代码
    char buffer[22];
    usize bytesRead = input.read(buffer, 21);
    buffer[21] = '\0';

    ASSERT_EQ(suite, static_cast<size_t>(21), bytesRead, "Should read all 21 bytes");
    ASSERT_EQ(suite, source, Str(buffer), "Content should match source");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF");
    ASSERT_EQ(suite, Str("snippet.lua"), input.getSourceName(), "Source name should be preserved");
}

// =====================================================================
// 测试注册函数
// =====================================================================

/**
 * @brief 注册所有 InputStream 字符串模式测试
 */
void registerInputStreamStringTests() {
    auto& registry = TestRegistry::getInstance();

    // 基础功能测试
    registry.registerTest("InputStream (String Mode)", "String construction", testStringConstruction);
    registry.registerTest("InputStream (String Mode)", "Empty string construction", testEmptyStringConstruction);
    registry.registerTest("InputStream (String Mode)", "Default source name", testDefaultSourceName);

    // 字符读取测试
    registry.registerTest("InputStream (String Mode)", "getChar", testGetChar);
    registry.registerTest("InputStream (String Mode)", "peekChar", testPeekChar);
    registry.registerTest("InputStream (String Mode)", "peekChar at EOF", testPeekCharAtEof);
    registry.registerTest("InputStream (String Mode)", "Read to EOF", testReadToEof);

    // 批量读取测试
    registry.registerTest("InputStream (String Mode)", "read (batch)", testRead);
    registry.registerTest("InputStream (String Mode)", "read all", testReadAll);
    registry.registerTest("InputStream (String Mode)", "read partial", testReadPartial);
    registry.registerTest("InputStream (String Mode)", "read beyond available", testReadBeyondAvailable);
    registry.registerTest("InputStream (String Mode)", "read zero bytes", testReadZeroBytes);

    // 位置和状态测试
    registry.registerTest("InputStream (String Mode)", "Position tracking", testPositionTracking);
    registry.registerTest("InputStream (String Mode)", "EOF state", testEofState);
    registry.registerTest("InputStream (String Mode)", "Set source name", testSetSourceName);

    // 边界条件测试
    registry.registerTest("InputStream (String Mode)", "Single char", testSingleChar);
    registry.registerTest("InputStream (String Mode)", "Null character", testNullCharacter);
    registry.registerTest("InputStream (String Mode)", "UTF-8 bytes", testUtf8Bytes);
    registry.registerTest("InputStream (String Mode)", "Large string", testLargeString);
    registry.registerTest("InputStream (String Mode)", "Special characters", testSpecialCharacters);

    // 零拷贝验证测试
    registry.registerTest("InputStream (String Mode)", "Zero-copy", testZeroCopy);

    // 实际使用场景测试
    registry.registerTest("InputStream (String Mode)", "Lexer identifier scenario", testLexerIdentifierScenario);
    registry.registerTest("InputStream (String Mode)", "Lexer lookahead scenario", testLexerLookaheadScenario);
    registry.registerTest("InputStream (String Mode)", "Lexer string literal scenario", testLexerStringLiteralScenario);
    registry.registerTest("InputStream (String Mode)", "Mixed operations", testMixedOperations);
    registry.registerTest("InputStream (String Mode)", "Lua code snippet", testLuaCodeSnippet);
}




