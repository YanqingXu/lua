#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "debug/trace_sink.hpp"
#include "gc/garbage_collector.hpp"
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifndef LUA_RUNTIME_BENCH_CONTEXT_LOCAL_TRACE
#define LUA_RUNTIME_BENCH_CONTEXT_LOCAL_TRACE 1
#endif

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::size_t kRequiredClosureCount = 100000;
constexpr std::size_t kRequiredCiGcPauseSamples = 10000;
constexpr std::size_t kHeapAbsoluteGrowthAllowanceBytes = std::size_t{64} * 1024;
constexpr std::size_t kHeapGrowthAllowanceDivisor = 10;
constexpr std::size_t kHeapMinimumSlopeAllowanceBytesPerMillionFrames = std::size_t{256} * 1024;
constexpr double kBytesPerMiB = 1024.0 * 1024.0;

struct Config {
    std::string profile = "ci";
    std::filesystem::path jsonPath = "runtime-bench.json";
    std::size_t samples = 3;
    std::size_t parseIterations = 1;
    std::size_t vmIterations = 100000;
    std::size_t cppToLuaCalls = 2000;
    std::size_t luaToCppCalls = 20000;
    std::size_t coroutineYields = 1000;
    std::size_t tableIterations = 50000;
    std::size_t closureSamples = 1;
    std::size_t gcPauseFrames = kRequiredCiGcPauseSamples;
    std::size_t heapWarmupFrames = 1000;
    std::size_t heapFrames = 20000;
    int gcStepSize = 4;
};

struct Metric {
    std::string name;
    std::string unit;
    std::string direction;
    std::vector<double> samples;
};

struct HeapCheckpoint {
    std::size_t frame = 0;
    std::size_t allocatorLiveBytes = 0;
    std::size_t gcManagedBytes = 0;
    std::size_t gcObjectCount = 0;
};

struct Report {
    Config config;
    std::vector<Metric> metrics;
    std::vector<double> gcPauseSamplesUs;
    std::vector<HeapCheckpoint> heapCheckpoints;
    std::size_t closureCount = 0;
    std::size_t gcCycles = 0;
    std::size_t heapGcCycles = 0;
    std::size_t heapBaselineBytes = 0;
    std::size_t heapFinalBytes = 0;
    std::size_t heapAllowedGrowthBytes = 0;
    std::size_t heapMaxGrowthBytesPerMillionFrames = 0;
    std::size_t allocatorLiveAfterClose = 0;
    double heapGrowthBytesPerMillionFrames = 0.0;
    bool heapStable = false;
};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

double elapsedSeconds(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double>(end - start).count();
}

double median(std::vector<double> values) {
    require(!values.empty(), "cannot compute the median of an empty sample set");
    std::sort(values.begin(), values.end());
    const std::size_t middle = values.size() / 2;
    if ((values.size() % 2) != 0) {
        return values[middle];
    }
    return (values[middle - 1] + values[middle]) / 2.0;
}

double nearestRankPercentile(std::vector<double> values, double percentile) {
    require(!values.empty(), "cannot compute a percentile of an empty sample set");
    require(percentile > 0.0 && percentile <= 1.0, "percentile must be in (0, 1]");
    std::sort(values.begin(), values.end());
    const double rank = std::ceil(percentile * static_cast<double>(values.size()));
    const std::size_t index = static_cast<std::size_t>(rank) - 1;
    return values[std::min(index, values.size() - 1)];
}

void addMetric(Report& report, std::string name, std::string unit, std::string direction, std::vector<double> samples) {
    require(!samples.empty(), "metric " + name + " has no samples");
    for (double sample : samples) {
        require(std::isfinite(sample), "metric " + name + " contains a non-finite sample");
    }
    report.metrics.push_back(Metric{std::move(name), std::move(unit), std::move(direction), std::move(samples)});
}

std::string luaError(lua_State* state, const std::string& operation) {
    const char* message = lua_tostring(state, -1);
    return operation + " failed: " + (message != nullptr ? message : "non-string Lua error");
}

void requireLuaStatus(lua_State* state, int status, const std::string& operation) {
    if (status != LUA_OK) {
        fail(luaError(state, operation));
    }
}

class LuaStateOwner {
public:
    explicit LuaStateOwner(lua_State* state) : state_(state) {
        require(state_ != nullptr, "failed to create Lua state");
    }

    ~LuaStateOwner() {
        close();
    }

    LuaStateOwner(const LuaStateOwner&) = delete;
    LuaStateOwner& operator=(const LuaStateOwner&) = delete;

    [[nodiscard]] lua_State* get() const noexcept {
        return state_;
    }

    void close() noexcept {
        if (state_ != nullptr) {
            lua_close(state_);
            state_ = nullptr;
        }
    }

private:
    lua_State* state_;
};

struct CountingAllocator {
    std::size_t liveBytes = 0;
    std::size_t peakBytes = 0;
    std::size_t grantedBytes = 0;
    std::size_t allocationCalls = 0;
    std::size_t freeCalls = 0;
    bool accountingError = false;

    void resetPeak() noexcept {
        peakBytes = liveBytes;
    }
};

void* countingAllocator(void* userData, void* pointer, std::size_t oldSize, std::size_t newSize) {
    auto* probe = static_cast<CountingAllocator*>(userData);
    if (newSize == 0) {
        if (pointer != nullptr) {
            if (oldSize > probe->liveBytes) {
                probe->accountingError = true;
                probe->liveBytes = 0;
            } else {
                probe->liveBytes -= oldSize;
            }
            ++probe->freeCalls;
            std::free(pointer);
        }
        return nullptr;
    }

    void* result = std::realloc(pointer, newSize);
    if (result == nullptr) {
        return nullptr;
    }

    if (pointer != nullptr) {
        if (oldSize > probe->liveBytes) {
            probe->accountingError = true;
            probe->liveBytes = 0;
        } else {
            probe->liveBytes -= oldSize;
        }
    }
    probe->liveBytes += newSize;
    probe->peakBytes = std::max(probe->peakBytes, probe->liveBytes);
    probe->grantedBytes += newSize;
    ++probe->allocationCalls;
    return result;
}

