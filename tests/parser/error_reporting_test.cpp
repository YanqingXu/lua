#include "../../src/parser/enhanced_parser.hpp"
#include "../../src/parser/error_formatter.hpp"
#include "../../src/test_framework/core/test_macros.hpp"
#include <vector>
#include <string>

using namespace Lua;

/**
 * @brief Test suite for enhanced error reporting
 */
class ErrorReportingTest {
public:
    /**
     * @brief Test Lua 5.1 error format compatibility
     */
    static void testLua51ErrorFormat() {
        TEST_SUITE_BEGIN("Lua 5.1 Error Format Compatibility");
        
        // Test case 1: Unexpected symbol
        {
            Str source = "local x = 1 @";
            EnhancedParser parser = ParserFactory::createLua51Parser(source);
            
            try {
                parser.parseWithEnhancedErrors();
            } catch (...) {
                // Expected to fail
            }
            
            Str errorOutput = parser.getFormattedErrors();
            Str expectedPattern = "stdin:1: unexpected symbol near '@'";
            
            TEST_ASSERT_CONTAINS(errorOutput, "unexpected symbol near");
            TEST_ASSERT_CONTAINS(errorOutput, "'@'");
            TEST_ASSERT_CONTAINS(errorOutput, "stdin:1:");
        }
        
        // Test case 2: Missing 'end'
        {
            Str source = "if true then\n  print('hello')\n";
            EnhancedParser parser = ParserFactory::createLua51Parser(source);
            
            try {
                parser.parseWithEnhancedErrors();
            } catch (...) {
                // Expected to fail
            }
            
            Str errorOutput = parser.getFormattedErrors();
            
            TEST_ASSERT_CONTAINS(errorOutput, "'end' expected");
            TEST_ASSERT_CONTAINS(errorOutput, "stdin:");
        }
        
        // Test case 3: Unfinished string
        {
            Str source = "local s = \"hello world";
            EnhancedParser parser = ParserFactory::createLua51Parser(source);
            
            try {
                parser.parseWithEnhancedErrors();
            } catch (...) {
                // Expected to fail
            }
            
            Str errorOutput = parser.getFormattedErrors();
            
            TEST_ASSERT_CONTAINS(errorOutput, "unfinished string near");
            TEST_ASSERT_CONTAINS(errorOutput, "stdin:1:");
        }
        
        // Test case 4: Malformed number
        {
            Str source = "local n = 123.45.67";
            EnhancedParser parser = ParserFactory::createLua51Parser(source);
            
            try {
                parser.parseWithEnhancedErrors();
            } catch (...) {
                // Expected to fail
            }
            
            Str errorOutput = parser.getFormattedErrors();
            
            TEST_ASSERT_CONTAINS(errorOutput, "malformed number near");
            TEST_ASSERT_CONTAINS(errorOutput, "stdin:1:");
        }
        
        // Test case 5: Unexpected EOF
        {
            Str source = "function test()\n  print('hello')";
            EnhancedParser parser = ParserFactory::createLua51Parser(source);
            
            try {
                parser.parseWithEnhancedErrors();
            } catch (...) {
                // Expected to fail
            }
            
            Str errorOutput = parser.getFormattedErrors();
            
            TEST_ASSERT_CONTAINS(errorOutput, "'end' expected");
            TEST_ASSERT_CONTAINS(errorOutput, "stdin:");
        }
        
        TEST_SUITE_END();
    }
    
