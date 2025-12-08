/**
 * @file test_input_stream_stream.cpp
 * @brief InputStream 类（流模式）单元测试
 * 
 * 测试 InputStream 使用 std::istream 的流模式功能：
 * - 流模式构造函数
 * - 缓冲区管理和重新填充
 * - 跨缓冲区边界的读取
 * - 不同流类型的兼容性
 * - 大数据流处理
 * 
 * @author Lua C++ Implementation Team
 * @version 0.1.0
 * @date 2025-12-08
 */

#include "../framework/test_framework.hpp"
#include "common/types.hpp"
#include "io/input_stream.hpp"
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>

using namespace Lua;
using namespace Lua::IO;
using namespace LuaTest;

// =====================================================================
// 基础功能测试
// =====================================================================

/**
 * @brief 测试流模式构造函数
 */
static void testStreamModeConstruction(TestSuite& suite) {
    std::istringstream iss("hello");
    InputStream input(iss);
    
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF initially");
    ASSERT_EQ(suite, 0u, input.getPosition(), "Initial position should be 0");
    ASSERT_EQ(suite, "stream", input.getSourceName(), "Default source name should be 'stream'");
}

/**
 * @brief 测试空流构造
 */
static void testEmptyStreamConstruction(TestSuite& suite) {
    std::istringstream iss("");
    InputStream input(iss);
    
    // 空流在构造时会调用 fillBuffer()，发现没有数据后设置 eof_
    ASSERT_TRUE(suite, input.isEof(), "Empty stream should be EOF immediately");
    ASSERT_EQ(suite, 0u, input.getPosition(), "Position should be 0");
}

/**
 * @brief 测试自定义缓冲区大小
 */
static void testCustomBufferSize(TestSuite& suite) {
    std::istringstream iss("test data");
    InputStream input(iss, 4);  // 4 字节缓冲区
    
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF initially");
    
    // 读取应该正常工作，即使缓冲区很小
    i32 ch = input.getChar();
    ASSERT_EQ(suite, 't', ch, "First char should be 't'");
}

// =====================================================================
// 字符读取测试
// =====================================================================

/**
 * @brief 测试从流读取字符
 */
static void testGetCharFromStream(TestSuite& suite) {
    std::istringstream iss("abc");
    InputStream input(iss);
    
    i32 ch1 = input.getChar();
    ASSERT_EQ(suite, 'a', ch1, "First char should be 'a'");
    ASSERT_EQ(suite, 1u, input.getPosition(), "Position should be 1");
    
    i32 ch2 = input.getChar();
    ASSERT_EQ(suite, 'b', ch2, "Second char should be 'b'");
    ASSERT_EQ(suite, 2u, input.getPosition(), "Position should be 2");
    
    i32 ch3 = input.getChar();
    ASSERT_EQ(suite, 'c', ch3, "Third char should be 'c'");
    ASSERT_EQ(suite, 3u, input.getPosition(), "Position should be 3");
    
    i32 ch4 = input.getChar();
    ASSERT_EQ(suite, -1, ch4, "Should return -1 at EOF");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all chars");
}

/**
 * @brief 测试从流前瞻字符
 */
static void testPeekCharFromStream(TestSuite& suite) {
    std::istringstream iss("xy");
    InputStream input(iss);
    
    i32 peek1 = input.peekChar();
    ASSERT_EQ(suite, 'x', peek1, "Peek should return 'x'");
    ASSERT_EQ(suite, 0u, input.getPosition(), "Peek should not advance position");
    
    i32 peek2 = input.peekChar();
    ASSERT_EQ(suite, 'x', peek2, "Multiple peeks should return same char");
    ASSERT_EQ(suite, 0u, input.getPosition(), "Position should still be 0");
    
    i32 ch = input.getChar();
    ASSERT_EQ(suite, 'x', ch, "getChar should return peeked char");
    ASSERT_EQ(suite, 1u, input.getPosition(), "Position should advance after getChar");
    
    i32 peek3 = input.peekChar();
    ASSERT_EQ(suite, 'y', peek3, "Peek should now return 'y'");
}

