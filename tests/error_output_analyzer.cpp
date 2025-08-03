#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <map>

/**
 * @brief Error Output Analyzer
 * Analyzes error output format and compares with Lua 5.1 standards
 */
class ErrorOutputAnalyzer {
public:
    struct ErrorComponents {
        std::string filename;
        int line = 0;
        int column = 0;
        std::string errorType;
        std::string message;
        std::string nearToken;
        bool isValid = false;
    };
    
    /**
     * @brief Parse error message into components
     */
    static ErrorComponents parseErrorMessage(const std::string& errorMsg) {
        ErrorComponents components;
        
        // Pattern: filename:line: error_message
        std::regex locationPattern(R"(^([^:]+):(\d+):\s*(.+)$)");
        std::smatch matches;
        
        if (std::regex_match(errorMsg, matches, locationPattern)) {
            components.filename = matches[1].str();
            components.line = std::stoi(matches[2].str());
            components.message = matches[3].str();
            components.isValid = true;
            
            // Extract error type and near token
            extractErrorDetails(components);
        }
        
        return components;
    }
    
    /**
     * @brief Compare two error messages for similarity
     */
    static double calculateSimilarity(const std::string& actual, const std::string& expected) {
        auto actualComponents = parseErrorMessage(actual);
        auto expectedComponents = parseErrorMessage(expected);
        
        if (!actualComponents.isValid || !expectedComponents.isValid) {
            return 0.0;
        }
        
        double score = 0.0;
        double maxScore = 5.0;
        
        // Check filename format (stdin vs actual filename)
        if (actualComponents.filename == expectedComponents.filename) {
            score += 1.0;
        } else if (actualComponents.filename == "stdin" || expectedComponents.filename == "stdin") {
            score += 0.5; // Partial credit for stdin format
        }
        
        // Check line number
        if (actualComponents.line == expectedComponents.line) {
            score += 1.0;
        }
        
        // Check error message similarity
        score += compareErrorMessages(actualComponents.message, expectedComponents.message);
        
        return score / maxScore;
    }
    
    /**
     * @brief Validate Lua 5.1 format compliance
     */
    static bool isLua51Compliant(const std::string& errorMsg) {
        auto components = parseErrorMessage(errorMsg);
        
        if (!components.isValid) {
            return false;
        }
        
        // Check location format
        if (components.filename.empty() || components.line <= 0) {
            return false;
        }
        
        // Check for Lua 5.1 standard error patterns
        return containsLua51Patterns(components.message);
    }
    
    /**
     * @brief Generate compliance report
     */
    static void generateComplianceReport(const std::vector<std::pair<std::string, std::string>>& testCases) {
        std::cout << "\n📋 Lua 5.1 Compliance Report" << std::endl;
        std::cout << std::string(50, '=') << std::endl;
        
        int compliantCount = 0;
        double totalSimilarity = 0.0;
        
        for (size_t i = 0; i < testCases.size(); ++i) {
            const auto& actual = testCases[i].first;
            const auto& expected = testCases[i].second;
            
            bool compliant = isLua51Compliant(actual);
            double similarity = calculateSimilarity(actual, expected);
            
            std::cout << "\nTest Case " << (i + 1) << ":" << std::endl;
            std::cout << "Actual  : " << actual << std::endl;
            std::cout << "Expected: " << expected << std::endl;
            std::cout << "Compliant: " << (compliant ? "✅ Yes" : "❌ No") << std::endl;
            std::cout << "Similarity: " << (similarity * 100.0) << "%" << std::endl;
            
            if (compliant) compliantCount++;
            totalSimilarity += similarity;
            
            if (!compliant || similarity < 0.8) {
                analyzeNonCompliance(actual, expected);
            }
        }
        
        std::cout << "\n" << std::string(50, '-') << std::endl;
        std::cout << "Summary:" << std::endl;
        std::cout << "Compliant Cases: " << compliantCount << "/" << testCases.size() << std::endl;
        std::cout << "Average Similarity: " << (totalSimilarity / testCases.size() * 100.0) << "%" << std::endl;
        
        if (compliantCount == testCases.size() && (totalSimilarity / testCases.size()) >= 0.9) {
            std::cout << "\n🎉 Excellent! Error format is highly Lua 5.1 compliant." << std::endl;
        } else if ((totalSimilarity / testCases.size()) >= 0.7) {
            std::cout << "\n✅ Good! Error format is mostly Lua 5.1 compliant." << std::endl;
        } else {
            std::cout << "\n⚠️  Needs improvement to match Lua 5.1 standard." << std::endl;
        }
    }
    
private:
    static void extractErrorDetails(ErrorComponents& components) {
        const std::string& msg = components.message;
        
        // Extract error type
        if (msg.find("unexpected symbol") != std::string::npos) {
            components.errorType = "unexpected_symbol";
        } else if (msg.find("unfinished string") != std::string::npos) {
            components.errorType = "unfinished_string";
        } else if (msg.find("malformed number") != std::string::npos) {
            components.errorType = "malformed_number";
        } else if (msg.find("expected") != std::string::npos) {
            components.errorType = "missing_token";
        } else {
            components.errorType = "syntax_error";
        }
        
        // Extract near token
        std::regex nearPattern(R"(near\s+'([^']*)')");
        std::smatch nearMatch;
        if (std::regex_search(msg, nearMatch, nearPattern)) {
            components.nearToken = nearMatch[1].str();
        }
    }
    
