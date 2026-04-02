/**
 * @file test_framework.hpp
 * @brief Lightweight C++ test framework - generic testing utilities independent of any project
 * 
 * Provides basic test assertions and test reporting functionality without external dependencies.
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#ifndef TEST_FRAMEWORK_HPP
#define TEST_FRAMEWORK_HPP

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <iomanip>
#include <sstream>

namespace TestFramework {

/**
 * @brief Test result
 */
struct TestResult {
    std::string testName;
    bool passed;
    std::string message;
    
    TestResult(const std::string& name, bool pass, const std::string& msg = "")
        : testName(name), passed(pass), message(msg) {}
};

/**
 * @brief Test suite
 */
class TestSuite {
public:
    TestSuite(const std::string& name) : suiteName_(name), passCount_(0), failCount_(0) {}
    
    void addResult(const TestResult& result) {
        results_.push_back(result);
        if (result.passed) {
            passCount_++;
        } else {
            failCount_++;
        }
    }
    
    void printReport() const {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Test Suite: " << suiteName_ << std::endl;
        std::cout << "========================================" << std::endl;
        
        for (const auto& result : results_) {
            std::cout << "  [" << (result.passed ? "PASS" : "FAIL") << "] " 
                      << result.testName;
            if (!result.message.empty()) {
                std::cout << " - " << result.message;
            }
            std::cout << std::endl;
        }
        
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Total: " << (passCount_ + failCount_) 
                  << " | Pass: " << passCount_ 
                  << " | Fail: " << failCount_ << std::endl;
        std::cout << "========================================\n" << std::endl;
    }
    
    int getFailCount() const { return failCount_; }
    int getPassCount() const { return passCount_; }
    int getTotalCount() const { return passCount_ + failCount_; }
    const std::string& getName() const { return suiteName_; }
    
private:
    std::string suiteName_;
    std::vector<TestResult> results_;
    int passCount_;
    int failCount_;
};

/**
 * @brief Global test registry
 */
class TestRegistry {
public:
    using TestFunction = std::function<void(TestSuite&)>;
    
    static TestRegistry& getInstance() {
        static TestRegistry instance;
        return instance;
    }
    
    void registerTest(const std::string& suiteName, const std::string& testName, TestFunction func) {
        tests_.push_back({suiteName, testName, func});
    }
    
    int runAllTests() {
        int totalFail = 0;
        std::string currentSuite;
        TestSuite* suite = nullptr;
        
        for (const auto& test : tests_) {
            if (test.suiteName != currentSuite) {
                if (suite) {
                    suite->printReport();
                    totalFail += suite->getFailCount();
                    delete suite;
                }
                currentSuite = test.suiteName;
                suite = new TestSuite(currentSuite);
            }
            
            try {
                test.func(*suite);
            } catch (const std::exception& e) {
                suite->addResult(TestResult(test.testName, false, std::string("Exception: ") + e.what()));
            }
        }
        
        if (suite) {
            suite->printReport();
            totalFail += suite->getFailCount();
            delete suite;
        }
        
        return totalFail;
    }
    
private:
    TestRegistry() = default;
    
    struct TestEntry {
        std::string suiteName;
        std::string testName;
        TestFunction func;
    };
    
    std::vector<TestEntry> tests_;
};

// ============================================================================
// Test assertion macros
// ============================================================================

#define TF_ASSERT_TRUE(suite, condition, testName) \
    do { \
        bool bool_result = (condition); \
        suite.addResult(TestFramework::TestResult(testName, bool_result, bool_result ? "" : "Expected true")); \
    } while(0)

#define TF_ASSERT_FALSE(suite, condition, testName) \
    do { \
        bool bool_result = !(condition); \
        suite.addResult(TestFramework::TestResult(testName, bool_result, bool_result ? "" : "Expected false")); \
    } while(0)

#define TF_ASSERT_EQ(suite, expected, actual, testName) \
    do { \
        bool bool_result = ((expected) == (actual)); \
        suite.addResult(TestFramework::TestResult(testName, bool_result, bool_result ? "" : "Values not equal")); \
    } while(0)

} // namespace TestFramework

#endif // TEST_FRAMEWORK_HPP