/**
 * @brief 测试 EOF 时的 peek
 */
static void testPeekCharAtEofFromStream(TestSuite& suite) {
    std::istringstream iss("a");
    InputStream input(iss);
    
    input.getChar();  // 读取 'a'
    
    i32 peek = input.peekChar();
    ASSERT_EQ(suite, -1, peek, "Peek at EOF should return -1");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF");
}

/**
 * @brief 测试 EOF 时多次 getChar
 */
static void testMultipleGetCharAtEof(TestSuite& suite) {
    std::istringstream iss("x");
    InputStream input(iss);
    
    input.getChar();  // 读取 'x'
    
    i32 ch1 = input.getChar();
    ASSERT_EQ(suite, -1, ch1, "getChar at EOF should return -1");
    
    i32 ch2 = input.getChar();
    ASSERT_EQ(suite, -1, ch2, "Multiple getChar at EOF should return -1");
}

// =====================================================================
// 批量读取测试
// =====================================================================

/**
 * @brief 测试从流批量读取（小于缓冲区）
 */
static void testReadFromStream(TestSuite& suite) {
    std::istringstream iss("hello world");
    InputStream input(iss);

    char buffer[6] = {0};
    usize bytesRead = input.read(buffer, 5);

    ASSERT_EQ(suite, 5u, bytesRead, "Should read 5 bytes");
    ASSERT_EQ(suite, "hello", Str(buffer, 5), "Buffer should contain 'hello'");
    ASSERT_EQ(suite, 5u, input.getPosition(), "Position should be 5");
}

/**
 * @brief 测试读取到 EOF
 */
static void testReadToEofFromStream(TestSuite& suite) {
    std::istringstream iss("test");
    InputStream input(iss);

    char buffer[10] = {0};
    usize bytesRead = input.read(buffer, 10);

    ASSERT_EQ(suite, 4u, bytesRead, "Should only read 4 bytes (all available)");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all");
    ASSERT_EQ(suite, 4u, input.getPosition(), "Position should be 4");
}

/**
 * @brief 测试部分读取
 */
static void testPartialReadFromStream(TestSuite& suite) {
    std::istringstream iss("abcdef");
    InputStream input(iss);

    char buffer1[3] = {0};
    usize read1 = input.read(buffer1, 3);
    ASSERT_EQ(suite, 3u, read1, "First read should get 3 bytes");

    char buffer2[3] = {0};
    usize read2 = input.read(buffer2, 3);
    ASSERT_EQ(suite, 3u, read2, "Second read should get 3 bytes");

    ASSERT_EQ(suite, 6u, input.getPosition(), "Position should be 6");
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF yet");

    char buffer3[1] = {0};
    usize read3 = input.read(buffer3, 1);
    ASSERT_EQ(suite, 0u, read3, "Should read 0 bytes at EOF");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF");
}

/**
 * @brief 测试 EOF 时读取
 */
static void testReadAtEof(TestSuite& suite) {
    std::istringstream iss("x");
    InputStream input(iss);

    char buffer[1];
    input.read(buffer, 1);  // 读取所有数据

    char buffer2[10];
    usize bytesRead = input.read(buffer2, 10);
    ASSERT_EQ(suite, 0u, bytesRead, "Should read 0 bytes at EOF");
    ASSERT_EQ(suite, 1u, input.getPosition(), "Position should not change");
}

// =====================================================================
// 缓冲区管理测试
// =====================================================================

/**
 * @brief 测试小缓冲区读取大数据
 */
static void testSmallBufferLargeData(TestSuite& suite) {
    std::istringstream iss("0123456789ABCDEF");  // 16 字节
    InputStream input(iss, 4);  // 4 字节缓冲区

    // 逐字符读取，应该触发多次 fillBuffer()
    Str data = "";
    for (int i = 0; i < 16; ++i) {
        i32 ch = input.getChar();
        ASSERT_TRUE(suite, ch != -1, "Should read valid char");
        data += static_cast<char>(ch);
    }

    Str expected = "0123456789ABCDEF";
    ASSERT_EQ(suite, expected, data, "Should read all data correctly");
    ASSERT_EQ(suite, 16u, input.getPosition(), "Position should be 16");
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF yet");

    i32 eof = input.getChar();
    ASSERT_EQ(suite, -1, eof, "Should return -1 at EOF");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF");
}

