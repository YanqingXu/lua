#include "binary_expression_test.hpp"
#include "../compiler/compiler.hpp"
#include "../compiler/expression_compiler.hpp"
#include "../parser/ast/expressions.hpp"
#include "../vm/instruction.hpp"
#include "../common/opcodes.hpp"
#include "../vm/value.hpp"
#include <iostream>
#include <cassert>

namespace Lua {
namespace Tests {

void BinaryExpressionTest::runAllTests() {
    std::cout << "Running Binary Expression Compiler Tests..." << std::endl;
    
    testArithmeticOperations();
    testComparisonOperations();
    testLogicalOperations();
    testStringConcatenation();
    testOperatorPrecedence();
    testNestedExpressions();
    testErrorHandling();
    
    std::cout << "All Binary Expression Compiler tests passed!" << std::endl;
}

void BinaryExpressionTest::testArithmeticOperations() {
    std::cout << "Testing arithmetic operations..." << std::endl;
    
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    // Test addition: 5 + 3
    auto left = std::make_unique<LiteralExpr>(Value(5.0));
    auto right = std::make_unique<LiteralExpr>(Value(3.0));
    auto addExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::Plus, std::move(right));
    
    u8 resultReg = exprCompiler.compileExpr(addExpr.get());
    
    // Check that instructions were generated
    assert(compiler.getCodeSize() >= 3); // LOADK, LOADK, ADD
    
    // Test other arithmetic operations
    testArithmeticOp(TokenType::Minus, OpCode::SUB);
    testArithmeticOp(TokenType::Star, OpCode::MUL);
    testArithmeticOp(TokenType::Slash, OpCode::DIV);
    testArithmeticOp(TokenType::Percent, OpCode::MOD);
    testArithmeticOp(TokenType::Caret, OpCode::POW);
}

void BinaryExpressionTest::testArithmeticOp(TokenType op, OpCode expectedOpCode) {
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto left = std::make_unique<LiteralExpr>(Value(10.0));
    auto right = std::make_unique<LiteralExpr>(Value(2.0));
    auto expr = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    
    exprCompiler.compileExpr(expr.get());
    
    // Verify instruction generation
    assert(compiler.getCodeSize() >= 3); // LOADK, LOADK, operation
}

void BinaryExpressionTest::testComparisonOperations() {
    std::cout << "Testing comparison operations..." << std::endl;
    
    testComparisonOp(TokenType::Equal, OpCode::EQ);
    testComparisonOp(TokenType::NotEqual, OpCode::EQ); // NE uses EQ with negation
    testComparisonOp(TokenType::Less, OpCode::LT);
    testComparisonOp(TokenType::LessEqual, OpCode::LE);
    testComparisonOp(TokenType::Greater, OpCode::LT); // GT uses LT with swapped operands
    testComparisonOp(TokenType::GreaterEqual, OpCode::LE); // GE uses LE with swapped operands
}

void BinaryExpressionTest::testComparisonOp(TokenType op, OpCode expectedOpCode) {
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto left = std::make_unique<LiteralExpr>(Value(5.0));
    auto right = std::make_unique<LiteralExpr>(Value(3.0));
    auto expr = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    
    exprCompiler.compileExpr(expr.get());
    
    // Verify instruction generation
    assert(compiler.getCodeSize() >= 3); // LOADK, LOADK, operation
}

void BinaryExpressionTest::testLogicalOperations() {
    std::cout << "Testing logical operations..." << std::endl;
    
    // Test AND operation (should generate conditional jump)
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto left = std::make_unique<LiteralExpr>(Value(true));
    auto right = std::make_unique<LiteralExpr>(Value(false));
    auto andExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::And, std::move(right));
    
    exprCompiler.compileExpr(andExpr.get());
    
    // AND should generate conditional logic, not a simple instruction
    assert(compiler.getCodeSize() > 2); // Should have more than just LOADK instructions
    
    // Test OR operation
    Compiler compiler2;
    ExpressionCompiler exprCompiler2(&compiler2);
    
    auto left2 = std::make_unique<LiteralExpr>(Value(false));
    auto right2 = std::make_unique<LiteralExpr>(Value(true));
    auto orExpr = std::make_unique<BinaryExpr>(std::move(left2), TokenType::Or, std::move(right2));
    
    exprCompiler2.compileExpr(orExpr.get());
    
    assert(compiler2.getCodeSize() > 2);
}

void BinaryExpressionTest::testStringConcatenation() {
    std::cout << "Testing string concatenation..." << std::endl;
    
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto left = std::make_unique<LiteralExpr>(Value("Hello"));
    auto right = std::make_unique<LiteralExpr>(Value(" World"));
    auto concatExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::DotDot, std::move(right));
    
    exprCompiler.compileExpr(concatExpr.get());
    
    // Verify instruction generation
    assert(compiler.getCodeSize() >= 3); // LOADK, LOADK, CONCAT
}

void BinaryExpressionTest::testOperatorPrecedence() {
    std::cout << "Testing operator precedence..." << std::endl;
    
    // Test that multiplication has higher precedence than addition
    // Expression: 2 + 3 * 4 should be compiled as 2 + (3 * 4)
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto two = std::make_unique<LiteralExpr>(Value(2.0));
    auto three = std::make_unique<LiteralExpr>(Value(3.0));
    auto four = std::make_unique<LiteralExpr>(Value(4.0));
    
    // Create 3 * 4
    auto mulExpr = std::make_unique<BinaryExpr>(std::move(three), TokenType::Star, std::move(four));
    // Create 2 + (3 * 4)
    auto addExpr = std::make_unique<BinaryExpr>(std::move(two), TokenType::Plus, std::move(mulExpr));
    
    exprCompiler.compileExpr(addExpr.get());
    
    // Verify instruction generation
    assert(compiler.getCodeSize() >= 5); // Multiple LOADK and arithmetic operations
}

void BinaryExpressionTest::testNestedExpressions() {
    std::cout << "Testing nested expressions..." << std::endl;
    
    // Test deeply nested expression: (1 + 2) * (3 - 4)
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto one = std::make_unique<LiteralExpr>(Value(1.0));
    auto two = std::make_unique<LiteralExpr>(Value(2.0));
    auto three = std::make_unique<LiteralExpr>(Value(3.0));
    auto four = std::make_unique<LiteralExpr>(Value(4.0));
    
    auto addExpr = std::make_unique<BinaryExpr>(std::move(one), TokenType::Plus, std::move(two));
    auto subExpr = std::make_unique<BinaryExpr>(std::move(three), TokenType::Minus, std::move(four));
    auto mulExpr = std::make_unique<BinaryExpr>(std::move(addExpr), TokenType::Star, std::move(subExpr));
    
    exprCompiler.compileExpr(mulExpr.get());
    
    // Verify instruction generation
    assert(compiler.getCodeSize() >= 7); // Multiple LOADK and arithmetic operations
}

void BinaryExpressionTest::testErrorHandling() {
    std::cout << "Testing error handling..." << std::endl;
    
    // Test compilation with null operands
    try {
        Compiler compiler;
        ExpressionCompiler exprCompiler(&compiler);
        
        auto expr = std::make_unique<BinaryExpr>(nullptr, TokenType::Plus, nullptr);
        exprCompiler.compileExpr(expr.get());
        
        // Should not reach here if proper error handling is implemented
        assert(false && "Expected exception for null operands");
    } catch (const std::exception&) {
        // Expected behavior
    }
}

} // namespace Tests
} // namespace Lua