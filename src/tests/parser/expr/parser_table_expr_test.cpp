#include "parser_table_expr_test.hpp"

#include "../../../parser/parser.hpp"
#include "../../../parser/ast/expressions.hpp"
#include "../../../test_framework/core/test_macros.hpp"
#include "../../../test_framework/core/test_utils.hpp"

using namespace Lua;
using namespace Tests;
using namespace TestFramework;

void ParserTableExprTest::runAllTests() {
    // Basic table tests
    RUN_TEST_GROUP("Basic table tests", []() {
        RUN_TEST(ParserTableExprTest, testEmptyTables);
        RUN_TEST(ParserTableExprTest, testArrayTables);
        RUN_TEST(ParserTableExprTest, testHashTables);
        RUN_TEST(ParserTableExprTest, testMixedTables);
	});
    
    // Array-style table tests
    RUN_TEST_GROUP("Array-style table tests", []() {
        RUN_TEST(ParserTableExprTest, testArrayWithLiterals);
        RUN_TEST(ParserTableExprTest, testArrayWithVariables);
        RUN_TEST(ParserTableExprTest, testArrayWithExpressions);
        RUN_TEST(ParserTableExprTest, testArrayWithMixedElements);
    });
    
    // Hash-style table tests
    RUN_TEST_GROUP("Hash-style table tests", []() {
        RUN_TEST(ParserTableExprTest, testHashWithStringKeys);
        RUN_TEST(ParserTableExprTest, testHashWithIdentifierKeys);
        RUN_TEST(ParserTableExprTest, testHashWithExpressionKeys);
        RUN_TEST(ParserTableExprTest, testHashWithMixedKeys);
    });
    
    // Value type tests
    RUN_TEST_GROUP("Value type tests", []() {
        RUN_TEST(ParserTableExprTest, testTablesWithLiteralValues);
        RUN_TEST(ParserTableExprTest, testTablesWithVariableValues);
        RUN_TEST(ParserTableExprTest, testTablesWithExpressionValues);
        RUN_TEST(ParserTableExprTest, testTablesWithFunctionCallValues);
    });
    
    // Complex table tests
    RUN_TEST_GROUP("Complex table tests", []() {
        RUN_TEST(ParserTableExprTest, testNestedTables);
        RUN_TEST(ParserTableExprTest, testTablesWithComplexExpressions);
        RUN_TEST(ParserTableExprTest, testTablesWithUnaryExpressions);
        RUN_TEST(ParserTableExprTest, testTablesWithBinaryExpressions);
    });
    
    // Special syntax tests
    RUN_TEST_GROUP("Special syntax tests", []() {
        RUN_TEST(ParserTableExprTest, testTablesWithTrailingCommas);
        RUN_TEST(ParserTableExprTest, testTablesWithSemicolons);
        RUN_TEST(ParserTableExprTest, testTablesWithMixedSeparators);
        RUN_TEST(ParserTableExprTest, testTablesWithWhitespace);
    });
    
    // Edge case tests
    RUN_TEST_GROUP("Edge case tests", []() {
        RUN_TEST(ParserTableExprTest, testLargeTables);
        RUN_TEST(ParserTableExprTest, testTablesWithComplexKeys);
        RUN_TEST(ParserTableExprTest, testTablesWithDuplicateKeys);
        RUN_TEST(ParserTableExprTest, testTablesInExpressions);
    });
    
    // Error handling tests
    RUN_TEST_GROUP("Error handling tests", []() {
        RUN_TEST(ParserTableExprTest, testInvalidTableSyntax);
        RUN_TEST(ParserTableExprTest, testMalformedTableElements);
        RUN_TEST(ParserTableExprTest, testUnterminatedTables);
        RUN_TEST(ParserTableExprTest, testInvalidKeySyntax);
    });
}

void ParserTableExprTest::testEmptyTables() {
    testTableParsing("{}", 0, "Empty table");
    testTableParsing("{ }", 0, "Empty table with spaces");
    testTableParsing("{\n}", 0, "Empty table with newline");
    testTableParsing("{\t}", 0, "Empty table with tab");
}

