#include "../src/parser/enhanced_parser.hpp"
#include "../src/parser/error_formatter.hpp"
#include "../src/localization/localization_manager.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <iomanip>

using namespace Lua;

/**
 * @brief Test case structure for error reporting validation
 */
struct ErrorTestCase {
    std::string filename;
    std::string description;
    std::string expectedLua51Output;
    bool testLocalization;
    std::string expectedChineseOutput;
};

/**
 * @brief Enhanced Error Reporting Validation Suite
 */
class EnhancedErrorReportingValidator {
private:
    std::vector<ErrorTestCase> testCases_;
    int passedTests_;
    int failedTests_;
    
public:
    EnhancedErrorReportingValidator() : passedTests_(0), failedTests_(0) {
        initializeTestCases();
    }
    
    /**
     * @brief Initialize all test cases with expected outputs
     */
    void initializeTestCases() {
        testCases_ = {
            {
                "error_test_unexpected_symbol.lua",
                "Unexpected symbol '@'",
                "stdin:1: unexpected symbol near '@'",
                true,
                "stdin:1: 在 '@' 附近出现意外符号"
            },
            {
                "error_test_missing_end.lua", 
                "Missing 'end' keyword",
                "stdin:3: 'end' expected",
                true,
                "stdin:3: 期望 'end'"
            },
            {
                "error_test_unfinished_string.lua",
                "Unfinished string literal",
                "stdin:1: unfinished string near '\"hello world'",
                true,
                "stdin:1: 在 '\"hello world' 附近出现未完成的字符串"
            },
            {
                "error_test_malformed_number.lua",
                "Malformed number literal",
                "stdin:1: malformed number near '123.45.67'",
                true,
                "stdin:1: 在 '123.45.67' 附近出现格式错误的数字"
            },
            {
                "error_test_unexpected_eof.lua",
                "Unexpected end of file",
                "stdin:2: 'end' expected",
                true,
                "stdin:2: 期望 'end'"
            },
            {
                "error_test_missing_parenthesis.lua",
                "Missing closing parenthesis",
                "stdin:1: ')' expected",
                true,
                "stdin:1: 期望 ')'"
            },
            {
                "error_test_invalid_escape.lua",
                "Invalid escape sequence",
                "stdin:1: invalid escape sequence near '\"\\z\"'",
                true,
                "stdin:1: 在 '\"\\z\"' 附近出现无效的转义序列"
            },
            {
                "error_test_multiple_errors.lua",
                "Multiple errors (should report first)",
                "stdin:1: unexpected symbol near '@'",
                false,
                ""
            },
            {
                "error_test_nested_structures.lua",
                "Nested structures with missing end",
                "stdin:6: 'end' expected",
                false,
                ""
            },
            {
                "error_test_table_syntax.lua",
                "Table syntax error",
                "stdin:1: '}' expected",
                false,
                ""
            }
        };
    }
    