Lua::LuaState* internalState(lua_State* state) {
    return reinterpret_cast<Lua::LuaState*>(state);
}

int loadReturnedFunction(lua_State* state, std::string_view source, std::string_view name) {
    requireLuaStatus(state, luaL_loadbuffer(state, source.data(), source.size(), std::string(name).c_str()),
                     "load function factory");
    requireLuaStatus(state, lua_pcall(state, 0, 1, 0), "run function factory");
    require(lua_isfunction(state, -1) != 0, "function factory did not return a function");
    return luaL_ref(state, LUA_REGISTRYINDEX);
}

double expectedVmChecksum(std::size_t iterations) {
    double total = 0.0;
    for (std::size_t i = 1; i <= iterations; ++i) {
        total += static_cast<double>(i % 7);
    }
    return total;
}

double invokeNumberFunction(lua_State* state, int reference, double argument, const std::string& operation) {
    const int base = lua_gettop(state);
    luaL_getref(state, reference);
    require(lua_isfunction(state, -1) != 0, operation + " registry reference is not a function");
    lua_pushnumber(state, argument);
    requireLuaStatus(state, lua_pcall(state, 1, 1, 0), operation);
    require(lua_isnumber(state, -1) != 0, operation + " did not return a number");
    const double result = lua_tonumber(state, -1);
    lua_pop(state, 1);
    require(lua_gettop(state) == base, operation + " did not restore the host stack");
    return result;
}

std::string makeCompilerFixture() {
    std::string source;
    source.reserve(std::size_t{3} * 1024);
    std::size_t block = 0;
    while (source.size() < std::size_t{2} * 1024) {
        source += "do\n";
        source += "  local seed = " + std::to_string(block + 1) + "\n";
        source += "  local values = { seed, seed + 1, label = \"fixture\" }\n";
        source += "  local function transform(value)\n";
        source += "    if value % 2 == 0 then return value / 2 end\n";
        source += "    return value * 3 + 1\n";
        source += "  end\n";
        source += "  for i = 1, 16 do values[i] = transform(seed + i) end\n";
        source += "  if values[1] > 0 then seed = values[1] else seed = 0 end\n";
        source += "end\n";
        ++block;
    }
    source += "return true\n";
    return source;
}

void benchmarkParseCompile(Report& report) {
    std::cerr << "[bench] parse/compile throughput\n";
    const std::string source = makeCompilerFixture();
    std::vector<double> throughput;
    throughput.reserve(report.config.samples);

    Lua::EngineContext context;
    Lua::RuntimeServices services = context.services();

    for (std::size_t sample = 0; sample < report.config.samples; ++sample) {
        std::size_t generatedInstructions = 0;
        const auto start = Clock::now();
        for (std::size_t iteration = 0; iteration < report.config.parseIterations; ++iteration) {
            Lua::Parser parser(source, services);
            auto parsed = parser.parse();
            require(parsed.has_value(), "compiler fixture failed to parse");
            Lua::CodeGenerator generator(services);
            auto generated = generator.tryGenerate(*parsed, "runtime_bench_fixture");
            require(generated.has_value() && *generated != nullptr, "compiler fixture failed code generation");
            generatedInstructions += (*generated)->getCode().size();
        }
        const auto end = Clock::now();
        require(generatedInstructions > 0, "compiler fixture generated no instructions");
        const double seconds = elapsedSeconds(start, end);
        const double bytes = static_cast<double>(source.size() * report.config.parseIterations);
        throughput.push_back((bytes / kBytesPerMiB) / seconds);
        (void)context.gc().collect(context.strings());
    }

    addMetric(report, "parse_compile_mib_per_second", "MiB/s", "higher", std::move(throughput));
}

class CountingTraceSink final : public Lua::ITraceSink {
public:
    void onInstruction(const Lua::TraceEvent&) override {
        ++instructions;
    }
    void onCall(const Lua::TraceEvent&) override {}
    void onReturn(const Lua::TraceEvent&) override {}
    void onError(const Lua::TraceEvent&) override {}
    void flush() override {}

    std::uint64_t instructions = 0;
};

class TraceScope {
public:
#if LUA_RUNTIME_BENCH_CONTEXT_LOCAL_TRACE
    TraceScope(lua_State* state, Lua::ITraceSink* sink)
        : services_(reinterpret_cast<Lua::LuaState*>(state)->getGlobalState()) {
        Lua::VM::setTraceDiffEnabled(services_, false);
        Lua::VM::setTraceSink(services_, sink);
    }
    ~TraceScope() {
        Lua::VM::setTraceSink(services_, nullptr);
    }

private:
    Lua::RuntimeServices services_;
#else
    TraceScope(lua_State*, Lua::ITraceSink* sink) {
        Lua::VM::setTraceDiffEnabled(false);
        Lua::VM::setTraceSink(sink);
    }
    ~TraceScope() {
        Lua::VM::setTraceSink(nullptr);
    }
#endif
};

std::uint64_t countVmInstructions(lua_State* state, int reference, std::size_t iterations) {
    CountingTraceSink sink;
    {
        TraceScope trace(state, &sink);
        const double result =
            invokeNumberFunction(state, reference, static_cast<double>(iterations), "VM instruction calibration");
        require(result == expectedVmChecksum(iterations), "VM instruction calibration checksum mismatch");
    }
    return sink.instructions;
}