void ParserTableExprTest::testArrayTables() {
    testTableParsing("{1}", 1, "Single element array");
    testTableParsing("{1, 2}", 2, "Two element array");
    testTableParsing("{1, 2, 3}", 3, "Three element array");
    testTableParsing("{1, 2, 3, 4, 5}", 5, "Five element array");
}

void ParserTableExprTest::testHashTables() {
    testTableParsing("{x = 1}", 1, "Single key-value pair");
    testTableParsing("{x = 1, y = 2}", 2, "Two key-value pairs");
    testTableParsing("{name = \"John\", age = 25}", 2, "String and number values");
    testTableParsing("{a = 1, b = 2, c = 3}", 3, "Three key-value pairs");
}

void ParserTableExprTest::testMixedTables() {
    testTableParsing("{1, x = 2}", 2, "Array element and key-value pair");
    testTableParsing("{\"hello\", name = \"world\"}", 2, "String element and key-value pair");
    testTableParsing("{1, 2, x = 3, y = 4}", 4, "Two array elements and two key-value pairs");
    testTableParsing("{a = 1, 2, b = 3, 4}", 4, "Mixed array and hash elements");
}

void ParserTableExprTest::testArrayWithLiterals() {
    testTableParsing("{42}", 1, "Array with integer");
    testTableParsing("{3.14}", 1, "Array with float");
    testTableParsing("{\"hello\"}", 1, "Array with string");
    testTableParsing("{true}", 1, "Array with boolean");
    testTableParsing("{nil}", 1, "Array with nil");
    testTableParsing("{42, 3.14, \"hello\", true, nil}", 5, "Array with mixed literals");
}

void ParserTableExprTest::testArrayWithVariables() {
    testTableParsing("{x}", 1, "Array with variable");
    testTableParsing("{count}", 1, "Array with named variable");
    testTableParsing("{x, y}", 2, "Array with two variables");
    testTableParsing("{a, b, c}", 3, "Array with three variables");
    testTableParsing("{_private, _internal}", 2, "Array with underscore variables");
}

void ParserTableExprTest::testArrayWithExpressions() {
    testTableParsing("{a + b}", 1, "Array with addition expression");
    testTableParsing("{x * 2}", 1, "Array with multiplication expression");
    testTableParsing("{not flag}", 1, "Array with logical not expression");
    testTableParsing("{-value}", 1, "Array with unary minus expression");
    testTableParsing("{a + b, x * y}", 2, "Array with two expressions");
}

void ParserTableExprTest::testArrayWithMixedElements() {
    testTableParsing("{1, x}", 2, "Array with literal and variable");
    testTableParsing("{\"hello\", a + b}", 2, "Array with literal and expression");
    testTableParsing("{true, count, x * 2}", 3, "Array with literal, variable, and expression");
    testTableParsing("{42, \"test\", flag, getValue()}", 4, "Array with mixed element types");
}

void ParserTableExprTest::testHashWithStringKeys() {
    testTableParsing("{[\"key\"] = \"value\"}", 1, "Hash with string key");
    testTableParsing("{[\"name\"] = \"John\", [\"age\"] = 25}", 2, "Hash with two string keys");
    testTableParsing("{[\"x\"] = 1, [\"y\"] = 2, [\"z\"] = 3}", 3, "Hash with three string keys");
}

void ParserTableExprTest::testHashWithIdentifierKeys() {
    testTableParsing("{name = \"John\"}", 1, "Hash with identifier key");
    testTableParsing("{x = 1, y = 2}", 2, "Hash with two identifier keys");
    testTableParsing("{width = 100, height = 200, depth = 50}", 3, "Hash with three identifier keys");
    testTableParsing("{_private = true, _internal = false}", 2, "Hash with underscore identifier keys");
}

