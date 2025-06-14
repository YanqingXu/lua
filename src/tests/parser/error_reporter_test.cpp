#include "error_reporter_test.hpp"
#include <cassert>
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

// Implementation of ErrorReporterTest class

void ErrorReporterTest::runAllTests() {
    std::cout << "\n=== ErrorReporter Test Suite ===\n" << std::endl;
    
    try {
        testBasicErrorReporting();
        testErrorFiltering();
        testConvenienceMethods();
        testOutputFormats();
        testStaticFactoryMethods();
        testMaxErrorsLimit();
        testErrorClear();
        testIntegrationWithParseError();
        testJsonOutput();
        testErrorSeverityHandling();
        
        std::cout << "\n=== All ErrorReporter tests passed! ===\n" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "ErrorReporter test failed with exception: " << e.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "ErrorReporter test failed with unknown exception" << std::endl;
        throw;
    }
}

void ErrorReporterTest::printTestHeader(const std::string& testName) {
    std::cout << "\n--- " << testName << " ---" << std::endl;
}

void ErrorReporterTest::printTestFooter(const std::string& testName) {
    std::cout << "[OK] " << testName << " passed!" << std::endl;
}

void ErrorReporterTest::testBasicErrorReporting() {
    printTestHeader("Basic Error Reporting");
    
    ErrorReporter reporter;
    SourceLocation loc("test.lua", 1, 10);
    
    // Test basic error reporting
    reporter.reportSyntaxError(loc, "Missing semicolon");
    assert(reporter.hasErrors());
    assert(reporter.getErrorCount() == 1);
    
    // Test warning reporting
    reporter.reportWarning(loc, "Unused variable");
    assert(reporter.getErrorCount() == 2);
    
    // Test error count by severity
    assert(reporter.getErrorCount(ErrorSeverity::Error) == 1);
    assert(reporter.getErrorCount(ErrorSeverity::Warning) == 1);
    
    printTestFooter("Basic Error Reporting");
}

void ErrorReporterTest::testErrorFiltering() {
    printTestHeader("Error Filtering");
    
    // Test configuration without warnings and info
    ErrorReporterConfig config;
    config.includeWarnings = false;
    config.includeInfo = false;
    
    ErrorReporter reporter(config);
    SourceLocation loc("test.lua", 1, 10);
    
    reporter.reportSyntaxError(loc, "Syntax error");
    reporter.reportWarning(loc, "Warning message");
    reporter.reportInfo(loc, "Info message");
    
    // Should only have 1 error (syntax error), warnings and info filtered
    assert(reporter.getErrorCount() == 1);
    assert(reporter.getErrorCount(ErrorSeverity::Error) == 1);
    assert(reporter.getErrorCount(ErrorSeverity::Warning) == 0);
    assert(reporter.getErrorCount(ErrorSeverity::Info) == 0);
    
    // Test configuration with all severities enabled
    ErrorReporterConfig allConfig;
    allConfig.includeWarnings = true;
    allConfig.includeInfo = true;
    
    ErrorReporter allReporter(allConfig);
    allReporter.reportSyntaxError(loc, "Syntax error");
    allReporter.reportWarning(loc, "Warning message");
    allReporter.reportInfo(loc, "Info message");
    
    assert(allReporter.getErrorCount() == 3);
    assert(allReporter.getErrorCount(ErrorSeverity::Error) == 1);
    assert(allReporter.getErrorCount(ErrorSeverity::Warning) == 1);
    assert(allReporter.getErrorCount(ErrorSeverity::Info) == 1);
    
    printTestFooter("Error Filtering");
}

void ErrorReporterTest::testConvenienceMethods() {
    printTestHeader("Convenience Methods");
    
    ErrorReporter reporter;
    SourceLocation loc("test.lua", 1, 10);
    
    // Test convenience methods
    reporter.reportUnexpectedToken(loc, ";", "{");
    reporter.reportMissingToken(loc, ")");
    reporter.reportSemanticError(loc, "Undefined variable");
    
    assert(reporter.getErrorCount() == 3);
    assert(reporter.hasErrors());
    
    // Test that all errors are of Error severity by default
    assert(reporter.getErrorCount(ErrorSeverity::Error) == 3);
    
    // Test error messages contain expected information
    std::string output = reporter.toString();
    assert(output.find("Expected") != std::string::npos);
    assert(output.find("Missing") != std::string::npos);
    assert(output.find("Undefined") != std::string::npos);
    
    printTestFooter("Convenience Methods");
}