void benchmarkVmDispatch(Report& report) {
    std::cerr << "[bench] VM instructions per second\n";
    constexpr std::string_view source = R"lua(
return function(count)
  local total = 0
  for i = 1, count do
    total = total + (i % 7)
  end
  return total
end
)lua";

    LuaStateOwner owner(lua_open());
    lua_State* state = owner.get();
    const int functionReference = loadReturnedFunction(state, source, "=runtime_bench_vm");

    const std::uint64_t count100 = countVmInstructions(state, functionReference, 100);
    const std::uint64_t count101 = countVmInstructions(state, functionReference, 101);
    require(count101 > count100, "VM instruction count did not increase with loop iterations");
    const std::uint64_t instructionsPerIteration = count101 - count100;
    require(count100 >= instructionsPerIteration * 100, "VM instruction calibration intercept underflow");
    const std::uint64_t fixedInstructions = count100 - instructionsPerIteration * 100;
    const std::uint64_t count1000 = countVmInstructions(state, functionReference, 1000);
    require(count1000 == fixedInstructions + instructionsPerIteration * 1000,
            "VM instruction count is not linear for the deterministic loop fixture");

    std::vector<double> rates;
    rates.reserve(report.config.samples);
    const std::uint64_t measuredInstructions =
        fixedInstructions + instructionsPerIteration * report.config.vmIterations;
    for (std::size_t sample = 0; sample < report.config.samples; ++sample) {
        const auto start = Clock::now();
        const double result = invokeNumberFunction(
            state, functionReference, static_cast<double>(report.config.vmIterations), "VM throughput run");
        const auto end = Clock::now();
        require(result == expectedVmChecksum(report.config.vmIterations), "VM throughput checksum mismatch");
        rates.push_back(static_cast<double>(measuredInstructions) / elapsedSeconds(start, end));
    }

    luaL_unref(state, LUA_REGISTRYINDEX, functionReference);
    addMetric(report, "vm_instructions_per_second", "instructions/s", "higher", std::move(rates));
}

void benchmarkCppToLua(Report& report) {
    std::cerr << "[bench] C++ -> Lua protected call cost\n";
    constexpr std::string_view source = "return function(value) return value + 1 end";
    LuaStateOwner owner(lua_open());
    lua_State* state = owner.get();
    const int functionReference = loadReturnedFunction(state, source, "=runtime_bench_cpp_to_lua");

    std::vector<double> costs;
    costs.reserve(report.config.samples);
    for (std::size_t sample = 0; sample < report.config.samples; ++sample) {
        double checksum = 0.0;
        const auto start = Clock::now();
        for (std::size_t call = 1; call <= report.config.cppToLuaCalls; ++call) {
            checksum += invokeNumberFunction(state, functionReference, static_cast<double>(call), "C++ to Lua call");
        }
        const auto end = Clock::now();
        const double count = static_cast<double>(report.config.cppToLuaCalls);
        const double expected = count * (count + 1.0) / 2.0 + count;
        require(checksum == expected, "C++ to Lua checksum mismatch");
        costs.push_back(elapsedSeconds(start, end) * 1.0e9 / count);
    }

    luaL_unref(state, LUA_REGISTRYINDEX, functionReference);
    addMetric(report, "cpp_to_lua_ns_per_call", "ns/call", "lower", std::move(costs));
}

std::uint64_t gHostCallCount = 0;

int hostIncrement(lua_State* state) {
    const double value = lua_tonumber(state, 1);
    ++gHostCallCount;
    lua_pushnumber(state, value + 1.0);
    return 1;
}

void benchmarkLuaToCpp(Report& report) {
    std::cerr << "[bench] Lua -> C++ call cost\n";
    constexpr std::string_view source = R"lua(
local host = host_increment
return function(count)
  local total = 0
  for i = 1, count do total = total + host(i) end
  return total
end
)lua";

    LuaStateOwner owner(lua_open());
    lua_State* state = owner.get();
    lua_pushcclosure(state, hostIncrement, 0);
    lua_setglobal(state, "host_increment");
    const int functionReference = loadReturnedFunction(state, source, "=runtime_bench_lua_to_cpp");

    std::vector<double> costs;
    costs.reserve(report.config.samples);
    for (std::size_t sample = 0; sample < report.config.samples; ++sample) {
        gHostCallCount = 0;
        const auto start = Clock::now();
        const double result = invokeNumberFunction(
            state, functionReference, static_cast<double>(report.config.luaToCppCalls), "Lua to C++ call loop");
        const auto end = Clock::now();
        require(gHostCallCount == report.config.luaToCppCalls, "Lua to C++ host call count mismatch");
        const double count = static_cast<double>(report.config.luaToCppCalls);
        const double expected = count * (count + 1.0) / 2.0 + count;
        require(result == expected, "Lua to C++ checksum mismatch");
        costs.push_back(elapsedSeconds(start, end) * 1.0e9 / count);
    }

    luaL_unref(state, LUA_REGISTRYINDEX, functionReference);
    addMetric(report, "lua_to_cpp_ns_per_call", "ns/call", "lower", std::move(costs));
}

