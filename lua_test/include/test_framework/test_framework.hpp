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
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>

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
            std::cout << "  [" << (result.passed ? "PASS" : "FAIL") << "] " << result.testName;
            if (!result.message.empty()) {
                std::cout << " - " << result.message;
            }
            std::cout << std::endl;
        }

        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Total: " << (passCount_ + failCount_) << " | Pass: " << passCount_ << " | Fail: " << failCount_
                  << std::endl;
        std::cout << "========================================\n" << std::endl;
    }

    int getFailCount() const {
        return failCount_;
    }
    int getPassCount() const {
        return passCount_;
    }
    int getTotalCount() const {
        return passCount_ + failCount_;
    }
    const std::string& getName() const {
        return suiteName_;
    }
    const std::vector<TestResult>& getResults() const {
        return results_;
    }

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

    struct TestEntry {
        std::string suiteName;
        std::string testName;
        TestFunction func;
    };

    struct RunOptions {
        std::string filter;
        std::string excludeFilter;
        bool printReports = true;
        bool captureSuites = false;
    };

    static TestRegistry& getInstance() {
        static TestRegistry instance;
        return instance;
    }

    void registerTest(const std::string& suiteName, const std::string& testName, TestFunction func) {
        tests_.push_back({suiteName, testName, func});
    }

    int runAllTests() {
        return runTests(RunOptions{});
    }

    int runTests(const RunOptions& options) {
        int totalFail = 0;
        int totalPass = 0;
        std::string currentSuite;
        TestSuite* suite = nullptr;

        lastSuites_.clear();
        lastRunTestCount_ = 0;

        for (const auto& test : tests_) {
            if (!matchesFilter(test, options.filter) ||
                (!options.excludeFilter.empty() && matchesFilter(test, options.excludeFilter))) {
                continue;
            }

            if (test.suiteName != currentSuite) {
                if (suite) {
                    if (options.printReports) {
                        suite->printReport();
                    }
                    totalFail += suite->getFailCount();
                    totalPass += suite->getPassCount();
                    if (options.captureSuites) {
                        lastSuites_.push_back(*suite);
                    }
                    delete suite;
                }
                currentSuite = test.suiteName;
                suite = new TestSuite(currentSuite);
            }

            lastRunTestCount_++;
            try {
                test.func(*suite);
            } catch (const std::exception& e) {
                suite->addResult(TestResult(test.testName, false, std::string("Exception: ") + e.what()));
            }
        }

        if (suite) {
            if (options.printReports) {
                suite->printReport();
            }
            totalFail += suite->getFailCount();
            totalPass += suite->getPassCount();
            if (options.captureSuites) {
                lastSuites_.push_back(*suite);
            }
            delete suite;
        }

        lastPassCount_ = totalPass;
        lastFailCount_ = totalFail;
        lastTotalCount_ = totalPass + totalFail;
        return totalFail;
    }

    int getRegisteredTestCount() const {
        return static_cast<int>(tests_.size());
    }

    const std::vector<TestEntry>& getTests() const {
        return tests_;
    }
    const std::vector<TestSuite>& getLastSuites() const {
        return lastSuites_;
    }
    int getLastRunTestCount() const {
        return lastRunTestCount_;
    }
    int getLastPassCount() const {
        return lastPassCount_;
    }
    int getLastFailCount() const {
        return lastFailCount_;
    }
    int getLastTotalCount() const {
        return lastTotalCount_;
    }

    bool writeJUnitReport(const std::string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out.is_open()) {
            return false;
        }

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out << "<testsuites tests=\"" << lastTotalCount_ << "\" failures=\"" << lastFailCount_ << "\">\n";
        for (const auto& suite : lastSuites_) {
            out << "  <testsuite name=\"" << escapeXml(suite.getName()) << "\" tests=\"" << suite.getTotalCount()
                << "\" failures=\"" << suite.getFailCount() << "\">\n";

            for (const auto& result : suite.getResults()) {
                out << "    <testcase classname=\"" << escapeXml(suite.getName()) << "\" name=\""
                    << escapeXml(result.testName) << "\"";
                if (result.passed) {
                    out << " />\n";
                } else {
                    out << ">\n";
                    out << "      <failure message=\"" << escapeXml(result.message) << "\">"
                        << escapeXml(result.message) << "</failure>\n";
                    out << "    </testcase>\n";
                }
            }

            out << "  </testsuite>\n";
        }
        out << "</testsuites>\n";

        return static_cast<bool>(out);
    }

private:
    TestRegistry() = default;

    static bool containsCaseInsensitive(const std::string& haystack, const std::string& needle) {
        if (needle.empty()) {
            return true;
        }

        auto toLower = [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); };

        std::string loweredHaystack;
        loweredHaystack.reserve(haystack.size());
        std::transform(haystack.begin(), haystack.end(), std::back_inserter(loweredHaystack), toLower);

        std::string loweredNeedle;
        loweredNeedle.reserve(needle.size());
        std::transform(needle.begin(), needle.end(), std::back_inserter(loweredNeedle), toLower);

        return loweredHaystack.find(loweredNeedle) != std::string::npos;
    }

    static bool matchesFilter(const TestEntry& test, const std::string& filter) {
        if (filter.empty()) {
            return true;
        }

        const std::string fullName = test.suiteName + "::" + test.testName;
        return containsCaseInsensitive(test.suiteName, filter) || containsCaseInsensitive(test.testName, filter) ||
               containsCaseInsensitive(fullName, filter);
    }

    static std::string escapeXml(const std::string& text) {
        std::string escaped;
        escaped.reserve(text.size());

        for (char ch : text) {
            switch (ch) {
            case '&':
                escaped += "&amp;";
                break;
            case '<':
                escaped += "&lt;";
                break;
            case '>':
                escaped += "&gt;";
                break;
            case '"':
                escaped += "&quot;";
                break;
            case '\'':
                escaped += "&apos;";
                break;
            default:
                escaped += ch;
                break;
            }
        }

        return escaped;
    }

    std::vector<TestEntry> tests_;
    std::vector<TestSuite> lastSuites_;
    int lastRunTestCount_ = 0;
    int lastPassCount_ = 0;
    int lastFailCount_ = 0;
    int lastTotalCount_ = 0;
};

// ============================================================================
// Test assertion macros
// ============================================================================

#define TF_ASSERT_TRUE(suite, condition, testName)                                                                     \
    do {                                                                                                               \
        bool bool_result = (condition);                                                                                \
        suite.addResult(TestFramework::TestResult(testName, bool_result, bool_result ? "" : "Expected true"));         \
    } while (0)

#define TF_ASSERT_FALSE(suite, condition, testName)                                                                    \
    do {                                                                                                               \
        bool bool_result = !(condition);                                                                               \
        suite.addResult(TestFramework::TestResult(testName, bool_result, bool_result ? "" : "Expected false"));        \
    } while (0)

#define TF_ASSERT_EQ(suite, expected, actual, testName)                                                                \
    do {                                                                                                               \
        bool bool_result = ((expected) == (actual));                                                                   \
        suite.addResult(TestFramework::TestResult(testName, bool_result, bool_result ? "" : "Values not equal"));      \
    } while (0)

} // namespace TestFramework

#endif // TEST_FRAMEWORK_HPP