/**
 * @brief 测试大缓冲区读取小数据
 */
static void testLargeBufferSmallData(TestSuite& suite) {
    std::istringstream iss("tiny");
    InputStream input(iss, 4096);  // 4KB 缓冲区

    char buffer[10];
    usize bytesRead = input.read(buffer, 10);

    ASSERT_EQ(suite, 4u, bytesRead, "Should read 4 bytes");
    ASSERT_EQ(suite, "tiny", Str(buffer, 4), "Content should match");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF");
}

/**
 * @brief 测试跨缓冲区边界的连续读取
 */
static void testCrossBoundaryRead(TestSuite& suite) {
    std::istringstream iss("ABCDEFGHIJ");  // 10 字节
    InputStream input(iss, 4);  // 4 字节缓冲区

    // 第一次 fillBuffer: "ABCD"
    ASSERT_EQ(suite, 'A', input.getChar(), "Char 1");
    ASSERT_EQ(suite, 'B', input.getChar(), "Char 2");
    ASSERT_EQ(suite, 'C', input.getChar(), "Char 3");
    ASSERT_EQ(suite, 'D', input.getChar(), "Char 4");

    // 第二次 fillBuffer: "EFGH"
    ASSERT_EQ(suite, 'E', input.getChar(), "Char 5 (cross boundary)");
    ASSERT_EQ(suite, 'F', input.getChar(), "Char 6");
    ASSERT_EQ(suite, 'G', input.getChar(), "Char 7");
    ASSERT_EQ(suite, 'H', input.getChar(), "Char 8");

    // 第三次 fillBuffer: "IJ"
    ASSERT_EQ(suite, 'I', input.getChar(), "Char 9 (cross boundary)");
    ASSERT_EQ(suite, 'J', input.getChar(), "Char 10");

    ASSERT_EQ(suite, 10u, input.getPosition(), "Position should be 10");
}

/**
 * @brief 测试批量读取跨越缓冲区边界
 */
static void testBatchReadCrossBoundary(TestSuite& suite) {
    std::istringstream iss("123456789012345");  // 15 字节
    InputStream input(iss, 5);  // 5 字节缓冲区

    char buffer[12];
    usize bytesRead = input.read(buffer, 12);  // 读取 12 字节，需要跨越多个缓冲区

    ASSERT_EQ(suite, 12u, bytesRead, "Should read 12 bytes");
    ASSERT_EQ(suite, "123456789012", Str(buffer, 12), "Content should match");
    ASSERT_EQ(suite, 12u, input.getPosition(), "Position should be 12");
}

// =====================================================================
// 位置和状态测试
// =====================================================================

/**
 * @brief 测试位置跟踪（跨缓冲区）
 */
static void testPositionTracking(TestSuite& suite) {
    std::istringstream iss("0123456789");
    InputStream input(iss, 3);  // 3 字节缓冲区

    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF initially");
    ASSERT_EQ(suite, 0u, input.getPosition(), "Initial position should be 0");

    input.getChar();  // '0'
    ASSERT_EQ(suite, 1u, input.getPosition(), "Position should be 1");

    input.getChar();  // '1'
    ASSERT_EQ(suite, 2u, input.getPosition(), "Position should be 2");

    char buffer[4];
    input.read(buffer, 4);  // "2345"
    ASSERT_EQ(suite, 6u, input.getPosition(), "Position should be 6 after read");
}

/**
 * @brief 测试 EOF 状态
 */
static void testEofState(TestSuite& suite) {
    std::istringstream iss("abc");
    InputStream input(iss);

    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF initially");

    input.getChar();  // 'a'
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF after first char");

    input.getChar();  // 'b'
    input.getChar();  // 'c'
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF after last char");

    input.getChar();  // EOF
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all");

    ASSERT_TRUE(suite, input.isEof(), "Should still be EOF");
}

