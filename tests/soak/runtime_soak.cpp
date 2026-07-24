/**
 * @file runtime_soak.cpp
 * @brief 生产运行时生命周期、弱表、终结器、取消与多 State 长稳驱动
 */

#include "lua.h"
#include "lauxlib.h"
#include "lua_runtime.h"
#include "lualib.h"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace {

using Clock = std::chrono::steady_clock;

struct Options {
    std::uint64_t iterations = 1000;
    std::uint64_t durationSeconds = 0;
    std::uint64_t maxCancellationLatencyMilliseconds = 250;
    std::string jsonPath;
};

struct AllocatorProbe {
    size_t limit = 64U * 1024U * 1024U;
    size_t live = 0;
    size_t peak = 0;
    bool accountingError = false;
};

struct StateFixture {
    AllocatorProbe allocator;
    lua_State* state = nullptr;
};

struct SoakMetrics {
    std::uint64_t iterations = 0;
    std::uint64_t statesCreated = 0;
    std::uint64_t statesClosed = 0;
    std::uint64_t coroutineCycles = 0;
    std::uint64_t weakValuesCollected = 0;
    std::uint64_t finalizersObserved = 0;
    std::uint64_t cancellationChecks = 0;
    std::uint64_t maxCancellationLatencyMicroseconds = 0;
    size_t maxAllocatorPeakBytes = 0;
};

std::atomic<std::uint64_t> gFinalizerCalls{0};

void* quotaAllocate(void* userData, void* pointer, size_t oldSize, size_t newSize) noexcept {
    auto* probe = static_cast<AllocatorProbe*>(userData);
    if (probe == nullptr) {
        return nullptr;
    }
    if (pointer == nullptr) {
        oldSize = 0;
    }
    if (oldSize > probe->live) {
        probe->accountingError = true;
        return nullptr;
    }

    if (newSize == 0) {
        std::free(pointer);
        probe->live -= oldSize;
        return nullptr;
    }

    const size_t retained = probe->live - oldSize;
    if (newSize > probe->limit || retained > probe->limit - newSize) {
        return nullptr;
    }

    void* result = std::realloc(pointer, newSize);
    if (result == nullptr) {
        return nullptr;
    }
    probe->live = retained + newSize;
    probe->peak = std::max(probe->peak, probe->live);
    return result;
}

int countFinalizer(lua_State*) {
    gFinalizerCalls.fetch_add(1, std::memory_order_relaxed);
    return 0;
}

int makeFinalizedUserdata(lua_State* L) {
    (void)lua_newuserdata(L, 32);
    lua_createtable(L, 0, 1);
    lua_pushstring(L, "__gc");
    lua_pushcclosure(L, countFinalizer, 0);
    lua_settable(L, -3);
    (void)lua_setmetatable(L, -2);
    return 1;
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

bool readValue(int argc, char** argv, int& index, std::uint64_t& value, std::string& error) {
    if (index + 1 >= argc) {
        error = std::string(argv[index]) + " requires a value";
        return false;
    }
    ++index;
    if (!parseUnsigned(argv[index], value)) {
        error = std::string(argv[index - 1]) + " requires an unsigned integer";
        return false;
    }
    return true;
}

bool parseOptions(int argc, char** argv, Options& options, std::string& error) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        if (argument == "--iterations") {
            if (!readValue(argc, argv, index, options.iterations, error)) {
                return false;
            }
        } else if (argument == "--duration-seconds") {
            if (!readValue(argc, argv, index, options.durationSeconds, error)) {
                return false;
            }
        } else if (argument == "--max-cancel-latency-ms") {
            if (!readValue(argc, argv, index, options.maxCancellationLatencyMilliseconds, error)) {
                return false;
            }
        } else if (argument == "--json") {
            if (index + 1 >= argc) {
                error = "--json requires a path";
                return false;
            }
            options.jsonPath = argv[++index];
        } else {
            error = "unknown option: " + std::string(argument);
            return false;
        }
    }
    if (options.iterations == 0 && options.durationSeconds == 0) {
        error = "iterations and duration cannot both be zero";
        return false;
    }
    if (options.maxCancellationLatencyMilliseconds == 0) {
        error = "cancellation latency threshold must be non-zero";
        return false;
    }
    return true;
}

std::string luaError(lua_State* L, std::string_view operation) {
    std::string message(operation);
    message += ": ";
    const char* detail = lua_tostring(L, -1);
    message += detail != nullptr ? detail : "unknown Lua error";
    return message;
}

std::unique_ptr<StateFixture> createState(SoakMetrics& metrics) {
    auto fixture = std::make_unique<StateFixture>();
    lua_RuntimeConfig config{};
    lua_runtime_config_init_gameserver(&config);

    int runtimeStatus = LUA_RUNTIME_OK;
    fixture->state = lua_newstate_configured(quotaAllocate, &fixture->allocator, &config, &runtimeStatus);
    if (fixture->state == nullptr) {
        throw std::runtime_error("configured State creation failed with runtime status " +
                                 std::to_string(runtimeStatus));
    }
    luaL_openlibs(fixture->state);
    ++metrics.statesCreated;
    return fixture;
}

