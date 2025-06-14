#include "parse_error_test.hpp"
#include "../../parser/ast/parse_error.hpp"
#include <cassert>
#include <iostream>

namespace Lua::Tests {

void ParseErrorTest::runAllTests() {
    std::cout << "Running ParseError tests..." << std::endl;
    testBasicErrorCreation();
    testErrorWithSuggestions();
    testStaticFactoryMethods();
    testErrorFormatting();
    testErrorCollector();
    testErrorSeverity();
    testRelatedErrors();
    std::cout << "All ParseError tests passed!" << std::endl;
}

void ParseErrorTest::testBasicErrorCreation() {
    std::cout << "Testing basic error creation: ";
    
    SourceLocation loc("test.lua", 10, 5);
    ParseError error(ErrorType::UnexpectedToken, loc, "Unexpected token 'end'");
    
    assert(error.getType() == ErrorType::UnexpectedToken);
    assert(error.getLocation().getFilename() == "test.lua");
    assert(error.getLocation().getLine() == 10);
    assert(error.getLocation().getColumn() == 5);
    assert(error.getMessage() == "Unexpected token 'end'");
    assert(error.getSeverity() == ErrorSeverity::Error);
    
    std::cout << "PASS" << std::endl;
}

void ParseErrorTest::testErrorWithSuggestions() {
    std::cout << "Testing error with suggestions: ";
    
    SourceLocation loc("test.lua", 15, 8);
    ParseError error(ErrorType::MissingToken, loc, "Missing ';'");
    
    error.addSuggestion(FixType::Insert, loc, "Insert semicolon", ";");
    error.addSuggestion(FixType::Replace, loc, "Replace with 'end'", "end");
    
    const auto& suggestions = error.getSuggestions();
    assert(suggestions.size() == 2);
    assert(suggestions[0].type == FixType::Insert);
    assert(suggestions[0].description == "Insert semicolon");
    assert(suggestions[0].newText == ";");
    assert(suggestions[1].type == FixType::Replace);
    
    std::cout << "PASS" << std::endl;
}

void ParseErrorTest::testStaticFactoryMethods() {
    std::cout << "Testing static factory methods: ";
    
    SourceLocation loc("test.lua", 20, 10);
    
    // Test unexpectedToken
    auto error1 = ParseError::unexpectedToken(loc, "end", "if");
    assert(error1.getType() == ErrorType::UnexpectedToken);
    assert(error1.getMessage() == "Expected 'end', but found 'if'");
    assert(error1.getSuggestions().size() == 1);
    assert(error1.getSuggestions()[0].type == FixType::Replace);
    
    // Test missingToken
    auto error2 = ParseError::missingToken(loc, ")");
    assert(error2.getType() == ErrorType::MissingToken);
    assert(error2.getMessage() == "Missing ')'");
    assert(error2.getSuggestions().size() == 1);
    assert(error2.getSuggestions()[0].type == FixType::Insert);
    
    // Test undefinedVariable
    auto error3 = ParseError::undefinedVariable(loc, "myVar");
    assert(error3.getType() == ErrorType::UndefinedVariable);
    assert(error3.getMessage() == "Undefined variable 'myVar'");
    assert(error3.getSuggestions().size() == 1);
    
    // Test invalidExpression
    auto error4 = ParseError::invalidExpression(loc, "malformed syntax");
    assert(error4.getType() == ErrorType::InvalidExpression);
    assert(error4.getMessage() == "Invalid expression: malformed syntax");
    
    std::cout << "PASS" << std::endl;
}

void ParseErrorTest::testErrorFormatting() {
    std::cout << "Testing error formatting: ";
    
    SourceLocation loc("test.lua", 25, 12);
    ParseError error(ErrorType::UnexpectedToken, loc, "Unexpected 'then'");
    error.setDetails("Expected 'do' after 'while' condition");
    error.addSuggestion(FixType::Replace, loc, "Replace with 'do'", "do");
    
    Str basicStr = error.toString();
    assert(!basicStr.empty());
    assert(basicStr.find("test.lua") != Str::npos);
    assert(basicStr.find("25") != Str::npos);
    assert(basicStr.find("12") != Str::npos);
    
    Str detailedStr = error.toDetailedString();
    assert(!detailedStr.empty());
    assert(detailedStr.find("Expected 'do' after 'while' condition") != Str::npos);
    assert(detailedStr.find("Replace with 'do'") != Str::npos);
    
    Str shortStr = error.toShortString();
    assert(!shortStr.empty());
    assert(shortStr.length() <= basicStr.length());
    
    std::cout << "PASS" << std::endl;
}

void ParseErrorTest::testErrorCollector() {
    std::cout << "Testing error collector: ";
    
    ErrorCollector collector(3);
    assert(collector.getErrorCount() == 0);
    assert(!collector.hasErrors());
    
    SourceLocation loc1("test.lua", 10, 5);
    SourceLocation loc2("test.lua", 15, 8);
    SourceLocation loc3("test.lua", 20, 12);
    
    collector.addError(ParseError(ErrorType::UnexpectedToken, loc1, "Error 1"));
    assert(collector.getErrorCount() == 1);
    assert(collector.hasErrors());
    
    collector.addError(ParseError(ErrorType::MissingToken, loc2, "Error 2"));
    collector.addError(ParseError(ErrorType::InvalidExpression, loc3, "Error 3"));
    assert(collector.getErrorCount() == 3);
    assert(collector.hasMaxErrors());
    
    const auto& errors = collector.getErrors();
    assert(errors.size() == 3);
    assert(errors[0].getMessage() == "Error 1");
    assert(errors[1].getMessage() == "Error 2");
    assert(errors[2].getMessage() == "Error 3");
    
    Str collectorStr = collector.toString();
    assert(!collectorStr.empty());
    assert(collectorStr.find("Error 1") != Str::npos);
    
    collector.clear();
    assert(collector.getErrorCount() == 0);
    assert(!collector.hasErrors());
    
    std::cout << "PASS" << std::endl;
}

void ParseErrorTest::testErrorSeverity() {
    std::cout << "Testing error severity: ";
    
    SourceLocation loc("test.lua", 30, 15);
    
    ParseError warning(ErrorType::UnexpectedToken, loc, "Warning message", ErrorSeverity::Warning);
    assert(warning.getSeverity() == ErrorSeverity::Warning);
    
    ParseError error(ErrorType::MissingToken, loc, "Error message", ErrorSeverity::Error);
    assert(error.getSeverity() == ErrorSeverity::Error);
    
    ParseError fatal(ErrorType::InvalidExpression, loc, "Fatal message", ErrorSeverity::Fatal);
    assert(fatal.getSeverity() == ErrorSeverity::Fatal);
    
    ErrorCollector collector;
    collector.addError(std::move(warning));
    collector.addError(std::move(error));
    collector.addError(std::move(fatal));
    
    auto warnings = collector.getErrorsBySeverity(ErrorSeverity::Warning);
    auto errors = collector.getErrorsBySeverity(ErrorSeverity::Error);
    auto fatals = collector.getErrorsBySeverity(ErrorSeverity::Fatal);
    
    assert(warnings.size() == 1);
    assert(errors.size() == 1);
    assert(fatals.size() == 1);
    
    assert(warnings[0]->getMessage() == "Warning message");
    assert(errors[0]->getMessage() == "Error message");
    assert(fatals[0]->getMessage() == "Fatal message");
    
    std::cout << "PASS" << std::endl;
}

void ParseErrorTest::testRelatedErrors() {
    std::cout << "Testing related errors: ";
            
    SourceLocation loc1("test.lua", 35, 10);
    SourceLocation loc2("test.lua", 40, 15);
    
    ParseError mainError(ErrorType::MismatchedParentheses, loc1, "Unmatched '('");
    auto relatedError = std::make_unique<ParseError>(ErrorType::MissingToken, loc2, "Missing ')'");
    
    mainError.setRelatedError(std::move(relatedError));
    
    assert(mainError.getRelatedError() != nullptr);
    assert(mainError.getRelatedError()->getType() == ErrorType::MissingToken);
    assert(mainError.getRelatedError()->getMessage() == "Missing ')'");
    
    // Test detailed string includes related error
    Str detailedStr = mainError.toDetailedString();
    assert(detailedStr.find("Related:") != Str::npos);
    assert(detailedStr.find("Missing ')'") != Str::npos);
    
    std::cout << "PASS" << std::endl;
}

} // namespace Lua::Tests