/**
 * @brief 测试源名称管理
 */
static void testSourceNameManagement(TestSuite& suite) {
    std::istringstream iss("data");
    InputStream input(iss);

    ASSERT_EQ(suite, "stream", input.getSourceName(), "Default source name should be 'stream'");

    input.setSourceName("test.lua");
    ASSERT_EQ(suite, "test.lua", input.getSourceName(), "Source name should be updated");

    input.setSourceName("@stdin");
    ASSERT_EQ(suite, "@stdin", input.getSourceName(), "Source name should be updated again");
}

// =====================================================================
// 边界条件测试
// =====================================================================

/**
 * @brief 测试单字符流
 */
static void testSingleCharStream(TestSuite& suite) {
    std::istringstream iss("x");
    InputStream input(iss);

    i32 ch = input.getChar();
    ASSERT_EQ(suite, 'x', ch, "Should read 'x'");
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF after reading char");

    i32 eof = input.getChar();
    ASSERT_EQ(suite, -1, eof, "Should return -1 at EOF");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF");
}

/**
 * @brief 测试包含 null 字符的流
 */
static void testNullCharacterInStream(TestSuite& suite) {
    Str source = "";
    source += 'a';
    source += '\0';
    source += 'b';

    std::istringstream iss(source);
    InputStream input(iss);

    i32 ch1 = input.getChar();
    ASSERT_EQ(suite, 'a', ch1, "First char should be 'a'");

    i32 ch2 = input.getChar();
    ASSERT_EQ(suite, 0, ch2, "Second char should be null (0)");

    i32 ch3 = input.getChar();
    ASSERT_EQ(suite, 'b', ch3, "Third char should be 'b'");

    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF yet");
}

/**
 * @brief 测试 UTF-8 字节序列
 */
static void testUtf8BytesInStream(TestSuite& suite) {
    std::istringstream iss("中");  // UTF-8: 3 字节
    InputStream input(iss);

    char buffer[3];
    usize bytesRead = input.read(buffer, 3);

    ASSERT_EQ(suite, 2u, bytesRead, "Should read 3 bytes for UTF-8 char");
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF yet");

    // 验证字节在有效范围内（UTF-8 字节）
    ASSERT_TRUE(suite, static_cast<u8>(buffer[0]) >= 0x80, "Byte 1 should be in valid range");
    ASSERT_TRUE(suite, static_cast<u8>(buffer[1]) >= 0x80, "Byte 2 should be in valid range");
    ASSERT_TRUE(suite, static_cast<u8>(buffer[2]) >= 0x80, "Byte 3 should be in valid range");
}

/**
 * @brief 测试大数据流
 */
static void testLargeStreamData(TestSuite& suite) {
    // 创建 10000 字节的数据
    Str largeData(10000, 'x');
    std::istringstream iss(largeData);
    InputStream input(iss, 512);  // 512 字节缓冲区

    // 逐字符读取前 100 个字符
    for (int i = 0; i < 100; ++i) {
        i32 ch = input.getChar();
        ASSERT_EQ(suite, 'x', ch, "Each char should be 'x'");
    }

    ASSERT_EQ(suite, 100u, input.getPosition(), "Position should be 100");
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF yet");

    // 批量读取剩余数据
    char buffer[10000];
    usize bytesRead = input.read(buffer, 9900);
    ASSERT_EQ(suite, 9900u, bytesRead, "Should read remaining 9900 bytes");
    ASSERT_FALSE(suite, input.isEof(), "Should not be EOF yet");

    i32 eof = input.getChar();
    ASSERT_EQ(suite, -1, eof, "Should be EOF");
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all");
}

/**
 * @brief 测试特殊字符
 */
