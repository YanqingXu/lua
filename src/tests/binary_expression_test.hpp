#pragma once

#include "../common/opcodes.hpp"
#include "../parser/ast/expressions.hpp"
#include "../lexer/lexer.hpp"

namespace Lua {
namespace Tests {

class BinaryExpressionTest {
public:
    static void runAllTests();
    
private:
    static void testArithmeticOperations();
    static void testArithmeticOp(TokenType op, OpCode expectedOpCode);
    static void testComparisonOperations();
    static void testComparisonOp(TokenType op, OpCode expectedOpCode);
    static void testLogicalOperations();
    static void testStringConcatenation();
    static void testOperatorPrecedence();
    static void testNestedExpressions();
    static void testErrorHandling();
};

} // namespace Tests
} // namespace Lua