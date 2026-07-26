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
enum class TestStatus {
    Passed,
    Failed,
    ExpectedSkip,
    UnexpectedSkip,
};

struct TestResult {
    std::string testName;
    bool passed;
    std::string message;
    TestStatus status;

    TestResult(const std::string& name, bool pass, const std::string& msg = "")
        : testName(name), passed(pass), message(msg), status(pass ? TestStatus::Passed : TestStatus::Failed) {}

    TestResult(const std::string& name, TestStatus resultStatus, const std::string& msg)
        : testName(name), passed(resultStatus == TestStatus::Passed || resultStatus == TestStatus::ExpectedSkip),
          message(msg), status(resultStatus) {}

    static TestResult expectedSkip(const std::string& name, const std::string& reason) {
        return TestResult(name, TestStatus::ExpectedSkip, reason);
    }

    static TestResult unexpectedSkip(const std::string& name, const std::string& reason) {
        return TestResult(name, TestStatus::UnexpectedSkip, reason);
    }
};

/**
 * @brief Test suite
 */
class TestSuite {
public:
    TestSuite(const std::string& name)
        : suiteName_(name), passCount_(0), failCount_(0), expectedSkipCount_(0), unexpectedSkipCount_(0) {}

    void addResult(const TestResult& result) {
        results_.push_back(result);
        switch (result.status) {
        case TestStatus::Passed:
            passCount_++;
            break;
        case TestStatus::Failed:
            failCount_++;
            break;
        case TestStatus::ExpectedSkip:
            expectedSkipCount_++;
            break;
        case TestStatus::UnexpectedSkip:
            unexpectedSkipCount_++;
            break;
        }
    }

    void printReport() const {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Test Suite: " << suiteName_ << std::endl;
        std::cout << "========================================" << std::endl;

        for (const auto& result : results_) {
            const char* status = "FAIL";
            switch (result.status) {
            case TestStatus::Passed:
                status = "PASS";
                break;
            case TestStatus::Failed:
                status = "FAIL";
                break;
            case TestStatus::ExpectedSkip:
                status = "SKIP-EXPECTED";
                break;
            case TestStatus::UnexpectedSkip:
                status = "SKIP-UNEXPECTED";
                break;
            }
            std::cout << "  [" << status << "] " << result.testName;
            if (!result.message.empty()) {
                std::cout << " - " << result.message;
            }
            std::cout << std::endl;
        }

        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Total: " << getTotalCount() << " | Pass: " << passCount_ << " | Fail: " << failCount_
                  << " | Expected Skip: " << expectedSkipCount_ << " | Unexpected Skip: " << unexpectedSkipCount_
                  << std::endl;
        std::cout << "========================================\n" << std::endl;
    }

    void writeJUnitXml(std::ostream& out) const {
        out << "  <testsuite name=\"" << escapeXml(suiteName_) << "\" tests=\"" << getTotalCount() << "\" failures=\""
            << getBlockingCount() << "\" skipped=\"" << expectedSkipCount_ << "\">\n";

        for (const auto& result : results_) {
            out << "    <testcase classname=\"" << escapeXml(suiteName_) << "\" name=\"" << escapeXml(result.testName)
                << "\"";
            switch (result.status) {
            case TestStatus::Passed:
                out << " />\n";
                break;
            case TestStatus::ExpectedSkip:
                out << ">\n";
                out << "      <skipped message=\"" << escapeXml(result.message) << "\" />\n";
                out << "    </testcase>\n";
                break;
            case TestStatus::Failed:
                out << ">\n";
                out << "      <failure message=\"" << escapeXml(result.message) << "\">" << escapeXml(result.message)
                    << "</failure>\n";
                out << "    </testcase>\n";
                break;
            case TestStatus::UnexpectedSkip:
                out << ">\n";
                out << "      <failure type=\"unexpected-skip\" message=\"" << escapeXml(result.message) << "\">"
                    << escapeXml(result.message) << "</failure>\n";
                out << "    </testcase>\n";
                break;
            }
        }

        out << "  </testsuite>\n";
    }

    int getFailCount() const {
        return failCount_;
    }
    int getPassCount() const {
        return passCount_;
    }
    int getExpectedSkipCount() const {
        return expectedSkipCount_;
    }
    int getUnexpectedSkipCount() const {
        return unexpectedSkipCount_;
    }
    int getBlockingCount() const {
        return failCount_ + unexpectedSkipCount_;
    }
    int getTotalCount() const {
        return passCount_ + failCount_ + expectedSkipCount_ + unexpectedSkipCount_;
    }
    const std::string& getName() const {
        return suiteName_;
    }
    const std::vector<TestResult>& getResults() const {
        return results_;
    }

private:
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

    std::string suiteName_;
    std::vector<TestResult> results_;
    int passCount_;
    int failCount_;
    int expectedSkipCount_;
    int unexpectedSkipCount_;
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
        int totalFailed = 0;
        int totalPass = 0;
        int totalExpectedSkip = 0;
        int totalUnexpectedSkip = 0;
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
                    totalFailed += suite->getFailCount();
                    totalPass += suite->getPassCount();
                    totalExpectedSkip += suite->getExpectedSkipCount();
                    totalUnexpectedSkip += suite->getUnexpectedSkipCount();
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
            totalFailed += suite->getFailCount();
            totalPass += suite->getPassCount();
            totalExpectedSkip += suite->getExpectedSkipCount();
            totalUnexpectedSkip += suite->getUnexpectedSkipCount();
            if (options.captureSuites) {
                lastSuites_.push_back(*suite);
            }
            delete suite;
        }

        lastPassCount_ = totalPass;
        lastExpectedSkipCount_ = totalExpectedSkip;
        lastUnexpectedSkipCount_ = totalUnexpectedSkip;
        lastFailCount_ = totalFailed + totalUnexpectedSkip;
        lastTotalCount_ = totalPass + totalFailed + totalExpectedSkip + totalUnexpectedSkip;
        return lastFailCount_;
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
    int getLastExpectedSkipCount() const {
        return lastExpectedSkipCount_;
    }
    int getLastUnexpectedSkipCount() const {
        return lastUnexpectedSkipCount_;
    }

    bool writeJUnitReport(const std::string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out.is_open()) {
            return false;
        }

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        out << "<testsuites tests=\"" << lastTotalCount_ << "\" failures=\"" << lastFailCount_ << "\" skipped=\""
            << lastExpectedSkipCount_ << "\">\n";
        for (const auto& suite : lastSuites_) {
            suite.writeJUnitXml(out);
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

    std::vector<TestEntry> tests_;
    std::vector<TestSuite> lastSuites_;
    int lastRunTestCount_ = 0;
    int lastPassCount_ = 0;
    int lastFailCount_ = 0;
    int lastExpectedSkipCount_ = 0;
    int lastUnexpectedSkipCount_ = 0;
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