static void testSpecialCharacters(TestSuite& suite) {
    std::istringstream iss("\n\r\t\\ \"'");
    InputStream input(iss);

    ASSERT_EQ(suite, '\n', input.getChar(), "Should read newline");
    ASSERT_EQ(suite, '\r', input.getChar(), "Should read carriage return");
    ASSERT_EQ(suite, '\t', input.getChar(), "Should read tab");
    ASSERT_EQ(suite, '\\', input.getChar(), "Should read backslash");
    ASSERT_EQ(suite, ' ', input.getChar(), "Should read space");
    ASSERT_EQ(suite, '"', input.getChar(), "Should read double quote");
    ASSERT_EQ(suite, '\'', input.getChar(), "Should read single quote");
}

// =====================================================================
// 流类型测试
// =====================================================================

/**
 * @brief 测试 std::istringstream
 */
static void testIStringStream(TestSuite& suite) {
    std::istringstream iss("string stream test");
    InputStream input(iss);

    char buffer[20];
    usize bytesRead = input.read(buffer, 18);

    ASSERT_EQ(suite, 18u, bytesRead, "Should read 18 bytes");
    ASSERT_EQ(suite, "string stream test", Str(buffer, 18), "Content should match");
}

/**
 * @brief 测试 std::ifstream（文件流）
 */
static void testFileStreamIntegration(TestSuite& suite) {
    // 创建临时测试文件（以二进制模式，避免换行符转换）
    const char* tempFilePath = "build/test_temp_stream_data.txt";
    const char* testContent = "File stream content\nLine 2\nLine 3";
    usize contentLength = std::strlen(testContent);  // 33 字节

    {
        std::ofstream ofs(tempFilePath, std::ios::binary);
        ofs << testContent;
    }

    // 使用 InputStream 读取文件（以二进制模式）
    std::ifstream ifs(tempFilePath, std::ios::binary);
    ASSERT_TRUE(suite, ifs.is_open(), "File should be opened");

    InputStream input(ifs);
    input.setSourceName(tempFilePath);

    char buffer[50] = {0};
    usize bytesRead = input.read(buffer, 50);

    ASSERT_EQ(suite, contentLength, bytesRead, "Should read all bytes");
    ASSERT_EQ(suite, testContent, Str(buffer, contentLength), "Content should match");
    ASSERT_EQ(suite, tempFilePath, input.getSourceName(), "Source name should be preserved");

    ifs.close();

    // 清理临时文件
    std::remove(tempFilePath);
}

/**
 * @brief 测试大文件流
 */
static void testLargeFileStream(TestSuite& suite) {
    const char* tempFilePath = "build/test_temp_large_stream.txt";

    // 创建大文件（5000 字节）
    {
        std::ofstream ofs(tempFilePath);
        for (int i = 0; i < 5000; ++i) {
            ofs << 'A';
        }
    }

    std::ifstream ifs(tempFilePath);
    ASSERT_TRUE(suite, ifs.is_open(), "File should be opened");

    InputStream input(ifs, 256);  // 256 字节缓冲区

    // 逐字符读取前 1000 个字符
    for (int i = 0; i < 1000; ++i) {
        i32 ch = input.getChar();
        ASSERT_EQ(suite, 'A', ch, "Each char should be 'A'");
    }

    // 批量读取剩余 4000 字节
    char buffer[4000];
    usize bytesRead = input.read(buffer, 4000);
    ASSERT_EQ(suite, 4000u, bytesRead, "Should read remaining 4000 bytes");

    ASSERT_EQ(suite, 5000u, input.getPosition(), "Position should be 5000");

    ifs.close();
    std::remove(tempFilePath);
}

// =====================================================================
// 实际使用场景测试
// =====================================================================

/**
 * @brief 测试词法分析器标识符场景
 */
static void testLexerIdentifierScenario(TestSuite& suite) {
    std::istringstream iss("local variable = 123");
    InputStream input(iss);

    // 读取 "local"
    Str token1 = "";
    for (int i = 0; i < 5; ++i) {
        token1 += static_cast<char>(input.getChar());
    }
    ASSERT_EQ(suite, "local", token1, "First token should be 'local'");

    // 跳过空格
    ASSERT_EQ(suite, ' ', input.getChar(), "Should read space");

    // 读取 "variable"
    Str token2 = "";
    for (int i = 0; i < 8; ++i) {
        token2 += static_cast<char>(input.getChar());
    }
    ASSERT_EQ(suite, "variable", token2, "Second token should be 'variable'");
}

