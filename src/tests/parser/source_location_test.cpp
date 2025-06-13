#include "source_location_test.hpp"
#include "../../parser/ast/expressions.hpp"
#include "../../parser/ast/statements.hpp"

namespace Lua {
namespace Tests {

void SourceLocationTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running SourceLocation Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testBasicConstruction();
    testFromLineColumn();
    testFromToken();
    testFormatting();
    testComparison();
    testSourceRange();
    testSetters();
    testASTIntegration();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "SourceLocation Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void SourceLocationTest::testBasicConstruction() {
    printSectionHeader("Basic Construction");
    
    // Test default constructor
    SourceLocation defaultLoc;
    bool test1 = (defaultLoc.getLine() == 0 && defaultLoc.getColumn() == 0 && defaultLoc.getFilename() == "<unknown>");
    printTestResult("Default constructor", test1);
    
    // Test parameterized constructor
    SourceLocation loc("test.lua", 10, 5);
    bool test2 = (loc.getLine() == 10 && loc.getColumn() == 5 && loc.getFilename() == "test.lua");
    printTestResult("Parameterized constructor", test2);
    
    // Test copy constructor
    SourceLocation copyLoc(loc);
    bool test3 = (copyLoc.getLine() == 10 && copyLoc.getColumn() == 5 && copyLoc.getFilename() == "test.lua");
    printTestResult("Copy constructor", test3);
}

void SourceLocationTest::testFromLineColumn() {
    printSectionHeader("From Line Column");
    
    SourceLocation loc = SourceLocation::fromLineColumn(15, 8);
    bool test1 = (loc.getLine() == 15 && loc.getColumn() == 8);
    printTestResult("fromLineColumn basic", test1);
    
    SourceLocation loc2 = SourceLocation::fromLineColumn(1, 1);
    bool test2 = (loc2.getLine() == 1 && loc2.getColumn() == 1);
    printTestResult("fromLineColumn edge case", test2);
}

void SourceLocationTest::testFromToken() {
    printSectionHeader("From Token");
    
    // Create a mock token
    Token token;
    token.type = TokenType::Name;
    token.lexeme = "test";
    token.line = 20;
    token.column = 12;
    
    SourceLocation loc = SourceLocation::fromToken(token);
    bool test1 = (loc.getLine() == 20 && loc.getColumn() == 12);
    printTestResult("fromToken basic", test1);
    
    // Test with different token
    Token token2;
    token2.type = TokenType::Number;
    token2.lexeme = "42";
    token2.line = 1;
    token2.column = 1;
    
    SourceLocation loc2 = SourceLocation::fromToken(token2);
    bool test2 = (loc2.getLine() == 1 && loc2.getColumn() == 1);
    printTestResult("fromToken different token", test2);
}

void SourceLocationTest::testFormatting() {
    printSectionHeader("Formatting");
    
    SourceLocation loc("script.lua", 25, 10);
    std::string formatted = loc.toString();
    bool test1 = (formatted == "script.lua:25:10");
    printTestResult("toString with filename", test1);
    
    SourceLocation loc2("", 5, 3);
    std::string formatted2 = loc2.toString();
    bool test2 = (formatted2 == ":5:3");
    printTestResult("toString without filename", test2);
    
    SourceLocation defaultLoc;
    std::string formatted3 = defaultLoc.toString();
    bool test3 = (formatted3 == "<unknown>:?:?");
    printTestResult("toString default location", test3);
}

void SourceLocationTest::testComparison() {
    printSectionHeader("Comparison Operations");
    
    SourceLocation loc1("test.lua", 10, 5);
    SourceLocation loc2("test.lua", 10, 5);
    SourceLocation loc3("test.lua", 10, 6);
    SourceLocation loc4("test.lua", 11, 5);
    SourceLocation loc5("other.lua", 10, 5);
    
    // Test equality
    bool test1 = (loc1 == loc2);
    printTestResult("Equality (same location)", test1);
    
    bool test2 = !(loc1 == loc3);
    printTestResult("Equality (different column)", test2);
    
    bool test3 = !(loc1 == loc4);
    printTestResult("Equality (different line)", test3);
    
    bool test4 = !(loc1 == loc5);
    printTestResult("Equality (different file)", test4);
    
    // Test inequality
    bool test5 = (loc1 != loc3);
    printTestResult("Inequality", test5);
    
    // Test less than
    bool test6 = (loc1 < loc3);  // same line, earlier column
    printTestResult("Less than (same line)", test6);
    
    bool test7 = (loc1 < loc4);  // earlier line
    printTestResult("Less than (different line)", test7);
    
    // Test greater than
    bool test8 = (loc3 > loc1);
    printTestResult("Greater than", test8);
    
    // Test less than or equal
    bool test9 = (loc1 <= loc2);
    printTestResult("Less than or equal (equal)", test9);
    
    bool test10 = (loc1 <= loc3);
    printTestResult("Less than or equal (less)", test10);
    
    // Test greater than or equal
    bool test11 = (loc1 >= loc2);
    printTestResult("Greater than or equal (equal)", test11);
    
    bool test12 = (loc3 >= loc1);
    printTestResult("Greater than or equal (greater)", test12);
}

void SourceLocationTest::testSourceRange() {
    printSectionHeader("Source Range");
    
    SourceLocation start("test.lua", 10, 5);
    SourceLocation end("test.lua", 12, 8);
    SourceRange range(start, end);
    
    bool test1 = (range.getStart() == start && range.getEnd() == end);
    printTestResult("Range construction", test1);
    
    std::string rangeStr = range.toString();
    bool test2 = (rangeStr == "test.lua:10:5-12:8");
    printTestResult("Range toString", test2);
    
    // Test contains
    SourceLocation middle("test.lua", 11, 3);
    bool test3 = range.contains(middle);
    printTestResult("Range contains (inside)", test3);
    
    SourceLocation outside("test.lua", 9, 1);
    bool test4 = !range.contains(outside);
    printTestResult("Range contains (outside)", test4);
    
    SourceLocation boundary("test.lua", 10, 5);
    bool test5 = range.contains(boundary);
    printTestResult("Range contains (boundary start)", test5);
    
    SourceLocation endBoundary("test.lua", 12, 8);
    bool test6 = range.contains(endBoundary);
    printTestResult("Range contains (boundary end)", test6);
}

void SourceLocationTest::testSetters() {
    printSectionHeader("Setter Methods");
    
    SourceLocation loc;
    
    loc.setFilename("new_file.lua");
    bool test1 = (loc.getFilename() == "new_file.lua");
    printTestResult("setFilename", test1);
    
    loc.setLine(42);
    bool test2 = (loc.getLine() == 42);
    printTestResult("setLine", test2);
    
    loc.setColumn(15);
    bool test3 = (loc.getColumn() == 15);
    printTestResult("setColumn", test3);
    
    // Test chaining effect
    std::string result = loc.toString();
    bool test4 = (result == "new_file.lua:42:15");
    printTestResult("Combined setters", test4);
}

void SourceLocationTest::testASTIntegration() {
    printSectionHeader("AST Integration");
    
    SourceLocation loc("test.lua", 5, 10);
    
    // Test with LiteralExpr
    auto literalExpr = std::make_unique<LiteralExpr>(Value(42), loc);
    bool test1 = (literalExpr->getLocation() == loc);
    printTestResult("LiteralExpr with location", test1);
    
    // Test with VariableExpr
    auto varExpr = std::make_unique<VariableExpr>("x", loc);
    bool test2 = (varExpr->getLocation() == loc);
    printTestResult("VariableExpr with location", test2);
    
    // Test with ExprStmt
    auto exprStmt = std::make_unique<ExprStmt>(std::make_unique<LiteralExpr>(Value(42)), loc);
    bool test3 = (exprStmt->getLocation() == loc);
    printTestResult("ExprStmt with location", test3);
    
    // Test with LocalStmt
    auto localStmt = std::make_shared<LocalStmt>("y", std::make_unique<LiteralExpr>(Value(10)), loc);
    bool test4 = (localStmt->getLocation() == loc);
    printTestResult("LocalStmt with location", test4);
    
    // Test default construction (should have default location)
    auto defaultExpr = std::make_unique<LiteralExpr>(Value(0));
    SourceLocation defaultLoc;
    bool test5 = (defaultExpr->getLocation() == defaultLoc);
    printTestResult("Default AST node location", test5);
}

void SourceLocationTest::printTestResult(const std::string& testName, bool passed) {
    std::cout << "  " << testName << ": " << (passed ? "PASS" : "FAIL") << std::endl;
}

void SourceLocationTest::printSectionHeader(const std::string& sectionName) {
    std::cout << "\n--- " << sectionName << " ---" << std::endl;
}

} // namespace Tests
} // namespace Lua