void benchmarkCoroutine(Report& report) {
    std::cerr << "[bench] coroutine resume/yield cost\n";
    LuaStateOwner owner(lua_open());
    lua_State* mainState = owner.get();
    luaL_openlibs(mainState);

    std::vector<double> costs;
    costs.reserve(report.config.samples);
    for (std::size_t sample = 0; sample < report.config.samples; ++sample) {
        lua_State* child = lua_newthread(mainState);
        require(child != nullptr, "failed to create benchmark coroutine");
        const std::string source = "for i = 1, " + std::to_string(report.config.coroutineYields) +
                                   " do coroutine.yield(i) end return " + std::to_string(report.config.coroutineYields);
        requireLuaStatus(child, luaL_loadbuffer(child, source.data(), source.size(), "=runtime_bench_coroutine"),
                         "load coroutine fixture");

        double yieldingSeconds = 0.0;
        for (std::size_t expectedYield = 1; expectedYield <= report.config.coroutineYields; ++expectedYield) {
            const auto start = Clock::now();
            const int status = lua_resume(child, 0);
            const auto end = Clock::now();
            yieldingSeconds += elapsedSeconds(start, end);
            require(status == LUA_YIELD, "coroutine did not yield at the expected boundary");
            require(lua_gettop(child) == 1, "coroutine yield did not expose exactly one result");
            require(lua_tonumber(child, -1) == static_cast<double>(expectedYield),
                    "coroutine yielded an unexpected sequence value");
            lua_settop(child, 0);
        }

        const int completionStatus = lua_resume(child, 0);
        require(completionStatus == LUA_OK, "coroutine did not complete after its final yield");
        require(lua_gettop(child) == 1, "completed coroutine did not expose exactly one return value");
        require(lua_tonumber(child, -1) == static_cast<double>(report.config.coroutineYields),
                "coroutine completion checksum mismatch");
        costs.push_back(yieldingSeconds * 1.0e9 / static_cast<double>(report.config.coroutineYields));
        lua_pop(mainState, 1);
    }

    addMetric(report, "coroutine_resume_yield_ns", "ns/round-trip", "lower", std::move(costs));
}

double expectedTableChecksum(std::size_t iterations) {
    std::vector<double> array(257, 0.0);
    std::vector<double> hash(65, 0.0);
    for (std::size_t i = 1; i <= 256; ++i) {
        array[i] = static_cast<double>(i);
    }
    for (std::size_t i = 1; i <= 64; ++i) {
        hash[i] = static_cast<double>(i);
    }

    double total = 0.0;
    for (std::size_t i = 1; i <= iterations; ++i) {
        const std::size_t arrayIndex = (i % 256) + 1;
        const std::size_t hashIndex = (i % 64) + 1;
        total += array[arrayIndex] + hash[hashIndex];
        array[arrayIndex] += 1.0;
        hash[hashIndex] += 1.0;
    }
    return total;
}

void benchmarkTableHotReadWrite(Report& report) {
    std::cerr << "[bench] table hot read/write\n";
    constexpr std::string_view source = R"lua(
local array, hash, keys = {}, {}, {}
for i = 1, 64 do keys[i] = "key_" .. i end
local function reset()
  for i = 1, 256 do array[i] = i end
  for i = 1, 64 do hash[keys[i]] = i end
end
local function run(count)
  local total = 0
  for i = 1, count do
    local array_index = (i % 256) + 1
    local array_value = array[array_index]
    array[array_index] = array_value + 1
    local hash_index = (i % 64) + 1
    local key = keys[hash_index]
    local hash_value = hash[key]
    hash[key] = hash_value + 1
    total = total + array_value + hash_value
  end
  return total
end
reset()
return run, reset
)lua";

    LuaStateOwner owner(lua_open());
    lua_State* state = owner.get();
    requireLuaStatus(state, luaL_loadbuffer(state, source.data(), source.size(), "=runtime_bench_table"),
                     "load table fixture");
    requireLuaStatus(state, lua_pcall(state, 0, 2, 0), "run table fixture factory");
    require(lua_isfunction(state, -2) != 0 && lua_isfunction(state, -1) != 0,
            "table fixture did not return run/reset functions");
    const int resetReference = luaL_ref(state, LUA_REGISTRYINDEX);
    const int runReference = luaL_ref(state, LUA_REGISTRYINDEX);

    const double expected = expectedTableChecksum(report.config.tableIterations);
    std::vector<double> rates;
    rates.reserve(report.config.samples);
    for (std::size_t sample = 0; sample < report.config.samples; ++sample) {
        luaL_getref(state, resetReference);
        requireLuaStatus(state, lua_pcall(state, 0, 0, 0), "reset table fixture");
        const auto start = Clock::now();
        const double result = invokeNumberFunction(
            state, runReference, static_cast<double>(report.config.tableIterations), "table hot read/write loop");
        const auto end = Clock::now();
        require(result == expected, "table hot read/write checksum mismatch");
        const double operations = static_cast<double>(report.config.tableIterations) * 4.0;
        rates.push_back(operations / elapsedSeconds(start, end));
    }

    luaL_unref(state, LUA_REGISTRYINDEX, resetReference);
    luaL_unref(state, LUA_REGISTRYINDEX, runReference);
    addMetric(report, "table_operations_per_second", "operations/s", "higher", std::move(rates));
}

