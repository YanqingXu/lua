/**
 * @file test_runner.cpp
 * @brief 测试运行器 - 统一的测试入口
 * 
 * 这个文件包含所有单元测试的主入口。
 * 它会自动运行所有注册的测试并生成报告。
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#include "test_framework.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

// 声明所有测试注册函数
extern void registerValueTests();
extern void registerGCStringTests();
extern void registerTableTests();
extern void registerVMCoreTests();
extern void registerFunctionCallTests();
extern void registerFunctionTests();
extern void registerGCTests();
extern void registerLuaStateInitTests();
extern void registerRuntimeServicesTests();
extern void registerBinaryUnaryExprTests();
extern void registerFunctionCodegenTests();
extern void registerCodegenConditionTests();
extern void registerCodegenMultiRetTests();
extern void registerCodegenResultTypeTests();
extern void registerCodegenStateTests();
extern void registerBytecodeBuilderTests();
extern void registerMathLibTests();
extern void registerLibCatalogTests();
extern void registerBaselibTests();
extern void registerStringLibTests();
extern void registerTableLibTests();
extern void registerOSlibTests();
extern void registerLuaFunctionTests();
extern void registerMetamethodArithTests();
extern void registerMetamethodCompleteTests();
extern void registerSyntaxSugarTests();
extern void registerIndexedAccessTests();
extern void registerMethodCallTests();
extern void registerStorevarTests();
extern void registerLValuePipelineTests();
extern void registerValuePipelineTests();
extern void registerLexerNumberTests();
extern void registerLexerLookaheadTests();
extern void registerParserRecursionTests();
extern void registerParserErrorRecoveryTests();
extern void registerParserMemoryPoolTests();
extern void registerCoroutineLibTests();
extern void registerDebugLibTests();
extern void registerPackageLibTests();
extern void registerCallPipelineTests();
extern void registerSymbolBindingTests();
extern void registerDynamicBufferTests();
extern void registerInputStreamStringTests();
extern void registerInputStreamStreamTests();
extern void registerInputStreamFileTests();
extern void registerFileLoaderTests();
extern void registerAppOptionsTests();

namespace {

constexpr const char* kDefaultJunitReportPath = "lua_test_junit.xml";

struct RunnerOptions {
    bool list = false;
    bool showHelp = false;
    std::string filter;
    bool writeJunit = false;
    std::string junitPath = kDefaultJunitReportPath;
};

bool startsWith(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

bool containsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    return needle.empty() || toLower(haystack).find(toLower(needle)) != std::string::npos;
}

bool testMatchesFilter(const LuaTest::TestRegistry::TestEntry& test, const std::string& filter) {
    if (filter.empty()) {
        return true;
    }

    const std::string fullName = test.suiteName + "::" + test.testName;
    return containsCaseInsensitive(test.suiteName, filter) ||
           containsCaseInsensitive(test.testName, filter) ||
           containsCaseInsensitive(fullName, filter);
}

bool parseArgs(int argc, char** argv, RunnerOptions& options) {
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index] ? argv[index] : "";

        if (arg == "--help" || arg == "-h") {
            options.showHelp = true;
        } else if (arg == "--list") {
            options.list = true;
        } else if (arg == "--filter") {
            if (index + 1 >= argc) {
                std::cerr << "error: --filter requires a suite or test name" << std::endl;
                return false;
            }
            options.filter = argv[++index] ? argv[index] : "";
        } else if (startsWith(arg, "--filter=")) {
            options.filter = arg.substr(std::string("--filter=").size());
        } else if (arg == "--report=junit") {
            options.writeJunit = true;
        } else if (startsWith(arg, "--report=junit:")) {
            options.writeJunit = true;
            options.junitPath = arg.substr(std::string("--report=junit:").size());
            if (options.junitPath.empty()) {
                options.junitPath = kDefaultJunitReportPath;
            }
        } else if (startsWith(arg, "--report=")) {
            std::cerr << "error: unsupported report format: " << arg.substr(std::string("--report=").size())
                      << std::endl;
            return false;
        } else {
            std::cerr << "error: unknown option: " << arg << std::endl;
            return false;
        }
    }

    return true;
}

void printUsage() {
    std::cout << "Usage: lua_test.exe [--list] [--filter <suite-or-name>] [--report=junit]" << std::endl;
    std::cout << "       lua_test.exe --report=junit:<path>" << std::endl;
}

void registerAllTests() {
    registerValueTests();
    registerGCStringTests();
    registerTableTests();
    registerVMCoreTests();
    registerFunctionCallTests();
    registerFunctionTests();
    registerGCTests();
    registerLuaStateInitTests();
    registerRuntimeServicesTests();
    registerBinaryUnaryExprTests();
    registerFunctionCodegenTests();
    registerCodegenConditionTests();
    registerCodegenMultiRetTests();
    registerCodegenResultTypeTests();
    registerCodegenStateTests();
    registerBytecodeBuilderTests();
    registerSyntaxSugarTests();
    registerLibCatalogTests();
    registerMathLibTests();
    registerBaselibTests();
    registerStringLibTests();
    registerTableLibTests();
    registerOSlibTests();
    registerLuaFunctionTests();
    registerMetamethodArithTests();
    registerMetamethodCompleteTests();
    registerIndexedAccessTests();
    registerMethodCallTests();
    registerStorevarTests();
    registerLValuePipelineTests();
    registerValuePipelineTests();
    registerLexerNumberTests();
    registerLexerLookaheadTests();
    registerParserRecursionTests();
    registerParserErrorRecoveryTests();
    registerParserMemoryPoolTests();
    registerCoroutineLibTests();
    registerDebugLibTests();
    registerPackageLibTests();
    registerCallPipelineTests();
    registerSymbolBindingTests();
    registerDynamicBufferTests();
    registerInputStreamStringTests();
    registerInputStreamStreamTests();
    registerInputStreamFileTests();
    registerFileLoaderTests();
    registerAppOptionsTests();
}

void printTestList(const LuaTest::TestRegistry& registry, const std::string& filter) {
    int count = 0;
    for (const auto& test : registry.getTests()) {
        if (!testMatchesFilter(test, filter)) {
            continue;
        }

        std::cout << test.suiteName << "::" << test.testName << std::endl;
        count++;
    }

    std::cout << "Total: " << count << std::endl;
}

} // namespace

/**
 * @brief 打印测试框架标题
 */
