/**
 * @file test_lexer_lookahead.cpp
 * @brief 测试词法分析器的Token预读机制（LL(1)支持）
 */

#include "../framework/test_framework.hpp"
#include "../framework/test_registry.hpp"
#include "compiler/lexer/lexer.hpp"
#include <string>

using namespace Lua;
using namespace LuaTest;

// 测试基本的Token预读功能
static void testBasicLookahead(TestSuite& suite) {
    Lexer lexer("local x = 42");
    
    // 预读第一个Token（不消费）
    Token peeked = lexer.peekToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Local), static_cast<int>(peeked.type), 
              "Peek should return 'local'");
    
    // 再次预读应该返回相同的Token
    Token peeked2 = lexer.peekToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Local), static_cast<int>(peeked2.type), 
              "Second peek should return same token");
    
    // 现在消费Token
    Token consumed = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Local), static_cast<int>(consumed.type), 
              "nextToken should return the peeked token");
    
    // 下一个Token应该是标识符 'x'
    Token next = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), static_cast<int>(next.type), 
              "Next token should be identifier");
}

// 测试预读和消费交替进行
static void testAlternatingPeekAndNext(TestSuite& suite) {
    Lexer lexer("if then else");
    
    // 预读 'if'
    Token peek1 = lexer.peekToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::If), static_cast<int>(peek1.type), 
              "First peek should be 'if'");
    
    // 消费 'if'
    Token next1 = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::If), static_cast<int>(next1.type), 
              "First next should be 'if'");
    
    // 预读 'then'
    Token peek2 = lexer.peekToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Then), static_cast<int>(peek2.type), 
              "Second peek should be 'then'");
    
    // 消费 'then'
    Token next2 = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Then), static_cast<int>(next2.type), 
              "Second next should be 'then'");
    
    // 消费 'else' (不预读)
    Token next3 = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Else), static_cast<int>(next3.type), 
              "Third next should be 'else'");
}

// 测试预读不影响词法分析器状态（行号、列号）
static void testLookaheadPreservesState(TestSuite& suite) {
    Lexer lexer("local\nx\n=\n42");
    
    // 第一个Token：local (第1行)
    Token token1 = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Local), static_cast<int>(token1.type), 
              "First token is 'local'");
    ASSERT_EQ(suite, 1, token1.line, "Token 'local' is on line 1");
    
    // 预读第二个Token：x (第2行)
    Token peeked = lexer.peekToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), static_cast<int>(peeked.type), 
              "Peeked token is identifier");
    ASSERT_EQ(suite, 2, peeked.line, "Peeked token is on line 2");
    
    // 消费第二个Token，应该返回相同的Token
    Token token2 = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), static_cast<int>(token2.type), 
              "Second token is identifier");
    ASSERT_EQ(suite, 2, token2.line, "Second token is on line 2");
    ASSERT_EQ(suite, peeked.lexeme, token2.lexeme, "Lexeme matches peeked token");
}

// 测试在文件末尾的预读
static void testLookaheadAtEOF(TestSuite& suite) {
    Lexer lexer("end");
    
    // 消费唯一的Token
    Token token1 = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::End), static_cast<int>(token1.type), 
              "Token is 'end'");
    
    // 预读应该返回EOF
    Token peeked = lexer.peekToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Eos), static_cast<int>(peeked.type), 
              "Peek at EOF returns Eos");
    
    // 消费应该也返回EOF
    Token token2 = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Eos), static_cast<int>(token2.type), 
              "Next at EOF returns Eos");
    
    // 多次调用nextToken应该继续返回EOF
    Token token3 = lexer.nextToken();
    ASSERT_EQ(suite, static_cast<int>(TokenType::Eos), static_cast<int>(token3.type), 
              "Multiple next at EOF returns Eos");
}

// 测试复杂表达式的预读
static void testLookaheadInExpression(TestSuite& suite) {
    Lexer lexer("a + b * c");
    
    // a
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), 
              static_cast<int>(lexer.nextToken().type), "Token: a");
    
    // 预读 '+'
    Token peeked1 = lexer.peekToken();
    ASSERT_EQ(suite, static_cast<int>(static_cast<TokenType>('+')), 
              static_cast<int>(peeked1.type), "Peek: +");
    
    // 消费 '+'
    ASSERT_EQ(suite, static_cast<int>(static_cast<TokenType>('+')), 
              static_cast<int>(lexer.nextToken().type), "Token: +");
    
    // b
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), 
              static_cast<int>(lexer.nextToken().type), "Token: b");
    
    // 预读 '*'
    Token peeked2 = lexer.peekToken();
    ASSERT_EQ(suite, static_cast<int>(static_cast<TokenType>('*')), 
              static_cast<int>(peeked2.type), "Peek: *");
    
    // 消费 '*'
    ASSERT_EQ(suite, static_cast<int>(static_cast<TokenType>('*')), 
              static_cast<int>(lexer.nextToken().type), "Token: *");
    
    // c
    ASSERT_EQ(suite, static_cast<int>(TokenType::Name), 
              static_cast<int>(lexer.nextToken().type), "Token: c");
}

// 主测试注册函数
void registerLexerLookaheadTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest("Lexer Lookahead", "Basic lookahead", testBasicLookahead);
    registry.registerTest("Lexer Lookahead", "Alternating peek and next", testAlternatingPeekAndNext);
    registry.registerTest("Lexer Lookahead", "Lookahead preserves state", testLookaheadPreservesState);
    registry.registerTest("Lexer Lookahead", "Lookahead at EOF", testLookaheadAtEOF);
    registry.registerTest("Lexer Lookahead", "Lookahead in expression", testLookaheadInExpression);
}
