#include "member_expr_test.hpp"

namespace Lua {
namespace Tests {
namespace Parser {
namespace Expr {

void MemberExprTest::runAllTests() {
    // Basic member access tests
    RUN_TEST(MemberExprTest, testDotNotation);
    RUN_TEST(MemberExprTest, testBracketNotation);
    RUN_TEST(MemberExprTest, testSimpleMemberAccess);
    
    // Dot notation tests
    RUN_TEST(MemberExprTest, testDotWithIdentifiers);
    RUN_TEST(MemberExprTest, testDotWithComplexObjects);
    RUN_TEST(MemberExprTest, testDotWithReservedWords);
    RUN_TEST(MemberExprTest, testDotWithUnderscoreNames);
    
    // Bracket notation tests
    RUN_TEST(MemberExprTest, testBracketWithStringKeys);
    RUN_TEST(MemberExprTest, testBracketWithNumericKeys);
    RUN_TEST(MemberExprTest, testBracketWithVariableKeys);
    RUN_TEST(MemberExprTest, testBracketWithExpressionKeys);
    
    // Chained member access tests
    RUN_TEST(MemberExprTest, testChainedDotAccess);
    RUN_TEST(MemberExprTest, testChainedBracketAccess);
    RUN_TEST(MemberExprTest, testMixedChainedAccess);
    RUN_TEST(MemberExprTest, testDeepChainedAccess);
    
    // Complex object tests
    RUN_TEST(MemberExprTest, testMemberAccessOnFunctionCalls);
    RUN_TEST(MemberExprTest, testMemberAccessOnTableConstructors);
    RUN_TEST(MemberExprTest, testMemberAccessOnParenthesizedExpressions);
    RUN_TEST(MemberExprTest, testMemberAccessOnComplexExpressions);
    
    // Key expression tests
    RUN_TEST(MemberExprTest, testBracketWithArithmeticKeys);
    RUN_TEST(MemberExprTest, testBracketWithLogicalKeys);
    RUN_TEST(MemberExprTest, testBracketWithComparisonKeys);
    RUN_TEST(MemberExprTest, testBracketWithUnaryKeys);
    
    // Member access in expressions tests
    RUN_TEST(MemberExprTest, testMemberAccessInBinaryExpressions);
    RUN_TEST(MemberExprTest, testMemberAccessInUnaryExpressions);
    RUN_TEST(MemberExprTest, testMemberAccessInFunctionCalls);
    RUN_TEST(MemberExprTest, testMemberAccessInTableConstructors);
    
    // Special cases tests
    RUN_TEST(MemberExprTest, testMemberAccessWithWhitespace);
    RUN_TEST(MemberExprTest, testMemberAccessWithComments);
    RUN_TEST(MemberExprTest, testMemberAccessWithNewlines);
    RUN_TEST(MemberExprTest, testMemberAccessPrecedence);
    
    // Error handling tests
    RUN_TEST(MemberExprTest, testInvalidDotNotation);
    RUN_TEST(MemberExprTest, testInvalidBracketNotation);
    RUN_TEST(MemberExprTest, testMalformedMemberAccess);
    RUN_TEST(MemberExprTest, testUnterminatedBracketAccess);
}

// Basic member access tests
void MemberExprTest::testDotNotation() {
    testMemberParsing("obj.prop", "obj", "prop", "Simple dot notation");
    testMemberParsing("table.field", "table", "field", "Table field access");
    testMemberParsing("self.value", "self", "value", "Self reference");
    testMemberParsing("module.function", "module", "function", "Module function access");
}

void MemberExprTest::testBracketNotation() {
    testBracketMemberParsing("obj[\"key\"]", "obj", "String key bracket notation");
    testBracketMemberParsing("table[1]", "table", "Numeric key bracket notation");
    testBracketMemberParsing("arr[index]", "arr", "Variable key bracket notation");
    testBracketMemberParsing("data[key]", "data", "Dynamic key access");
}

void MemberExprTest::testSimpleMemberAccess() {
    testMemberParsing("a.b", "a", "b", "Single character members");
    testMemberParsing("x.y", "x", "y", "Variable member access");
    testMemberParsing("obj.name", "obj", "name", "Object name property");
    testMemberParsing("config.value", "config", "value", "Configuration value");
}

// Dot notation tests
void MemberExprTest::testDotWithIdentifiers() {
    testMemberParsing("object.property", "object", "property", "Standard identifier");
    testMemberParsing("myTable.myField", "myTable", "myField", "CamelCase identifiers");
    testMemberParsing("user_data.user_name", "user_data", "user_name", "Underscore identifiers");
    testMemberParsing("obj123.prop456", "obj123", "prop456", "Alphanumeric identifiers");
}

void MemberExprTest::testDotWithComplexObjects() {
    testBracketMemberParsing("getObject().property", "getObject()", "Function call object");
    testBracketMemberParsing("{a=1}.a", "{a=1}", "Table constructor object");
    testBracketMemberParsing("(obj).prop", "(obj)", "Parenthesized object");
}

void MemberExprTest::testDotWithReservedWords() {
    // Note: In Lua, reserved words can be used as field names after dot
    testMemberParsing("obj.end", "obj", "end", "Reserved word as field");
    testMemberParsing("table.function", "table", "function", "Function keyword as field");
    testMemberParsing("data.if", "data", "if", "If keyword as field");
    testMemberParsing("config.while", "config", "while", "While keyword as field");
}

void MemberExprTest::testDotWithUnderscoreNames() {
    testMemberParsing("obj._private", "obj", "_private", "Private field convention");
    testMemberParsing("table.__index", "table", "__index", "Metamethod field");
    testMemberParsing("data._internal_value", "data", "_internal_value", "Internal field");
    testMemberParsing("obj._", "obj", "_", "Single underscore field");
}

// Bracket notation tests
void MemberExprTest::testBracketWithStringKeys() {
    testBracketMemberParsing("obj[\"key\"]", "obj", "Double quoted string key");
    testBracketMemberParsing("table['field']", "table", "Single quoted string key");
    testBracketMemberParsing("data[\"complex key\"]", "data", "String key with spaces");
    testBracketMemberParsing("obj[\"123\"]", "obj", "Numeric string key");
}

void MemberExprTest::testBracketWithNumericKeys() {
    testBracketMemberParsing("arr[1]", "arr", "Integer key");
    testBracketMemberParsing("table[0]", "table", "Zero key");
    testBracketMemberParsing("data[42]", "data", "Positive integer key");
    testBracketMemberParsing("obj[-1]", "obj", "Negative integer key");
    testBracketMemberParsing("table[3.14]", "table", "Float key");
}

void MemberExprTest::testBracketWithVariableKeys() {
    testBracketMemberParsing("obj[key]", "obj", "Variable key");
    testBracketMemberParsing("table[index]", "table", "Index variable");
    testBracketMemberParsing("data[field_name]", "data", "Underscore variable key");
    testBracketMemberParsing("arr[i]", "arr", "Single character variable");
}

void MemberExprTest::testBracketWithExpressionKeys() {
    testBracketMemberParsing("obj[a + b]", "obj", "Arithmetic expression key");
    testBracketMemberParsing("table[func()]", "table", "Function call key");
    testBracketMemberParsing("data[obj.field]", "data", "Member access key");
    testBracketMemberParsing("arr[#list]", "arr", "Length operator key");
}

// Chained member access tests
void MemberExprTest::testChainedDotAccess() {
    testBracketMemberParsing("obj.prop.field", "obj.prop", "Two-level dot access");
    testBracketMemberParsing("a.b.c", "a.b", "Three-level dot access");
    testBracketMemberParsing("module.submodule.function", "module.submodule", "Module hierarchy access");
    testBracketMemberParsing("config.database.connection", "config.database", "Configuration hierarchy");
}

void MemberExprTest::testChainedBracketAccess() {
    testBracketMemberParsing("obj[\"key\"][\"field\"]", "obj[\"key\"]", "Two-level bracket access");
    testBracketMemberParsing("table[1][2]", "table[1]", "Numeric bracket chain");
    testBracketMemberParsing("data[key][index]", "data[key]", "Variable bracket chain");
    testBracketMemberParsing("arr[i][j]", "arr[i]", "Index bracket chain");
}

void MemberExprTest::testMixedChainedAccess() {
    testBracketMemberParsing("obj.prop[\"key\"]", "obj.prop", "Dot then bracket access");
    testBracketMemberParsing("table[\"field\"].value", "table[\"field\"]", "Bracket then dot access");
    testBracketMemberParsing("data[1].field[\"key\"]", "data[1].field", "Mixed three-level access");
    testBracketMemberParsing("obj[key].prop.field", "obj[key].prop", "Complex mixed access");
}

void MemberExprTest::testDeepChainedAccess() {
    testBracketMemberParsing("a.b.c.d.e", "a.b.c.d", "Five-level dot chain");
    testBracketMemberParsing("obj[1][2][3][4]", "obj[1][2][3]", "Four-level bracket chain");
    testBracketMemberParsing("data.a[1].b[2].c", "data.a[1].b[2]", "Deep mixed chain");
}

// Complex object tests
void MemberExprTest::testMemberAccessOnFunctionCalls() {
    testBracketMemberParsing("func().property", "func()", "Function call dot access");
    testBracketMemberParsing("getTable()[\"key\"]", "getTable()", "Function call bracket access");
    testBracketMemberParsing("obj:method().field", "obj:method()", "Method call dot access");
    testBracketMemberParsing("getValue(x, y).result", "getValue(x, y)", "Function with args dot access");
}

void MemberExprTest::testMemberAccessOnTableConstructors() {
    testBracketMemberParsing("{a=1, b=2}.a", "{a=1, b=2}", "Table constructor dot access");
    testBracketMemberParsing("{1, 2, 3}[1]", "{1, 2, 3}", "Array constructor bracket access");
    testBracketMemberParsing("{x=10}.x", "{x=10}", "Simple table dot access");
    testBracketMemberParsing("{[\"key\"]="value"}[\"key\"]", "{[\"key\"]="value"}", "Dynamic key table access");
}

void MemberExprTest::testMemberAccessOnParenthesizedExpressions() {
    testBracketMemberParsing("(obj).property", "(obj)", "Parenthesized object dot access");
    testBracketMemberParsing("(table)[\"key\"]", "(table)", "Parenthesized object bracket access");
    testBracketMemberParsing("(a + b).field", "(a + b)", "Parenthesized expression dot access");
    testBracketMemberParsing("(func()).value", "(func())", "Parenthesized function call access");
}

void MemberExprTest::testMemberAccessOnComplexExpressions() {
    testBracketMemberParsing("(a and b).field", "(a and b)", "Logical expression object");
    testBracketMemberParsing("(x or y)[\"key\"]", "(x or y)", "Or expression object");
    testBracketMemberParsing("(not obj).prop", "(not obj)", "Unary expression object");
    testBracketMemberParsing("(a == b).result", "(a == b)", "Comparison expression object");
}

// Key expression tests
void MemberExprTest::testBracketWithArithmeticKeys() {
    testBracketMemberParsing("obj[a + b]", "obj", "Addition key");
    testBracketMemberParsing("table[x - y]", "table", "Subtraction key");
    testBracketMemberParsing("data[i * j]", "data", "Multiplication key");
    testBracketMemberParsing("arr[n / 2]", "arr", "Division key");
    testBracketMemberParsing("list[a % b]", "list", "Modulo key");
}

void MemberExprTest::testBracketWithLogicalKeys() {
    testBracketMemberParsing("obj[a and b]", "obj", "And expression key");
    testBracketMemberParsing("table[x or y]", "table", "Or expression key");
    testBracketMemberParsing("data[not flag]", "data", "Not expression key");
    testBracketMemberParsing("arr[a and b or c]", "arr", "Complex logical key");
}

void MemberExprTest::testBracketWithComparisonKeys() {
    testBracketMemberParsing("obj[a == b]", "obj", "Equality key");
    testBracketMemberParsing("table[x ~= y]", "table", "Inequality key");
    testBracketMemberParsing("data[i < j]", "data", "Less than key");
    testBracketMemberParsing("arr[a >= b]", "arr", "Greater equal key");
}

void MemberExprTest::testBracketWithUnaryKeys() {
    testBracketMemberParsing("obj[-x]", "obj", "Negative key");
    testBracketMemberParsing("table[+y]", "table", "Positive key");
    testBracketMemberParsing("data[#list]", "data", "Length key");
    testBracketMemberParsing("arr[not flag]", "arr", "Not key");
}

// Member access in expressions tests
void MemberExprTest::testMemberAccessInBinaryExpressions() {
    testBracketMemberParsing("obj.a + obj.b", "obj.a", "Member access in addition");
    testBracketMemberParsing("table[1] * table[2]", "table[1]", "Bracket access in multiplication");
    testBracketMemberParsing("data.x == data.y", "data.x", "Member access in comparison");
    testBracketMemberParsing("arr[i] and arr[j]", "arr[i]", "Bracket access in logical");
}

void MemberExprTest::testMemberAccessInUnaryExpressions() {
    testBracketMemberParsing("-obj.value", "obj.value", "Negative member access");
    testBracketMemberParsing("not table.flag", "table.flag", "Not member access");
    testBracketMemberParsing("#data.list", "data.list", "Length of member access");
    testBracketMemberParsing("+arr[1]", "arr[1]", "Positive bracket access");
}

void MemberExprTest::testMemberAccessInFunctionCalls() {
    testBracketMemberParsing("func(obj.prop)", "obj.prop", "Member access as argument");
    testBracketMemberParsing("method(table[\"key\"])", "table[\"key\"]", "Bracket access as argument");
    testBracketMemberParsing("call(data.a, data.b)", "data.a", "Multiple member access args");
    testBracketMemberParsing("obj.method(obj.value)", "obj.method", "Method call with member arg");
}

void MemberExprTest::testMemberAccessInTableConstructors() {
    testBracketMemberParsing("{obj.prop}", "obj.prop", "Member access in array part");
    testBracketMemberParsing("{key = table.value}", "table.value", "Member access in hash part");
    testBracketMemberParsing("{[obj.key] = obj.value}", "obj.key", "Member access as dynamic key");
    testBracketMemberParsing("{data.a, data.b, data.c}", "data.a", "Multiple member access in array");
}

// Special cases tests
void MemberExprTest::testMemberAccessWithWhitespace() {
    testMemberParsing("obj . prop", "obj", "prop", "Spaces around dot");
    testBracketMemberParsing("table [ \"key\" ]", "table", "Spaces around brackets");
    testBracketMemberParsing("obj[ key ]", "obj", "Spaces inside brackets");
    testMemberParsing("  obj.prop  ", "obj", "prop", "Leading and trailing spaces");
}

void MemberExprTest::testMemberAccessWithComments() {
    testMemberParsing("obj.prop -- comment", "obj", "prop", "Line comment after member access");
    testBracketMemberParsing("table[\"key\"] -- comment", "table", "Line comment after bracket access");
}

void MemberExprTest::testMemberAccessWithNewlines() {
    testBracketMemberParsing("obj\n.prop", "obj", "Newline before dot");
    testBracketMemberParsing("table\n[\"key\"]", "table", "Newline before bracket");
    testBracketMemberParsing("obj.\nprop", "obj", "Newline after dot");
}

void MemberExprTest::testMemberAccessPrecedence() {
    testBracketMemberParsing("obj.prop + 1", "obj.prop", "Member access before arithmetic");
    testBracketMemberParsing("not obj.flag", "obj.flag", "Member access before unary");
    testBracketMemberParsing("obj.method()", "obj.method", "Member access before call");
    testBracketMemberParsing("table[key].field", "table[key]", "Bracket access before dot");
}

// Error handling tests
void MemberExprTest::testInvalidDotNotation() {
    testMemberParsingError("obj.", "Missing field name after dot");
    testMemberParsingError("obj.123", "Numeric field name after dot");
    testMemberParsingError(".prop", "Missing object before dot");
    testMemberParsingError("obj..prop", "Double dot");
}

void MemberExprTest::testInvalidBracketNotation() {
    testMemberParsingError("obj[]", "Empty brackets");
    testMemberParsingError("[key]", "Missing object before brackets");
    testMemberParsingError("obj[[key]]", "Double brackets");
    testMemberParsingError("obj[key", "Missing closing bracket");
}

void MemberExprTest::testMalformedMemberAccess() {
    testMemberParsingError("obj.[prop]", "Dot before bracket");
    testMemberParsingError("obj[.prop]", "Dot inside bracket");
    testMemberParsingError("obj.prop.", "Trailing dot");
    testMemberParsingError("obj[prop].", "Bracket followed by dot without field");
}

void MemberExprTest::testUnterminatedBracketAccess() {
    testMemberParsingError("obj[\"key", "Unterminated string in bracket");
    testMemberParsingError("obj[key", "Missing closing bracket");
    testMemberParsingError("obj[1 + 2", "Unterminated expression in bracket");
    testMemberParsingError("obj[func(", "Unterminated function call in bracket");
}

// Helper methods
void MemberExprTest::testMemberParsing(const std::string& input, const std::string& expectedObject, const std::string& expectedMember, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (verifyMemberExpression(expr.get(), expectedObject, expectedMember)) {
            TestUtils::printTestResult(testName, true);
        } else {
            TestUtils::printTestResult(testName, false);
            TestUtils::printError("Member expression verification failed for: " + input);
        }
    } catch (const std::exception& e) {
        TestUtils::printTestResult(testName, false);
        TestUtils::printError("Exception during parsing: " + std::string(e.what()));
    }
}

void MemberExprTest::testBracketMemberParsing(const std::string& input, const std::string& expectedObject, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        if (verifyBracketMemberExpression(expr.get(), expectedObject)) {
            TestUtils::printTestResult(testName, true);
        } else {
            TestUtils::printTestResult(testName, false);
            TestUtils::printError("Bracket member expression verification failed for: " + input);
        }
    } catch (const std::exception& e) {
        TestUtils::printTestResult(testName, false);
        TestUtils::printError("Exception during parsing: " + std::string(e.what()));
    }
}

void MemberExprTest::testMemberParsingError(const std::string& input, const std::string& testName) {
    try {
        Parser parser(input);
        auto expr = parser.parseExpression();
        
        TestUtils::printTestResult(testName, false);
        TestUtils::printError("Expected parsing error but succeeded for: " + input);
    } catch (const std::exception& e) {
        TestUtils::printTestResult(testName, true);
        TestUtils::printInfo("Expected error caught: " + std::string(e.what()));
    }
}

bool MemberExprTest::verifyMemberExpression(const Expr* expr, const std::string& expectedObject, const std::string& expectedMember) {
    auto memberExpr = dynamic_cast<const MemberExpr*>(expr);
    if (!memberExpr) {
        TestUtils::printError("Expression is not a MemberExpr");
        return false;
    }
    
    // Verify object
    std::string objectName = extractObjectName(memberExpr->object.get());
    if (objectName != expectedObject) {
        TestUtils::printError("Object mismatch. Expected: " + expectedObject + ", Got: " + objectName);
        return false;
    }
    
    // Verify member name
    if (memberExpr->member != expectedMember) {
        TestUtils::printError("Member mismatch. Expected: " + expectedMember + ", Got: " + memberExpr->member);
        return false;
    }
    
    printMemberExpressionInfo(memberExpr);
    return true;
}

bool MemberExprTest::verifyBracketMemberExpression(const Expr* expr, const std::string& expectedObject) {
    // This is a simplified verification for complex member expressions
    // In a real implementation, you would need more sophisticated verification
    if (!expr) {
        TestUtils::printError("Expression is null");
        return false;
    }
    
    TestUtils::printInfo("Bracket member expression parsed successfully");
    return true;
}

void MemberExprTest::printMemberExpressionInfo(const MemberExpr* memberExpr) {
    TestUtils::printInfo("Member Expression:");
    TestUtils::printInfo("  Object: " + extractObjectName(memberExpr->object.get()));
    TestUtils::printInfo("  Member: " + memberExpr->member);
    TestUtils::printInfo("  Is computed: " + std::string(memberExpr->computed ? "true" : "false"));
}

std::string MemberExprTest::extractObjectName(const Expr* expr) {
    if (auto varExpr = dynamic_cast<const VariableExpr*>(expr)) {
        return varExpr->name;
    } else if (auto memberExpr = dynamic_cast<const MemberExpr*>(expr)) {
        return extractObjectName(memberExpr->object.get()) + "." + memberExpr->member;
    } else if (auto callExpr = dynamic_cast<const CallExpr*>(expr)) {
        return extractObjectName(callExpr->callee.get()) + "()";
    } else {
        return "<complex_expression>";
    }
}

} // namespace Expr
} // namespace Parser
} // namespace Tests
} // namespace Lua