void benchmarkClosureLifecycle(Report& report) {
    std::cerr << "[bench] 100000 closure/upvalue lifecycle\n";
    constexpr std::string_view source = R"lua(
return function(count)
  local function make(value)
    return function() return value end
  end
  local values = {}
  for i = 1, count do values[i] = make(i) end
  return values, values[1](), values[count / 2](), values[count]()
end
)lua";

    CountingAllocator allocator;
    LuaStateOwner owner(lua_newstate(countingAllocator, &allocator));
    lua_State* state = owner.get();
    const int functionReference = loadReturnedFunction(state, source, "=runtime_bench_closure");
    Lua::LuaState* internal = internalState(state);
    Lua::GarbageCollector& gc = internal->getGlobalState().getGC();
    (void)gc.collect(internal);
    const std::size_t baselineObjects = gc.getObjectCount();

    std::vector<double> lifecycleRates;
    std::vector<double> allocationRates;
    lifecycleRates.reserve(report.config.closureSamples);
    allocationRates.reserve(report.config.closureSamples);

    for (std::size_t sample = 0; sample < report.config.closureSamples; ++sample) {
        const std::size_t grantedBefore = allocator.grantedBytes;
        allocator.resetPeak();
        const auto start = Clock::now();
        luaL_getref(state, functionReference);
        lua_pushnumber(state, static_cast<double>(kRequiredClosureCount));
        requireLuaStatus(state, lua_pcall(state, 1, 4, 0), "create 100000 captured closures");
        const auto created = Clock::now();
        require(lua_istable(state, -4) != 0,
                "closure lifecycle did not return its retaining table (top=" + std::to_string(lua_gettop(state)) +
                    ", types=" + lua_typename(state, lua_type(state, -4)) + "," +
                    lua_typename(state, lua_type(state, -3)) + "," + lua_typename(state, lua_type(state, -2)) + "," +
                    lua_typename(state, lua_type(state, -1)) + ")");
        require(lua_tonumber(state, -3) == 1.0, "first closure captured the wrong upvalue");
        require(lua_tonumber(state, -2) == static_cast<double>(kRequiredClosureCount / 2),
                "middle closure captured the wrong upvalue");
        require(lua_tonumber(state, -1) == static_cast<double>(kRequiredClosureCount),
                "last closure captured the wrong upvalue");
        lua_pop(state, 4);
        (void)gc.collect(internal);
        const auto reclaimed = Clock::now();
        require(gc.getObjectCount() <= baselineObjects + 8,
                "100000 closure lifecycle left unreachable GC objects behind");
        require(!allocator.accountingError, "allocator accounting failed during closure lifecycle");

        const double lifecycleSeconds = elapsedSeconds(start, reclaimed);
        const double allocationSeconds = elapsedSeconds(start, created);
        lifecycleRates.push_back(static_cast<double>(kRequiredClosureCount) / lifecycleSeconds);
        const double granted = static_cast<double>(allocator.grantedBytes - grantedBefore);
        allocationRates.push_back((granted / kBytesPerMiB) / allocationSeconds);
    }

    report.closureCount = kRequiredClosureCount;
    luaL_unref(state, LUA_REGISTRYINDEX, functionReference);
    owner.close();
    require(!allocator.accountingError, "allocator old-size accounting failed while closing closure state");
    require(allocator.liveBytes == 0, "closure benchmark allocator retained bytes after lua_close");

    addMetric(report, "closure_upvalue_lifecycle_per_second", "closures/s", "higher", std::move(lifecycleRates));
    addMetric(report, "allocation_mib_per_second", "MiB/s", "higher", std::move(allocationRates));
}

int loadTransientFrameFunction(lua_State* state, std::string_view name) {
    constexpr std::string_view source = R"lua(
return function(frame)
  local checksum = 0
  for i = 1, 4 do
    local transient = { frame, i, frame + i }
    checksum = checksum + transient[3]
  end
  return checksum
end
)lua";
    return loadReturnedFunction(state, source, name);
}

double expectedTransientChecksum(std::size_t frame) {
    return static_cast<double>(frame * 4 + 10);
}

void benchmarkGcPause(Report& report) {
    std::cerr << "[bench] fixed-budget per-frame GC pause distribution\n";
    CountingAllocator allocator;
    LuaStateOwner owner(lua_newstate(countingAllocator, &allocator));
    lua_State* state = owner.get();
    const int functionReference = loadTransientFrameFunction(state, "=runtime_bench_gc_pause");
    Lua::LuaState* internal = internalState(state);
    Lua::GarbageCollector& gc = internal->getGlobalState().getGC();
    gc.stopAutomatic();

    report.gcPauseSamplesUs.reserve(report.config.gcPauseFrames);
    std::size_t completedCycles = 0;
    for (std::size_t frame = 1; frame <= report.config.gcPauseFrames; ++frame) {
        const double result =
            invokeNumberFunction(state, functionReference, static_cast<double>(frame), "GC frame allocation fixture");
        require(result == expectedTransientChecksum(frame), "GC frame checksum mismatch");

        const auto start = Clock::now();
        const bool completed = gc.step(internal, report.config.gcStepSize);
        const auto end = Clock::now();
        report.gcPauseSamplesUs.push_back(elapsedSeconds(start, end) * 1.0e6);
        if (completed) {
            ++completedCycles;
        }
    }

    require(report.gcPauseSamplesUs.size() >= kRequiredCiGcPauseSamples,
            "GC pause distribution has fewer than 10000 frame samples");
    require(completedCycles > 0, "fixed-budget GC did not complete a collection cycle");
    report.gcCycles = completedCycles;

    const double p50 = nearestRankPercentile(report.gcPauseSamplesUs, 0.50);
    const double p95 = nearestRankPercentile(report.gcPauseSamplesUs, 0.95);
    const double p99 = nearestRankPercentile(report.gcPauseSamplesUs, 0.99);
    const double maximum = *std::max_element(report.gcPauseSamplesUs.begin(), report.gcPauseSamplesUs.end());
    require(p50 <= p95 && p95 <= p99 && p99 <= maximum, "GC pause percentiles are not monotonic");

    addMetric(report, "gc_pause_p50_us", "us", "lower", {p50});
    addMetric(report, "gc_pause_p95_us", "us", "lower", {p95});
    addMetric(report, "gc_pause_p99_us", "us", "lower", {p99});
    addMetric(report, "gc_pause_max_us", "us", "lower", {maximum});

    luaL_unref(state, LUA_REGISTRYINDEX, functionReference);
    (void)gc.collect(internal);
    owner.close();
    require(!allocator.accountingError, "allocator accounting failed during GC pause benchmark");
    require(allocator.liveBytes == 0, "GC pause allocator retained bytes after lua_close");
}

