#include "enhanced_parser_error_test.hpp"
#include "../../parser/ast/parse_error.hpp"
#include <iostream>
#include <cassert>

// Simplified test without full parser dependencies
namespace Lua {
    // Mock Parser class for testing
    class MockParser {
    private:
        ErrorReporter errorReporter_;
        bool hadError = false;
        
    public:
        MockParser() : errorReporter_(ErrorReporter::createDefault()) {}
        
        void error(const Str& message) {
            hadError = true;
            SourceLocation location("test", 1, 1);
            errorReporter_.reportError(ErrorType::Unknown, location, message);
        }
        
        void error(ErrorType type, const Str& message) {
            hadError = true;
            SourceLocation location("test", 1, 1);
            errorReporter_.reportError(type, location, message);
        }
        
        void error(ErrorType type, const Str& message, const Str& details) {
            hadError = true;
            SourceLocation location("test", 1, 1);
            errorReporter_.reportError(type, location, message, details);
        }
        
        void error(ErrorType type, const SourceLocation& location, const Str& message) {
            hadError = true;
            errorReporter_.reportError(type, location, message);
        }
        
        void error(ErrorType type, const SourceLocation& location, const Str& message, const Str& details) {
            hadError = true;
            errorReporter_.reportError(type, location, message, details);
        }
        
        bool hasError() const { return hadError; }
        const ErrorReporter& getErrorReporter() const { return errorReporter_; }
        const Vec<ParseError>& getErrors() const { return errorReporter_.getErrors(); }
        size_t getErrorCount() const { return errorReporter_.getErrorCount(); }
        bool hasErrorsOrWarnings() const { return errorReporter_.hasErrorsOrWarnings(); }
        void clearErrors() { errorReporter_.clear(); hadError = false; }
    };
}

namespace Lua {
    void EnhancedParserErrorTest::runAllTests() {
        std::cout << "\n=== Enhanced Parser Error Test Suite ===\n" << std::endl;
        
        try {
            testBasicErrorReporting();
            testDetailedErrorMessages();
            testErrorTypeClassification();
            testErrorLocationTracking();
            testMultipleErrorHandling();
            testErrorRecovery();
            testConsumeMethodErrors();
            testLexicalErrorHandling();
            
            std::cout << "\n=== All Enhanced Parser Error tests passed! ===\n" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Enhanced Parser Error test failed with exception: " << e.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "Enhanced Parser Error test failed with unknown exception" << std::endl;
            throw;
        }
    }
    
    void EnhancedParserErrorTest::printTestHeader(const std::string& testName) {
        std::cout << "Testing " << testName << "..." << std::endl;
    }
    
    void EnhancedParserErrorTest::printTestFooter(const std::string& testName) {
        std::cout << "[OK] " << testName << " passed\n" << std::endl;
    }
    
    void EnhancedParserErrorTest::testBasicErrorReporting() {
        printTestHeader("Basic Error Reporting");
        
        // Test simple error message
        MockParser parser;
        parser.error("Test error message");
        
        assert(parser.hasError());
        assert(parser.getErrorCount() > 0);
        
        const auto& errors = parser.getErrors();
        assert(!errors.empty());
        assert(errors[0].getMessage() == "Test error message");
        assert(errors[0].getType() == ErrorType::Unknown);
        
        printTestFooter("Basic Error Reporting");
    }
    
    void EnhancedParserErrorTest::testDetailedErrorMessages() {
        printTestHeader("Detailed Error Messages");
        
        MockParser parser;
        
        // Test error with type and details
        parser.error(ErrorType::MissingToken, "Missing semicolon", "Expected ';' at end of statement");
        
        const auto& errors = parser.getErrors();
        assert(!errors.empty());
        assert(errors[0].getType() == ErrorType::MissingToken);
        assert(errors[0].getMessage() == "Missing semicolon");
        assert(errors[0].getDetails() == "Expected ';' at end of statement");
        
        printTestFooter("Detailed Error Messages");
    }
    
