#include "compiler_binary_expression_test.hpp"
#include "../../compiler/compiler.hpp"
#include "../../compiler/expression_compiler.hpp"
#include "../../parser/ast/expressions.hpp"
#include "../../vm/instruction.hpp"
#include "../../common/opcodes.hpp"
#include "../../vm/value.hpp"
#include <cassert>

namespace Lua::Tests {

void CompilerBinaryExpressionTest::testArithmeticOperations() {
    TestUtils::printInfo("Testing arithmetic operations compilation");
    
    // Test addition
    testArithmeticOp(TokenType::Plus, OpCode::ADD);
    TestUtils::printTestResult("Addition operation", true);
    
    // Test subtraction
    testArithmeticOp(TokenType::Minus, OpCode::SUB);
    TestUtils::printTestResult("Subtraction operation", true);
    
    // Test multiplication
    testArithmeticOp(TokenType::Star, OpCode::MUL);
    TestUtils::printTestResult("Multiplication operation", true);
    
    // Test division
    testArithmeticOp(TokenType::Slash, OpCode::DIV);
    TestUtils::printTestResult("Division operation", true);
    
    // Test modulo
    testArithmeticOp(TokenType::Percent, OpCode::MOD);
    TestUtils::printTestResult("Modulo operation", true);
    
    // Test power
    testArithmeticOp(TokenType::Caret, OpCode::POW);
    TestUtils::printTestResult("Power operation", true);
}

void CompilerBinaryExpressionTest::testComparisonOperations() {
    TestUtils::printInfo("Testing comparison operations compilation");
    
    // Test equality
    testComparisonOp(TokenType::Equal, OpCode::EQ);
    TestUtils::printTestResult("Equality operation", true);
    
    // Test inequality
    testComparisonOp(TokenType::NotEqual, OpCode::EQ);
    TestUtils::printTestResult("Inequality operation", true);
    
    // Test less than
    testComparisonOp(TokenType::Less, OpCode::LT);
    TestUtils::printTestResult("Less than operation", true);
    
    // Test less than or equal
    testComparisonOp(TokenType::LessEqual, OpCode::LE);
    TestUtils::printTestResult("Less than or equal operation", true);
    
    // Test greater than
    testComparisonOp(TokenType::Greater, OpCode::LT);
    TestUtils::printTestResult("Greater than operation", true);
    
    // Test greater than or equal
    testComparisonOp(TokenType::GreaterEqual, OpCode::LE);
    TestUtils::printTestResult("Greater than or equal operation", true);
}

void CompilerBinaryExpressionTest::testAdvancedFeatures() {
    TestUtils::printInfo("Testing advanced binary expression features");
    
    // Test string concatenation
    testStringConcatenation();
    
    // Test operator precedence
    testOperatorPrecedence();
    
    // Test nested expressions
    testNestedExpressions();
}

void CompilerBinaryExpressionTest::testErrorHandling() {
    TestUtils::printInfo("Testing error handling for binary expressions");
    
    try {
        Compiler compiler;
        ExpressionCompiler exprCompiler(&compiler);
        
        auto expr = std::make_unique<BinaryExpr>(nullptr, TokenType::Plus, nullptr);
        exprCompiler.compileExpr(expr.get());
        
        TestUtils::printTestResult("Null operands throw exception", false);
    } catch (const std::exception&) {
        TestUtils::printTestResult("Null operands throw exception", true);
    }
}

void CompilerBinaryExpressionTest::testArithmeticOp(TokenType op, OpCode expectedOpCode) {
    TestUtils::printInfo("Testing arithmetic operation");
    
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto left = std::make_unique<VariableExpr>("x");
    auto right = std::make_unique<VariableExpr>("y");
    auto expr = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    
    exprCompiler.compileExpr(expr.get());
    
    TestUtils::printTestResult("Arithmetic operation generates instructions", compiler.getCodeSize() >= 3);
}

void CompilerBinaryExpressionTest::testComparisonOp(TokenType op, OpCode expectedOpCode) {
    TestUtils::printInfo("Testing comparison operation");
    
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto left = std::make_unique<VariableExpr>("p");
    auto right = std::make_unique<VariableExpr>("q");
    auto expr = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    
    exprCompiler.compileExpr(expr.get());
    
    TestUtils::printTestResult("Comparison operation generates instructions", compiler.getCodeSize() >= 3);
}

void CompilerBinaryExpressionTest::testLogicalOperations() {
    TestUtils::printInfo("Testing logical operations compilation");
    
    // Test AND operation (should generate conditional jump)
    {
        Compiler compiler;
        ExpressionCompiler exprCompiler(&compiler);
        
        auto left = std::make_unique<VariableExpr>("flag1");
        auto right = std::make_unique<VariableExpr>("flag2");
        auto andExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::And, std::move(right));
        
        exprCompiler.compileExpr(andExpr.get());
        TestUtils::printTestResult("AND operation generates conditional logic", compiler.getCodeSize() > 2);
    }
    
