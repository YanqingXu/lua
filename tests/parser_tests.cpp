#include <gtest/gtest.h>
#include "parser/parser.hpp"
#include "lexer/lexer.hpp"

using namespace Lua;

class ParserTest : public ::testing::Test {
protected:
    // 辅助函数：解析Lua代码字符串
    std::unique_ptr<AstNode> parse(const std::string& source) {
        Lexer lexer(source);
        Parser parser(&lexer);
        return parser.parse();
    }
};

// 测试变量声明
TEST_F(ParserTest, VariableDeclaration) {
    auto ast = parse("local x = 10");
    ASSERT_NE(nullptr, ast);
    
    // 验证AST结构，根据你的AST设计进行适当调整
    auto block = dynamic_cast<BlockNode*>(ast.get());
    ASSERT_NE(nullptr, block);
    ASSERT_EQ(1, block->statements.size());
    
    auto localDecl = dynamic_cast<LocalDeclNode*>(block->statements[0].get());
    ASSERT_NE(nullptr, localDecl);
    ASSERT_EQ(1, localDecl->names.size());
    ASSERT_EQ("x", localDecl->names[0]);
    ASSERT_EQ(1, localDecl->values.size());
    
    auto numLiteral = dynamic_cast<NumberLiteralNode*>(localDecl->values[0].get());
    ASSERT_NE(nullptr, numLiteral);
    ASSERT_DOUBLE_EQ(10.0, numLiteral->value);
}

// 测试表达式
TEST_F(ParserTest, Expressions) {
    auto ast = parse("local result = 10 + 20 * 2");
    ASSERT_NE(nullptr, ast);
    
    auto block = dynamic_cast<BlockNode*>(ast.get());
    ASSERT_NE(nullptr, block);
    
    auto localDecl = dynamic_cast<LocalDeclNode*>(block->statements[0].get());
    ASSERT_NE(nullptr, localDecl);
    
    auto binaryExpr = dynamic_cast<BinaryOpNode*>(localDecl->values[0].get());
    ASSERT_NE(nullptr, binaryExpr);
    ASSERT_EQ(BinaryOpType::Add, binaryExpr->op);
    
    auto leftOperand = dynamic_cast<NumberLiteralNode*>(binaryExpr->left.get());
    ASSERT_NE(nullptr, leftOperand);
    ASSERT_DOUBLE_EQ(10.0, leftOperand->value);
    
    auto rightOperand = dynamic_cast<BinaryOpNode*>(binaryExpr->right.get());
    ASSERT_NE(nullptr, rightOperand);
    ASSERT_EQ(BinaryOpType::Mul, rightOperand->op);
    
    auto rightLeft = dynamic_cast<NumberLiteralNode*>(rightOperand->left.get());
    ASSERT_NE(nullptr, rightLeft);
    ASSERT_DOUBLE_EQ(20.0, rightLeft->value);
    
    auto rightRight = dynamic_cast<NumberLiteralNode*>(rightOperand->right.get());
    ASSERT_NE(nullptr, rightRight);
    ASSERT_DOUBLE_EQ(2.0, rightRight->value);
}

// 测试函数定义
TEST_F(ParserTest, FunctionDefinition) {
    auto ast = parse("function add(a, b)\n  return a + b\nend");
    ASSERT_NE(nullptr, ast);
    
    auto block = dynamic_cast<BlockNode*>(ast.get());
    ASSERT_NE(nullptr, block);
    
    auto funcDef = dynamic_cast<FunctionDefNode*>(block->statements[0].get());
    ASSERT_NE(nullptr, funcDef);
    ASSERT_EQ("add", funcDef->name->fullName());
    ASSERT_EQ(2, funcDef->params.size());
    ASSERT_EQ("a", funcDef->params[0]);
    ASSERT_EQ("b", funcDef->params[1]);
    
    auto funcBody = funcDef->body.get();
    ASSERT_NE(nullptr, funcBody);
    ASSERT_EQ(1, funcBody->statements.size());
    
    auto returnStmt = dynamic_cast<ReturnStatNode*>(funcBody->statements[0].get());
    ASSERT_NE(nullptr, returnStmt);
}