void closeState(std::unique_ptr<StateFixture>& fixture, SoakMetrics& metrics) {
    if (fixture == nullptr || fixture->state == nullptr) {
        return;
    }
    lua_close(fixture->state);
    fixture->state = nullptr;
    ++metrics.statesClosed;
    metrics.maxAllocatorPeakBytes = std::max(metrics.maxAllocatorPeakBytes, fixture->allocator.peak);
    if (fixture->allocator.live != 0 || fixture->allocator.accountingError) {
        throw std::runtime_error("State close violated allocator accounting");
    }
}

void beginWindow(lua_State* L, std::uint64_t instructionBudget, std::uint64_t timeoutMilliseconds) {
    lua_RuntimeExecutionLimits limits{};
    lua_runtime_execution_limits_init(&limits);
    limits.instruction_budget = instructionBudget;
    limits.native_work_budget = 16U * 1024U * 1024U;
    limits.finalizer_budget_per_drain = 256;
    limits.timeout_ms = timeoutMilliseconds;
    const int status = lua_runtime_begin_execution(L, &limits);
    if (status != LUA_RUNTIME_OK) {
        throw std::runtime_error("cannot begin execution window: " + std::to_string(status));
    }
}

void runLifecycleWorkload(lua_State* L, SoakMetrics& metrics) {
    static constexpr char source[] = R"lua(
        local weak_values = setmetatable({}, { __mode = "v" })
        do
            local temporary = { payload = string.rep("w", 256) }
            weak_values[1] = temporary
        end

        for index = 1, 16 do
            local worker = coroutine.create(function(value)
                local resumed = coroutine.yield(value + 1)
                return resumed + 1
            end)
            local ok, yielded = coroutine.resume(worker, index)
            assert(ok and yielded == index + 1)
            local completed, result = coroutine.resume(worker, yielded)
            assert(completed and result == index + 2)
        end

        for _ = 1, 32 do
            new_finalized_userdata()
        end

        soak_marker = "primary"
        return weak_values
    )lua";

    lua_pushcclosure(L, makeFinalizedUserdata, 0);
    lua_setglobal(L, "new_finalized_userdata");

    const std::uint64_t finalizersBefore = gFinalizerCalls.load(std::memory_order_relaxed);
    if (luaL_loadbuffer(L, source, sizeof(source) - 1, "=(runtime soak)") != LUA_OK) {
        throw std::runtime_error(luaError(L, "soak workload compile"));
    }
    beginWindow(L, 1'000'000, 1000);
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        throw std::runtime_error(luaError(L, "soak workload execute"));
    }
    if (!lua_istable(L, -1)) {
        throw std::runtime_error("soak workload did not return its weak table");
    }

    (void)lua_gc(L, LUA_GCCOLLECT, 0);
    lua_rawgeti(L, -1, 1);
    if (!lua_isnil(L, -1)) {
        throw std::runtime_error("weak table retained an unreachable value");
    }
    lua_pop(L, 2);

    const std::uint64_t finalizersAfter = gFinalizerCalls.load(std::memory_order_relaxed);
    if (finalizersAfter - finalizersBefore != 32) {
        throw std::runtime_error("GC did not run every expected userdata finalizer");
    }
    metrics.finalizersObserved += 32;
    metrics.coroutineCycles += 16;
    ++metrics.weakValuesCollected;
}

void verifyContextIsolation(lua_State* primary, lua_State* secondary) {
    lua_getglobal(primary, "soak_marker");
    const char* primaryMarker = lua_tostring(primary, -1);
    if (primaryMarker == nullptr || std::string_view(primaryMarker) != "primary") {
        throw std::runtime_error("primary State lost its global marker");
    }
    lua_pop(primary, 1);

    lua_getglobal(secondary, "soak_marker");
    if (!lua_isnil(secondary, -1)) {
        throw std::runtime_error("independent State observed another context's global");
    }
    lua_pop(secondary, 1);
    lua_pushstring(secondary, "secondary");
    lua_setglobal(secondary, "soak_marker");
}