void ParserTableExprTest::testHashWithExpressionKeys() {
    testTableParsing("{[x] = 1}", 1, "Hash with variable key");
    testTableParsing("{[a + b] = value}", 1, "Hash with expression key");
    testTableParsing("{[getValue()] = result}", 1, "Hash with function call key");
    testTableParsing("{[1 + 2] = \"three\", [2 * 3] = \"six\"}", 2, "Hash with arithmetic expression keys");
}

void ParserTableExprTest::testHashWithMixedKeys() {
    testTableParsing("{name = \"John\", [\"age\"] = 25}", 2, "Hash with identifier and string keys");
    testTableParsing("{x = 1, [y] = 2}", 2, "Hash with identifier and variable keys");
    testTableParsing("{a = 1, [\"b\"] = 2, [c] = 3}", 3, "Hash with mixed key types");
    testTableParsing("{name = \"test\", [1] = \"first\", [getValue()] = \"dynamic\"}", 3, "Hash with complex mixed keys");
}

void ParserTableExprTest::testTablesWithLiteralValues() {
    testTableParsing("{x = 42}", 1, "Table with integer value");
    testTableParsing("{pi = 3.14}", 1, "Table with float value");
    testTableParsing("{name = \"John\"}", 1, "Table with string value");
    testTableParsing("{flag = true}", 1, "Table with boolean value");
    testTableParsing("{value = nil}", 1, "Table with nil value");
    testTableParsing("{a = 1, b = 2.5, c = \"test\", d = true, e = nil}", 5, "Table with mixed literal values");
}

void ParserTableExprTest::testTablesWithVariableValues() {
    testTableParsing("{x = y}", 1, "Table with variable value");
    testTableParsing("{name = userName}", 1, "Table with named variable value");
    testTableParsing("{a = x, b = y}", 2, "Table with two variable values");
    testTableParsing("{width = w, height = h, depth = d}", 3, "Table with three variable values");
}

void ParserTableExprTest::testTablesWithExpressionValues() {
    testTableParsing("{x = a + b}", 1, "Table with addition expression value");
    testTableParsing("{result = x * y}", 1, "Table with multiplication expression value");
    testTableParsing("{flag = not condition}", 1, "Table with logical not expression value");
    testTableParsing("{value = -amount}", 1, "Table with unary minus expression value");
    testTableParsing("{sum = a + b, product = x * y}", 2, "Table with two expression values");
}

void ParserTableExprTest::testTablesWithFunctionCallValues() {
    testTableParsing("{time = getTime()}", 1, "Table with function call value");
    testTableParsing("{name = getName()}", 1, "Table with named function call value");
    testTableParsing("{result = calculate(x, y)}", 1, "Table with function call with arguments");
    testTableParsing("{a = func1(), b = func2()}", 2, "Table with two function call values");
}

void ParserTableExprTest::testNestedTables() {
    testTableParsing("{{1}}", 1, "Table with nested array");
    testTableParsing("{x = {1, 2}}", 1, "Table with nested array value");
    testTableParsing("{inner = {a = 1}}", 1, "Table with nested hash value");
    testTableParsing("{a = {b = {c = 1}}}", 1, "Deeply nested tables");
    testTableParsing("{arr = {1, 2}, hash = {x = 1, y = 2}}", 2, "Table with nested array and hash");
}

void ParserTableExprTest::testTablesWithComplexExpressions() {
    testTableParsing("{result = (a + b) * c}", 1, "Table with complex arithmetic expression");
    testTableParsing("{flag = a and b or c}", 1, "Table with complex logical expression");
    testTableParsing("{comparison = x < y and y < z}", 1, "Table with complex comparison expression");
    testTableParsing("{message = \"result: \" .. getValue()}", 1, "Table with concatenation expression");
}

void ParserTableExprTest::testTablesWithUnaryExpressions() {
    testTableParsing("{negative = -x}", 1, "Table with unary minus");
    testTableParsing("{positive = +value}", 1, "Table with unary plus");
    testTableParsing("{inverted = not flag}", 1, "Table with logical not");
    testTableParsing("{length = #array}", 1, "Table with length operator");
    testTableParsing("{a = -x, b = +y, c = not z}", 3, "Table with multiple unary expressions");
}