/**
 * @brief 测试词法分析器数字场景
 */
static void testLexerNumberScenario(TestSuite& suite) {
    std::istringstream iss("123abc");
    InputStream input(iss);

    // 读取数字 "123"
    Str number = "";
    i32 ch;
    while ((ch = input.peekChar()) >= '0' && ch <= '9') {
        number += static_cast<char>(input.getChar());
    }

    ASSERT_EQ(suite, "123", number, "Should read number '123'");
    ASSERT_EQ(suite, 3u, input.getPosition(), "Position should be 3");

    // 下一个字符应该是 'a'
    ASSERT_EQ(suite, 'a', input.peekChar(), "Next char should be 'a'");
}

/**
 * @brief 测试词法分析器字符串字面量场景
 */
static void testLexerStringLiteralScenario(TestSuite& suite) {
    std::istringstream iss("\"hello world\"");
    InputStream input(iss);

    // 读取开头的引号
    ASSERT_EQ(suite, '"', input.getChar(), "Should read opening quote");

    // 读取字符串内容
    Str content = "";
    i32 ch;
    while ((ch = input.getChar()) != '"' && ch != -1) {
        content += static_cast<char>(ch);
    }

    ASSERT_EQ(suite, "hello world", content, "Should read string content");
    ASSERT_EQ(suite, 13u, input.getPosition(), "Position should be 13");
}

/**
 * @brief 测试混合操作（peek + getChar + read）
 */
static void testMixedOperations(TestSuite& suite) {
    std::istringstream iss("abcdefghij");
    InputStream input(iss, 4);  // 4 字节缓冲区

    // peek
    ASSERT_EQ(suite, 'a', input.peekChar(), "Should peek 'a'");

    // getChar
    ASSERT_EQ(suite, 'a', input.getChar(), "Should read 'a'");

    // read
    char buffer[3];
    usize bytesRead = input.read(buffer, 3);
    ASSERT_EQ(suite, 3u, bytesRead, "Should read 3 bytes");
    ASSERT_EQ(suite, "bcd", Str(buffer, 3), "Buffer should contain 'bcd'");

    // peek again
    ASSERT_EQ(suite, 'e', input.peekChar(), "Should peek 'e'");

    // getChar
    ASSERT_EQ(suite, 'e', input.getChar(), "Should read 'e'");

    ASSERT_EQ(suite, 5u, input.getPosition(), "Position should be 5");
}

/**
 * @brief 测试完整源代码读取
 */
static void testCompleteSourceRead(TestSuite& suite) {
    Str source = "local x = 42\nprint(x)\n";
    std::istringstream iss(source);
    InputStream input(iss);
    input.setSourceName("test_script.lua");

    char buffer[100];
    usize bytesRead = input.read(buffer, 100);

    ASSERT_EQ(suite, source.size(), bytesRead, "Should read all bytes");
    ASSERT_EQ(suite, source, Str(buffer, bytesRead), "Content should match source");
    // 读取完所有数据后即到达 EOF（与其他 read 到 EOF 的测试保持一致）
    ASSERT_TRUE(suite, input.isEof(), "Should be EOF after reading all");
    ASSERT_EQ(suite, "test_script.lua", input.getSourceName(), "Source name should be preserved");
}

/**
 * @brief 测试缓冲区边界处的 peek
 */
static void testPeekAtBufferBoundary(TestSuite& suite) {
    std::istringstream iss("ABCDEFGH");  // 8 字节
    InputStream input(iss, 4);  // 4 字节缓冲区

    // 读取前 3 个字符
    input.getChar();  // 'A'
    input.getChar();  // 'B'
    input.getChar();  // 'C'

    // 现在在缓冲区边界前
    ASSERT_EQ(suite, 'D', input.peekChar(), "Should peek 'D'");
    ASSERT_EQ(suite, 'D', input.getChar(), "Should read 'D'");

    // 跨越缓冲区边界
    ASSERT_EQ(suite, 'E', input.peekChar(), "Should peek 'E' (cross boundary)");
    ASSERT_EQ(suite, 'E', input.getChar(), "Should read 'E'");
}

