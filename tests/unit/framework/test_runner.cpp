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
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cstring>
#include <sys/resource.h>
#endif

// 声明所有测试注册函数
extern void registerValueTests();
extern void registerGCStringTests();
extern void registerTableTests();
extern void registerVMCoreTests();
extern void registerVMDispatchTests();
extern void registerVMInternalBoundaryTests();
extern void registerFunctionCallTests();
extern void registerFunctionTests();
extern void registerGCTests();
extern void registerLuaStateInitTests();
extern void registerRuntimeServicesTests();
extern void registerVMTraceDebugTests();
extern void registerAstVisitorTests();
extern void registerBinaryUnaryExprTests();
extern void registerFunctionCodegenTests();
extern void registerCodegenCharacterizationTests();
extern void registerCodegenConditionTests();
extern void registerCodegenMultiRetTests();
extern void registerCodegenResultTypeTests();
extern void registerCodegenStateTests();
extern void registerJumpPatcherTests();
extern void registerScopeManagerTests();
extern void registerExpressionEmitterTests();
extern void registerStatementEmitterTests();
extern void registerBytecodeBuilderTests();
extern void registerBytecodePrinterTests();
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
extern void registerParserBoundaryTests();
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
extern void registerReplCommandTests();
extern void registerOfficialSuiteTests();

namespace {

constexpr const char* kDefaultJunitReportPath = "lua_test_junit.xml";
constexpr std::size_t kDefaultMemoryLimitMb = 512;
constexpr std::size_t kMinMemoryLimitMb = 64;

struct RunnerOptions {
    bool list = false;
    bool showHelp = false;
    std::string filter;
    std::string excludeFilter;
    bool writeJunit = false;
    std::string junitPath = kDefaultJunitReportPath;
    bool memoryLimitEnabled = true;
    std::size_t memoryLimitMb = kDefaultMemoryLimitMb;
};

#ifdef _WIN32
HANDLE gMemoryLimitJob = nullptr;
#endif

bool startsWith(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

std::string toLower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return text;
}

bool containsCaseInsensitive(const std::string& haystack, const std::string& needle) {
    return needle.empty() || toLower(haystack).find(toLower(needle)) != std::string::npos;
}

bool parseMemoryLimitMb(const std::string& text, std::size_t& value) {
    if (text.empty()) {
        return false;
    }

    errno = 0;
    char* end = nullptr;
    unsigned long long parsed = std::strtoull(text.c_str(), &end, 10);
    if (errno != 0 || end == text.c_str() || *end != '\0' || parsed < kMinMemoryLimitMb ||
        parsed > (std::numeric_limits<std::size_t>::max() / (1024ull * 1024ull))) {
        return false;
    }

    value = static_cast<std::size_t>(parsed);
    return true;
}

std::string readEnvironmentVariable(const char* name) {
#ifdef _WIN32
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) {
        return "";
    }

    std::string result(value);
    std::free(value);
    return result;
#else
    const char* value = std::getenv(name);
    return value == nullptr ? "" : value;
#endif
}

bool envVarIsTruthy(const std::string& value) {
    if (value.empty()) {
        return false;
    }

    std::string normalized = toLower(value);
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}

bool loadMemoryLimitFromEnvironment(RunnerOptions& options) {
    if (envVarIsTruthy(readEnvironmentVariable("LUA_TEST_DISABLE_MEMORY_LIMIT"))) {
        options.memoryLimitEnabled = false;
        return true;
    }

    const std::string envLimit = readEnvironmentVariable("LUA_TEST_MAX_MEMORY_MB");
    if (envLimit.empty()) {
        return true;
    }

    std::size_t parsed = 0;
    if (!parseMemoryLimitMb(envLimit, parsed)) {
        std::cerr << "error: LUA_TEST_MAX_MEMORY_MB must be an integer >= " << kMinMemoryLimitMb << std::endl;
        return false;
    }

    options.memoryLimitMb = parsed;
    return true;
}

bool installProcessMemoryLimit(std::size_t limitMb, std::string& message) {
    const std::size_t limitBytes = limitMb * 1024ull * 1024ull;

#ifdef _WIN32
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job == nullptr) {
        message = "CreateJobObject failed: " + std::to_string(GetLastError());
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY | JOB_OBJECT_LIMIT_JOB_MEMORY;
    limits.ProcessMemoryLimit = static_cast<SIZE_T>(limitBytes);
    limits.JobMemoryLimit = static_cast<SIZE_T>(limitBytes);

    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        message = "SetInformationJobObject failed: " + std::to_string(GetLastError());
        CloseHandle(job);
        return false;
    }

    if (!AssignProcessToJobObject(job, GetCurrentProcess())) {
        message = "AssignProcessToJobObject failed: " + std::to_string(GetLastError()) +
                  " (use --no-memory-limit only inside an already memory-capped runner)";
        CloseHandle(job);
        return false;
    }

    gMemoryLimitJob = job;
    message = "Process memory limit: " + std::to_string(limitMb) + " MB";
    return true;
#else
    struct rlimit limit{};
    limit.rlim_cur = static_cast<rlim_t>(limitBytes);
    limit.rlim_max = static_cast<rlim_t>(limitBytes);

    if (setrlimit(RLIMIT_AS, &limit) != 0) {
        message = std::string("setrlimit(RLIMIT_AS) failed: ") + std::strerror(errno);
        return false;
    }

    message = "Process memory limit: " + std::to_string(limitMb) + " MB";
    return true;
