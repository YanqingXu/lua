#include <gtest/gtest.h>
#include "lexer/lexer.hpp"

using namespace Lua;

// 词法分析器基本测试
TEST(LexerTest, BasicTokens) {
    Lexer lexer("local x = 10");
    
    // 测试'local'关键字
    Token token = lexer.nextToken();
    EXPECT_EQ(TokenType::Local, token.type);
    EXPECT_EQ("local", token.lexeme);
    
    // 测试标识符'x'
    token = lexer.nextToken();
    EXPECT_EQ(TokenType::Identifier, token.type);
    EXPECT_EQ("x", token.lexeme);
    
    // 测试'='
    token = lexer.nextToken();
    EXPECT_EQ(TokenType::Assign, token.type);
    EXPECT_EQ("=", token.lexeme);
    
    // 测试数字'10'
    token = lexer.nextToken();
    EXPECT_EQ(TokenType::Number, token.type);
    EXPECT_EQ("10", token.lexeme);
    
    // 测试文件结束
    token = lexer.nextToken();
    EXPECT_EQ(TokenType::Eof, token.type);
}

// 测试所有关键字
TEST(LexerTest, Keywords) {
    Lexer lexer("and break do else elseif end false for function if in local nil not or repeat return then true until while");
    
    std::vector<TokenType> expectedTypes = {
        TokenType::And, TokenType::Break, TokenType::Do, 
        TokenType::Else, TokenType::Elseif, TokenType::End, 
        TokenType::False, TokenType::For, TokenType::Function, 
        TokenType::If, TokenType::In, TokenType::Local, 
        TokenType::Nil, TokenType::Not, TokenType::Or, 
        TokenType::Repeat, TokenType::Return, TokenType::Then, 
        TokenType::True, TokenType::Until, TokenType::While
    };
    
    for (const auto& expectedType : expectedTypes) {
        Token token = lexer.nextToken();
        EXPECT_EQ(expectedType, token.type);
    }
}

// 测试所有运算符
TEST(LexerTest, Operators) {
    Lexer lexer("+ - * / % ^ # == ~= <= >= < > = ( ) { } [ ] ; : , . .. ...");
    
    std::vector<TokenType> expectedTypes = {
        TokenType::Plus, TokenType::Minus, TokenType::OpMul, 
        TokenType::OpDiv, TokenType::OpMod, TokenType::OpPow, 
        TokenType::OpLen, TokenType::OpEq, TokenType::OpNe, 
        TokenType::OpLe, TokenType::OpGe, TokenType::OpLt, 
        TokenType::OpGt, TokenType::OpAssign, 
        TokenType::SymLeftParen, TokenType::SymRightParen, 
        TokenType::SymLeftBrace, TokenType::SymRightBrace, 
        TokenType::SymLeftBracket, TokenType::SymRightBracket, 
        TokenType::SymSemicolon, TokenType::SymColon, 
        TokenType::SymComma, TokenType::SymDot, 
        TokenType::SymConcat, TokenType::SymVararg
    };
    
    for (const auto& expectedType : expectedTypes) {
        Token token = lexer.nextToken();
        EXPECT_EQ(expectedType, token.type);
    }
}

// 测试字符串
TEST(LexerTest, Strings) {
    // 单引号字符串
    Lexer lexer1("'Hello, Lua!'");
    Token token1 = lexer1.nextToken();
    EXPECT_EQ(TokenType::String, token1.type);
    EXPECT_EQ("Hello, Lua!", token1.lexeme);
    
    // 双引号字符串
    Lexer lexer2("\"Hello, Lua!\"");
    Token token2 = lexer2.nextToken();
    EXPECT_EQ(TokenType::String, token2.type);
    EXPECT_EQ("Hello, Lua!", token2.lexeme);
    
    // 转义序列
    Lexer lexer3("'\\n\\t\\\"\\'\\\\'");
    Token token3 = lexer3.nextToken();
    EXPECT_EQ(TokenType::String, token3.type);
    EXPECT_EQ("\n\t\"\'\\\", token3.lexeme);
}

// 测试注释
TEST(LexerTest, Comments) {
    // 单行注释
    Lexer lexer1("-- This is a comment\nlocal x = 10");
    Token token1 = lexer1.nextToken();
    EXPECT_EQ(TokenType::Local, token1.type); // 注释被跳过
    
    // 多行注释
    Lexer lexer2("--[[ This is a\nmulti-line comment ]]\nlocal x = 10");
    Token token2 = lexer2.nextToken();
    EXPECT_EQ(TokenType::Local, token2.type); // 注释被跳过
}

// 测试行号和列号
TEST(LexerTest, LineAndColumnNumbers) {
    Lexer lexer("local x = 10\ny = 20");
    
    // 测试'local'
    Token token = lexer.nextToken();
    EXPECT_EQ(1, token.line);
    EXPECT_EQ(1, token.column);
    
    // 跳到下一行
    lexer.nextToken(); // x
    lexer.nextToken(); // =
    lexer.nextToken(); // 10
    token = lexer.nextToken(); // y
    EXPECT_EQ(2, token.line);
    EXPECT_EQ(1, token.column);
}

// 测试错误处理
TEST(LexerTest, ErrorHandling) {
    // 无效的词法单元
    Lexer lexer("@");
    Token token = lexer.nextToken();
    EXPECT_EQ(TokenType::Error, token.type);
    
    // 不匹配的字符串
    Lexer lexer2("'unclosed string");
    token = lexer2.nextToken();
    EXPECT_EQ(TokenType::Error, token.type);
}
