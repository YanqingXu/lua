#include "parser_binary_expr_test.hpp"
#include "../../../parser/parser.hpp"
#include "../../../test_framework/core/test_macros.hpp"
#include "../../../test_framework/core/test_utils.hpp"

using namespace Lua;
using namespace Tests;
using namespace TestFramework;

void ParserBinaryExprTest::runAllTests() {
    // Group tests
    RUN_TEST(ParserBinaryExprTest, testArithmeticOperators);
    RUN_TEST(ParserBinaryExprTest, testComparisonOperators);
    RUN_TEST(ParserBinaryExprTest, testLogicalOperators);
    
    // Individual arithmetic tests
    RUN_TEST(ParserBinaryExprTest, testAddition);
    RUN_TEST(ParserBinaryExprTest, testSubtraction);
    RUN_TEST(ParserBinaryExprTest, testMultiplication);
    RUN_TEST(ParserBinaryExprTest, testDivision);
    RUN_TEST(ParserBinaryExprTest, testModulo);
    RUN_TEST(ParserBinaryExprTest, testExponentiation);
    
    // Individual comparison tests
    RUN_TEST(ParserBinaryExprTest, testEquality);
    RUN_TEST(ParserBinaryExprTest, testInequality);
    RUN_TEST(ParserBinaryExprTest, testLessThan);
    RUN_TEST(ParserBinaryExprTest, testLessEqual);
    RUN_TEST(ParserBinaryExprTest, testGreaterThan);
    RUN_TEST(ParserBinaryExprTest, testGreaterEqual);

    // Individual logical tests
    RUN_TEST(ParserBinaryExprTest, testLogicalAnd);
    RUN_TEST(ParserBinaryExprTest, testLogicalOr);
    
    // Other tests
    RUN_TEST(ParserBinaryExprTest, testStringConcatenation);
    RUN_TEST(ParserBinaryExprTest, testOperatorPrecedence);
    RUN_TEST(ParserBinaryExprTest, testLeftAssociativity);
    RUN_TEST(ParserBinaryExprTest, testRightAssociativity);
    RUN_TEST(ParserBinaryExprTest, testMixedPrecedence);
    RUN_TEST(ParserBinaryExprTest, testNestedExpressions);
    
    // Complex expression tests
    RUN_TEST(ParserBinaryExprTest, testParenthesizedExpressions);
    RUN_TEST(ParserBinaryExprTest, testComplexArithmetic);
    RUN_TEST(ParserBinaryExprTest, testComplexLogical);
    RUN_TEST(ParserBinaryExprTest, testMixedOperatorTypes);
    
    // Edge case tests
    RUN_TEST(ParserBinaryExprTest, testWithLiterals);
    RUN_TEST(ParserBinaryExprTest, testWithVariables);
    RUN_TEST(ParserBinaryExprTest, testWithUnaryExpressions);
    RUN_TEST(ParserBinaryExprTest, testChainedComparisons);
    
    // Error handling tests
    RUN_TEST(ParserBinaryExprTest, testInvalidOperators);
    RUN_TEST(ParserBinaryExprTest, testMissingOperands);
    RUN_TEST(ParserBinaryExprTest, testInvalidSyntax);
}