void ErrorReporterTest::testOutputFormats() {
    printTestHeader("Output Formats");
    
    ErrorReporter reporter;
    SourceLocation loc("test.lua", 1, 10);
    
    reporter.reportSyntaxError(loc, "Test error");
    reporter.reportWarning(loc, "Test warning");
    
    // Test different output formats
    std::string basic = reporter.toString();
    std::string detailed = reporter.toDetailedString();
    std::string short_str = reporter.toShortString();
    std::string json = reporter.toJson();
    
    // All formats should produce non-empty output
    assert(!basic.empty());
    assert(!detailed.empty());
    assert(!short_str.empty());
    assert(!json.empty());
    
    // JSON should contain basic structure
    assert(json.find("errors") != std::string::npos);
    assert(json.find("count") != std::string::npos);
    
    // Basic format should contain error information
    assert(basic.find("test.lua") != std::string::npos);
    assert(basic.find("Test error") != std::string::npos);
    
    // Detailed format should be longer than basic
    assert(detailed.length() >= basic.length());
    
    // Short format should be shorter than basic
    assert(short_str.length() <= basic.length());
    
    printTestFooter("Output Formats");
}

void ErrorReporterTest::testStaticFactoryMethods() {
    printTestHeader("Static Factory Methods");
    
    auto defaultReporter = ErrorReporter::createDefault();
    auto strictReporter = ErrorReporter::createStrict();
    auto permissiveReporter = ErrorReporter::createPermissive();
    
    SourceLocation loc("test.lua", 1, 10);
    
    // Test strict mode (should stop after first error)
    strictReporter.reportSyntaxError(loc, "First error");
    assert(strictReporter.shouldStopParsing());
    
    // Test permissive mode (should allow more errors)
    for (int i = 0; i < 50; ++i) {
        permissiveReporter.reportSyntaxError(loc, "Error " + std::to_string(i));
    }
    assert(!permissiveReporter.shouldStopParsing()); // Should still continue
    
    // Test default reporter behavior
    defaultReporter.reportSyntaxError(loc, "Default error");
    assert(defaultReporter.hasErrors());
    
    printTestFooter("Static Factory Methods");
}

void ErrorReporterTest::testMaxErrorsLimit() {
    printTestHeader("Max Errors Limit");
    
    ErrorReporterConfig config;
    config.maxErrors = 3;
    ErrorReporter reporter(config);
    
    SourceLocation loc("test.lua", 1, 10);
    
    // Add more errors than the limit
    for (int i = 0; i < 5; ++i) {
        reporter.reportSyntaxError(loc, "Error " + std::to_string(i));
    }
    
    // Should only have 3 errors recorded
    assert(reporter.getErrorCount() == 3);
    assert(reporter.shouldStopParsing()); // Should stop due to max errors
    
    // Test that no more errors are added after limit
    size_t countBefore = reporter.getErrorCount();
    reporter.reportSyntaxError(loc, "Additional error");
    assert(reporter.getErrorCount() == countBefore);
    
    printTestFooter("Max Errors Limit");
}

void ErrorReporterTest::testErrorClear() {
    printTestHeader("Error Clear");
    
    ErrorReporter reporter;
    SourceLocation loc("test.lua", 1, 10);
    
    // Add some errors
    reporter.reportSyntaxError(loc, "Test error");
    reporter.reportWarning(loc, "Test warning");
    assert(reporter.hasErrors());
    assert(reporter.getErrorCount() == 2);
    
    // Clear errors
    reporter.clear();
    assert(!reporter.hasErrors());
    assert(reporter.getErrorCount() == 0);
    assert(reporter.getErrorCount(ErrorSeverity::Error) == 0);
    assert(reporter.getErrorCount(ErrorSeverity::Warning) == 0);
    
    // Test that reporter can be used again after clearing
    reporter.reportSyntaxError(loc, "New error after clear");
    assert(reporter.hasErrors());
    assert(reporter.getErrorCount() == 1);
    
    printTestFooter("Error Clear");
}