int loadHeapStabilityFunction(lua_State* state) {
    constexpr std::string_view source = R"lua(
local retained = {}
for i = 1, 128 do retained[i] = { i, i * 2 } end
return function(frame)
  local checksum = 0
  for i = 1, 4 do
    local transient = { frame, i, frame + i }
    checksum = checksum + transient[3]
  end
  local slot = (frame % 128) + 1
  retained[slot][1] = frame
  return checksum + retained[slot][2]
end
)lua";
    return loadReturnedFunction(state, source, "=runtime_bench_heap_stability");
}

double expectedHeapChecksum(std::size_t frame) {
    const std::size_t slot = (frame % 128) + 1;
    return expectedTransientChecksum(frame) + static_cast<double>(slot * 2);
}

HeapCheckpoint captureHeapCheckpoint(std::size_t frame, const CountingAllocator& allocator,
                                     const Lua::GarbageCollector& gc) {
    return HeapCheckpoint{frame, allocator.liveBytes, gc.getTotalMemory(), gc.getObjectCount()};
}

double heapSlopeBytesPerMillionFrames(const std::vector<HeapCheckpoint>& checkpoints) {
    require(checkpoints.size() >= 3, "heap stability needs at least three checkpoints");
    const std::size_t begin = checkpoints.size() / 5;
    const std::size_t count = checkpoints.size() - begin;
    double meanFrame = 0.0;
    double meanBytes = 0.0;
    for (std::size_t i = begin; i < checkpoints.size(); ++i) {
        meanFrame += static_cast<double>(checkpoints[i].frame);
        meanBytes += static_cast<double>(checkpoints[i].allocatorLiveBytes);
    }
    meanFrame /= static_cast<double>(count);
    meanBytes /= static_cast<double>(count);

    double numerator = 0.0;
    double denominator = 0.0;
    for (std::size_t i = begin; i < checkpoints.size(); ++i) {
        const double x = static_cast<double>(checkpoints[i].frame) - meanFrame;
        const double y = static_cast<double>(checkpoints[i].allocatorLiveBytes) - meanBytes;
        numerator += x * y;
        denominator += x * x;
    }
    require(denominator > 0.0, "heap checkpoint frames do not span an interval");
    return (numerator / denominator) * 1.0e6;
}

void benchmarkHeapStability(Report& report) {
    std::cerr << "[bench] long-running heap stability\n";
    CountingAllocator allocator;
    LuaStateOwner owner(lua_newstate(countingAllocator, &allocator));
    lua_State* state = owner.get();
    const int functionReference = loadHeapStabilityFunction(state);
    Lua::LuaState* internal = internalState(state);
    Lua::GarbageCollector& gc = internal->getGlobalState().getGC();
    gc.stopAutomatic();

    for (std::size_t frame = 1; frame <= report.config.heapWarmupFrames; ++frame) {
        const double result =
            invokeNumberFunction(state, functionReference, static_cast<double>(frame), "heap stability warmup");
        require(result == expectedHeapChecksum(frame), "heap warmup checksum mismatch");
        (void)gc.step(internal, report.config.gcStepSize);
    }
    (void)gc.collect(internal);
    report.heapBaselineBytes = allocator.liveBytes;

    const std::size_t checkpointInterval = std::max<std::size_t>(1, report.config.heapFrames / 100);
    report.heapCheckpoints.push_back(captureHeapCheckpoint(0, allocator, gc));
    std::size_t completedCycles = 0;
    std::size_t nextCheckpointFrame = checkpointInterval;
    for (std::size_t frame = 1; frame <= report.config.heapFrames; ++frame) {
        const double result =
            invokeNumberFunction(state, functionReference, static_cast<double>(frame), "heap stability frame");
        require(result == expectedHeapChecksum(frame), "heap stability checksum mismatch");
        const bool completed = gc.step(internal, report.config.gcStepSize);
        if (completed) {
            ++completedCycles;
            if (frame >= nextCheckpointFrame) {
                report.heapCheckpoints.push_back(captureHeapCheckpoint(frame, allocator, gc));
                nextCheckpointFrame = frame + checkpointInterval;
            }
        }
    }
    require(completedCycles > 0, "heap stability workload completed no incremental GC cycle");
    report.heapGcCycles = completedCycles;

    (void)gc.collect(internal);
    report.heapFinalBytes = allocator.liveBytes;
    if (report.heapCheckpoints.back().frame == report.config.heapFrames) {
        report.heapCheckpoints.back() = captureHeapCheckpoint(report.config.heapFrames, allocator, gc);
    } else {
        report.heapCheckpoints.push_back(captureHeapCheckpoint(report.config.heapFrames, allocator, gc));
    }
    report.heapGrowthBytesPerMillionFrames = heapSlopeBytesPerMillionFrames(report.heapCheckpoints);
    report.heapAllowedGrowthBytes =
        std::max(kHeapAbsoluteGrowthAllowanceBytes, report.heapBaselineBytes / kHeapGrowthAllowanceDivisor);
    report.heapMaxGrowthBytesPerMillionFrames =
        std::max(kHeapMinimumSlopeAllowanceBytesPerMillionFrames, report.heapBaselineBytes);
    const bool finalSizeStable = report.heapFinalBytes <= report.heapBaselineBytes ||
                                 report.heapFinalBytes - report.heapBaselineBytes <= report.heapAllowedGrowthBytes;
    const bool trendStable =
        report.heapGrowthBytesPerMillionFrames <= static_cast<double>(report.heapMaxGrowthBytesPerMillionFrames);
    report.heapStable = finalSizeStable && trendStable;
    require(report.heapStable, "heap did not return to its warmed stable range after a full collection");

    addMetric(report, "heap_growth_bytes_per_million_frames", "bytes/1M-frames", "lower",
              {report.heapGrowthBytesPerMillionFrames});
    luaL_unref(state, LUA_REGISTRYINDEX, functionReference);
    owner.close();
    report.allocatorLiveAfterClose = allocator.liveBytes;
    require(!allocator.accountingError, "allocator accounting failed during heap stability benchmark");
    require(report.allocatorLiveAfterClose == 0, "heap benchmark allocator retained bytes after lua_close");
}