    void EnhancedParserErrorTest::testErrorTypeClassification() {
        printTestHeader("Error Type Classification");
        
        MockParser parser;
        
        // Test different error types
        parser.error(ErrorType::UnexpectedToken, "Unexpected token");
        parser.error(ErrorType::MissingToken, "Missing semicolon");
        parser.error(ErrorType::InvalidExpression, "Invalid expression");
        parser.error(ErrorType::UndefinedVariable, "Undefined variable");
        
        assert(parser.hasError());
        const auto& errors = parser.getErrors();
        assert(errors.size() == 4);
        
        assert(errors[0].getType() == ErrorType::UnexpectedToken);
        assert(errors[1].getType() == ErrorType::MissingToken);
        assert(errors[2].getType() == ErrorType::InvalidExpression);
        assert(errors[3].getType() == ErrorType::UndefinedVariable);
        
        printTestFooter("Error Type Classification");
    }
    
    void EnhancedParserErrorTest::testErrorLocationTracking() {
        printTestHeader("Error Location Tracking");
        
        MockParser parser;
        
        // Test with specific location
        SourceLocation location("test.lua", 2, 5);
        parser.error(ErrorType::UnexpectedToken, location, "Location test");
        
        const auto& errors = parser.getErrors();
        assert(!errors.empty());
        
        const auto& errorLocation = errors[0].getLocation();
        assert(errorLocation.isValid());
        assert(errorLocation.getLine() == 2);
        assert(errorLocation.getColumn() == 5);
        
        printTestFooter("Error Location Tracking");
    }
    
    void EnhancedParserErrorTest::testMultipleErrorHandling() {
        printTestHeader("Multiple Error Handling");
        
        MockParser parser;
        
        // Add multiple errors
        for (int i = 0; i < 5; ++i) {
            parser.error(ErrorType::UnexpectedToken, "Error " + std::to_string(i + 1));
        }
        
        assert(parser.hasError());
        assert(parser.getErrorCount() == 5);
        
        const auto& errors = parser.getErrors();
        assert(errors.size() == 5);
        
        for (size_t i = 0; i < errors.size(); ++i) {
            assert(errors[i].getMessage() == "Error " + std::to_string(i + 1));
        }
        
        printTestFooter("Multiple Error Handling");
    }
    
    void EnhancedParserErrorTest::testErrorRecovery() {
        printTestHeader("Error Recovery");
        
        MockParser parser;
        
        // Test error clearing
        parser.error(ErrorType::UnexpectedToken, "Test error");
        assert(parser.hasError());
        assert(parser.getErrorCount() > 0);
        
        parser.clearErrors();
        assert(!parser.hasError());
        assert(parser.getErrorCount() == 0);
        
        printTestFooter("Error Recovery");
    }
    
    void EnhancedParserErrorTest::testConsumeMethodErrors() {
        printTestHeader("Consume Method Errors");
        
        // Test missing token error simulation
        MockParser parser;
        
        // Simulate consume method error
        SourceLocation location("test.lua", 1, 10);
        parser.error(ErrorType::MissingToken, location, "Expected semicolon", "Found 'identifier' instead");
        
        assert(parser.hasError());
        const auto& errors = parser.getErrors();
        assert(!errors.empty());
        assert(errors[0].getType() == ErrorType::MissingToken);
        assert(errors[0].getMessage() == "Expected semicolon");
        assert(errors[0].getDetails() == "Found 'identifier' instead");
        
        printTestFooter("Consume Method Errors");
    }
    
    void EnhancedParserErrorTest::testLexicalErrorHandling() {
        printTestHeader("Lexical Error Handling");
        
        // Test lexical error simulation
        MockParser parser;
        
        // Simulate lexical error
        SourceLocation location("test.lua", 1, 7);
        parser.error(ErrorType::UnexpectedCharacter, location, "Unexpected character '@'", "Invalid character in identifier");
        
        assert(parser.hasError());
        const auto& errors = parser.getErrors();
        assert(!errors.empty());
        assert(errors[0].getType() == ErrorType::UnexpectedCharacter);
        assert(errors[0].getMessage() == "Unexpected character '@'");
        assert(errors[0].getDetails() == "Invalid character in identifier");
        
        printTestFooter("Lexical Error Handling");
    }
}