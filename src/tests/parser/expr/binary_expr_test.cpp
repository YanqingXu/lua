#include "binary_expr_test.hpp"
#include "../../../parser/parser.hpp"
#include "../../test_utils.hpp"
#include <iostream>
#include <stdexcept>

using namespace Lua;
using namespace Tests;

void BinaryExprTest::runAllTests() {
    // Group tests
    SAFE_RUN_TEST(BinaryExprTest, testArithmeticOperators);
    SAFE_RUN_TEST(BinaryExprTest, testComparisonOperators);
    SAFE_RUN_TEST(BinaryExprTest, testLogicalOperators);
    
    // Individual arithmetic tests
    SAFE_RUN_TEST(BinaryExprTest, testAddition);
    SAFE_RUN_TEST(BinaryExprTest, testSubtraction);
    SAFE_RUN_TEST(BinaryExprTest, testMultiplication);
    SAFE_RUN_TEST(BinaryExprTest, testDivision);
    SAFE_RUN_TEST(BinaryExprTest, testModulo);
    SAFE_RUN_TEST(BinaryExprTest, testExponentiation);
    
    // Individual comparison tests
    SAFE_RUN_TEST(BinaryExprTest, testEquality);
    SAFE_RUN_TEST(BinaryExprTest, testInequality);
    SAFE_RUN_TEST(BinaryExprTest, testLessThan);
    SAFE_RUN_TEST(BinaryExprTest, testLessEqual);
    SAFE_RUN_TEST(BinaryExprTest, testGreaterThan);
    SAFE_RUN_TEST(BinaryExprTest, testGreaterEqual);

    // Individual logical tests
    SAFE_RUN_TEST(BinaryExprTest, testLogicalAnd);
    SAFE_RUN_TEST(BinaryExprTest, testLogicalOr);
    
    // Other tests
    SAFE_RUN_TEST(BinaryExprTest, testStringConcatenation);
    SAFE_RUN_TEST(BinaryExprTest, testOperatorPrecedence);
    SAFE_RUN_TEST(BinaryExprTest, testLeftAssociativity);
    SAFE_RUN_TEST(BinaryExprTest, testRightAssociativity);
    SAFE_RUN_TEST(BinaryExprTest, testMixedPrecedence);
    SAFE_RUN_TEST(BinaryExprTest, testNestedExpressions);
    
    // Complex expression tests
    SAFE_RUN_TEST(BinaryExprTest, testParenthesizedExpressions);
    SAFE_RUN_TEST(BinaryExprTest, testComplexArithmetic);
    SAFE_RUN_TEST(BinaryExprTest, testComplexLogical);
    SAFE_RUN_TEST(BinaryExprTest, testMixedOperatorTypes);
    
    // Edge case tests
    SAFE_RUN_TEST(BinaryExprTest, testWithLiterals);
    SAFE_RUN_TEST(BinaryExprTest, testWithVariables);
    SAFE_RUN_TEST(BinaryExprTest, testWithUnaryExpressions);
    SAFE_RUN_TEST(BinaryExprTest, testChainedComparisons);
    
    // Error handling tests
    SAFE_RUN_TEST(BinaryExprTest, testInvalidOperators);
    SAFE_RUN_TEST(BinaryExprTest, testMissingOperands);
    SAFE_RUN_TEST(BinaryExprTest, testInvalidSyntax);
}