void ParserTableExprTest::testTablesWithBinaryExpressions() {
    testTableParsing("{sum = a + b}", 1, "Table with addition");
    testTableParsing("{product = x * y}", 1, "Table with multiplication");
    testTableParsing("{equal = a == b}", 1, "Table with equality");
    testTableParsing("{logical = x and y}", 1, "Table with logical and");
    testTableParsing("{a = x + y, b = m * n}", 2, "Table with multiple binary expressions");
}

void ParserTableExprTest::testTablesWithTrailingCommas() {
    testTableParsing("{1,}", 1, "Array with trailing comma");
    testTableParsing("{1, 2,}", 2, "Array with trailing comma after multiple elements");
    testTableParsing("{x = 1,}", 1, "Hash with trailing comma");
    testTableParsing("{a = 1, b = 2,}", 2, "Hash with trailing comma after multiple pairs");
    testTableParsing("{1, x = 2,}", 2, "Mixed table with trailing comma");
}

void ParserTableExprTest::testTablesWithSemicolons() {
    testTableParsing("{1; 2}", 2, "Array with semicolon separator");
    testTableParsing("{x = 1; y = 2}", 2, "Hash with semicolon separator");
    testTableParsing("{1; x = 2}", 2, "Mixed table with semicolon separator");
    testTableParsing("{a = 1; 2; b = 3}", 3, "Mixed table with semicolons");
}

void ParserTableExprTest::testTablesWithMixedSeparators() {
    testTableParsing("{1, 2; 3}", 3, "Array with mixed comma and semicolon");
    testTableParsing("{x = 1, y = 2; z = 3}", 3, "Hash with mixed separators");
    testTableParsing("{1; x = 2, 3}", 3, "Mixed table with mixed separators");
    testTableParsing("{a = 1, 2; b = 3, 4}", 4, "Complex mixed separators");
}

void ParserTableExprTest::testTablesWithWhitespace() {
    testTableParsing("{ 1 , 2 }", 2, "Array with spaces around elements");
    testTableParsing("{\n  1,\n  2\n}", 2, "Array with newlines and indentation");
    testTableParsing("{ x = 1 , y = 2 }", 2, "Hash with spaces around pairs");
    testTableParsing("{\n  x = 1,\n  y = 2\n}", 2, "Hash with newlines and indentation");
}

void ParserTableExprTest::testLargeTables() {
    testTableParsing("{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}", 10, "Large array table");
    testTableParsing("{a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8}", 8, "Large hash table");
    testTableParsing("{1, 2, a=3, b=4, 5, 6, c=7, d=8}", 8, "Large mixed table");
}

void ParserTableExprTest::testTablesWithComplexKeys() {
    testTableParsing("{[a + b] = 1}", 1, "Table with arithmetic expression key");
    testTableParsing("{[func()] = value}", 1, "Table with function call key");
    testTableParsing("{[obj.property] = data}", 1, "Table with member access key");
    testTableParsing("{[\"prefix\" .. suffix] = result}", 1, "Table with concatenation key");
}

void ParserTableExprTest::testTablesWithDuplicateKeys() {
    testTableParsing("{x = 1, x = 2}", 2, "Table with duplicate identifier keys");
    testTableParsing("{[\"key\"] = 1, [\"key\"] = 2}", 2, "Table with duplicate string keys");
    testTableParsing("{[1] = \"first\", [1] = \"second\"}", 2, "Table with duplicate numeric keys");
}

void ParserTableExprTest::testTablesInExpressions() {
    testTableParsing("{1, 2} + {3, 4}", 2, "Table in binary expression (left operand)");
    testTableParsing("func({1, 2})", 2, "Table as function argument");
    testTableParsing("not {}", 0, "Table in unary expression");
    testTableParsing("#{1, 2, 3}", 3, "Table with length operator");
}

void ParserTableExprTest::testInvalidTableSyntax() {
    testTableParsingError("{", "Unterminated table");
    testTableParsingError("}", "Invalid table start");
    testTableParsingError("{,}", "Invalid comma in empty table");
    testTableParsingError("{;}", "Invalid semicolon in empty table");
    testTableParsingError("{=}", "Invalid equals in table");
}