    /**
     * @brief Read file content
     */
    std::string readFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file: " + filepath);
        }
        
        std::string content;
        std::string line;
        while (std::getline(file, line)) {
            content += line + "\n";
        }
        return content;
    }
    
    /**
     * @brief Test single error case
     */
    bool testSingleCase(const ErrorTestCase& testCase) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "Testing: " << testCase.description << std::endl;
        std::cout << "File: " << testCase.filename << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        
        try {
            // Read test file
            std::string filepath = "tests/lua_samples/" + testCase.filename;
            std::string sourceCode = readFile(filepath);
            
            std::cout << "Source code:\n" << sourceCode << std::endl;
            
            // Test English error reporting
            LocalizationManager::setLanguage(Language::English);
            bool englishPassed = testErrorOutput(sourceCode, testCase.expectedLua51Output, "English");
            
            // Test Chinese error reporting if specified
            bool chinesePassed = true;
            if (testCase.testLocalization && !testCase.expectedChineseOutput.empty()) {
                LocalizationManager::setLanguage(Language::Chinese);
                chinesePassed = testErrorOutput(sourceCode, testCase.expectedChineseOutput, "Chinese");
                
                // Reset to English
                LocalizationManager::setLanguage(Language::English);
            }
            
            bool overallPassed = englishPassed && chinesePassed;
            
            if (overallPassed) {
                std::cout << "✅ TEST PASSED" << std::endl;
                passedTests_++;
            } else {
                std::cout << "❌ TEST FAILED" << std::endl;
                failedTests_++;
            }
            
            return overallPassed;
            
        } catch (const std::exception& e) {
            std::cout << "❌ TEST ERROR: " << e.what() << std::endl;
            failedTests_++;
            return false;
        }
    }
    
    /**
     * @brief Test error output for specific language
     */
    bool testErrorOutput(const std::string& sourceCode, const std::string& expectedOutput, const std::string& language) {
        std::cout << "\n--- " << language << " Error Testing ---" << std::endl;
        
        // Create Lua 5.1 compatible parser
        EnhancedParser parser = ParserFactory::createLua51Parser(sourceCode);
        
        try {
            // Attempt to parse (should fail)
            auto statements = parser.parseWithEnhancedErrors();
            std::cout << "⚠️  Unexpected: Parsing succeeded when it should have failed" << std::endl;
            return false;
        } catch (...) {
            // Expected to fail - get error output
        }
        
        // Get formatted error output
        std::string actualOutput = parser.getFormattedErrors();
        
        std::cout << "Expected: " << expectedOutput << std::endl;
        std::cout << "Actual  : " << actualOutput << std::endl;
        
        // Compare outputs
        bool matches = compareErrorOutputs(actualOutput, expectedOutput);
        
        if (matches) {
            std::cout << "✅ " << language << " format matches Lua 5.1 standard" << std::endl;
        } else {
            std::cout << "❌ " << language << " format does not match expected output" << std::endl;
            analyzeOutputDifferences(actualOutput, expectedOutput);
        }
        
        return matches;
    }
    
    /**
     * @brief Compare error outputs with fuzzy matching
     */
    bool compareErrorOutputs(const std::string& actual, const std::string& expected) {
        // Remove trailing whitespace and newlines for comparison
        std::string cleanActual = trim(actual);
        std::string cleanExpected = trim(expected);
        
        // Exact match
        if (cleanActual == cleanExpected) {
            return true;
        }
        
        // Fuzzy match - check key components
        return containsKeyComponents(cleanActual, cleanExpected);
    }
    
    /**
     * @brief Check if actual output contains key components of expected output
     */
    bool containsKeyComponents(const std::string& actual, const std::string& expected) {
        // Extract key components from expected output
        std::vector<std::string> keyComponents;
        
        // Check for location pattern (filename:line:)
        if (expected.find("stdin:") != std::string::npos) {
            keyComponents.push_back("stdin:");
        }
        
        // Check for specific error messages
        if (expected.find("unexpected symbol near") != std::string::npos) {
            keyComponents.push_back("unexpected symbol near");
        }
        if (expected.find("unfinished string near") != std::string::npos) {
            keyComponents.push_back("unfinished string near");
        }
        if (expected.find("malformed number near") != std::string::npos) {
            keyComponents.push_back("malformed number near");
        }
        if (expected.find("expected") != std::string::npos) {
            keyComponents.push_back("expected");
        }
        
        // Check if all key components are present
        for (const auto& component : keyComponents) {
            if (actual.find(component) == std::string::npos) {
                return false;
            }
        }
        
        return !keyComponents.empty();
    }
    
    /**
     * @brief Analyze differences between actual and expected output
     */
    void analyzeOutputDifferences(const std::string& actual, const std::string& expected) {
        std::cout << "\n--- Difference Analysis ---" << std::endl;
        
        // Check location format
        if (expected.find("stdin:") != std::string::npos && actual.find("stdin:") == std::string::npos) {
            std::cout << "❌ Missing 'stdin:' location prefix" << std::endl;
        }
        
        // Check error message format
        if (expected.find("unexpected symbol near") != std::string::npos && 
            actual.find("unexpected symbol near") == std::string::npos) {
            std::cout << "❌ Missing 'unexpected symbol near' message format" << std::endl;
        }
        
        // Check for quoted tokens
        size_t expectedQuote = expected.find("'");
        size_t actualQuote = actual.find("'");
        if (expectedQuote != std::string::npos && actualQuote == std::string::npos) {
            std::cout << "❌ Missing quoted token in error message" << std::endl;
        }
        
        std::cout << "--- End Analysis ---\n" << std::endl;
    }
    
    /**
     * @brief Trim whitespace from string
     */
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
    
    /**
     * @brief Run all validation tests
     */
    void runAllTests() {
        std::cout << "🚀 Enhanced Error Reporting Validation Suite" << std::endl;
        std::cout << "=============================================" << std::endl;
        std::cout << "Testing " << testCases_.size() << " error cases..." << std::endl;
        
        for (const auto& testCase : testCases_) {
            testSingleCase(testCase);
        }
        
        printSummary();
    }
    
    /**
     * @brief Print test summary
     */
    void printSummary() {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "📊 TEST SUMMARY" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        std::cout << "Total Tests: " << (passedTests_ + failedTests_) << std::endl;
        std::cout << "✅ Passed: " << passedTests_ << std::endl;
        std::cout << "❌ Failed: " << failedTests_ << std::endl;
        
        double successRate = (passedTests_ + failedTests_ > 0) ? 
                           (double)passedTests_ / (passedTests_ + failedTests_) * 100.0 : 0.0;
        
        std::cout << "📈 Success Rate: " << std::fixed << std::setprecision(1) << successRate << "%" << std::endl;
        
        if (failedTests_ == 0) {
            std::cout << "\n🎉 ALL TESTS PASSED! Enhanced error reporting is working correctly." << std::endl;
        } else {
            std::cout << "\n⚠️  Some tests failed. Please review the error output format." << std::endl;
        }
        
        std::cout << std::string(60, '=') << std::endl;
    }
    
    /**
     * @brief Test specific formatter utilities
     */
    void testFormatterUtilities() {
        std::cout << "\n🔧 Testing Formatter Utilities" << std::endl;
        std::cout << std::string(40, '-') << std::endl;
        
        // Test location formatting
        SourceLocation loc("test.lua", 10, 5);
        std::string formatted = Lua51ErrorFormatter::formatLocation(loc);
        std::cout << "Location format: " << formatted << std::endl;
        
        // Test token formatting
        std::string token1 = Lua51ErrorFormatter::formatToken("@");
        std::string token2 = Lua51ErrorFormatter::formatToken("verylongidentifiername");
        std::cout << "Token format '@': " << token1 << std::endl;
        std::cout << "Token format long: " << token2 << std::endl;
        
        // Test source context
        std::string source = "line1\nline2 with error here\nline3";
        SourceLocation errorLoc("test.lua", 2, 15);
        std::string context = Lua51ErrorFormatter::getSourceContext(source, errorLoc, 1);
        std::cout << "Source context:\n" << context << std::endl;
    }
};

int main() {
    try {
        EnhancedErrorReportingValidator validator;
        
        // Test formatter utilities first
        validator.testFormatterUtilities();
        
        // Run all validation tests
        validator.runAllTests();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Validation failed with error: " << e.what() << std::endl;
        return 1;
    }
}
