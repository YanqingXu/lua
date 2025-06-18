#include "binary_expr_test.hpp"
#include "../../../parser/parser.hpp"
#include "../../test_utils.hpp"
#include <iostream>
#include <stdexcept>

using namespace Lua;
using namespace Tests;

void BinaryExprTest::runAllTests() {
    SAFE_RUN_TEST(BinaryExprTest, testAddition);
    SAFE_RUN_TEST(BinaryExprTest, testSubtraction);
    SAFE_RUN_TEST(BinaryExprTest, testMultiplication);
    SAFE_RUN_TEST(BinaryExprTest, testDivision);

    SAFE_RUN_TEST(BinaryExprTest, testModulo);
    SAFE_RUN_TEST(BinaryExprTest, testExponentiation);
    SAFE_RUN_TEST(BinaryExprTest, testEquality);
    SAFE_RUN_TEST(BinaryExprTest, testInequality);

    SAFE_RUN_TEST(BinaryExprTest, testLessThan);
    SAFE_RUN_TEST(BinaryExprTest, testLessEqual);
    SAFE_RUN_TEST(BinaryExprTest, testGreaterThan);
    SAFE_RUN_TEST(BinaryExprTest, testGreaterEqual);

    SAFE_RUN_TEST(BinaryExprTest, testLogicalAnd);
    SAFE_RUN_TEST(BinaryExprTest, testLogicalOr);
    SAFE_RUN_TEST(BinaryExprTest, testStringConcatenation);
    SAFE_RUN_TEST(BinaryExprTest, testOperatorPrecedence);

    SAFE_RUN_TEST(BinaryExprTest, testLeftAssociativity);
    SAFE_RUN_TEST(BinaryExprTest, testRightAssociativity);
    SAFE_RUN_TEST(BinaryExprTest, testMixedPrecedence);
    SAFE_RUN_TEST(BinaryExprTest, testNestedExpressions);

    SAFE_RUN_TEST(BinaryExprTest, testChainedComparisons);
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