std::string jsonEscape(std::string_view value) {
    std::ostringstream output;
    for (unsigned char character : value) {
        switch (character) {
        case '\"':
            output << "\\\"";
            break;
        case '\\':
            output << "\\\\";
            break;
        case '\b':
            output << "\\b";
            break;
        case '\f':
            output << "\\f";
            break;
        case '\n':
            output << "\\n";
            break;
        case '\r':
            output << "\\r";
            break;
        case '\t':
            output << "\\t";
            break;
        default:
            if (character < 0x20) {
                output << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(character)
                       << std::dec;
            } else {
                output << static_cast<char>(character);
            }
        }
    }
    return output.str();
}

std::string compilerName() {
#if defined(__clang__)
    return "Clang " __clang_version__;
#elif defined(__GNUC__)
    return "GCC " __VERSION__;
#elif defined(_MSC_VER)
    return "MSVC " + std::to_string(_MSC_VER);
#else
    return "unknown";
#endif
}

std::string operatingSystemName() {
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#else
    return "unknown";
#endif
}

std::string gitSha() {
#if defined(_WIN32)
    char* sha = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&sha, &length, "GITHUB_SHA") != 0 || sha == nullptr) {
        return "unknown";
    }
    std::string result(sha);
    std::free(sha);
    return result;
#else
    const char* sha = std::getenv("GITHUB_SHA");
    return sha != nullptr ? sha : "unknown";
#endif
}

void writeDoubleArray(std::ostream& output, const std::vector<double>& values) {
    output << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            output << ',';
        }
        output << std::setprecision(17) << values[i];
    }
    output << ']';
}

void writeReport(const Report& report) {
    std::filesystem::path parent = report.config.jsonPath.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    std::ofstream output(report.config.jsonPath, std::ios::binary | std::ios::trunc);
    require(output.is_open(), "cannot open benchmark JSON output: " + report.config.jsonPath.string());

    output << "{\n";
    output << "  \"schema_version\": 1,\n";
    output << "  \"success\": true,\n";
    output << "  \"profile\": \"" << jsonEscape(report.config.profile) << "\",\n";
#ifdef NDEBUG
    output << "  \"build_type\": \"Release\",\n";
#else
    output << "  \"build_type\": \"Debug\",\n";
#endif
    output << "  \"compiler\": \"" << jsonEscape(compilerName()) << "\",\n";
    output << "  \"os\": \"" << jsonEscape(operatingSystemName()) << "\",\n";
    output << "  \"git_sha\": \"" << jsonEscape(gitSha()) << "\",\n";
    output << "  \"sample_count\": " << report.config.samples << ",\n";
    output << "  \"closure_count\": " << report.closureCount << ",\n";
    output << "  \"gc_pause_sample_count\": " << report.gcPauseSamplesUs.size() << ",\n";
    output << "  \"gc_step_size\": " << report.config.gcStepSize << ",\n";
    output << "  \"gc_cycles\": " << report.gcCycles << ",\n";
    output << "  \"heap_gc_cycles\": " << report.heapGcCycles << ",\n";
    output << "  \"workload\": {\n";
    output << "    \"timing_samples\": " << report.config.samples << ",\n";
    output << "    \"parse_iterations\": " << report.config.parseIterations << ",\n";
    output << "    \"vm_iterations\": " << report.config.vmIterations << ",\n";
    output << "    \"cpp_to_lua_calls\": " << report.config.cppToLuaCalls << ",\n";
    output << "    \"lua_to_cpp_calls\": " << report.config.luaToCppCalls << ",\n";
    output << "    \"coroutine_yields\": " << report.config.coroutineYields << ",\n";
    output << "    \"table_iterations\": " << report.config.tableIterations << ",\n";
    output << "    \"closure_samples\": " << report.config.closureSamples << ",\n";
    output << "    \"closure_count\": " << report.closureCount << ",\n";
    output << "    \"gc_pause_frames\": " << report.config.gcPauseFrames << ",\n";
    output << "    \"gc_step_size\": " << report.config.gcStepSize << ",\n";
    output << "    \"heap_warmup_frames\": " << report.config.heapWarmupFrames << ",\n";
    output << "    \"heap_frames\": " << report.config.heapFrames << "\n";
    output << "  },\n";
    output << "  \"metrics\": [\n";
    for (std::size_t i = 0; i < report.metrics.size(); ++i) {
        const Metric& metric = report.metrics[i];
        output << "    {\"name\":\"" << jsonEscape(metric.name) << "\",\"unit\":\"" << jsonEscape(metric.unit)
               << "\",\"direction\":\"" << jsonEscape(metric.direction) << "\",\"median\":" << std::setprecision(17)
               << median(metric.samples) << ",\"samples\":";
        writeDoubleArray(output, metric.samples);
        output << '}';
        if (i + 1 != report.metrics.size()) {
            output << ',';
        }
        output << '\n';
    }
    output << "  ],\n";
    output << "  \"gc_pause_samples_us\": ";
    writeDoubleArray(output, report.gcPauseSamplesUs);
    output << ",\n";
    output << "  \"heap\": {\n";
    output << "    \"stable\": " << (report.heapStable ? "true" : "false") << ",\n";
    output << "    \"checkpoint_policy\": \"completed_gc_cycle\",\n";
    output << "    \"baseline_bytes\": " << report.heapBaselineBytes << ",\n";
    output << "    \"final_bytes\": " << report.heapFinalBytes << ",\n";
    output << "    \"allowed_growth_bytes\": " << report.heapAllowedGrowthBytes << ",\n";
    output << "    \"max_growth_bytes_per_million_frames\": " << report.heapMaxGrowthBytesPerMillionFrames << ",\n";
    output << "    \"allocator_live_after_close\": " << report.allocatorLiveAfterClose << ",\n";
    output << "    \"growth_bytes_per_million_frames\": " << std::setprecision(17)
           << report.heapGrowthBytesPerMillionFrames << ",\n";
    output << "    \"checkpoints\": [\n";
    for (std::size_t i = 0; i < report.heapCheckpoints.size(); ++i) {
        const HeapCheckpoint& checkpoint = report.heapCheckpoints[i];
        output << "      {\"frame\":" << checkpoint.frame
               << ",\"allocator_live_bytes\":" << checkpoint.allocatorLiveBytes
               << ",\"gc_managed_bytes\":" << checkpoint.gcManagedBytes
               << ",\"gc_object_count\":" << checkpoint.gcObjectCount << '}';
        if (i + 1 != report.heapCheckpoints.size()) {
            output << ',';
        }
        output << '\n';
    }
    output << "    ]\n";
    output << "  }\n";
    output << "}\n";
    require(output.good(), "failed while writing benchmark JSON output");
}

