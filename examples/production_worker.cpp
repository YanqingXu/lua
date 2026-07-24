/**
 * @file production_worker.cpp
 * @brief 只依赖已安装公开 C API 的有限生产 worker 参考宿主
 */

#include "lua.h"
#include "lauxlib.h"
#include "lua_runtime.h"
#include "lualib.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#elif defined(__linux__)
#include <sys/resource.h>
#endif

namespace {

struct WorkerOptions {
    std::string scriptPath;
    std::uint64_t processMemoryBytes = 512ULL * 1024ULL * 1024ULL;
    std::uint64_t allocatorMemoryBytes = 64ULL * 1024ULL * 1024ULL;
    std::uint64_t instructionBudget = 1'000'000;
    std::uint64_t nativeWorkBudget = 8ULL * 1024ULL * 1024ULL;
    std::uint64_t timeoutMilliseconds = 100;
    std::uint64_t cpuSeconds = 2;
    std::uint64_t maxOutputBytes = 1ULL * 1024ULL * 1024ULL;
};

struct QuotaAllocator {
    size_t limit = 0;
    size_t live = 0;
    size_t peak = 0;
    bool accountingError = false;
};

void* quotaAllocate(void* userData, void* pointer, size_t oldSize, size_t newSize) noexcept {
    auto* quota = static_cast<QuotaAllocator*>(userData);
    if (quota == nullptr) {
        return nullptr;
    }

    if (pointer == nullptr) {
        oldSize = 0;
    }
    if (oldSize > quota->live) {
        quota->accountingError = true;
        return nullptr;
    }

    if (newSize == 0) {
        std::free(pointer);
        quota->live -= oldSize;
        return nullptr;
    }

    const size_t retained = quota->live - oldSize;
    if (newSize > quota->limit || retained > quota->limit - newSize) {
        return nullptr;
    }

    void* result = std::realloc(pointer, newSize);
    if (result == nullptr) {
        return nullptr;
    }

    quota->live = retained + newSize;
    quota->peak = std::max(quota->peak, quota->live);
    return result;
}

bool parseUnsigned(std::string_view text, std::uint64_t& value) {
    if (text.empty()) {
        return false;
    }
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

bool multiplyMegabytes(std::uint64_t megabytes, std::uint64_t& bytes) {
    constexpr std::uint64_t bytesPerMegabyte = 1024ULL * 1024ULL;
    if (megabytes > std::numeric_limits<std::uint64_t>::max() / bytesPerMegabyte) {
        return false;
    }
    bytes = megabytes * bytesPerMegabyte;
    return true;
}

bool readOptionValue(int argc, char** argv, int& index, std::string_view option, std::uint64_t& value,
                     std::string& error) {
    if (index + 1 >= argc) {
        error = std::string(option) + " requires a value";
        return false;
    }
    ++index;
    if (!parseUnsigned(argv[index], value)) {
        error = std::string(option) + " requires an unsigned integer";
        return false;
    }
    return true;
}

bool parseOptions(int argc, char** argv, WorkerOptions& options, std::string& error) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        std::uint64_t value = 0;
        if (argument == "--process-memory-mb") {
            if (!readOptionValue(argc, argv, index, argument, value, error) ||
                !multiplyMegabytes(value, options.processMemoryBytes)) {
                error = "invalid --process-memory-mb value";
                return false;
            }
        } else if (argument == "--allocator-memory-mb") {
            if (!readOptionValue(argc, argv, index, argument, value, error) ||
                !multiplyMegabytes(value, options.allocatorMemoryBytes)) {
                error = "invalid --allocator-memory-mb value";
                return false;
            }
        } else if (argument == "--instruction-budget") {
            if (!readOptionValue(argc, argv, index, argument, options.instructionBudget, error)) {
                return false;
            }
        } else if (argument == "--native-work-budget") {
            if (!readOptionValue(argc, argv, index, argument, options.nativeWorkBudget, error)) {
                return false;
            }
        } else if (argument == "--timeout-ms") {
            if (!readOptionValue(argc, argv, index, argument, options.timeoutMilliseconds, error)) {
                return false;
            }
        } else if (argument == "--cpu-seconds") {
            if (!readOptionValue(argc, argv, index, argument, options.cpuSeconds, error)) {
                return false;
            }
        } else if (argument == "--max-output-bytes") {
            if (!readOptionValue(argc, argv, index, argument, options.maxOutputBytes, error)) {
                return false;
            }
        } else if (!argument.empty() && argument.front() == '-') {
            error = "unknown option: " + std::string(argument);
            return false;
        } else if (!options.scriptPath.empty()) {
            error = "exactly one script path is required";
            return false;
        } else {
            options.scriptPath = std::string(argument);
        }
    }

