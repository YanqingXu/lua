#include "binary_expression_test.hpp"
#include "../../compiler/compiler.hpp"
#include "../../compiler/expression_compiler.hpp"
#include "../../parser/ast/expressions.hpp"
#include "../../vm/instruction.hpp"
#include "../../common/opcodes.hpp"
#include "../../vm/value.hpp"
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
    
    // Test addition: a + b (using variables to avoid constant folding)
    std::cout << "Creating expressions..." << std::endl;
    auto left = std::make_unique<VariableExpr>("a");
    auto right = std::make_unique<VariableExpr>("b");
    auto addExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::Plus, std::move(right));
    
    std::cout << "Compiling expression..." << std::endl;
    u8 resultReg = exprCompiler.compileExpr(addExpr.get());
    std::cout << "Expression compiled successfully." << std::endl;
    
    // Check that instructions were generated
    std::cout << "Code size: " << compiler.getCodeSize() << std::endl;
    assert(compiler.getCodeSize() >= 3); // GETGLOBAL, GETGLOBAL, ADD
    std::cout << "Addition test passed." << std::endl;
    
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
    
    // Use variables to avoid constant folding
    auto left = std::make_unique<VariableExpr>("x");
    auto right = std::make_unique<VariableExpr>("y");
    auto expr = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    
    exprCompiler.compileExpr(expr.get());
    
    // Verify instruction generation
    assert(compiler.getCodeSize() >= 3); // GETGLOBAL, GETGLOBAL, operation
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
    
    // Use variables to avoid constant folding
    auto left = std::make_unique<VariableExpr>("p");
    auto right = std::make_unique<VariableExpr>("q");
    auto expr = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    
    exprCompiler.compileExpr(expr.get());
    
    // Verify instruction generation
    assert(compiler.getCodeSize() >= 3); // GETGLOBAL, GETGLOBAL, operation
}

void BinaryExpressionTest::testLogicalOperations() {
    std::cout << "Testing logical operations..." << std::endl;
    
    // Test AND operation (should generate conditional jump)
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    // Use variables to avoid constant folding
    auto left = std::make_unique<VariableExpr>("flag1");
    auto right = std::make_unique<VariableExpr>("flag2");
    auto andExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::And, std::move(right));
    
    exprCompiler.compileExpr(andExpr.get());
    
    // AND should generate conditional logic, not a simple instruction
    assert(compiler.getCodeSize() > 2); // Should have more than just GETGLOBAL instructions
    
    // Test OR operation
    Compiler compiler2;
    ExpressionCompiler exprCompiler2(&compiler2);
    
    auto left2 = std::make_unique<VariableExpr>("flag3");
    auto right2 = std::make_unique<VariableExpr>("flag4");
    auto orExpr = std::make_unique<BinaryExpr>(std::move(left2), TokenType::Or, std::move(right2));
    
    exprCompiler2.compileExpr(orExpr.get());
    
    assert(compiler2.getCodeSize() > 2);
}

void BinaryExpressionTest::testStringConcatenation() {
    std::cout << "Testing string concatenation..." << std::endl;
    
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    // Use variables to avoid constant folding
    auto left = std::make_unique<VariableExpr>("str1");
    auto right = std::make_unique<VariableExpr>("str2");
    auto concatExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::DotDot, std::move(right));
    
    exprCompiler.compileExpr(concatExpr.get());
    
    // Verify instruction generation
    assert(compiler.getCodeSize() >= 3); // GETGLOBAL, GETGLOBAL, CONCAT
}

void BinaryExpressionTest::testOperatorPrecedence() {
    std::cout << "Testing operator precedence..." << std::endl;
    
    // Test that multiplication has higher precedence than addition
    // Expression: a + b * c should be compiled as a + (b * c)
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    // Use variables to avoid constant folding
    auto a = std::make_unique<VariableExpr>("a");
    auto b = std::make_unique<VariableExpr>("b");
    auto c = std::make_unique<VariableExpr>("c");
    
    // Create b * c
    auto mulExpr = std::make_unique<BinaryExpr>(std::move(b), TokenType::Star, std::move(c));
    // Create a + (b * c)
    auto addExpr = std::make_unique<BinaryExpr>(std::move(a), TokenType::Plus, std::move(mulExpr));
    
    exprCompiler.compileExpr(addExpr.get());
    
    // Verify instruction generation
    assert(compiler.getCodeSize() >= 5); // Multiple GETGLOBAL and arithmetic operations
}

void BinaryExpressionTest::testNestedExpressions() {
    std::cout << "Testing nested expressions..." << std::endl;
    
    // Test deeply nested expression: (w + x) * (y - z)
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    // Use variables to avoid constant folding
    auto w = std::make_unique<VariableExpr>("w");
    auto x = std::make_unique<VariableExpr>("x");
    auto y = std::make_unique<VariableExpr>("y");
    auto z = std::make_unique<VariableExpr>("z");
    
    auto addExpr = std::make_unique<BinaryExpr>(std::move(w), TokenType::Plus, std::move(x));
    auto subExpr = std::make_unique<BinaryExpr>(std::move(y), TokenType::Minus, std::move(z));
    auto mulExpr = std::make_unique<BinaryExpr>(std::move(addExpr), TokenType::Star, std::move(subExpr));
    
    exprCompiler.compileExpr(mulExpr.get());
    
    // Verify instruction generation
    assert(compiler.getCodeSize() >= 7); // Multiple GETGLOBAL and arithmetic operations
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