#endif
}

bool testMatchesFilter(const LuaTest::TestRegistry::TestEntry& test, const std::string& filter) {
    if (filter.empty()) {
        return true;
    }

    const std::string fullName = test.suiteName + "::" + test.testName;
    return containsCaseInsensitive(test.suiteName, filter) || containsCaseInsensitive(test.testName, filter) ||
           containsCaseInsensitive(fullName, filter);
}

bool parseArgs(int argc, char** argv, RunnerOptions& options) {
    if (!loadMemoryLimitFromEnvironment(options)) {
        return false;
    }

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
        } else if (arg == "--exclude-filter") {
            if (index + 1 >= argc) {
                std::cerr << "error: --exclude-filter requires a suite or test name" << std::endl;
                return false;
            }
            options.excludeFilter = argv[++index] ? argv[index] : "";
        } else if (startsWith(arg, "--exclude-filter=")) {
            options.excludeFilter = arg.substr(std::string("--exclude-filter=").size());
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
        } else if (arg == "--max-memory-mb") {
            if (index + 1 >= argc) {
                std::cerr << "error: --max-memory-mb requires an integer >= " << kMinMemoryLimitMb << std::endl;
                return false;
            }

            std::size_t parsed = 0;
            if (!parseMemoryLimitMb(argv[++index] ? argv[index] : "", parsed)) {
                std::cerr << "error: --max-memory-mb requires an integer >= " << kMinMemoryLimitMb << std::endl;
                return false;
            }

            options.memoryLimitEnabled = true;
            options.memoryLimitMb = parsed;
        } else if (startsWith(arg, "--max-memory-mb=")) {
            std::size_t parsed = 0;
            if (!parseMemoryLimitMb(arg.substr(std::string("--max-memory-mb=").size()), parsed)) {
                std::cerr << "error: --max-memory-mb requires an integer >= " << kMinMemoryLimitMb << std::endl;
                return false;
            }

            options.memoryLimitEnabled = true;
            options.memoryLimitMb = parsed;
        } else if (arg == "--no-memory-limit") {
            options.memoryLimitEnabled = false;
        } else {
            std::cerr << "error: unknown option: " << arg << std::endl;
            return false;
        }
    }

    return true;
}

void printUsage() {
    std::cout << "Usage: lua_test.exe [--list] [--filter <suite-or-name>] "
                 "[--exclude-filter <suite-or-name>] [--report=junit]"
              << std::endl;
    std::cout << "       lua_test.exe --report=junit:<path>" << std::endl;
    std::cout << "       lua_test.exe [--max-memory-mb <mb>|--no-memory-limit]" << std::endl;
    std::cout << "Default memory cap: " << kDefaultMemoryLimitMb
              << " MB (env: LUA_TEST_MAX_MEMORY_MB, LUA_TEST_DISABLE_MEMORY_LIMIT=1)" << std::endl;
}

void registerAllTests() {
    registerValueTests();
    registerGCStringTests();
    registerTableTests();
    registerVMCoreTests();
    registerVMDispatchTests();
    registerVMInternalBoundaryTests();
    registerFunctionCallTests();
    registerFunctionTests();
    registerGCTests();
    registerLuaStateInitTests();
    registerRuntimeServicesTests();
    registerVMTraceDebugTests();
    registerAstVisitorTests();
    registerBinaryUnaryExprTests();
    registerFunctionCodegenTests();
    registerCodegenCharacterizationTests();
    registerCodegenConditionTests();
    registerCodegenMultiRetTests();
    registerCodegenResultTypeTests();
    registerCodegenStateTests();
    registerJumpPatcherTests();
    registerScopeManagerTests();
    registerExpressionEmitterTests();
    registerStatementEmitterTests();
    registerBytecodeBuilderTests();
    registerBytecodePrinterTests();
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
    registerParserBoundaryTests();
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
    registerReplCommandTests();
    registerOfficialSuiteTests();
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

    std::string memoryLimitMessage;
    if (options.memoryLimitEnabled) {
        if (!installProcessMemoryLimit(options.memoryLimitMb, memoryLimitMessage)) {
            std::cerr << "error: failed to install lua_test memory cap: " << memoryLimitMessage << std::endl;
            return 2;
        }
    } else {
        memoryLimitMessage = "Process memory limit: disabled";
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
    std::cout << "[INFO] " << memoryLimitMessage << std::endl;
    if (!options.filter.empty()) {
        std::cout << "[INFO] Filter: " << options.filter << std::endl;
    }
    if (!options.excludeFilter.empty()) {
        std::cout << "[INFO] Exclude filter: " << options.excludeFilter << std::endl;
    }
    std::cout << "[INFO] Starting test execution...\n" << std::endl;

    // 运行所有测试
    LuaTest::TestRegistry::RunOptions runOptions;
    runOptions.filter = options.filter;
    runOptions.excludeFilter = options.excludeFilter;
    runOptions.captureSuites = options.writeJunit;
    int failedTests = registry.runTests(runOptions);

    // 打印总结
    printSummary(registry.getRegisteredTestCount(), registry.getLastRunTestCount(), registry.getLastTotalCount(),
                 failedTests);

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