    /**
     * @brief Test error message localization
     */
    static void testErrorLocalization() {
        TEST_SUITE_BEGIN("Error Message Localization");
        
        // Test English error messages
        {
            LocalizationManager::setLanguage(Language::English);
            
            Str source = "local x = 1 @";
            EnhancedParser parser = ParserFactory::createLua51Parser(source);
            
            try {
                parser.parseWithEnhancedErrors();
            } catch (...) {
                // Expected to fail
            }
            
            Str errorOutput = parser.getFormattedErrors();
            TEST_ASSERT_CONTAINS(errorOutput, "unexpected symbol near");
        }
        
        // Test Chinese error messages
        {
            LocalizationManager::setLanguage(Language::Chinese);
            
            Str source = "local x = 1 @";
            EnhancedParser parser = ParserFactory::createLua51Parser(source);
            
            try {
                parser.parseWithEnhancedErrors();
            } catch (...) {
                // Expected to fail
            }
            
            Str errorOutput = parser.getFormattedErrors();
            TEST_ASSERT_CONTAINS(errorOutput, "意外符号");
        }
        
        // Reset to English
        LocalizationManager::setLanguage(Language::English);
        
        TEST_SUITE_END();
    }
    
    /**
     * @brief Test error formatter utility functions
     */
    static void testErrorFormatterUtils() {
        TEST_SUITE_BEGIN("Error Formatter Utilities");
        
        // Test location formatting
        {
            SourceLocation loc("test.lua", 10, 5);
            Str formatted = Lua51ErrorFormatter::formatLocation(loc);
            TEST_ASSERT_EQUALS(formatted, "test.lua:10:");
        }
        
        // Test token formatting
        {
            Str token1 = Lua51ErrorFormatter::formatToken("@");
            TEST_ASSERT_EQUALS(token1, "'@'");
            
            Str token2 = Lua51ErrorFormatter::formatToken("<eof>");
            TEST_ASSERT_EQUALS(token2, "<eof>");
            
            Str longToken = Lua51ErrorFormatter::formatToken("verylongidentifiername");
            TEST_ASSERT_CONTAINS(longToken, "...");
        }
        
        // Test source context extraction
        {
            Str source = "line1\nline2 with error\nline3";
            SourceLocation loc("test.lua", 2, 10);
            Str context = Lua51ErrorFormatter::getSourceContext(source, loc, 1);
            
            TEST_ASSERT_CONTAINS(context, "line2 with error");
            TEST_ASSERT_CONTAINS(context, ">>>");
        }
        
        TEST_SUITE_END();
    }
    
    /**
     * @brief Test comparison with actual Lua 5.1 output
     */
    static void testLua51Compatibility() {
        TEST_SUITE_BEGIN("Lua 5.1 Compatibility Verification");
        
        struct TestCase {
            Str source;
            Str expectedLua51Output;
        };
        
        std::vector<TestCase> testCases = {
            {
                "local x = 1 @",
                "stdin:1: unexpected symbol near '@'"
            },
            {
                "if true then",
                "stdin:1: 'end' expected (to close 'if' at line 1)"
            },
            {
                "local s = \"hello",
                "stdin:1: unfinished string near '\"hello'"
            },
            {
                "local n = 123.45.67",
                "stdin:1: malformed number near '123.45.67'"
            },
            {
                "function test()",
                "stdin:1: 'end' expected (to close 'function' at line 1)"
            }
        };
        
        for (const auto& testCase : testCases) {
            EnhancedParser parser = ParserFactory::createLua51Parser(testCase.source);
            
            try {
                parser.parseWithEnhancedErrors();
            } catch (...) {
                // Expected to fail
            }
            
            Str ourOutput = parser.getFormattedErrors();
            double similarity = ErrorComparisonUtil::compareWithLua51(ourOutput, testCase.expectedLua51Output);
            
            // Should have high similarity (>= 0.8)
            TEST_ASSERT_GREATER_EQUAL(similarity, 0.8);
        }
        
        TEST_SUITE_END();
    }
    
    /**
     * @brief Run all error reporting tests
     */
    static void runAllTests() {
        testLua51ErrorFormat();
        testErrorLocalization();
        testErrorFormatterUtils();
        testLua51Compatibility();
    }
};

// Test runner
int main() {
    try {
        ErrorReportingTest::runAllTests();
        std::cout << "All error reporting tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
