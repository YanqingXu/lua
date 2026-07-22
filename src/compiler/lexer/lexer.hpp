#pragma once

/**
 * @file lexer.hpp
 * @brief Lua词法分析器
 *
 * 实现 Lua 5.1 的词法分析器，将源代码文本转换为词法单元流。
 *
 * 核心功能：
 * - 识别Lua 5.1的所有关键字、运算符和字面量
 * - 支持单行注释（--）和多行注释（--[[ ]]）
 * - 支持长字符串（[[ ]]和[=[ ]=]）
 * - 精确的行号和列号跟踪
 * - 详细的错误报告
 * - 词法单元预读机制，支持 LL(1) 语法分析
 * - 哈希表优化的关键字识别
 */

#include "compiler/parser/token.hpp"
#include "lexer_cursor.hpp"
#include "common/types.hpp"
#include "io/input_stream.hpp"
#include <optional>

namespace Lua {

/**
 * @brief Lua词法分析器类
 *
 * 使用单遍扫描算法，从源代码字符串中提取词法单元。
 *
 * 特性：
 * - 流式处理，支持大文件
 * - 词法单元预读机制，支持 LL(1) 语法分析
 * - 前瞻一个字符的词法分析
 * - 自动跳过空白和注释
 * - 支持所有Lua 5.1词法规则
 * - 使用哈希表优化关键字识别（O(1)时间复杂度）
 */
class Lexer {
public:
    /**
     * @brief 从字符串构造词法分析器（向后兼容）
     * @param source 源代码字符串
     *
     * 内部会创建字符串输入流，保持向后兼容性。
     */
    explicit Lexer(const Str& source);
    Lexer(const Str& source, LuaAllocator* allocator);

    /**
     * @brief 从输入流构造词法分析器
     * @param input 输入流引用
     *
     * @note input 必须在词法分析器生命周期内保持有效。
     */
    explicit Lexer(IO::InputStream& input);
    Lexer(IO::InputStream& input, LuaAllocator* allocator);

    Lexer(const Lexer&) = delete;
    Lexer& operator=(const Lexer&) = delete;
    Lexer(Lexer&&) = delete;
    Lexer& operator=(Lexer&&) = delete;

    /**
     * @brief 获取下一个词法单元
     * @return 词法单元对象
     *
     * 如果存在预读词法单元，则返回并清除预读状态；
     * 否则从输入流中解析新词法单元。
     */
    Token nextToken();

    /**
     * @brief 预读下一个词法单元而不消费当前词法单元
     * @return 预读的词法单元对象
     *
     * 支持 LL(1) 语法分析的前瞻功能。预读的词法单元会被缓存，
     * 下次获取词法单元时会返回该词法单元。
     */
    Token peekToken();

    /**
     * @brief 检查是否到达源代码末尾
     */
    bool isAtEnd() const noexcept;

    /**
     * @brief 获取当前行号
     */
    i32 getCurrentLine() const noexcept {
        return inputCursor_.line();
    }

    /**
     * @brief 获取当前列号
     */
    i32 getCurrentColumn() const noexcept {
        return inputCursor_.column();
    }

private:
    // =====================================================================
    // 字符操作
    // =====================================================================

    /**
     * @brief 前进一个字符并返回
     */
    char advance();

    /**
     * @brief 查看当前字符（不前进）
     */
    char peek() const noexcept;

    /**
     * @brief 查看下一个字符（不前进）
     */
    char peekNext() const noexcept;

    /**
     * @brief 如果当前字符匹配，则前进
     */
    bool match(char expected);

    /**
     * @brief 判断是否为 Lua 换行字符
     */
    static bool isNewline(char c) noexcept;

    /**
     * @brief 判断字符分类（避免对负 char 调用 <cctype>）
     */
    static bool isAlpha(char c) noexcept;
    static bool isAlphaNum(char c) noexcept;
    static bool isDigit(char c) noexcept;
    static bool isHexDigit(char c) noexcept;

    /**
     * @brief 记录当前词法单元的起始位置
     */
    void beginToken();

    /**
     * @brief 消费 Lua 换行序列（LF、CR、CRLF 或 LFCR）
     */
    void consumeNewlineSequence();

    /**
     * @brief 若已消费换行序列首字符，则消费可选的第二个配对换行字符
     */
    void consumeNewlinePairRemainder(char firstNewline);

    // =====================================================================
    // 词法单元创建
    // =====================================================================

    /**
     * @brief 创建词法单元
     */
    Token makeToken(TokenType type);

    /**
     * @brief 创建错误词法单元
     */
    Token errorToken(const Str& message);

    // =====================================================================
    // 跳过空白和注释
    // =====================================================================

    /**
     * @brief 跳过空白字符和注释
     */
    Opt<Token> skipWhitespace();

    /**
     * @brief 跳过注释（包括短注释和长注释）
     *
     * 当检测到 '--' 时调用此函数，自动识别是短注释还是长注释
     * 并调用相应的处理函数。
     */
    Opt<Token> skipComment();

    /**
     * @brief 跳过单行注释
     */
    void skipLineComment();