void printHeader() {
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "Lua C++ Interpreter - Unit Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Test Framework: Custom Lightweight Framework" << std::endl;
    std::cout << "Date: 2025-11-14" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

/**
 * @brief 打印测试总结
 */
void printSummary(int registeredTests, int selectedTests, int totalResults, int totalFailed) {
    std::cout << "\n";
    std::cout << "========================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Registered Tests: " << registeredTests << std::endl;
    if (selectedTests != registeredTests) {
        std::cout << "Selected Tests: " << selectedTests << std::endl;
    }
    std::cout << "Total Results: " << totalResults << std::endl;
    std::cout << "Passed: " << (totalResults - totalFailed) << std::endl;
    std::cout << "Failed: " << totalFailed << std::endl;
    std::cout << "========================================" << std::endl;

    if (totalFailed == 0) {
        std::cout << "\n[OK] ALL TESTS PASSED!" << std::endl;
    } else {
        std::cout << "\n[FAILED] SOME TESTS FAILED!" << std::endl;
    }
    std::cout << "\n";
}

/**
 * @brief 主函数
 */
int main(int argc, char** argv) {
    RunnerOptions options;
    if (!parseArgs(argc, argv, options)) {
        printUsage();
        return 2;
    }

    if (options.showHelp) {
        printUsage();
        return 0;
    }

    registerAllTests();

    LuaTest::TestRegistry& registry = LuaTest::TestRegistry::getInstance();
    if (options.list) {
        printTestList(registry, options.filter);
        return 0;
    }

    printHeader();

    // 注册所有测试
    std::cout << "[INFO] All tests registered." << std::endl;
    if (!options.filter.empty()) {
        std::cout << "[INFO] Filter: " << options.filter << std::endl;
    }
    std::cout << "[INFO] Starting test execution...\n" << std::endl;

    // 运行所有测试
    LuaTest::TestRegistry::RunOptions runOptions;
    runOptions.filter = options.filter;
    int failedTests = registry.runTests(runOptions);
    
    // 打印总结
    printSummary(registry.getRegisteredTestCount(), registry.getLastRunTestCount(), registry.getLastTotalCount(), failedTests);

    if (options.writeJunit) {
        if (!registry.writeJUnitReport(options.junitPath)) {
            std::cerr << "[ERROR] Failed to write JUnit report: " << options.junitPath << std::endl;
            return 2;
        }
        std::cout << "[INFO] JUnit report written: " << options.junitPath << std::endl;
    }
    
    // 返回失败测试数量作为退出码
    return failedTests > 0 ? 1 : 0;
}

