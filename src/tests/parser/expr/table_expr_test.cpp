#include "table_expr_test.hpp"

namespace Lua {
namespace Tests {

void TableExprTest::runAllTests() {
    // Basic table tests
    RUN_TEST(TableExprTest, testEmptyTables);
    RUN_TEST(TableExprTest, testArrayTables);
    RUN_TEST(TableExprTest, testHashTables);
    RUN_TEST(TableExprTest, testMixedTables);
    
    // Array-style table tests
    RUN_TEST(TableExprTest, testArrayWithLiterals);
    RUN_TEST(TableExprTest, testArrayWithVariables);
    RUN_TEST(TableExprTest, testArrayWithExpressions);
    RUN_TEST(TableExprTest, testArrayWithMixedElements);
    
    // Hash-style table tests
    RUN_TEST(TableExprTest, testHashWithStringKeys);
    RUN_TEST(TableExprTest, testHashWithIdentifierKeys);
    RUN_TEST(TableExprTest, testHashWithExpressionKeys);
    RUN_TEST(TableExprTest, testHashWithMixedKeys);
    
    // Value type tests
    RUN_TEST(TableExprTest, testTablesWithLiteralValues);
    RUN_TEST(TableExprTest, testTablesWithVariableValues);
    RUN_TEST(TableExprTest, testTablesWithExpressionValues);
    RUN_TEST(TableExprTest, testTablesWithFunctionCallValues);
    
    // Complex table tests
    RUN_TEST(TableExprTest, testNestedTables);
    RUN_TEST(TableExprTest, testTablesWithComplexExpressions);
    RUN_TEST(TableExprTest, testTablesWithUnaryExpressions);
    RUN_TEST(TableExprTest, testTablesWithBinaryExpressions);
    
    // Special syntax tests
    RUN_TEST(TableExprTest, testTablesWithTrailingCommas);
    RUN_TEST(TableExprTest, testTablesWithSemicolons);
    RUN_TEST(TableExprTest, testTablesWithMixedSeparators);
    RUN_TEST(TableExprTest, testTablesWithWhitespace);
    
    // Edge case tests
    RUN_TEST(TableExprTest, testLargeTables);
    RUN_TEST(TableExprTest, testTablesWithComplexKeys);
    RUN_TEST(TableExprTest, testTablesWithDuplicateKeys);
    RUN_TEST(TableExprTest, testTablesInExpressions);
    
    // Error handling tests
    RUN_TEST(TableExprTest, testInvalidTableSyntax);
    RUN_TEST(TableExprTest, testMalformedTableElements);
    RUN_TEST(TableExprTest, testUnterminatedTables);
    RUN_TEST(TableExprTest, testInvalidKeySyntax);
}

void TableExprTest::testEmptyTables() {
    testTableParsing("{}", 0, "Empty table");
    testTableParsing("{ }", 0, "Empty table with spaces");
    testTableParsing("{\n}", 0, "Empty table with newline");
    testTableParsing("{\t}", 0, "Empty table with tab");
}

void TableExprTest::testArrayTables() {
    testTableParsing("{1}", 1, "Single element array");
    testTableParsing("{1, 2}", 2, "Two element array");
    testTableParsing("{1, 2, 3}", 3, "Three element array");
    testTableParsing("{1, 2, 3, 4, 5}", 5, "Five element array");
}

void TableExprTest::testHashTables() {
    testTableParsing("{x = 1}", 1, "Single key-value pair");
    testTableParsing("{x = 1, y = 2}", 2, "Two key-value pairs");
    testTableParsing("{name = \"John\", age = 25}", 2, "String and number values");
    testTableParsing("{a = 1, b = 2, c = 3}", 3, "Three key-value pairs");
}

void TableExprTest::testMixedTables() {
    testTableParsing("{1, x = 2}", 2, "Array element and key-value pair");
    testTableParsing("{\"hello\", name = \"world\"}", 2, "String element and key-value pair");
    testTableParsing("{1, 2, x = 3, y = 4}", 4, "Two array elements and two key-value pairs");
    testTableParsing("{a = 1, 2, b = 3, 4}", 4, "Mixed array and hash elements");
}

void TableExprTest::testArrayWithLiterals() {
    testTableParsing("{42}", 1, "Array with integer");
    testTableParsing("{3.14}", 1, "Array with float");
    testTableParsing("{\"hello\"}", 1, "Array with string");
    testTableParsing("{true}", 1, "Array with boolean");
    testTableParsing("{nil}", 1, "Array with nil");
    testTableParsing("{42, 3.14, \"hello\", true, nil}", 5, "Array with mixed literals");
}

void TableExprTest::testArrayWithVariables() {
    testTableParsing("{x}", 1, "Array with variable");
    testTableParsing("{count}", 1, "Array with named variable");
    testTableParsing("{x, y}", 2, "Array with two variables");
    testTableParsing("{a, b, c}", 3, "Array with three variables");
    testTableParsing("{_private, _internal}", 2, "Array with underscore variables");
}

void TableExprTest::testArrayWithExpressions() {
    testTableParsing("{a + b}", 1, "Array with addition expression");
    testTableParsing("{x * 2}", 1, "Array with multiplication expression");
    testTableParsing("{not flag}", 1, "Array with logical not expression");
    testTableParsing("{-value}", 1, "Array with unary minus expression");
    testTableParsing("{a + b, x * y}", 2, "Array with two expressions");
}

void TableExprTest::testArrayWithMixedElements() {
    testTableParsing("{1, x}", 2, "Array with literal and variable");
    testTableParsing("{\"hello\", a + b}", 2, "Array with literal and expression");
    testTableParsing("{true, count, x * 2}", 3, "Array with literal, variable, and expression");
    testTableParsing("{42, \"test\", flag, getValue()}", 4, "Array with mixed element types");
}

void TableExprTest::testHashWithStringKeys() {
    testTableParsing("{[\"key\"] = \"value\"}", 1, "Hash with string key");
    testTableParsing("{[\"name\"] = \"John\", [\"age\"] = 25}", 2, "Hash with two string keys");
    testTableParsing("{[\"x\"] = 1, [\"y\"] = 2, [\"z\"] = 3}", 3, "Hash with three string keys");
}

void TableExprTest::testHashWithIdentifierKeys() {
    testTableParsing("{name = \"John\"}", 1, "Hash with identifier key");
    testTableParsing("{x = 1, y = 2}", 2, "Hash with two identifier keys");
    testTableParsing("{width = 100, height = 200, depth = 50}", 3, "Hash with three identifier keys");
    testTableParsing("{_private = true, _internal = false}", 2, "Hash with underscore identifier keys");
}

void TableExprTest::testHashWithExpressionKeys() {
    testTableParsing("{[x] = 1}", 1, "Hash with variable key");
    testTableParsing("{[a + b] = value}", 1, "Hash with expression key");
    testTableParsing("{[getValue()] = result}", 1, "Hash with function call key");
    testTableParsing("{[1 + 2] = \"three\", [2 * 3] = \"six\"}", 2, "Hash with arithmetic expression keys");
}

void TableExprTest::testHashWithMixedKeys() {
    testTableParsing("{name = \"John\", [\"age\"] = 25}", 2, "Hash with identifier and string keys");
    testTableParsing("{x = 1, [y] = 2}", 2, "Hash with identifier and variable keys");
    testTableParsing("{a = 1, [\"b\"] = 2, [c] = 3}", 3, "Hash with mixed key types");
    testTableParsing("{name = \"test\", [1] = \"first\", [getValue()] = \"dynamic\"}", 3, "Hash with complex mixed keys");
}

void TableExprTest::testTablesWithLiteralValues() {
    testTableParsing("{x = 42}", 1, "Table with integer value");
    testTableParsing("{pi = 3.14}", 1, "Table with float value");
    testTableParsing("{name = \"John\"}", 1, "Table with string value");
    testTableParsing("{flag = true}", 1, "Table with boolean value");
    testTableParsing("{value = nil}", 1, "Table with nil value");
    testTableParsing("{a = 1, b = 2.5, c = \"test\", d = true, e = nil}", 5, "Table with mixed literal values");
}

void TableExprTest::testTablesWithVariableValues() {
    testTableParsing("{x = y}", 1, "Table with variable value");
    testTableParsing("{name = userName}", 1, "Table with named variable value");
    testTableParsing("{a = x, b = y}", 2, "Table with two variable values");
    testTableParsing("{width = w, height = h, depth = d}", 3, "Table with three variable values");
}

void TableExprTest::testTablesWithExpressionValues() {
    testTableParsing("{x = a + b}", 1, "Table with addition expression value");
    testTableParsing("{result = x * y}", 1, "Table with multiplication expression value");
    testTableParsing("{flag = not condition}", 1, "Table with logical not expression value");
    testTableParsing("{value = -amount}", 1, "Table with unary minus expression value");
    testTableParsing("{sum = a + b, product = x * y}", 2, "Table with two expression values");
}

void TableExprTest::testTablesWithFunctionCallValues() {
    testTableParsing("{time = getTime()}", 1, "Table with function call value");
    testTableParsing("{name = getName()}", 1, "Table with named function call value");
    testTableParsing("{result = calculate(x, y)}", 1, "Table with function call with arguments");
    testTableParsing("{a = func1(), b = func2()}", 2, "Table with two function call values");
}

void TableExprTest::testNestedTables() {
    testTableParsing("{{1}}", 1, "Table with nested array");
    testTableParsing("{x = {1, 2}}", 1, "Table with nested array value");
    testTableParsing("{inner = {a = 1}}", 1, "Table with nested hash value");
    testTableParsing("{a = {b = {c = 1}}}", 1, "Deeply nested tables");
    testTableParsing("{arr = {1, 2}, hash = {x = 1, y = 2}}", 2, "Table with nested array and hash");
}

void TableExprTest::testTablesWithComplexExpressions() {
    testTableParsing("{result = (a + b) * c}", 1, "Table with complex arithmetic expression");
    testTableParsing("{flag = a and b or c}", 1, "Table with complex logical expression");
    testTableParsing("{comparison = x < y and y < z}", 1, "Table with complex comparison expression");
    testTableParsing("{message = \"result: \" .. getValue()}", 1, "Table with concatenation expression");
}

void TableExprTest::testTablesWithUnaryExpressions() {
    testTableParsing("{negative = -x}", 1, "Table with unary minus");
    testTableParsing("{positive = +value}", 1, "Table with unary plus");
    testTableParsing("{inverted = not flag}", 1, "Table with logical not");
    testTableParsing("{length = #array}", 1, "Table with length operator");
    testTableParsing("{a = -x, b = +y, c = not z}", 3, "Table with multiple unary expressions");
}

void TableExprTest::testTablesWithBinaryExpressions() {
    testTableParsing("{sum = a + b}", 1, "Table with addition");
    testTableParsing("{product = x * y}", 1, "Table with multiplication");
    testTableParsing("{equal = a == b}", 1, "Table with equality");
    testTableParsing("{logical = x and y}", 1, "Table with logical and");
    testTableParsing("{a = x + y, b = m * n}", 2, "Table with multiple binary expressions");
}

void TableExprTest::testTablesWithTrailingCommas() {
    testTableParsing("{1,}", 1, "Array with trailing comma");
    testTableParsing("{1, 2,}", 2, "Array with trailing comma after multiple elements");
    testTableParsing("{x = 1,}", 1, "Hash with trailing comma");
    testTableParsing("{a = 1, b = 2,}", 2, "Hash with trailing comma after multiple pairs");
    testTableParsing("{1, x = 2,}", 2, "Mixed table with trailing comma");
}

void TableExprTest::testTablesWithSemicolons() {
    testTableParsing("{1; 2}", 2, "Array with semicolon separator");
    testTableParsing("{x = 1; y = 2}", 2, "Hash with semicolon separator");
    testTableParsing("{1; x = 2}", 2, "Mixed table with semicolon separator");
    testTableParsing("{a = 1; 2; b = 3}", 3, "Mixed table with semicolons");
}

void TableExprTest::testTablesWithMixedSeparators() {
    testTableParsing("{1, 2; 3}", 3, "Array with mixed comma and semicolon");
    testTableParsing("{x = 1, y = 2; z = 3}", 3, "Hash with mixed separators");
    testTableParsing("{1; x = 2, 3}", 3, "Mixed table with mixed separators");
    testTableParsing("{a = 1, 2; b = 3, 4}", 4, "Complex mixed separators");
}

void TableExprTest::testTablesWithWhitespace() {
    testTableParsing("{ 1 , 2 }", 2, "Array with spaces around elements");
    testTableParsing("{\n  1,\n  2\n}", 2, "Array with newlines and indentation");
    testTableParsing("{ x = 1 , y = 2 }", 2, "Hash with spaces around pairs");
    testTableParsing("{\n  x = 1,\n  y = 2\n}", 2, "Hash with newlines and indentation");
}

void TableExprTest::testLargeTables() {
    testTableParsing("{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}", 10, "Large array table");
    testTableParsing("{a=1, b=2, c=3, d=4, e=5, f=6, g=7, h=8}", 8, "Large hash table");
    testTableParsing("{1, 2, a=3, b=4, 5, 6, c=7, d=8}", 8, "Large mixed table");
}

void TableExprTest::testTablesWithComplexKeys() {
    testTableParsing("{[a + b] = 1}", 1, "Table with arithmetic expression key");
    testTableParsing("{[func()] = value}", 1, "Table with function call key");
    testTableParsing("{[obj.property] = data}", 1, "Table with member access key");
    testTableParsing("{[\"prefix\" .. suffix] = result}", 1, "Table with concatenation key");
}

void TableExprTest::testTablesWithDuplicateKeys() {
    testTableParsing("{x = 1, x = 2}", 2, "Table with duplicate identifier keys");
    testTableParsing("{[\"key\"] = 1, [\"key\"] = 2}", 2, "Table with duplicate string keys");
    testTableParsing("{[1] = \"first\", [1] = \"second\"}", 2, "Table with duplicate numeric keys");
}

void TableExprTest::testTablesInExpressions() {
    testTableParsing("{1, 2} + {3, 4}", 2, "Table in binary expression (left operand)");
    testTableParsing("func({1, 2})", 2, "Table as function argument");
    testTableParsing("not {}", 0, "Table in unary expression");
    testTableParsing("#{1, 2, 3}", 3, "Table with length operator");
}

void TableExprTest::testInvalidTableSyntax() {
    testTableParsingError("{", "Unterminated table");
    testTableParsingError("}", "Invalid table start");
    testTableParsingError("{,}", "Invalid comma in empty table");
    testTableParsingError("{;}", "Invalid semicolon in empty table");
    testTableParsingError("{=}", "Invalid equals in table");
}

void TableExprTest::testMalformedTableElements() {
    testTableParsingError("{1,,2}", "Double comma in table");
    testTableParsingError("{1 2}", "Missing separator in table");
    testTableParsingError("{x =}", "Missing value in key-value pair");
    testTableParsingError("{= 1}", "Missing key in key-value pair");
    testTableParsingError("{x = = 1}", "Double equals in key-value pair");
}

void TableExprTest::testUnterminatedTables() {
    testTableParsingError("{1, 2", "Missing closing brace");
    testTableParsingError("{x = 1, y = 2", "Missing closing brace in hash");
    testTableParsingError("{1, {2, 3", "Missing closing brace in nested table");
    testTableParsingError("{x = {y = 1", "Missing closing brace in nested hash");
}

void TableExprTest::testInvalidKeySyntax() {
    testTableParsingError("{[} = 1}", "Invalid key expression");
    testTableParsingError("{[] = 1}", "Empty key expression");
    testTableParsingError("{[1 = 1}", "Unterminated key expression");
    testTableParsingError("{1] = 1}", "Invalid key bracket");
}

void TableExprTest::testTableParsing(const std::string& input, int expectedElementCount, const std::string& testName) {
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

void TableExprTest::testTableParsingError(const std::string& input, const std::string& testName) {
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

bool TableExprTest::verifyTableExpression(const Expr* expr, int expectedElementCount) {
    if (!expr) {
        return false;
    }
    
    const TableExpr* tableExpr = dynamic_cast<const TableExpr*>(expr);
    if (!tableExpr) {
        return false;
    }
    
    return static_cast<int>(tableExpr->getFields().size()) == expectedElementCount;
}

void TableExprTest::printTableExpressionInfo(const TableExpr* tableExpr) {
    if (!tableExpr) {
        return;
    }
    
    TestUtils::printInfo("  Element count: " + std::to_string(tableExpr->getFields().size()));
    
    for (size_t i = 0; i < tableExpr->getFields().size() && i < 3; ++i) {
        const auto& element = tableExpr->getFields()[i];
        std::string elementType = getTableElementType(element);
        TestUtils::printInfo("  Element " + std::to_string(i + 1) + " type: " + elementType);
    }
    
    if (tableExpr->getFields().size() > 3) {
        TestUtils::printInfo("  ... and " + std::to_string(tableExpr->getFields().size() - 3) + " more elements");
    }
}

std::string TableExprTest::getTableElementType(const TableField& element) {
    if (element.key) {
        return "key-value pair";
    } else {
        return "array element";
    }
}

} // namespace Tests
} // namespace Lua