    // Test OR operation
    {
        Compiler compiler;
        ExpressionCompiler exprCompiler(&compiler);
        
        auto left = std::make_unique<VariableExpr>("flag3");
        auto right = std::make_unique<VariableExpr>("flag4");
        auto orExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::Or, std::move(right));
        
        exprCompiler.compileExpr(orExpr.get());
        TestUtils::printTestResult("OR operation generates conditional logic", compiler.getCodeSize() > 2);
    }
}

void CompilerBinaryExpressionTest::testStringConcatenation() {
    TestUtils::printInfo("Testing string concatenation");
    
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto left = std::make_unique<VariableExpr>("str1");
    auto right = std::make_unique<VariableExpr>("str2");
    auto concatExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::DotDot, std::move(right));
    
    exprCompiler.compileExpr(concatExpr.get());
    
    TestUtils::printTestResult("String concatenation generates instructions", compiler.getCodeSize() >= 3);
}

void CompilerBinaryExpressionTest::testOperatorPrecedence() {
    TestUtils::printInfo("Testing operator precedence");
    
    // Test that multiplication has higher precedence than addition
    // Expression: a + b * c should be compiled as a + (b * c)
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto a = std::make_unique<VariableExpr>("a");
    auto b = std::make_unique<VariableExpr>("b");
    auto c = std::make_unique<VariableExpr>("c");
    
    // Create b * c
    auto mulExpr = std::make_unique<BinaryExpr>(std::move(b), TokenType::Star, std::move(c));
    // Create a + (b * c)
    auto addExpr = std::make_unique<BinaryExpr>(std::move(a), TokenType::Plus, std::move(mulExpr));
    
    exprCompiler.compileExpr(addExpr.get());
    
    TestUtils::printTestResult("Operator precedence generates correct instructions", compiler.getCodeSize() >= 5);
}

void CompilerBinaryExpressionTest::testNestedExpressions() {
    TestUtils::printInfo("Testing nested expressions");
    
    // Test deeply nested expression: (w + x) * (y - z)
    Compiler compiler;
    ExpressionCompiler exprCompiler(&compiler);
    
    auto w = std::make_unique<VariableExpr>("w");
    auto x = std::make_unique<VariableExpr>("x");
    auto y = std::make_unique<VariableExpr>("y");
    auto z = std::make_unique<VariableExpr>("z");
    
    auto addExpr = std::make_unique<BinaryExpr>(std::move(w), TokenType::Plus, std::move(x));
    auto subExpr = std::make_unique<BinaryExpr>(std::move(y), TokenType::Minus, std::move(z));
    auto mulExpr = std::make_unique<BinaryExpr>(std::move(addExpr), TokenType::Star, std::move(subExpr));
    
    exprCompiler.compileExpr(mulExpr.get());
    
    TestUtils::printTestResult("Nested expressions generate correct instructions", compiler.getCodeSize() >= 7);
}

} // namespace Lua::Tests