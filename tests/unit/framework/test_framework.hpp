/**
 * @file test_framework.hpp
 * @brief 简单的测试框架 - 用于单元测试
 * 
 * 这是一个轻量级的测试框架，不依赖外部库（如Google Test）。
 * 提供基本的测试断言和测试报告功能。
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#ifndef LUA_TEST_FRAMEWORK_HPP
#define LUA_TEST_FRAMEWORK_HPP

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <iomanip>

#include "core/value.hpp"

namespace Lua {
class LuaState;
}

namespace LuaTest {

/**
 * @brief 测试结果
 */
struct TestResult {
    std::string testName;
    bool passed;
    std::string message;
    
    TestResult(const std::string& name, bool pass, const std::string& msg = "")
        : testName(name), passed(pass), message(msg) {}
};

/**
 * @brief 测试套件
 */
class TestSuite {
public:
    TestSuite(const std::string& name) : suiteName_(name), passCount_(0), failCount_(0) {}
    
    /**
     * @brief 添加测试结果
     */
    void addResult(const TestResult& result) {
        results_.push_back(result);
        if (result.passed) {
            passCount_++;
        } else {
            failCount_++;
        }
    }
    
    /**
     * @brief 打印测试报告
     */
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
    
    /**
     * @brief 获取失败数量
     */
    int getFailCount() const { return failCount_; }
    
    /**
     * @brief 获取通过数量
     */
    int getPassCount() const { return passCount_; }
    
private:
    std::string suiteName_;
    std::vector<TestResult> results_;
    int passCount_;
    int failCount_;
};

/**
 * @brief 全局测试注册器
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
    struct TestEntry {
        std::string suiteName;
        std::string testName;
        TestFunction func;
    };
    
    std::vector<TestEntry> tests_;
};

using StdLibOpenFunction = void (*)(Lua::LuaState*);

class LuaStdLibTestContext {
public:
    explicit LuaStdLibTestContext(StdLibOpenFunction openFunc = nullptr);
    ~LuaStdLibTestContext();

    LuaStdLibTestContext(const LuaStdLibTestContext&) = delete;
    LuaStdLibTestContext& operator=(const LuaStdLibTestContext&) = delete;
    LuaStdLibTestContext(LuaStdLibTestContext&&) = delete;
    LuaStdLibTestContext& operator=(LuaStdLibTestContext&&) = delete;

    Lua::LuaState* getState() const { return state_; }

    void clearStack() const;

    Lua::Value getGlobal(const char* name) const;

    bool ensureGlobalFunction(const char* name, TestSuite& suite, const std::string& message) const;

    int invoke(const char* name, const std::function<void(Lua::LuaState*)>& pushArgs) const;

private:
    Lua::LuaState* state_;
};

// 测试断言宏
#define ASSERT_TRUE(suite, condition, testName) \
    do { \
        bool result = (condition); \
        suite.addResult(LuaTest::TestResult(testName, result, result ? "" : "Expected true")); \
    } while(0)

#define ASSERT_FALSE(suite, condition, testName) \
    do { \
        bool result = !(condition); \
        suite.addResult(LuaTest::TestResult(testName, result, result ? "" : "Expected false")); \
    } while(0)

#define ASSERT_EQ(suite, expected, actual, testName) \
    do { \
        bool result = ((expected) == (actual)); \
        suite.addResult(LuaTest::TestResult(testName, result, result ? "" : "Values not equal")); \
    } while(0)

} // namespace LuaTest

#endif // LUA_TEST_FRAMEWORK_HPP