    if (options.scriptPath.empty()) {
        error = "script path is required";
        return false;
    }
    if (options.processMemoryBytes == 0 || options.allocatorMemoryBytes == 0 || options.cpuSeconds == 0) {
        error = "memory and CPU limits must be non-zero";
        return false;
    }
    if (options.processMemoryBytes > static_cast<std::uint64_t>(std::numeric_limits<size_t>::max()) ||
        options.allocatorMemoryBytes > static_cast<std::uint64_t>(std::numeric_limits<size_t>::max()) ||
        options.maxOutputBytes > static_cast<std::uint64_t>(std::numeric_limits<size_t>::max())) {
        error = "configured limit exceeds this platform's size_t";
        return false;
    }
    return true;
}

#if defined(_WIN32)
HANDLE gWorkerJob = nullptr;
#endif

bool applyProcessLimits(const WorkerOptions& options, std::string& error) {
#if defined(_WIN32)
    gWorkerJob = CreateJobObjectW(nullptr, nullptr);
    if (gWorkerJob == nullptr) {
        error = "CreateJobObjectW failed: " + std::to_string(GetLastError());
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_PROCESS_MEMORY | JOB_OBJECT_LIMIT_PROCESS_TIME;
    limits.ProcessMemoryLimit = static_cast<SIZE_T>(options.processMemoryBytes);
    if (options.cpuSeconds > static_cast<std::uint64_t>(std::numeric_limits<LONGLONG>::max() / 10'000'000LL)) {
        error = "CPU limit is too large";
        return false;
    }
    limits.BasicLimitInformation.PerProcessUserTimeLimit.QuadPart =
        static_cast<LONGLONG>(options.cpuSeconds) * 10'000'000LL;

    if (SetInformationJobObject(gWorkerJob, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) == 0) {
        error = "SetInformationJobObject failed: " + std::to_string(GetLastError());
        return false;
    }
    if (AssignProcessToJobObject(gWorkerJob, GetCurrentProcess()) == 0) {
        error = "AssignProcessToJobObject failed: " + std::to_string(GetLastError());
        return false;
    }
    return true;
#elif defined(__linux__)
    const rlimit addressSpace{static_cast<rlim_t>(options.processMemoryBytes),
                              static_cast<rlim_t>(options.processMemoryBytes)};
    if (setrlimit(RLIMIT_AS, &addressSpace) != 0) {
        error = "setrlimit(RLIMIT_AS) failed";
        return false;
    }

    const rlimit cpu{static_cast<rlim_t>(options.cpuSeconds), static_cast<rlim_t>(options.cpuSeconds)};
    if (setrlimit(RLIMIT_CPU, &cpu) != 0) {
        error = "setrlimit(RLIMIT_CPU) failed";
        return false;
    }

    constexpr rlim_t fileDescriptorLimit = 64;
    const rlimit fileDescriptors{fileDescriptorLimit, fileDescriptorLimit};
    if (setrlimit(RLIMIT_NOFILE, &fileDescriptors) != 0) {
        error = "setrlimit(RLIMIT_NOFILE) failed";
        return false;
    }
    return true;
#else
    (void)options;
    error = "hard process limits require a Windows Job Object or Linux rlimit";
    return false;
#endif
}

bool readScript(const WorkerOptions& options, size_t maxSourceBytes, std::string& source, std::string& error) {
    std::ifstream input(options.scriptPath, std::ios::binary | std::ios::ate);
    if (!input) {
        error = "cannot open script";
        return false;
    }

    const std::streamoff end = input.tellg();
    if (end < 0 || static_cast<std::uint64_t>(end) > static_cast<std::uint64_t>(maxSourceBytes)) {
        error = "script exceeds configured source limit";
        return false;
    }
    source.resize(static_cast<size_t>(end));
    input.seekg(0, std::ios::beg);
    if (!source.empty() && !input.read(source.data(), static_cast<std::streamsize>(source.size()))) {
        error = "cannot read script";
        return false;
    }
    return true;
}

std::string jsonEscape(std::string_view value) {
    static constexpr char hexDigits[] = "0123456789abcdef";
    std::string escaped;
    escaped.reserve(value.size());
    for (const unsigned char byte : value) {
        switch (byte) {
        case '"':
            escaped += "\\\"";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\b':
            escaped += "\\b";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\t':
            escaped += "\\t";
            break;
        default:
            if (byte >= 0x20U && byte <= 0x7EU) {
                escaped.push_back(static_cast<char>(byte));
            } else {
                escaped += "\\u00";
                escaped.push_back(hexDigits[(byte >> 4U) & 0x0FU]);
                escaped.push_back(hexDigits[byte & 0x0FU]);
            }
            break;
        }
    }
    return escaped;
}

std::string classifyOutcome(int status, std::string_view message, const lua_RuntimeMetrics& metrics) {
    if (status == LUA_OK) {
        return "success";
    }
    switch (metrics.last_stop_reason) {
    case LUA_RUNTIME_STOP_INSTRUCTION_BUDGET:
        return "instruction_budget";
    case LUA_RUNTIME_STOP_NATIVE_WORK_BUDGET:
        return "native_work_budget";
    case LUA_RUNTIME_STOP_DEADLINE:
        return "deadline";
    case LUA_RUNTIME_STOP_CANCELLED:
        return "cancelled";
    default:
        break;
    }
    if (status == LUA_ERRMEM) {
        return "allocator_limit";
    }
    if (message == "execution instruction budget exceeded") {
        return "instruction_budget";
    }
    if (message == "execution native work budget exceeded") {
        return "native_work_budget";
    }
    if (message == "execution deadline exceeded") {
        return "deadline";
    }
    if (message == "execution cancelled") {
        return "cancelled";
    }
    if (message.find("resource limit") != std::string_view::npos) {
        return "resource_limit";
    }
    return status == LUA_ERRSYNTAX ? "compile_error" : "runtime_error";
}

void writeResult(std::string_view outcome, int luaStatus, int runtimeStatus, std::uint64_t durationMicroseconds,
                 const WorkerOptions& options, const QuotaAllocator& quota, const lua_RuntimeMetrics& metrics,
                 std::string_view message) {
    std::cout << "{\"schema\":1,\"outcome\":\"" << outcome << "\",\"lua_status\":" << luaStatus
              << ",\"runtime_status\":" << runtimeStatus << ",\"duration_us\":" << durationMicroseconds
              << ",\"allocator_live_bytes\":" << quota.live << ",\"allocator_peak_bytes\":" << quota.peak
              << ",\"allocator_limit_bytes\":" << options.allocatorMemoryBytes
              << ",\"process_memory_limit_bytes\":" << options.processMemoryBytes
              << ",\"instruction_budget\":" << options.instructionBudget
              << ",\"native_work_budget\":" << options.nativeWorkBudget
              << ",\"timeout_ms\":" << options.timeoutMilliseconds
              << ",\"consumed_instructions\":" << metrics.consumed_instructions
              << ",\"remaining_instruction_budget\":" << metrics.remaining_instruction_budget
              << ",\"consumed_native_work\":" << metrics.consumed_native_work
              << ",\"remaining_native_work_budget\":" << metrics.remaining_native_work_budget
              << ",\"last_stop_reason\":" << metrics.last_stop_reason << ",\"message\":\"" << jsonEscape(message)
              << "\"}\n";
}

} // namespace

