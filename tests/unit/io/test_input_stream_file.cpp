/**
 * @file test_input_stream_file.cpp
 * @brief InputStream 类（文件模式）单元测试
 *
 * 测试 InputStream 的文件模式功能，包括：
 * - 文件路径构造（InputStream::fromFile）
 * - 文件读取（基本、大文件、二进制）
 * - 错误处理（文件不存在、读取错误）
 * - 源名称管理
 * - Lexer 集成
 */

#include "../framework/test_framework.hpp"
#include "common/types.hpp"
#include "compiler/lexer/lexer.hpp"
#include "io/input_stream.hpp"
#include "test_file_fixture.hpp"

using namespace Lua;
using namespace Lua::IO;
using namespace LuaTest;

// =====================================================================
// 基础功能测试
// =====================================================================

/**
 * @brief 测试文件路径构造函数 - 基本文件读取
 */
static void testFilePathConstruction(TestSuite& suite) {
    const Str filePath = "build/test_file_basic.txt";
    const Str content = "Hello, Lua!";

    TemporaryTestFile testFile(filePath, content);
    InputStream input(InputStream::fromFile, filePath);

    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF initially");
    ASSERT_EQ(suite, static_cast<usize>(0), input.getPosition(), "Initial position should be 0");

    Str result;
    i32 ch;
    while ((ch = input.getChar()) != -1) {
        result += static_cast<char>(ch);
    }

    ASSERT_EQ(suite, result, content, "Content should match");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all");
}

/**
 * @brief 测试文件不存在错误处理
 */
static void testFileNotFound(TestSuite& suite) {
    const Str filePath = "build/nonexistent_file_12345.txt";

    bool exceptionThrown = false;
    Str exceptionMessage;

    try {
        InputStream input(InputStream::fromFile, filePath);
    } catch (const std::runtime_error& e) {
        exceptionThrown = true;
        exceptionMessage = e.what();
    }

    ASSERT_TRUE(suite, exceptionThrown, "Should throw exception for nonexistent file");
    ASSERT_TRUE(suite, exceptionMessage.find(filePath) != Str::npos, "Exception message should contain file path");
}

/**
 * @brief 测试空文件处理
 */
static void testEmptyFile(TestSuite& suite) {
    const Str filePath = "build/test_file_empty.txt";

    TemporaryTestFile testFile(filePath, "");
    InputStream input(InputStream::fromFile, filePath);

    ASSERT_TRUE(suite, input.isEof(), "Empty file should be EOF immediately");
    ASSERT_EQ(suite, static_cast<i32>(-1), input.getChar(), "getChar should return -1");
    ASSERT_EQ(suite, static_cast<usize>(0), input.getPosition(), "Position should be 0");
}

/**
 * @brief 测试大文件读取（超过缓冲区大小）
 */
static void testLargeFile(TestSuite& suite) {
    const Str filePath = "build/test_file_large.txt";
    const usize fileSize = 8192; // 8KB，超过默认 4KB 缓冲区

    // 创建大文件（重复字符 'A'）
    Str content(fileSize, 'A');
    TemporaryTestFile testFile(filePath, content);
    InputStream input(InputStream::fromFile, filePath);

    usize count = 0;
    i32 ch;
    while ((ch = input.getChar()) != -1) {
        // ASSERT_EQ(suite, static_cast<i32>('A'), ch, "Each char should be 'A'");
        count++;
    }

    ASSERT_EQ(suite, fileSize, count, "Should read all bytes");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all");
}

/**
 * @brief 测试二进制文件（包含空字节）
 */