Config configForProfile(std::string profile) {
    Config config;
    config.profile = std::move(profile);
    if (config.profile == "ci") {
        return config;
    }
    if (config.profile == "full") {
        config.samples = 7;
        config.parseIterations = 3;
        config.vmIterations = 500000;
        config.cppToLuaCalls = 10000;
        config.luaToCppCalls = 100000;
        config.coroutineYields = 5000;
        config.tableIterations = 250000;
        config.closureSamples = 3;
        config.gcPauseFrames = 30000;
        config.heapWarmupFrames = 5000;
        config.heapFrames = 200000;
        return config;
    }
    if (config.profile == "endurance") {
        config.samples = 7;
        config.parseIterations = 3;
        config.vmIterations = 500000;
        config.cppToLuaCalls = 10000;
        config.luaToCppCalls = 100000;
        config.coroutineYields = 5000;
        config.tableIterations = 250000;
        config.closureSamples = 3;
        config.gcPauseFrames = 100000;
        config.heapWarmupFrames = 10000;
        config.heapFrames = 1000000;
        return config;
    }
    fail("unknown benchmark profile: " + config.profile);
}

Config parseArguments(int argc, char** argv) {
    std::string profile = "ci";
    std::filesystem::path jsonPath = "runtime-bench.json";
    std::size_t samplesOverride = 0;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        auto requireValue = [&](const std::string& option) -> std::string {
            if (i + 1 >= argc) {
                fail(option + " requires a value");
            }
            return argv[++i];
        };
        if (argument == "--profile") {
            profile = requireValue(argument);
        } else if (argument == "--json") {
            jsonPath = requireValue(argument);
        } else if (argument == "--samples") {
            samplesOverride = static_cast<std::size_t>(std::stoull(requireValue(argument)));
            require(samplesOverride > 0, "--samples must be greater than zero");
        } else if (argument == "--help") {
            std::cout << "usage: lua_runtime_bench [--profile ci|full|endurance] [--samples N] [--json PATH]\n";
            std::exit(0);
        } else {
            fail("unknown benchmark argument: " + argument);
        }
    }

    Config config = configForProfile(profile);
    config.jsonPath = std::move(jsonPath);
    if (samplesOverride != 0) {
        config.samples = samplesOverride;
    }
    require(config.closureSamples >= 1, "benchmark profile must execute the closure lifecycle");
    require(config.gcPauseFrames >= kRequiredCiGcPauseSamples,
            "benchmark profile must retain at least 10000 GC pause samples");
    return config;
}

void runBenchmarks(Report& report) {
    benchmarkParseCompile(report);
    benchmarkVmDispatch(report);
    benchmarkCppToLua(report);
    benchmarkLuaToCpp(report);
    benchmarkCoroutine(report);
    benchmarkTableHotReadWrite(report);
    benchmarkClosureLifecycle(report);
    benchmarkGcPause(report);
    benchmarkHeapStability(report);
}

void printSummary(const Report& report) {
    std::cout << "runtime benchmark profile: " << report.config.profile << '\n';
    for (const Metric& metric : report.metrics) {
        std::cout << "  " << metric.name << ": " << std::setprecision(8) << median(metric.samples) << ' ' << metric.unit
                  << '\n';
    }
    std::cout << "  closure_count: " << report.closureCount << '\n';
    std::cout << "  gc_pause_samples: " << report.gcPauseSamplesUs.size() << '\n';
    std::cout << "  heap_stable: " << (report.heapStable ? "true" : "false") << '\n';
    std::cout << "benchmark JSON: " << report.config.jsonPath.string() << '\n';
}

} // namespace

int main(int argc, char** argv) {
    try {
        Report report;
        report.config = parseArguments(argc, argv);
        runBenchmarks(report);
        writeReport(report);
        printSummary(report);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "runtime benchmark failed: " << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "runtime benchmark failed: unknown exception\n";
        return 1;
    }
}