    static double compareErrorMessages(const std::string& actual, const std::string& expected) {
        double score = 0.0;
        double maxScore = 3.0;
        
        // Check for key phrases
        std::vector<std::string> keyPhrases = {
            "unexpected symbol near",
            "unfinished string near", 
            "malformed number near",
            "expected"
        };
        
        for (const auto& phrase : keyPhrases) {
            bool actualHas = actual.find(phrase) != std::string::npos;
            bool expectedHas = expected.find(phrase) != std::string::npos;
            
            if (actualHas && expectedHas) {
                score += 1.0;
                break;
            }
        }
        
        // Check for quoted tokens
        bool actualHasQuotes = actual.find("'") != std::string::npos;
        bool expectedHasQuotes = expected.find("'") != std::string::npos;
        
        if (actualHasQuotes && expectedHasQuotes) {
            score += 1.0;
        } else if (actualHasQuotes || expectedHasQuotes) {
            score += 0.5;
        }
        
        // Check overall message similarity (simple word matching)
        score += calculateWordSimilarity(actual, expected);
        
        return std::min(score, maxScore);
    }
    
    static double calculateWordSimilarity(const std::string& actual, const std::string& expected) {
        // Simple word-based similarity
        std::vector<std::string> actualWords = splitWords(actual);
        std::vector<std::string> expectedWords = splitWords(expected);
        
        int commonWords = 0;
        for (const auto& word : actualWords) {
            if (std::find(expectedWords.begin(), expectedWords.end(), word) != expectedWords.end()) {
                commonWords++;
            }
        }
        
        int totalWords = std::max(actualWords.size(), expectedWords.size());
        return totalWords > 0 ? (double)commonWords / totalWords : 0.0;
    }
    
    static std::vector<std::string> splitWords(const std::string& text) {
        std::vector<std::string> words;
        std::string word;
        
        for (char c : text) {
            if (std::isalnum(c) || c == '_') {
                word += c;
            } else {
                if (!word.empty()) {
                    words.push_back(word);
                    word.clear();
                }
            }
        }
        
        if (!word.empty()) {
            words.push_back(word);
        }
        
        return words;
    }
    
    static bool containsLua51Patterns(const std::string& message) {
        std::vector<std::string> lua51Patterns = {
            "unexpected symbol near",
            "syntax error near",
            "unfinished string near",
            "malformed number near",
            "invalid escape sequence near",
            "expected",
            "unexpected end of file"
        };
        
        for (const auto& pattern : lua51Patterns) {
            if (message.find(pattern) != std::string::npos) {
                return true;
            }
        }
        
        return false;
    }
    
    static void analyzeNonCompliance(const std::string& actual, const std::string& expected) {
        std::cout << "Issues found:" << std::endl;
        
        auto actualComp = parseErrorMessage(actual);
        auto expectedComp = parseErrorMessage(expected);
        
        if (!actualComp.isValid) {
            std::cout << "  - Invalid error message format" << std::endl;
        }
        
        if (actualComp.filename != expectedComp.filename) {
            std::cout << "  - Filename mismatch: '" << actualComp.filename 
                      << "' vs '" << expectedComp.filename << "'" << std::endl;
        }
        
        if (actualComp.line != expectedComp.line) {
            std::cout << "  - Line number mismatch: " << actualComp.line 
                      << " vs " << expectedComp.line << std::endl;
        }
        
        if (!containsLua51Patterns(actualComp.message)) {
            std::cout << "  - Message doesn't match Lua 5.1 patterns" << std::endl;
        }
    }
};

// Example usage
int main() {
    std::cout << "🔍 Error Output Analyzer" << std::endl;
    std::cout << "========================" << std::endl;
    
    // Test cases: {actual_output, expected_output}
    std::vector<std::pair<std::string, std::string>> testCases = {
        {
            "stdin:1: unexpected symbol near '@'",
            "stdin:1: unexpected symbol near '@'"
        },
        {
            "test.lua:1: syntax error near '@'",
            "stdin:1: unexpected symbol near '@'"
        },
        {
            "stdin:1: unfinished string near '\"hello'",
            "stdin:1: unfinished string near '\"hello'"
        },
        {
            "Error at line 1: Unexpected token '@'",
            "stdin:1: unexpected symbol near '@'"
        }
    };
    
    ErrorOutputAnalyzer::generateComplianceReport(testCases);
    
    return 0;
}