void ParserTableExprTest::testMalformedTableElements() {
    testTableParsingError("{1,,2}", "Double comma in table");
    testTableParsingError("{1 2}", "Missing separator in table");
    testTableParsingError("{x =}", "Missing value in key-value pair");
    testTableParsingError("{= 1}", "Missing key in key-value pair");
    testTableParsingError("{x = = 1}", "Double equals in key-value pair");
}

void ParserTableExprTest::testUnterminatedTables() {
    testTableParsingError("{1, 2", "Missing closing brace");
    testTableParsingError("{x = 1, y = 2", "Missing closing brace in hash");
    testTableParsingError("{1, {2, 3", "Missing closing brace in nested table");
    testTableParsingError("{x = {y = 1", "Missing closing brace in nested hash");
}

void ParserTableExprTest::testInvalidKeySyntax() {
    testTableParsingError("{[} = 1}", "Invalid key expression");
    testTableParsingError("{[] = 1}", "Empty key expression");
    testTableParsingError("{[1 = 1}", "Unterminated key expression");
    testTableParsingError("{1] = 1}", "Invalid key bracket");
}

void ParserTableExprTest::testTableParsing(const std::string& input, int expectedElementCount, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printInfo("Failed to parse expression");
        TestUtils::printTestResult(testName, false);
            return;
        }
        
        if (!verifyTableExpression(expr.get(), expectedElementCount)) {
            TestUtils::printInfo("Expression is not a table expression or element count mismatch");
        TestUtils::printTestResult(testName, false);
            return;
        }
        
        TestUtils::printInfo("Successfully parsed table expression");
        TestUtils::printTestResult(testName, true);
        
        // Print additional info for debugging
        if (auto tableExpr = dynamic_cast<const TableExpr*>(expr.get())) {
            printTableExpressionInfo(tableExpr);
        }
        
    } catch (const std::exception& e) {
        TestUtils::printInfo(std::string("Exception: ") + e.what());
        TestUtils::printTestResult(testName, false);
    }
}

void ParserTableExprTest::testTableParsingError(const std::string& input, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (!expr) {
            TestUtils::printInfo("Correctly failed to parse invalid table expression");
        TestUtils::printTestResult(testName, true);
            return;
        }
        
        TestUtils::printInfo("Should have failed to parse invalid table expression");
        TestUtils::printTestResult(testName, false);
        
    } catch (const std::exception& e) {
        TestUtils::printInfo(std::string("Correctly threw exception: ") + e.what());
        TestUtils::printTestResult(testName, true);
    }
}

bool ParserTableExprTest::verifyTableExpression(const Expr* expr, int expectedElementCount) {
    if (!expr) {
        return false;
    }
    
    const TableExpr* tableExpr = dynamic_cast<const TableExpr*>(expr);
    if (!tableExpr) {
        return false;
    }
    
    return static_cast<int>(tableExpr->getFields().size()) == expectedElementCount;
}

void ParserTableExprTest::printTableExpressionInfo(const TableExpr* tableExpr) {
    if (!tableExpr) {
        return;
    }
    
    TestUtils::printInfo("  Element count: " + std::to_string(tableExpr->getFields().size()));
    
    for (size_t i = 0; i < tableExpr->getFields().size() && i < 3; ++i) {
        const auto& element = tableExpr->getFields()[i];
        std::string elementType = ParserTableExprTest::getTableElementType(element);
        TestUtils::printInfo("  Element " + std::to_string(i + 1) + " type: " + elementType);
    }
    
    if (tableExpr->getFields().size() > 3) {
        TestUtils::printInfo("  ... and " + std::to_string(tableExpr->getFields().size() - 3) + " more elements");
    }
}

std::string ParserTableExprTest::getTableElementType(const Lua::TableField& element) {
    if (element.key) {
        return "key-value pair";
    } else {
        return "array element";
    }
}