void ParserBinaryExprTest::testAddition() {
    std::cout << "Testing addition..." << std::endl;
    
    // Test simple addition
    std::string input1 = "3 + 5";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test addition with variables
    std::string input2 = "a + b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testSubtraction() {
    std::cout << "Testing subtraction..." << std::endl;
    
    // Test simple subtraction
    std::string input1 = "10 - 3";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test subtraction with variables
    std::string input2 = "x - y";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testMultiplication() {
    std::cout << "Testing multiplication..." << std::endl;
    
    // Test simple multiplication
    std::string input1 = "4 * 6";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test multiplication with variables
    std::string input2 = "a * b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testDivision() {
    std::cout << "Testing division..." << std::endl;
    
    // Test simple division
    std::string input1 = "15 / 3";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test division with variables
    std::string input2 = "x / y";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testModulo() {
    std::cout << "Testing modulo..." << std::endl;
    
    // Test simple modulo
    std::string input1 = "10 % 3";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test modulo with variables
    std::string input2 = "a % b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testExponentiation() {
    std::cout << "Testing exponentiation..." << std::endl;
    
    // Test simple exponentiation
    std::string input1 = "2 ^ 3";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test exponentiation with variables
    std::string input2 = "x ^ y";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testEquality() {
    std::cout << "Testing equality..." << std::endl;
    
    // Test simple equality
    std::string input1 = "5 == 5";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test equality with variables
    std::string input2 = "a == b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testInequality() {
    std::cout << "Testing inequality..." << std::endl;
    
    // Test simple inequality
    std::string input1 = "5 ~= 3";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test inequality with variables
    std::string input2 = "x ~= y";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testLessThan() {
    std::cout << "Testing less than..." << std::endl;
    
    // Test simple less than
    std::string input1 = "3 < 5";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test less than with variables
    std::string input2 = "a < b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testLessEqual() {
    std::cout << "Testing less than or equal..." << std::endl;
    
    // Test simple less than or equal
    std::string input1 = "3 <= 5";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test less than or equal with variables
    std::string input2 = "a <= b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testGreaterThan() {
    std::cout << "Testing greater than..." << std::endl;
    
    // Test simple greater than
    std::string input1 = "5 > 3";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test greater than with variables
    std::string input2 = "a > b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testGreaterEqual() {
    std::cout << "Testing greater than or equal..." << std::endl;
    
    // Test simple greater than or equal
    std::string input1 = "5 >= 3";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test greater than or equal with variables
    std::string input2 = "a >= b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testLogicalAnd() {
    std::cout << "Testing logical and..." << std::endl;
    
    // Test simple logical and
    std::string input1 = "true and false";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test logical and with variables
    std::string input2 = "a and b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testLogicalOr() {
    std::cout << "Testing logical or..." << std::endl;
    
    // Test simple logical or
    std::string input1 = "true or false";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test logical or with variables
    std::string input2 = "a or b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testStringConcatenation() {
    std::cout << "Testing string concatenation..." << std::endl;
    
    // Test simple string concatenation
    std::string input1 = "\"hello\" .. \"world\"";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test string concatenation with variables
    std::string input2 = "a .. b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testOperatorPrecedence() {
    std::cout << "Testing operator precedence..." << std::endl;
    
    // Test multiplication before addition
    std::string input1 = "2 + 3 * 4";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test exponentiation before multiplication
    std::string input2 = "2 * 3 ^ 2";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testLeftAssociativity() {
    std::cout << "Testing left associativity..." << std::endl;
    
    // Test left associativity for addition
    std::string input1 = "1 + 2 + 3";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test left associativity for subtraction
    std::string input2 = "10 - 3 - 2";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testRightAssociativity() {
    std::cout << "Testing right associativity..." << std::endl;
    
    // Test right associativity for exponentiation
    std::string input1 = "2 ^ 3 ^ 2";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test right associativity for string concatenation
    std::string input2 = "a .. b .. c";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testMixedPrecedence() {
    std::cout << "Testing mixed precedence..." << std::endl;
    
    // Test complex expression with mixed operators
    std::string input1 = "a + b * c - d / e";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test expression with comparison and arithmetic
    std::string input2 = "a + b < c * d";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testNestedExpressions() {
    std::cout << "Testing nested expressions..." << std::endl;
    
    // Test parenthesized expressions
    std::string input1 = "(a + b) * (c - d)";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test deeply nested expressions
    std::string input2 = "((a + b) * c) / (d - e)";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testChainedComparisons() {
    std::cout << "Testing chained comparisons..." << std::endl;
    
    // Test chained less than
    std::string input1 = "a < b < c";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test chained equality
    std::string input2 = "x == y == z";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Failed to parse '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testInvalidOperators() {
    std::cout << "Testing invalid operators..." << std::endl;
    
    // Test invalid bitwise and operator
    std::string input1 = "a & b";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (!expr1) {
            std::cout << "[OK] Correctly rejected '" << input1 << "'" << std::endl;
        } else {
            std::cout << "[Failed] Incorrectly accepted '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[OK] Correctly rejected '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test invalid bitwise or operator
    std::string input2 = "x | y";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (!expr2) {
            std::cout << "[OK] Correctly rejected '" << input2 << "'" << std::endl;
        } else {
            std::cout << "[Failed] Incorrectly accepted '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[OK] Correctly rejected '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testMissingOperands() {
    std::cout << "Testing missing operands..." << std::endl;
    
    // Test missing left operand
    std::string input1 = "+ 5";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (!expr1) {
            std::cout << "[OK] Correctly rejected '" << input1 << "'" << std::endl;
        } else {
            std::cout << "[Failed] Incorrectly accepted '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[OK] Correctly rejected '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test missing right operand
    std::string input2 = "5 +";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (!expr2) {
            std::cout << "[OK] Correctly rejected '" << input2 << "'" << std::endl;
        } else {
            std::cout << "[Failed] Incorrectly accepted '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[OK] Correctly rejected '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testInvalidSyntax() {
    std::cout << "Testing invalid syntax..." << std::endl;
    
    // Test double operators
    std::string input1 = "a + + b";
    try {
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (!expr1) {
            std::cout << "[OK] Correctly rejected '" << input1 << "'" << std::endl;
        } else {
            std::cout << "[Failed] Incorrectly accepted '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[OK] Correctly rejected '" << input1 << "': " << e.what() << std::endl;
    }
    
    // Test consecutive operators
    std::string input2 = "a * / b";
    try {
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (!expr2) {
            std::cout << "[OK] Correctly rejected '" << input2 << "'" << std::endl;
        } else {
            std::cout << "[Failed] Incorrectly accepted '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[OK] Correctly rejected '" << input2 << "': " << e.what() << std::endl;
    }
}

void ParserBinaryExprTest::testBinaryParsing(const std::string& input, TokenType expectedOp, const std::string& testName) {
    try {
        Lua::Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printInfo("Failed to parse expression");
            TestUtils::printTestResult(testName, false);
            return;
        }
        
        if (!verifyBinaryExpression(expr.get(), expectedOp)) {
            TestUtils::printInfo("Expression is not a binary expression or operator mismatch");
            TestUtils::printTestResult(testName, false);
            return;
        }
        
        TestUtils::printInfo("Successfully parsed binary expression");
        TestUtils::printTestResult(testName, true);
        
        // Print additional info for debugging
        if (auto binaryExpr = dynamic_cast<const BinaryExpr*>(expr.get())) {
            printBinaryExpressionInfo(binaryExpr);
        }
        
    } catch (const std::exception& e) {
        TestUtils::printInfo(std::string("Exception: ") + e.what());
        TestUtils::printTestResult(testName, false);
    }
}

void ParserBinaryExprTest::testBinaryParsingError(const std::string& input, const std::string& testName) {
    try {
        Lua::Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printInfo("Correctly failed to parse invalid binary expression");
            TestUtils::printTestResult(testName, true);
            return;
        }
        
        TestUtils::printInfo("Should have failed to parse invalid binary expression");
        TestUtils::printTestResult(testName, false);
        
    } catch (const std::exception& e) {
        TestUtils::printInfo(std::string("Correctly threw exception: ") + e.what());
        TestUtils::printTestResult(testName, true);
    }
}

bool ParserBinaryExprTest::verifyBinaryExpression(const Expr* expr, TokenType expectedOp) {
    if (!expr) {
        return false;
    }
    
    const BinaryExpr* binaryExpr = dynamic_cast<const BinaryExpr*>(expr);
    if (!binaryExpr) {
        return false;
    }
    
    return binaryExpr->getOperator() == expectedOp;
}

void ParserBinaryExprTest::printBinaryExpressionInfo(const BinaryExpr* binaryExpr) {
    if (!binaryExpr) {
        return;
    }
    
    std::string opStr = tokenTypeToString(binaryExpr->getOperator());
    
    TestUtils::printInfo("  Operator: " + opStr);
    TestUtils::printInfo("  Has left operand: " + std::string(binaryExpr->getLeft() ? "yes" : "no"));
    TestUtils::printInfo("  Has right operand: " + std::string(binaryExpr->getRight() ? "yes" : "no"));
}

std::string ParserBinaryExprTest::tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::Plus: return "+";
        case TokenType::Minus: return "-";
        case TokenType::Star: return "*";
        case TokenType::Slash: return "/";
        case TokenType::Percent: return "%";
        case TokenType::Caret: return "^";
        case TokenType::Equal: return "==";
        case TokenType::NotEqual: return "~=";
        case TokenType::Less: return "<";
        case TokenType::LessEqual: return "<=";
        case TokenType::Greater: return ">";
        case TokenType::GreaterEqual: return ">=";
        case TokenType::And: return "and";
        case TokenType::Or: return "or";
        case TokenType::DotDot: return "..";
        default: return "unknown";
    }
}

// Group test implementations
void ParserBinaryExprTest::testArithmeticOperators() {
    std::cout << "Testing arithmetic operators group..." << std::endl;
    
    // Test all arithmetic operators
    testBinaryParsing("3 + 5", TokenType::Plus, "Addition operator");
    testBinaryParsing("10 - 3", TokenType::Minus, "Subtraction operator");
    testBinaryParsing("4 * 6", TokenType::Star, "Multiplication operator");
    testBinaryParsing("15 / 3", TokenType::Slash, "Division operator");
    testBinaryParsing("10 % 3", TokenType::Percent, "Modulo operator");
    testBinaryParsing("2 ^ 3", TokenType::Caret, "Exponentiation operator");
    
    std::cout << "Arithmetic operators group test completed." << std::endl;
}

void ParserBinaryExprTest::testComparisonOperators() {
    std::cout << "Testing comparison operators group..." << std::endl;
    
    // Test all comparison operators
    testBinaryParsing("5 == 5", TokenType::Equal, "Equality operator");
    testBinaryParsing("5 ~= 3", TokenType::NotEqual, "Inequality operator");
    testBinaryParsing("3 < 5", TokenType::Less, "Less than operator");
    testBinaryParsing("3 <= 5", TokenType::LessEqual, "Less equal operator");
    testBinaryParsing("5 > 3", TokenType::Greater, "Greater than operator");
    testBinaryParsing("5 >= 3", TokenType::GreaterEqual, "Greater equal operator");
    
    std::cout << "Comparison operators group test completed." << std::endl;
}

void ParserBinaryExprTest::testLogicalOperators() {
    std::cout << "Testing logical operators group..." << std::endl;
    
    // Test all logical operators
    testBinaryParsing("true and false", TokenType::And, "Logical AND operator");
    testBinaryParsing("true or false", TokenType::Or, "Logical OR operator");
    testBinaryParsing("a and b", TokenType::And, "Logical AND with variables");
    testBinaryParsing("x or y", TokenType::Or, "Logical OR with variables");
    
    std::cout << "Logical operators group test completed." << std::endl;
}

// Complex expression test implementations
void ParserBinaryExprTest::testParenthesizedExpressions() {
    std::cout << "Testing parenthesized expressions..." << std::endl;
    
    // Test expressions with parentheses
    try {
        std::string input1 = "(3 + 5) * 2";
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing parenthesized expression: " << e.what() << std::endl;
    }
    
    try {
        std::string input2 = "2 * (a + b)";
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing parenthesized expression: " << e.what() << std::endl;
    }
    
    std::cout << "Parenthesized expressions test completed." << std::endl;
}

void ParserBinaryExprTest::testComplexArithmetic() {
    std::cout << "Testing complex arithmetic expressions..." << std::endl;
    
    // Test complex arithmetic combinations
    try {
        std::string input1 = "a + b * c - d / e";
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed complex arithmetic '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse complex arithmetic '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing complex arithmetic: " << e.what() << std::endl;
    }
    
    try {
        std::string input2 = "2 ^ 3 + 4 * 5 - 6 / 2";
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed complex arithmetic '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse complex arithmetic '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing complex arithmetic: " << e.what() << std::endl;
    }
    
    std::cout << "Complex arithmetic expressions test completed." << std::endl;
}

void ParserBinaryExprTest::testComplexLogical() {
    std::cout << "Testing complex logical expressions..." << std::endl;
    
    // Test complex logical combinations
    try {
        std::string input1 = "a and b or c";
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed complex logical '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse complex logical '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing complex logical: " << e.what() << std::endl;
    }
    
    try {
        std::string input2 = "(a > b) and (c < d) or (e == f)";
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed complex logical '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse complex logical '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing complex logical: " << e.what() << std::endl;
    }
    
    std::cout << "Complex logical expressions test completed." << std::endl;
}

void ParserBinaryExprTest::testMixedOperatorTypes() {
    std::cout << "Testing mixed operator types..." << std::endl;
    
    // Test mixing different operator types
    try {
        std::string input1 = "a + b > c";
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed mixed operators '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse mixed operators '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing mixed operators: " << e.what() << std::endl;
    }
    
    try {
        std::string input2 = "x * y == z and w";
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed mixed operators '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse mixed operators '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing mixed operators: " << e.what() << std::endl;
    }
    
    std::cout << "Mixed operator types test completed." << std::endl;
}

// Edge case test implementations
void ParserBinaryExprTest::testWithLiterals() {
    std::cout << "Testing binary expressions with literals..." << std::endl;
    
    // Test with different literal types
    try {
        std::string input1 = "42 + 3.14";
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed literals '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse literals '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing literals: " << e.what() << std::endl;
    }
    
    try {
        std::string input2 = "\"hello\" .. \"world\"";
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed string literals '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse string literals '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing string literals: " << e.what() << std::endl;
    }
    
    try {
        std::string input3 = "true and false";
        Lua::Parser parser3(input3);
        auto expr3 = parser3.parseExpression();
        if (expr3) {
            std::cout << "[OK] Parsed boolean literals '" << input3 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse boolean literals '" << input3 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing boolean literals: " << e.what() << std::endl;
    }
    
    std::cout << "Binary expressions with literals test completed." << std::endl;
}

void ParserBinaryExprTest::testWithVariables() {
    std::cout << "Testing binary expressions with variables..." << std::endl;
    
    // Test with different variable combinations
    try {
        std::string input1 = "x + y";
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed variables '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse variables '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing variables: " << e.what() << std::endl;
    }
    
    try {
        std::string input2 = "variable1 * variable2";
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed long variables '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse long variables '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing long variables: " << e.what() << std::endl;
    }
    
    try {
        std::string input3 = "a == b and c ~= d";
        Lua::Parser parser3(input3);
        auto expr3 = parser3.parseExpression();
        if (expr3) {
            std::cout << "[OK] Parsed variable comparison '" << input3 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse variable comparison '" << input3 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing variable comparison: " << e.what() << std::endl;
    }
    
    std::cout << "Binary expressions with variables test completed." << std::endl;
}

void ParserBinaryExprTest::testWithUnaryExpressions() {
    std::cout << "Testing binary expressions with unary expressions..." << std::endl;
    
    // Test with unary expressions as operands
    try {
        std::string input1 = "-a + b";
        Lua::Parser parser1(input1);
        auto expr1 = parser1.parseExpression();
        if (expr1) {
            std::cout << "[OK] Parsed unary operand '" << input1 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse unary operand '" << input1 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing unary operand: " << e.what() << std::endl;
    }
    
    try {
        std::string input2 = "not a and b";
        Lua::Parser parser2(input2);
        auto expr2 = parser2.parseExpression();
        if (expr2) {
            std::cout << "[OK] Parsed logical unary '" << input2 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse logical unary '" << input2 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing logical unary: " << e.what() << std::endl;
    }
    
    try {
        std::string input3 = "a + -b";
        Lua::Parser parser3(input3);
        auto expr3 = parser3.parseExpression();
        if (expr3) {
            std::cout << "[OK] Parsed right unary '" << input3 << "' successfully" << std::endl;
        } else {
            std::cout << "[Failed] Failed to parse right unary '" << input3 << "'" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "[Failed] Exception parsing right unary: " << e.what() << std::endl;
    }
    
    std::cout << "Binary expressions with unary expressions test completed." << std::endl;
}