static void testBinaryFile(TestSuite& suite) {
    const Str filePath = "build/test_file_binary.bin";

    // 创建包含空字节的二进制数据
    Str content;
    content += 'A';
    content += '\0'; // 空字节
    content += 'B';
    content += '\0';
    content += 'C';

    TemporaryTestFile testFile(filePath, content);
    InputStream input(InputStream::fromFile, filePath);

    ASSERT_EQ(suite, static_cast<i32>('A'), input.getChar(), "First byte should be 'A'");
    ASSERT_EQ(suite, static_cast<i32>('\0'), input.getChar(), "Second byte should be null");
    ASSERT_EQ(suite, static_cast<i32>('B'), input.getChar(), "Third byte should be 'B'");
    ASSERT_EQ(suite, static_cast<i32>('\0'), input.getChar(), "Fourth byte should be null");
    ASSERT_EQ(suite, static_cast<i32>('C'), input.getChar(), "Fifth byte should be 'C'");
    ASSERT_EQ(suite, static_cast<i32>(-1), input.getChar(), "Should be EOF");
}

/**
 * @brief 测试源名称自动设置
 */
static void testSourceNameAutoSet(TestSuite& suite) {
    const Str filePath = "build/test_file_source_name.txt";
    const Str content = "test";

    TemporaryTestFile testFile(filePath, content);
    InputStream input(InputStream::fromFile, filePath);

    ASSERT_EQ(suite, filePath, input.getSourceName(), "Source name should be file path");
}

// =====================================================================
// 集成测试
// =====================================================================

/**
 * @brief 测试 Lexer 集成 - 从文件读取 Lua 代码
 */
static void testLexerFileIntegration(TestSuite& suite) {
    const Str filePath = "build/test_file_lua_code.lua";
    const Str luaCode = "local x = 42\nreturn x + 1";

    TemporaryTestFile testFile(filePath, luaCode);
    InputStream fileInput(InputStream::fromFile, filePath);
    Lexer lexer(fileInput);

    auto tok1 = lexer.nextToken();
    ASSERT_EQ(suite, TokenType::Local, tok1.type, "First token should be 'local'");

    auto tok2 = lexer.nextToken();
    ASSERT_EQ(suite, TokenType::Name, tok2.type, "Second token should be identifier");
    ASSERT_EQ(suite, Str("x"), tok2.lexeme, "Identifier should be 'x'");

    auto tok3 = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<i32>('='), static_cast<i32>(tok3.type), "Third token should be '='");

    auto tok4 = lexer.nextToken();
    ASSERT_EQ(suite, TokenType::Number, tok4.type, "Fourth token should be number");
    ASSERT_EQ(suite, Str("42"), tok4.lexeme, "Number should be '42'");

    auto tok5 = lexer.nextToken();
    ASSERT_EQ(suite, TokenType::Return, tok5.type, "Fifth token should be 'return'");
}

/**
 * @brief 测试小缓冲区（验证缓冲区重新填充）
 */
static void testSmallBuffer(TestSuite& suite) {
    const Str filePath = "build/test_file_small_buffer.txt";
    const Str content = "0123456789ABCDEFGHIJ"; // 20 字节

    TemporaryTestFile testFile(filePath, content);
    InputStream input(InputStream::fromFile, filePath, 8);

    Str result;
    i32 ch;
    while ((ch = input.getChar()) != -1) {
        result += static_cast<char>(ch);
    }

    ASSERT_EQ(suite, result, content, "Content should match with small buffer");
}

// =====================================================================
// 测试注册
// =====================================================================

/**
 * @brief 注册所有 InputStream 文件模式测试
 */
void registerInputStreamFileTests() {
    auto& registry = TestRegistry::getInstance();

    // 基础功能测试
    registry.registerTest("InputStream (File Mode)", "File path construction", testFilePathConstruction);
    registry.registerTest("InputStream (File Mode)", "File not found", testFileNotFound);
    registry.registerTest("InputStream (File Mode)", "Empty file", testEmptyFile);
    registry.registerTest("InputStream (File Mode)", "Large file", testLargeFile);
    registry.registerTest("InputStream (File Mode)", "Binary file", testBinaryFile);
    registry.registerTest("InputStream (File Mode)", "Source name auto-set", testSourceNameAutoSet);

    // 集成测试
    registry.registerTest("InputStream (File Mode)", "Lexer integration", testLexerFileIntegration);
    registry.registerTest("InputStream (File Mode)", "Small buffer", testSmallBuffer);
}