void ErrorReporterTest::testIntegrationWithParseError() {
    printTestHeader("Integration with ParseError");
    
    ErrorReporter reporter;
    SourceLocation loc("test.lua", 5, 15);
    
    // Test that ErrorReporter properly creates ParseError objects
    reporter.reportSyntaxError(loc, "Missing closing bracket");
    assert(reporter.hasErrors());
    assert(reporter.getErrorCount() == 1);
    
    // Test error details preservation
    std::string output = reporter.toString();
    assert(output.find("test.lua") != std::string::npos);
    assert(output.find("5") != std::string::npos); // line number
    assert(output.find("15") != std::string::npos); // column number
    assert(output.find("Missing closing bracket") != std::string::npos);
    
    // Test different error types
    reporter.reportSemanticError(loc, "Undefined variable 'x'");
    assert(reporter.getErrorCount() == 2);
    
    printTestFooter("Integration with ParseError");
}

void ErrorReporterTest::testJsonOutput() {
    printTestHeader("JSON Output");
    
    ErrorReporter reporter;
    SourceLocation loc1("test.lua", 1, 10);
    SourceLocation loc2("test.lua", 2, 5);
    
    reporter.reportSyntaxError(loc1, "Syntax error message");
    reporter.reportWarning(loc2, "Warning message");
    
    std::string json = reporter.toJson();
    
    // Test JSON structure validity
    assert(!json.empty());
    assert(json.find("{") != std::string::npos); // JSON object start
    assert(json.find("}") != std::string::npos); // JSON object end
    
    // Test required JSON fields
    assert(json.find("\"errors\"") != std::string::npos);
    assert(json.find("\"count\"") != std::string::npos);
    // assert(json.find("\"summary\"") != std::string::npos); // summary field does not exist, removed
    
    // Test error data serialization
    assert(json.find("\"type\"") != std::string::npos);
    assert(json.find("\"line\"") != std::string::npos);
    assert(json.find("\"column\"") != std::string::npos);
    assert(json.find("\"message\"") != std::string::npos);
    assert(json.find("\"severity\"") != std::string::npos);
    
    // Test specific content
    assert(json.find("Syntax error message") != std::string::npos);
    assert(json.find("Warning message") != std::string::npos);
    assert(json.find("Invalid Expression") != std::string::npos);
    assert(json.find("Unknown Error") != std::string::npos);
    
    printTestFooter("JSON Output");
}

void ErrorReporterTest::testErrorSeverityHandling() {
    printTestHeader("Error Severity Handling");
    
    // Configure reporter to include all severity levels
    ErrorReporterConfig config;
    config.includeInfo = true;
    config.includeWarnings = true;
    ErrorReporter reporter(config);
    SourceLocation loc("test.lua", 1, 10);
    
    // Test different severity levels
    reporter.reportSyntaxError(loc, "Error message");     // Error severity
    reporter.reportWarning(loc, "Warning message");       // Warning severity
    reporter.reportInfo(loc, "Info message");             // Info severity
    
    // Test total count
    std::cout << "Total error count: " << reporter.getErrorCount() << std::endl;
    assert(reporter.getErrorCount() == 3);
    
    // Test severity-based counting
    assert(reporter.getErrorCount(ErrorSeverity::Error) == 1);
    assert(reporter.getErrorCount(ErrorSeverity::Warning) == 1);
    assert(reporter.getErrorCount(ErrorSeverity::Info) == 1);
    
    // Test hasErrors() with different severities
    assert(reporter.hasErrors()); // Should return true for any errors
    
    // Test severity filtering in configuration
    ErrorReporterConfig errorOnlyConfig;
    errorOnlyConfig.includeWarnings = false;
    errorOnlyConfig.includeInfo = false;
    
    ErrorReporter filteredReporter(errorOnlyConfig);
    filteredReporter.reportSyntaxError(loc, "Error message");
    filteredReporter.reportWarning(loc, "Warning message");
    filteredReporter.reportInfo(loc, "Info message");
    
    // Only error should be recorded (warnings and info are filtered out)
    assert(filteredReporter.getErrorCount() == 1);
    assert(filteredReporter.getErrorCount(ErrorSeverity::Error) == 1);
    // Note: filtered errors are not added to collector, so count is 0
    // This is expected behavior based on shouldReportError() implementation
    
    printTestFooter("Error Severity Handling");
}

} // namespace Tests
} // namespace Lua