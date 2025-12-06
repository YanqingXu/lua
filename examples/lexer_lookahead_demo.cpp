/**
 * @file example_lexer_lookahead.cpp
 * @brief Token预读机制的使用示例
 * 
 * 演示如何使用Lexer的peekToken()方法实现高效的LL(1)语法分析
 */

#include "compiler/lexer.hpp"
#include <iostream>

using namespace Lua;

/**
 * @brief 演示基本的Token预读
 */
void demonstrateBasicLookahead() {
    std::cout << "=== 基本Token预读演示 ===" << std::endl;
    
    Lexer lexer("local x = 42");
    
    // 预读但不消费
    Token peeked = lexer.peekToken();
    std::cout << "预读Token: " << peeked.lexeme << " (类型: " 
              << static_cast<int>(peeked.type) << ")" << std::endl;
    
    // 再次预读应该返回相同的Token
    Token peeked2 = lexer.peekToken();
    std::cout << "再次预读: " << peeked2.lexeme << " (相同: " 
              << (peeked.lexeme == peeked2.lexeme ? "是" : "否") << ")" << std::endl;
    
    // 消费Token
    Token consumed = lexer.nextToken();
    std::cout << "消费Token: " << consumed.lexeme << std::endl;
    
    // 下一个Token
    Token next = lexer.nextToken();
    std::cout << "下一个Token: " << next.lexeme << std::endl;
    
    std::cout << std::endl;
}

/**
 * @brief 演示在语法分析中使用预读
 * 
 * 模拟解析赋值语句或函数调用的场景：
 * - identifier '=' expression  -> 赋值语句
 * - identifier '(' args ')'    -> 函数调用
 */
void demonstrateSyntaxAnalysis() {
    std::cout << "=== 语法分析中的预读演示 ===" << std::endl;
    
    // 场景1：赋值语句
    {
        Lexer lexer("x = 10");
        Token identifier = lexer.nextToken();
        std::cout << "标识符: " << identifier.lexeme << std::endl;
        
        // 预读下一个Token以判断是赋值还是函数调用
        Token next = lexer.peekToken();
        if (next.type == static_cast<TokenType>('=')) {
            std::cout << "  -> 这是一个赋值语句" << std::endl;
            lexer.nextToken(); // 消费 '='
            Token value = lexer.nextToken();
            std::cout << "  -> 赋值为: " << value.lexeme << std::endl;
        }
    }
    
    std::cout << std::endl;
    
    // 场景2：函数调用
    {
        Lexer lexer("print(42)");
        Token identifier = lexer.nextToken();
        std::cout << "标识符: " << identifier.lexeme << std::endl;
        
        // 预读下一个Token
        Token next = lexer.peekToken();
        if (next.type == static_cast<TokenType>('(')) {
            std::cout << "  -> 这是一个函数调用" << std::endl;
            lexer.nextToken(); // 消费 '('
            Token arg = lexer.nextToken();
            std::cout << "  -> 参数: " << arg.lexeme << std::endl;
        }
    }
    
    std::cout << std::endl;
}

/**
 * @brief 演示预读保持词法分析器状态
 */
void demonstrateStatePreservation() {
    std::cout << "=== 状态保持演示 ===" << std::endl;
    
    Lexer lexer("line1\nline2\nline3");
    
    // 第一个Token
    Token token1 = lexer.nextToken();
    std::cout << "Token1: " << token1.lexeme << " (行号: " 
              << token1.line << ")" << std::endl;
    
    // 预读第二个Token（应该在第2行）
    Token peeked = lexer.peekToken();
    std::cout << "预读Token: " << peeked.lexeme << " (行号: " 
              << peeked.line << ")" << std::endl;
    
    // 消费第二个Token
    Token token2 = lexer.nextToken();
    std::cout << "Token2: " << token2.lexeme << " (行号: " 
              << token2.line << ")" << std::endl;
    
    // 验证行号正确
    std::cout << "行号正确: " << (peeked.line == token2.line ? "是" : "否") << std::endl;
    
    std::cout << std::endl;
}

/**
 * @brief 演示表达式解析中的预读
 * 
 * 使用预读判断运算符优先级
 */
void demonstrateExpressionParsing() {
    std::cout << "=== 表达式解析预读演示 ===" << std::endl;
    
    Lexer lexer("a + b * c");
    
    // 解析: a
    Token a = lexer.nextToken();
    std::cout << "操作数: " << a.lexeme << std::endl;
    
    // 预读运算符: +
    Token op1 = lexer.peekToken();
    std::cout << "运算符1: " << op1.lexeme << " (预读)" << std::endl;
    lexer.nextToken(); // 消费
    
    // 解析: b
    Token b = lexer.nextToken();
    std::cout << "操作数: " << b.lexeme << std::endl;
    
    // 预读运算符: *
    Token op2 = lexer.peekToken();
    std::cout << "运算符2: " << op2.lexeme << " (预读)" << std::endl;
    
    // 根据运算符优先级决定解析顺序
    std::cout << "  -> '*' 的优先级高于 '+'，先计算 b * c" << std::endl;
    
    lexer.nextToken(); // 消费 *
    Token c = lexer.nextToken();
    std::cout << "操作数: " << c.lexeme << std::endl;
    
    std::cout << std::endl;
}

int main() {
    std::cout << "Token预读机制演示程序" << std::endl;
    std::cout << "======================" << std::endl << std::endl;
    
    demonstrateBasicLookahead();
    demonstrateSyntaxAnalysis();
    demonstrateStatePreservation();
    demonstrateExpressionParsing();
    
    std::cout << "演示完成！" << std::endl;
    return 0;
}
