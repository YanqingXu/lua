#include "expression_compiler_test.hpp"
#include "../compiler/expression_compiler.hpp"
#include "../compiler/compiler.hpp"
#include "../parser/ast/expressions.hpp"
#include "../vm/value.hpp"
#include <iostream>
#include <cassert>

namespace Lua {
    void ExpressionCompilerTest::runAllTests() {
        std::cout << "Running Expression Compiler Tests..." << std::endl;
        
        testLiteralCompilation();
        testBinaryExpressionCompilation();
        testTableConstructorCompilation();
        testConstantFolding();
        testUnaryExpressionCompilation();
        
        std::cout << "All Expression Compiler Tests Passed!" << std::endl;
    }
    
    void ExpressionCompilerTest::testLiteralCompilation() {
        std::cout << "Testing literal compilation..." << std::endl;
        
        Compiler compiler;
        ExpressionCompiler* exprCompiler = compiler.getExpressionCompiler();
        
        // Test number literal
        auto numberExpr = std::make_unique<LiteralExpr>(Value(42.0));
        int reg = exprCompiler->compileExpr(numberExpr.get());
        assert(reg >= 0);
        
        // Test string literal
        auto stringExpr = std::make_unique<LiteralExpr>(Value("hello"));
        reg = exprCompiler->compileExpr(stringExpr.get());
        assert(reg >= 0);
        
        // Test boolean literal
        auto boolExpr = std::make_unique<LiteralExpr>(Value(true));
        reg = exprCompiler->compileExpr(boolExpr.get());
        assert(reg >= 0);
        
        // Test nil literal
        auto nilExpr = std::make_unique<LiteralExpr>(Value());
        reg = exprCompiler->compileExpr(nilExpr.get());
        assert(reg >= 0);
        
        std::cout << "Literal compilation tests passed." << std::endl;
    }
    
    void ExpressionCompilerTest::testBinaryExpressionCompilation() {
        std::cout << "Testing binary expression compilation..." << std::endl;
        
        Compiler compiler;
        ExpressionCompiler* exprCompiler = compiler.getExpressionCompiler();
        
        // Test arithmetic: 2 + 3
        auto left = std::make_unique<LiteralExpr>(Value(2.0));
        auto right = std::make_unique<LiteralExpr>(Value(3.0));
        auto addExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::Plus, std::move(right));
        
        int reg = exprCompiler->compileExpr(addExpr.get());
        assert(reg >= 0);
        
        // Test comparison: 5 < 10
        auto left2 = std::make_unique<LiteralExpr>(Value(5.0));
        auto right2 = std::make_unique<LiteralExpr>(Value(10.0));
        auto cmpExpr = std::make_unique<BinaryExpr>(std::move(left2), TokenType::Less, std::move(right2));
        
        reg = exprCompiler->compileExpr(cmpExpr.get());
        assert(reg >= 0);
        
        std::cout << "Binary expression compilation tests passed." << std::endl;
    }
    
    void ExpressionCompilerTest::testTableConstructorCompilation() {
        std::cout << "Testing table constructor compilation..." << std::endl;
        
        Compiler compiler;
        ExpressionCompiler* exprCompiler = compiler.getExpressionCompiler();
        
        // Test empty table: {}
        Vec<TableField> emptyFields;
        auto emptyTable = std::make_unique<TableExpr>(std::move(emptyFields));
        
        int reg = exprCompiler->compileExpr(emptyTable.get());
        assert(reg >= 0);
        
        // Test array-style table: {1, 2, 3}
        Vec<TableField> arrayFields;
        arrayFields.emplace_back(nullptr, std::make_unique<LiteralExpr>(Value(1.0)));
        arrayFields.emplace_back(nullptr, std::make_unique<LiteralExpr>(Value(2.0)));
        arrayFields.emplace_back(nullptr, std::make_unique<LiteralExpr>(Value(3.0)));
        
        auto arrayTable = std::make_unique<TableExpr>(std::move(arrayFields));
        reg = exprCompiler->compileExpr(arrayTable.get());
        assert(reg >= 0);
        
        // Test hash-style table: {x = 10, y = 20}
        Vec<TableField> hashFields;
        hashFields.emplace_back(
            std::make_unique<LiteralExpr>(Value("x")),
            std::make_unique<LiteralExpr>(Value(10.0))
        );
        hashFields.emplace_back(
            std::make_unique<LiteralExpr>(Value("y")),
            std::make_unique<LiteralExpr>(Value(20.0))
        );
        
        auto hashTable = std::make_unique<TableExpr>(std::move(hashFields));
        reg = exprCompiler->compileExpr(hashTable.get());
        assert(reg >= 0);
        
        std::cout << "Table constructor compilation tests passed." << std::endl;
    }
    
    void ExpressionCompilerTest::testConstantFolding() {
        std::cout << "Testing constant folding optimization..." << std::endl;
        
        Compiler compiler;
        ExpressionCompiler* exprCompiler = compiler.getExpressionCompiler();
        
        // Test constant arithmetic: 2 + 3 should be folded to 5
        auto left = std::make_unique<LiteralExpr>(Value(2.0));
        auto right = std::make_unique<LiteralExpr>(Value(3.0));
        auto constExpr = std::make_unique<BinaryExpr>(std::move(left), TokenType::Plus, std::move(right));
        
        // Check if expression is recognized as constant
        assert(exprCompiler->isConstantExpression(constExpr.get()));
        
        // Check if constant value is computed correctly
        Value result = exprCompiler->getConstantValue(constExpr.get());
        assert(result.type() == ValueType::Number);
        assert(result.asNumber() == 5.0);
        
        // Test constant comparison: 5 > 3 should be folded to true
        auto left2 = std::make_unique<LiteralExpr>(Value(5.0));
        auto right2 = std::make_unique<LiteralExpr>(Value(3.0));
        auto cmpExpr = std::make_unique<BinaryExpr>(std::move(left2), TokenType::Greater, std::move(right2));
        
        assert(exprCompiler->isConstantExpression(cmpExpr.get()));
        Value cmpResult = exprCompiler->getConstantValue(cmpExpr.get());
        assert(cmpResult.type() == ValueType::Boolean);
        assert(cmpResult.asBoolean() == true);
        
        std::cout << "Constant folding tests passed." << std::endl;
    }
    
    void ExpressionCompilerTest::testUnaryExpressionCompilation() {
        std::cout << "Testing unary expression compilation..." << std::endl;
        
        Compiler compiler;
        ExpressionCompiler* exprCompiler = compiler.getExpressionCompiler();
        
        // Test unary minus: -5
        auto operand = std::make_unique<LiteralExpr>(Value(5.0));
        auto unaryExpr = std::make_unique<UnaryExpr>(TokenType::Minus, std::move(operand));
        
        int reg = exprCompiler->compileExpr(unaryExpr.get());
        assert(reg >= 0);
        
        // Test logical not: not true
        auto boolOperand = std::make_unique<LiteralExpr>(Value(true));
        auto notExpr = std::make_unique<UnaryExpr>(TokenType::Not, std::move(boolOperand));
        
        reg = exprCompiler->compileExpr(notExpr.get());
        assert(reg >= 0);
        
        // Test length operator: #"hello"
        auto strOperand = std::make_unique<LiteralExpr>(Value("hello"));
        auto lenExpr = std::make_unique<UnaryExpr>(TokenType::Hash, std::move(strOperand));
        
        reg = exprCompiler->compileExpr(lenExpr.get());
        assert(reg >= 0);
        
        std::cout << "Unary expression compilation tests passed." << std::endl;
    }
}