    /**
     * @brief 跳过多行注释
     * @param level 分隔符等级（=的数量）
     */
    Opt<Token> skipLongComment(i32 level);

    // =====================================================================
    // 识别不同类型的词法单元
    // =====================================================================

    /**
     * @brief 识别标识符或关键字
     */
    Token identifier();

    /**
     * @brief 识别数字（十进制或十六进制）
     */
    Token decimalNumber();

    /**
     * @brief 识别十六进制数字
     */
    Token hexadecimalNumber();

    /**
     * @brief 识别字符串（单引号或双引号）
     */
    Token shortString(char quote);

    /**
     * @brief 识别长字符串
     * @param level 分隔符等级（=的数量）
     */
    Token longString(i32 level);

    /**
     * @brief 跳过长字符串/注释的分隔符 [=*[
     * @return 等号数；无效时返回
     * std::nullopt

     */
    Opt<i32> readLongBracketDelimiter();

private:
    /**
     * @brief 内部词法单元解析函数
     * @return 解析后的词法单元对象
     *
     * 实际执行词法分析的核心函数，由获取与预读词法单元的接口调用。
     */
    Token scanToken();

    /**
     * @brief 尝试扫描长字符串 [[ 或 [=[
     * @return 如果是长字符串，返回对应的词法单元；否则返回空值
     *
     * 当检测到'['字符时调用，用于判断是长字符串还是单字符标记。
     */
    Opt<Token> tryLongString();

    /**
     * @brief 在初始 '[' 已经消费后继续读取 =*[ 长字符串起始部分
     */
    Opt<i32> tryReadLongBracketStart();

    /**
     * @brief 长字符串/长注释起始分隔符后丢弃首个换行（Lua 5.1 行为）
     */
    void skipInitialLongLiteralNewline();

    /**
     * @brief 读取一个长字符串正文字符，并按 Lua 5.1 规则规范化换行
     */
    void appendLongStringChar(Str& result);

    /**
     * @brief 读取短字符串反斜杠转义
     */
    Opt<Token> appendShortStringEscape(Str& result);

    /**
     * @brief 读取短字符串十进制转义
     */
    Opt<Token> appendDecimalEscape(char firstDigit, Str& result);

    /**
     * @brief 消费十进制/十六进制数字字符
     */
    void consumeDecimalDigits();
    void consumeHexDigits();

    /**
     * @brief 消费非法数字后缀（如 123abc 或 0x1G）以形成完整错误词素
     */
    void consumeMalformedNumberSuffix();

    /**
     * @brief 处理运算符和分隔符
     * @param c 当前字符
     * @return 词法单元对象
     *
     * 处理所有单字符和多字符运算符、分隔符。
     */
    Token handleOperator(char c);

    /**
     * @brief 处理后缀可能为 '=' 的运算符
     */
    Token handleEqualsSuffix(TokenType singleType, TokenType compoundType);

    /**
     * @brief 处理 '~' 或 '~='
     */
    Token handleTildeOperator();

    /**
     * @brief 处理 '.'、'..'、'...' 或小数
     */
    Token handleDotOperator();

    // =====================================================================
    // 词法分析器状态保存/恢复（用于回溯）
    // =====================================================================

    /**
     * @brief 词法分析器状态快照
     *
     * 用于在解析失败时回退到之前的状态（如长字符串检测失败）。
     */
    struct LexerState {
        usize lexemeLength;
        InputCursor::State input;
        i32 tokenStartLine;
        i32 tokenStartColumn;
    };

    /**
     * @brief 保存当前词法分析器状态
     */
    LexerState saveState() const;

    /**
     * @brief 恢复之前保存的词法分析器状态
     */
    void restoreState(const LexerState& state);

private:
    // =====================================================================
    // 输入流管理
    // =====================================================================

    /** @brief 编译期回调快照；传给游标和词法单元。 */
    LuaAllocator allocator_;

    /**
     * @brief 从字符串构造词法分析器时保存源代码副本。
     * @note 保证输入流持有的字符串视图在词法分析器的整个生命周期内始终有效；
     * 此成员不再用于基于下标的字符访问，仅用于管理生命周期。
     */
    LuaOwnedString sourceStorage_;

    /** @brief 拥有的输入流（用于字符串构造） */
    UPtr<IO::InputStream> ownedInput_;

    // =====================================================================
    // 字符游标（用于实现 peek、peekNext、位置跟踪和内部回溯）
    // =====================================================================

    InputCursor inputCursor_;

    // =====================================================================
    // 词素累积缓冲区
    // =====================================================================

    /** @brief 累积当前词法单元的字符。 */
    LuaOwnedString lexemeBuffer_;

    // =====================================================================
    // 位置跟踪
    // =====================================================================

    /** @brief 当前词法单元的起始行号。 */
    i32 tokenStartLine_;
    /** @brief 当前词法单元的起始列号。 */
    i32 tokenStartColumn_;

    // =====================================================================
    // 词法单元预读机制（支持 LL(1) 语法分析）
    // =====================================================================

    /** @brief 预读词法单元缓存。 */
    Opt<Token> lookahead_;
};

} // namespace Lua