// 测试if语句
TEST_F(ParserTest, IfStatement) {
    auto ast = parse("if x > 0 then\n  print('positive')\nelseif x < 0 then\n  print('negative')\nelse\n  print('zero')\nend");
    ASSERT_NE(nullptr, ast);
    
    auto block = dynamic_cast<BlockNode*>(ast.get());
    ASSERT_NE(nullptr, block);
    
    auto ifStmt = dynamic_cast<IfStatNode*>(block->statements[0].get());
    ASSERT_NE(nullptr, ifStmt);
    ASSERT_EQ(2, ifStmt->conditions.size()); // 主条件 + 1个elseif
    ASSERT_EQ(3, ifStmt->blocks.size());     // 主块 + elseif块 + else块
    
    // 验证主条件 (x > 0)
    auto mainCond = dynamic_cast<BinaryOpNode*>(ifStmt->conditions[0].get());
    ASSERT_NE(nullptr, mainCond);
    ASSERT_EQ(BinaryOpType::Gt, mainCond->op);
    
    // 验证elseif条件 (x < 0)
    auto elseifCond = dynamic_cast<BinaryOpNode*>(ifStmt->conditions[1].get());
    ASSERT_NE(nullptr, elseifCond);
    ASSERT_EQ(BinaryOpType::Lt, elseifCond->op);
}

// 测试循环
TEST_F(ParserTest, Loops) {
    // while循环
    auto whileAst = parse("while i <= 10 do\n  print(i)\n  i = i + 1\nend");
    ASSERT_NE(nullptr, whileAst);
    
    auto whileBlock = dynamic_cast<BlockNode*>(whileAst.get());
    ASSERT_NE(nullptr, whileBlock);
    
    auto whileStmt = dynamic_cast<WhileStatNode*>(whileBlock->statements[0].get());
    ASSERT_NE(nullptr, whileStmt);
    
    // for循环
    auto forAst = parse("for i=1,10 do\n  print(i)\nend");
    ASSERT_NE(nullptr, forAst);
    
    auto forBlock = dynamic_cast<BlockNode*>(forAst.get());
    ASSERT_NE(nullptr, forBlock);
    
    auto forStmt = dynamic_cast<ForNumStatNode*>(forBlock->statements[0].get());
    ASSERT_NE(nullptr, forStmt);
    ASSERT_EQ("i", forStmt->varName);
}

// 测试表达式语句
TEST_F(ParserTest, ExpressionStatement) {
    auto ast = parse("print('hello')");
    ASSERT_NE(nullptr, ast);
    
    auto block = dynamic_cast<BlockNode*>(ast.get());
    ASSERT_NE(nullptr, block);
    
    auto exprStmt = dynamic_cast<ExprStatNode*>(block->statements[0].get());
    ASSERT_NE(nullptr, exprStmt);
    
    auto callExpr = dynamic_cast<CallExprNode*>(exprStmt->expr.get());
    ASSERT_NE(nullptr, callExpr);
}

// 测试表定义
TEST_F(ParserTest, TableConstructor) {
    auto ast = parse("local t = { name = 'lua', version = 5.1, [1] = 'first', 'second' }");
    ASSERT_NE(nullptr, ast);
    
    auto block = dynamic_cast<BlockNode*>(ast.get());
    ASSERT_NE(nullptr, block);
    
    auto localDecl = dynamic_cast<LocalDeclNode*>(block->statements[0].get());
    ASSERT_NE(nullptr, localDecl);
    
    auto tableConstr = dynamic_cast<TableConstructorNode*>(localDecl->values[0].get());
    ASSERT_NE(nullptr, tableConstr);
    ASSERT_EQ(4, tableConstr->fields.size());
}

// 测试错误恢复和报告
TEST_F(ParserTest, ErrorRecovery) {
    // 缺少end关键字
    Parser parser(new Lexer("if x > 0 then print('positive')"));
    EXPECT_THROW(parser.parse(), SyntaxError);
    
    // 不匹配的括号
    Parser parser2(new Lexer("local x = (10 + 20"));
    EXPECT_THROW(parser2.parse(), SyntaxError);
}