int main(int argc, char** argv) {
    WorkerOptions options;
    std::string error;
    if (!parseOptions(argc, argv, options, error)) {
        std::cerr << "lua_production_worker: " << error << '\n';
        return 64;
    }
    if (!applyProcessLimits(options, error)) {
        std::cerr << "lua_production_worker: process isolation unavailable: " << error << '\n';
        return 70;
    }

    lua_RuntimeConfig config{};
    lua_runtime_config_init_gameserver(&config);
    config.max_output_bytes = static_cast<size_t>(options.maxOutputBytes);

    std::string source;
    if (!readScript(options, config.max_source_bytes, source, error)) {
        std::cerr << "lua_production_worker: " << error << '\n';
        return 66;
    }

    QuotaAllocator quota{static_cast<size_t>(options.allocatorMemoryBytes)};
    lua_RuntimeMetrics metrics{};
    lua_runtime_metrics_init(&metrics);
    int runtimeStatus = LUA_RUNTIME_OK;
    lua_State* L = lua_newstate_configured(quotaAllocate, &quota, &config, &runtimeStatus);
    if (L == nullptr) {
        writeResult("state_create_error", LUA_ERRMEM, runtimeStatus, 0, options, quota, metrics,
                    "state creation failed");
        return 71;
    }
    luaL_openlibs(L);

    const auto started = std::chrono::steady_clock::now();
    int luaStatus = luaL_loadbuffer(L, source.data(), source.size(), options.scriptPath.c_str());
    if (luaStatus == LUA_OK) {
        lua_RuntimeExecutionLimits limits{};
        lua_runtime_execution_limits_init(&limits);
        limits.instruction_budget = options.instructionBudget;
        limits.native_work_budget = options.nativeWorkBudget;
        limits.finalizer_budget_per_drain = 128;
        limits.timeout_ms = options.timeoutMilliseconds;
        runtimeStatus = lua_runtime_begin_execution(L, &limits);
        if (runtimeStatus == LUA_RUNTIME_OK) {
            luaStatus = lua_pcall(L, 0, 0, 0);
        } else {
            luaStatus = LUA_ERRRUN;
        }
    }

    std::string message;
    if (luaStatus != LUA_OK && lua_gettop(L) > 0) {
        size_t length = 0;
        const char* text = lua_tolstring(L, -1, &length);
        if (text != nullptr) {
            message.assign(text, length);
        }
    }
    const int metricsStatus = lua_runtime_get_metrics(L, &metrics);
    const std::string outcome = metricsStatus != LUA_RUNTIME_OK
                                    ? "metrics_error"
                                    : (runtimeStatus == LUA_RUNTIME_OK ? classifyOutcome(luaStatus, message, metrics)
                                                                       : "execution_config_error");
    const auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started);

    lua_close(L);
    if (quota.live != 0 || quota.accountingError) {
        writeResult("allocator_accounting_error", luaStatus, runtimeStatus,
                    static_cast<std::uint64_t>(duration.count()), options, quota, metrics,
                    "allocator did not return to zero");
        return 72;
    }

    writeResult(outcome, luaStatus, runtimeStatus, static_cast<std::uint64_t>(duration.count()), options, quota,
                metrics, message);
    return luaStatus == LUA_OK && runtimeStatus == LUA_RUNTIME_OK && metricsStatus == LUA_RUNTIME_OK ? 0 : 20;
}