void BinaryExprTest::testAddition() {
    std::cout << "Testing addition..." << std::endl;
    
    // Test simple addition
    std::string input1 = "3 + 5";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testSubtraction() {
    std::cout << "Testing subtraction..." << std::endl;
    
    // Test simple subtraction
    std::string input1 = "10 - 3";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testMultiplication() {
    std::cout << "Testing multiplication..." << std::endl;
    
    // Test simple multiplication
    std::string input1 = "4 * 6";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testDivision() {
    std::cout << "Testing division..." << std::endl;
    
    // Test simple division
    std::string input1 = "15 / 3";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testModulo() {
    std::cout << "Testing modulo..." << std::endl;
    
    // Test simple modulo
    std::string input1 = "10 % 3";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testExponentiation() {
    std::cout << "Testing exponentiation..." << std::endl;
    
    // Test simple exponentiation
    std::string input1 = "2 ^ 3";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testEquality() {
    std::cout << "Testing equality..." << std::endl;
    
    // Test simple equality
    std::string input1 = "5 == 5";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testInequality() {
    std::cout << "Testing inequality..." << std::endl;
    
    // Test simple inequality
    std::string input1 = "5 ~= 3";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testLessThan() {
    std::cout << "Testing less than..." << std::endl;
    
    // Test simple less than
    std::string input1 = "3 < 5";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testLessEqual() {
    std::cout << "Testing less than or equal..." << std::endl;
    
    // Test simple less than or equal
    std::string input1 = "3 <= 5";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testGreaterThan() {
    std::cout << "Testing greater than..." << std::endl;
    
    // Test simple greater than
    std::string input1 = "5 > 3";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testGreaterEqual() {
    std::cout << "Testing greater than or equal..." << std::endl;
    
    // Test simple greater than or equal
    std::string input1 = "5 >= 3";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testLogicalAnd() {
    std::cout << "Testing logical and..." << std::endl;
    
    // Test simple logical and
    std::string input1 = "true and false";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testLogicalOr() {
    std::cout << "Testing logical or..." << std::endl;
    
    // Test simple logical or
    std::string input1 = "true or false";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testStringConcatenation() {
    std::cout << "Testing string concatenation..." << std::endl;
    
    // Test simple string concatenation
    std::string input1 = "\"hello\" .. \"world\"";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testOperatorPrecedence() {
    std::cout << "Testing operator precedence..." << std::endl;
    
    // Test multiplication before addition
    std::string input1 = "2 + 3 * 4";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testLeftAssociativity() {
    std::cout << "Testing left associativity..." << std::endl;
    
    // Test left associativity for addition
    std::string input1 = "1 + 2 + 3";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testRightAssociativity() {
    std::cout << "Testing right associativity..." << std::endl;
    
    // Test right associativity for exponentiation
    std::string input1 = "2 ^ 3 ^ 2";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testMixedPrecedence() {
    std::cout << "Testing mixed precedence..." << std::endl;
    
    // Test complex expression with mixed operators
    std::string input1 = "a + b * c - d / e";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testNestedExpressions() {
    std::cout << "Testing nested expressions..." << std::endl;
    
    // Test parenthesized expressions
    std::string input1 = "(a + b) * (c - d)";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testChainedComparisons() {
    std::cout << "Testing chained comparisons..." << std::endl;
    
    // Test chained less than
    std::string input1 = "a < b < c";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testInvalidOperators() {
    std::cout << "Testing invalid operators..." << std::endl;
    
    // Test invalid bitwise and operator
    std::string input1 = "a & b";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testMissingOperands() {
    std::cout << "Testing missing operands..." << std::endl;
    
    // Test missing left operand
    std::string input1 = "+ 5";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testInvalidSyntax() {
    std::cout << "Testing invalid syntax..." << std::endl;
    
    // Test double operators
    std::string input1 = "a + + b";
    try {
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testBinaryParsing(const std::string& input, TokenType expectedOp, const std::string& testName) {
    try {
        Parser parser(input);
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

void BinaryExprTest::testBinaryParsingError(const std::string& input, const std::string& testName) {
    try {
        Parser parser(input);
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

bool BinaryExprTest::verifyBinaryExpression(const Expr* expr, TokenType expectedOp) {
    if (!expr) {
        return false;
    }
    
    const BinaryExpr* binaryExpr = dynamic_cast<const BinaryExpr*>(expr);
    if (!binaryExpr) {
        return false;
    }
    
    return binaryExpr->getOperator() == expectedOp;
}

void BinaryExprTest::printBinaryExpressionInfo(const BinaryExpr* binaryExpr) {
    if (!binaryExpr) {
        return;
    }
    
    std::string opStr = tokenTypeToString(binaryExpr->getOperator());
    
    TestUtils::printInfo("  Operator: " + opStr);
    TestUtils::printInfo("  Has left operand: " + std::string(binaryExpr->getLeft() ? "yes" : "no"));
    TestUtils::printInfo("  Has right operand: " + std::string(binaryExpr->getRight() ? "yes" : "no"));
}

std::string BinaryExprTest::tokenTypeToString(TokenType type) {
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
void BinaryExprTest::testArithmeticOperators() {
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

void BinaryExprTest::testComparisonOperators() {
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

void BinaryExprTest::testLogicalOperators() {
    std::cout << "Testing logical operators group..." << std::endl;
    
    // Test all logical operators
    testBinaryParsing("true and false", TokenType::And, "Logical AND operator");
    testBinaryParsing("true or false", TokenType::Or, "Logical OR operator");
    testBinaryParsing("a and b", TokenType::And, "Logical AND with variables");
    testBinaryParsing("x or y", TokenType::Or, "Logical OR with variables");
    
    std::cout << "Logical operators group test completed." << std::endl;
}

// Complex expression test implementations
void BinaryExprTest::testParenthesizedExpressions() {
    std::cout << "Testing parenthesized expressions..." << std::endl;
    
    // Test expressions with parentheses
    try {
        std::string input1 = "(3 + 5) * 2";
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testComplexArithmetic() {
    std::cout << "Testing complex arithmetic expressions..." << std::endl;
    
    // Test complex arithmetic combinations
    try {
        std::string input1 = "a + b * c - d / e";
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testComplexLogical() {
    std::cout << "Testing complex logical expressions..." << std::endl;
    
    // Test complex logical combinations
    try {
        std::string input1 = "a and b or c";
        Parser parser1(input1);
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
        Parser parser2(input2);
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

void BinaryExprTest::testMixedOperatorTypes() {
    std::cout << "Testing mixed operator types..." << std::endl;
    
    // Test mixing different operator types
    try {
        std::string input1 = "a + b > c";
        Parser parser1(input1);
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
        Parser parser2(input2);
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
void BinaryExprTest::testWithLiterals() {
    std::cout << "Testing binary expressions with literals..." << std::endl;
    
    // Test with different literal types
    try {
        std::string input1 = "42 + 3.14";
        Parser parser1(input1);
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
        Parser parser2(input2);
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
        Parser parser3(input3);
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

void BinaryExprTest::testWithVariables() {
    std::cout << "Testing binary expressions with variables..." << std::endl;
    
    // Test with different variable combinations
    try {
        std::string input1 = "x + y";
        Parser parser1(input1);
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
        Parser parser2(input2);
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
        Parser parser3(input3);
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

void BinaryExprTest::testWithUnaryExpressions() {
    std::cout << "Testing binary expressions with unary expressions..." << std::endl;
    
    // Test with unary expressions as operands
    try {
        std::string input1 = "-a + b";
        Parser parser1(input1);
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
        Parser parser2(input2);
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
        Parser parser3(input3);
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