// =====================================================================
// 测试注册
// =====================================================================

void registerInputStreamStreamTests() {
    TestRegistry& registry = TestRegistry::getInstance();

    // 基础功能测试
    registry.registerTest("InputStream (Stream Mode)", "Stream mode construction", testStreamModeConstruction);
    registry.registerTest("InputStream (Stream Mode)", "Empty stream construction", testEmptyStreamConstruction);
    registry.registerTest("InputStream (Stream Mode)", "Custom buffer size", testCustomBufferSize);

    // 字符读取测试
    registry.registerTest("InputStream (Stream Mode)", "getChar from stream", testGetCharFromStream);
    registry.registerTest("InputStream (Stream Mode)", "peekChar from stream", testPeekCharFromStream);
    registry.registerTest("InputStream (Stream Mode)", "peekChar at EOF from stream", testPeekCharAtEofFromStream);
    registry.registerTest("InputStream (Stream Mode)", "Multiple getChar at EOF", testMultipleGetCharAtEof);

    // 批量读取测试
    registry.registerTest("InputStream (Stream Mode)", "read from stream", testReadFromStream);
    registry.registerTest("InputStream (Stream Mode)", "read to EOF from stream", testReadToEofFromStream);
    registry.registerTest("InputStream (Stream Mode)", "Partial read from stream", testPartialReadFromStream);
    registry.registerTest("InputStream (Stream Mode)", "read at EOF", testReadAtEof);

    // 缓冲区管理测试
    registry.registerTest("InputStream (Stream Mode)", "Small buffer large data", testSmallBufferLargeData);
    registry.registerTest("InputStream (Stream Mode)", "Large buffer small data", testLargeBufferSmallData);
    registry.registerTest("InputStream (Stream Mode)", "Cross boundary read", testCrossBoundaryRead);
    registry.registerTest("InputStream (Stream Mode)", "Batch read cross boundary", testBatchReadCrossBoundary);

    // 位置和状态测试
    registry.registerTest("InputStream (Stream Mode)", "Position tracking", testPositionTracking);
    registry.registerTest("InputStream (Stream Mode)", "EOF state", testEofState);
    registry.registerTest("InputStream (Stream Mode)", "Source name management", testSourceNameManagement);

    // 边界条件测试
    registry.registerTest("InputStream (Stream Mode)", "Single char stream", testSingleCharStream);
    registry.registerTest("InputStream (Stream Mode)", "Null character in stream", testNullCharacterInStream);
    registry.registerTest("InputStream (Stream Mode)", "UTF-8 bytes in stream", testUtf8BytesInStream);
    registry.registerTest("InputStream (Stream Mode)", "Large stream data", testLargeStreamData);
    registry.registerTest("InputStream (Stream Mode)", "Special characters", testSpecialCharacters);

    // 流类型测试
    registry.registerTest("InputStream (Stream Mode)", "std::istringstream", testIStringStream);
    registry.registerTest("InputStream (Stream Mode)", "File stream integration", testFileStreamIntegration);
    registry.registerTest("InputStream (Stream Mode)", "Large file stream", testLargeFileStream);

    // 实际使用场景测试
    registry.registerTest("InputStream (Stream Mode)", "Lexer identifier scenario", testLexerIdentifierScenario);
    registry.registerTest("InputStream (Stream Mode)", "Lexer number scenario", testLexerNumberScenario);
    registry.registerTest("InputStream (Stream Mode)", "Lexer string literal scenario", testLexerStringLiteralScenario);
    registry.registerTest("InputStream (Stream Mode)", "Mixed operations", testMixedOperations);
    registry.registerTest("InputStream (Stream Mode)", "Complete source read", testCompleteSourceRead);
    registry.registerTest("InputStream (Stream Mode)", "Peek at buffer boundary", testPeekAtBufferBoundary);
}