void verifyCancellation(lua_State* L, const Options& options, SoakMetrics& metrics) {
    if (luaL_loadstring(L, "while true do end") != LUA_OK) {
        throw std::runtime_error(luaError(L, "cancellation workload compile"));
    }
    beginWindow(L, LUA_RUNTIME_UNLIMITED, 5000);

    int runtimeStatus = LUA_RUNTIME_OK;
    lua_CancellationHandle* handle = lua_runtime_get_cancellation_handle(L, &runtimeStatus);
    if (handle == nullptr || runtimeStatus != LUA_RUNTIME_OK) {
        throw std::runtime_error("cannot obtain cancellation handle");
    }

    const auto started = Clock::now();
    std::thread canceller([handle]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        lua_runtime_request_cancellation(handle);
    });
    const int status = lua_pcall(L, 0, 0, 0);
    canceller.join();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - started);

    const char* message = lua_tostring(L, -1);
    if (status != LUA_ERRRUN || message == nullptr || std::string_view(message) != "execution cancelled") {
        lua_runtime_release_cancellation_handle(handle);
        throw std::runtime_error("cancellation workload did not stop with the stable cancellation error");
    }
    lua_pop(L, 1);
    lua_runtime_release_cancellation_handle(handle);

    const std::uint64_t latency = static_cast<std::uint64_t>(elapsed.count());
    metrics.maxCancellationLatencyMicroseconds = std::max(metrics.maxCancellationLatencyMicroseconds, latency);
    ++metrics.cancellationChecks;
    if (latency > options.maxCancellationLatencyMilliseconds * 1000ULL) {
        throw std::runtime_error("cancellation latency exceeded the configured SLO");
    }
}

void runIteration(const Options& options, SoakMetrics& metrics) {
    std::unique_ptr<StateFixture> primary = createState(metrics);
    std::unique_ptr<StateFixture> secondary = createState(metrics);
    try {
        runLifecycleWorkload(primary->state, metrics);
        verifyContextIsolation(primary->state, secondary->state);
        verifyCancellation(primary->state, options, metrics);
        closeState(secondary, metrics);
        closeState(primary, metrics);
    } catch (...) {
        closeState(secondary, metrics);
        closeState(primary, metrics);
        throw;
    }
}

std::string jsonEscape(std::string_view value) {
    std::string result;
    for (const char character : value) {
        if (character == '"' || character == '\\') {
            result.push_back('\\');
        }
        if (character == '\n') {
            result += "\\n";
        } else if (static_cast<unsigned char>(character) >= 0x20U) {
            result.push_back(character);
        }
    }
    return result;
}

std::string makeReport(std::string_view status, const SoakMetrics& metrics, std::uint64_t durationMilliseconds,
                       std::string_view error) {
    return "{\"schema\":1,\"status\":\"" + std::string(status) +
           "\",\"iterations\":" + std::to_string(metrics.iterations) +
           ",\"states_created\":" + std::to_string(metrics.statesCreated) +
           ",\"states_closed\":" + std::to_string(metrics.statesClosed) +
           ",\"coroutine_cycles\":" + std::to_string(metrics.coroutineCycles) +
           ",\"weak_values_collected\":" + std::to_string(metrics.weakValuesCollected) +
           ",\"finalizers_observed\":" + std::to_string(metrics.finalizersObserved) +
           ",\"cancellation_checks\":" + std::to_string(metrics.cancellationChecks) +
           ",\"max_cancellation_latency_us\":" + std::to_string(metrics.maxCancellationLatencyMicroseconds) +
           ",\"max_allocator_peak_bytes\":" + std::to_string(metrics.maxAllocatorPeakBytes) +
           ",\"duration_ms\":" + std::to_string(durationMilliseconds) + ",\"error\":\"" + jsonEscape(error) + "\"}\n";
}

void publishReport(const Options& options, const std::string& report) {
    std::cout << report;
    if (!options.jsonPath.empty()) {
        std::ofstream output(options.jsonPath, std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error("cannot open soak JSON output");
        }
        output << report;
        if (!output) {
            throw std::runtime_error("cannot write soak JSON output");
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    std::string optionError;
    if (!parseOptions(argc, argv, options, optionError)) {
        std::cerr << "lua_runtime_soak: " << optionError << '\n';
        return 64;
    }

    SoakMetrics metrics;
    const auto started = Clock::now();
    try {
        while (
            (options.iterations == 0 || metrics.iterations < options.iterations) &&
            (options.durationSeconds == 0 || Clock::now() - started < std::chrono::seconds(options.durationSeconds))) {
            runIteration(options, metrics);
            ++metrics.iterations;
            if (metrics.iterations % 100 == 0) {
                std::cerr << "lua_runtime_soak: completed " << metrics.iterations << " iterations\n";
            }
        }

        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started);
        if (metrics.statesCreated != metrics.statesClosed) {
            throw std::runtime_error("created and closed State counts differ");
        }
        publishReport(options, makeReport("passed", metrics, static_cast<std::uint64_t>(duration.count()), ""));
    } catch (const std::exception& error) {
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started);
        const std::string report =
            makeReport("failed", metrics, static_cast<std::uint64_t>(duration.count()), error.what());
        try {
            publishReport(options, report);
        } catch (...) {
            std::cerr << report;
        }
        return 1;
    }
    return 0;
}
