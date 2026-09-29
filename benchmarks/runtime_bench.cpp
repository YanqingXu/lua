#include "common/types.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "gc/garbage_collector.hpp"
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"

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

namespace {

using Clock = std::chrono::steady_clock;

constexpr Lua::usize kRequiredClosureCount = 100000;
constexpr Lua::usize kRequiredCiGcPauseSamples = 10000;
constexpr Lua::usize kHeapAbsoluteGrowthAllowanceBytes = Lua::usize{64} * 1024;
constexpr Lua::usize kHeapGrowthAllowanceDivisor = 10;
constexpr Lua::usize kHeapMinimumSlopeAllowanceBytesPerMillionFrames = Lua::usize{256} * 1024;
constexpr Lua::f64 kBytesPerMiB = 1024.0 * 1024.0;

struct Config {
    Lua::Str profile = "ci";
    std::filesystem::path jsonPath = "runtime-bench.json";
    Lua::usize samples = 3;
    Lua::usize parseIterations = 1;
    Lua::usize vmIterations = 100000;
    Lua::usize cppToLuaCalls = 2000;
    Lua::usize luaToCppCalls = 20000;
    Lua::usize coroutineYields = 1000;
    Lua::usize tableIterations = 50000;
    Lua::usize closureSamples = 1;
    Lua::usize gcPauseFrames = kRequiredCiGcPauseSamples;
    Lua::usize heapWarmupFrames = 1000;
    Lua::usize heapFrames = 20000;
    int gcStepSize = 4;
};

struct Metric {
    Lua::Str name;
    Lua::Str unit;
    Lua::Str direction;
    Lua::Vec<Lua::f64> samples;
};

struct HeapCheckpoint {
    Lua::usize frame = 0;
    Lua::usize allocatorLiveBytes = 0;
    Lua::usize gcManagedBytes = 0;
    Lua::usize gcObjectCount = 0;
};

struct Report {
    Config config;
    Lua::Vec<Metric> metrics;
    Lua::Vec<Lua::f64> gcPauseSamplesUs;
    Lua::Vec<HeapCheckpoint> heapCheckpoints;
    Lua::usize closureCount = 0;
    Lua::usize gcCycles = 0;
    Lua::usize heapGcCycles = 0;
    Lua::usize heapBaselineBytes = 0;
    Lua::usize heapFinalBytes = 0;
    Lua::usize heapAllowedGrowthBytes = 0;
    Lua::usize heapMaxGrowthBytesPerMillionFrames = 0;
    Lua::usize allocatorLiveAfterClose = 0;
    Lua::f64 heapGrowthBytesPerMillionFrames = 0.0;
    bool heapStable = false;
};

[[noreturn]] void fail(const Lua::Str& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const Lua::Str& message) {
    if (!condition) {
        fail(message);
    }
}

Lua::f64 elapsedSeconds(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<Lua::f64>(end - start).count();
}

Lua::f64 median(Lua::Vec<Lua::f64> values) {
    require(!values.empty(), "cannot compute the median of an empty sample set");
    std::sort(values.begin(), values.end());
    const Lua::usize middle = values.size() / 2;
    if ((values.size() % 2) != 0) {
        return values[middle];
    }
    return (values[middle - 1] + values[middle]) / 2.0;
}

Lua::f64 nearestRankPercentile(Lua::Vec<Lua::f64> values, Lua::f64 percentile) {
    require(!values.empty(), "cannot compute a percentile of an empty sample set");
    require(percentile > 0.0 && percentile <= 1.0, "percentile must be in (0, 1]");
    std::sort(values.begin(), values.end());
    const Lua::f64 rank = std::ceil(percentile * static_cast<Lua::f64>(values.size()));
    const Lua::usize index = static_cast<Lua::usize>(rank) - 1;
    return values[std::min(index, values.size() - 1)];
}

void addMetric(Report& report, Lua::Str name, Lua::Str unit, Lua::Str direction, Lua::Vec<Lua::f64> samples) {
    require(!samples.empty(), "metric " + name + " has no samples");
    for (Lua::f64 sample : samples) {
        require(std::isfinite(sample), "metric " + name + " contains a non-finite sample");
    }
    report.metrics.push_back(Metric{std::move(name), std::move(unit), std::move(direction), std::move(samples)});
}

Lua::Str luaError(lua_State* state, const Lua::Str& operation) {
    Lua::CharPtr message = lua_tostring(state, -1);
    return operation + " failed: " + (message != nullptr ? message : "non-string Lua error");
}

void requireLuaStatus(lua_State* state, int status, const Lua::Str& operation) {
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
    Lua::usize liveBytes = 0;
    Lua::usize peakBytes = 0;
    Lua::usize grantedBytes = 0;
    Lua::usize allocationCalls = 0;
    Lua::usize freeCalls = 0;
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

int loadReturnedFunction(lua_State* state, Lua::StrView source, Lua::StrView name) {
    requireLuaStatus(state, luaL_loadbuffer(state, source.data(), source.size(), Lua::Str(name).c_str()),
                     "load function factory");
    requireLuaStatus(state, lua_pcall(state, 0, 1, 0), "run function factory");
    require(lua_isfunction(state, -1) != 0, "function factory did not return a function");
    return luaL_ref(state, LUA_REGISTRYINDEX);
}

Lua::f64 expectedVmChecksum(Lua::usize iterations) {
    Lua::f64 total = 0.0;
    for (Lua::usize i = 1; i <= iterations; ++i) {
        total += static_cast<Lua::f64>(i % 7);
    }
    return total;
}

Lua::f64 invokeNumberFunction(lua_State* state, int reference, Lua::f64 argument, const Lua::Str& operation) {
    const int base = lua_gettop(state);
    luaL_getref(state, reference);
    require(lua_isfunction(state, -1) != 0, operation + " registry reference is not a function");
    lua_pushnumber(state, argument);
    requireLuaStatus(state, lua_pcall(state, 1, 1, 0), operation);
    require(lua_isnumber(state, -1) != 0, operation + " did not return a number");
    const Lua::f64 result = lua_tonumber(state, -1);
    lua_pop(state, 1);
    require(lua_gettop(state) == base, operation + " did not restore the host stack");
    return result;
}

Lua::Str makeCompilerFixture() {
    Lua::Str source;
    source.reserve(Lua::usize{3} * 1024);
    Lua::usize block = 0;
    while (source.size() < Lua::usize{2} * 1024) {
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
    const Lua::Str source = makeCompilerFixture();
    Lua::Vec<Lua::f64> throughput;
    throughput.reserve(report.config.samples);

    Lua::EngineContext context;
    Lua::RuntimeServices services = context.services();

    for (Lua::usize sample = 0; sample < report.config.samples; ++sample) {
        Lua::usize generatedInstructions = 0;
        const auto start = Clock::now();
        for (Lua::usize iteration = 0; iteration < report.config.parseIterations; ++iteration) {
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
        const Lua::f64 seconds = elapsedSeconds(start, end);
        const Lua::f64 bytes = static_cast<Lua::f64>(source.size() * report.config.parseIterations);
        throughput.push_back((bytes / kBytesPerMiB) / seconds);
        (void)context.gc().collect(context.strings());
    }

    addMetric(report, "parse_compile_mib_per_second", "MiB/s", "higher", std::move(throughput));
}

thread_local Lua::u64 gVmInstructionCount = 0;

void countVmInstructionHook(lua_State*, lua_Debug*) {
    ++gVmInstructionCount;
}

class InstructionCountScope {
public:
    explicit InstructionCountScope(lua_State* state) : state_(state) {
        gVmInstructionCount = 0;
        lua_sethook(state_, countVmInstructionHook, LUA_MASKCOUNT, 1);
    }
    ~InstructionCountScope() {
        lua_sethook(state_, nullptr, 0, 0);
    }

    [[nodiscard]] Lua::u64 count() const noexcept {
        return gVmInstructionCount;
    }

private:
    lua_State* state_;
};

Lua::u64 countVmInstructions(lua_State* state, int reference, Lua::usize iterations) {
    Lua::u64 instructions = 0;
    {
        InstructionCountScope counter(state);
        const Lua::f64 result =
            invokeNumberFunction(state, reference, static_cast<Lua::f64>(iterations), "VM instruction calibration");
        require(result == expectedVmChecksum(iterations), "VM instruction calibration checksum mismatch");
        instructions = counter.count();
    }
    return instructions;
}

void benchmarkVmDispatch(Report& report) {
    std::cerr << "[bench] VM instructions per second\n";
    constexpr Lua::StrView source = R"lua(
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

    const Lua::u64 count100 = countVmInstructions(state, functionReference, 100);
    const Lua::u64 count101 = countVmInstructions(state, functionReference, 101);
    require(count101 > count100, "VM instruction count did not increase with loop iterations");
    const Lua::u64 instructionsPerIteration = count101 - count100;
    require(count100 >= instructionsPerIteration * 100, "VM instruction calibration intercept underflow");
    const Lua::u64 fixedInstructions = count100 - instructionsPerIteration * 100;
    const Lua::u64 count1000 = countVmInstructions(state, functionReference, 1000);
    require(count1000 == fixedInstructions + instructionsPerIteration * 1000,
            "VM instruction count is not linear for the deterministic loop fixture");

    Lua::Vec<Lua::f64> rates;
    rates.reserve(report.config.samples);
    const Lua::u64 measuredInstructions = fixedInstructions + instructionsPerIteration * report.config.vmIterations;
    for (Lua::usize sample = 0; sample < report.config.samples; ++sample) {
        const auto start = Clock::now();
        const Lua::f64 result = invokeNumberFunction(
            state, functionReference, static_cast<Lua::f64>(report.config.vmIterations), "VM throughput run");
        const auto end = Clock::now();
        require(result == expectedVmChecksum(report.config.vmIterations), "VM throughput checksum mismatch");
        rates.push_back(static_cast<Lua::f64>(measuredInstructions) / elapsedSeconds(start, end));
    }

    luaL_unref(state, LUA_REGISTRYINDEX, functionReference);
    addMetric(report, "vm_instructions_per_second", "instructions/s", "higher", std::move(rates));
}

void benchmarkCppToLua(Report& report) {
    std::cerr << "[bench] C++ -> Lua protected call cost\n";
    constexpr Lua::StrView source = "return function(value) return value + 1 end";
    LuaStateOwner owner(lua_open());
    lua_State* state = owner.get();
    const int functionReference = loadReturnedFunction(state, source, "=runtime_bench_cpp_to_lua");

    Lua::Vec<Lua::f64> costs;
    costs.reserve(report.config.samples);
    for (Lua::usize sample = 0; sample < report.config.samples; ++sample) {
        Lua::f64 checksum = 0.0;
        const auto start = Clock::now();
        for (Lua::usize call = 1; call <= report.config.cppToLuaCalls; ++call) {
            checksum += invokeNumberFunction(state, functionReference, static_cast<Lua::f64>(call), "C++ to Lua call");
        }
        const auto end = Clock::now();
        const Lua::f64 count = static_cast<Lua::f64>(report.config.cppToLuaCalls);
        const Lua::f64 expected = count * (count + 1.0) / 2.0 + count;
        require(checksum == expected, "C++ to Lua checksum mismatch");
        costs.push_back(elapsedSeconds(start, end) * 1.0e9 / count);
    }

    luaL_unref(state, LUA_REGISTRYINDEX, functionReference);
    addMetric(report, "cpp_to_lua_ns_per_call", "ns/call", "lower", std::move(costs));
}

Lua::u64 gHostCallCount = 0;

int hostIncrement(lua_State* state) {
    const Lua::f64 value = lua_tonumber(state, 1);
    ++gHostCallCount;
    lua_pushnumber(state, value + 1.0);
    return 1;
}

void benchmarkLuaToCpp(Report& report) {
    std::cerr << "[bench] Lua -> C++ call cost\n";
    constexpr Lua::StrView source = R"lua(
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

    Lua::Vec<Lua::f64> costs;
    costs.reserve(report.config.samples);
    for (Lua::usize sample = 0; sample < report.config.samples; ++sample) {
        gHostCallCount = 0;
        const auto start = Clock::now();
        const Lua::f64 result = invokeNumberFunction(
            state, functionReference, static_cast<Lua::f64>(report.config.luaToCppCalls), "Lua to C++ call loop");
        const auto end = Clock::now();
        require(gHostCallCount == report.config.luaToCppCalls, "Lua to C++ host call count mismatch");
        const Lua::f64 count = static_cast<Lua::f64>(report.config.luaToCppCalls);
        const Lua::f64 expected = count * (count + 1.0) / 2.0 + count;
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

    Lua::Vec<Lua::f64> costs;
    costs.reserve(report.config.samples);
    for (Lua::usize sample = 0; sample < report.config.samples; ++sample) {
        lua_State* child = lua_newthread(mainState);
        require(child != nullptr, "failed to create benchmark coroutine");
        const Lua::Str source = "for i = 1, " + std::to_string(report.config.coroutineYields) +
                                " do coroutine.yield(i) end return " + std::to_string(report.config.coroutineYields);
        requireLuaStatus(child, luaL_loadbuffer(child, source.data(), source.size(), "=runtime_bench_coroutine"),
                         "load coroutine fixture");

        Lua::f64 yieldingSeconds = 0.0;
        for (Lua::usize expectedYield = 1; expectedYield <= report.config.coroutineYields; ++expectedYield) {
            const auto start = Clock::now();
            const int status = lua_resume(child, 0);
            const auto end = Clock::now();
            yieldingSeconds += elapsedSeconds(start, end);
            require(status == LUA_YIELD, "coroutine did not yield at the expected boundary");
            require(lua_gettop(child) == 1, "coroutine yield did not expose exactly one result");
            require(lua_tonumber(child, -1) == static_cast<Lua::f64>(expectedYield),
                    "coroutine yielded an unexpected sequence value");
            lua_settop(child, 0);
        }

        const int completionStatus = lua_resume(child, 0);
        require(completionStatus == LUA_OK, "coroutine did not complete after its final yield");
        require(lua_gettop(child) == 1, "completed coroutine did not expose exactly one return value");
        require(lua_tonumber(child, -1) == static_cast<Lua::f64>(report.config.coroutineYields),
                "coroutine completion checksum mismatch");
        costs.push_back(yieldingSeconds * 1.0e9 / static_cast<Lua::f64>(report.config.coroutineYields));
        lua_pop(mainState, 1);
    }

    addMetric(report, "coroutine_resume_yield_ns", "ns/round-trip", "lower", std::move(costs));
}

Lua::f64 expectedTableChecksum(Lua::usize iterations) {
    Lua::Vec<Lua::f64> array(257, 0.0);
    Lua::Vec<Lua::f64> hash(65, 0.0);
    for (Lua::usize i = 1; i <= 256; ++i) {
        array[i] = static_cast<Lua::f64>(i);
    }
    for (Lua::usize i = 1; i <= 64; ++i) {
        hash[i] = static_cast<Lua::f64>(i);
    }

    Lua::f64 total = 0.0;
    for (Lua::usize i = 1; i <= iterations; ++i) {
        const Lua::usize arrayIndex = (i % 256) + 1;
        const Lua::usize hashIndex = (i % 64) + 1;
        total += array[arrayIndex] + hash[hashIndex];
        array[arrayIndex] += 1.0;
        hash[hashIndex] += 1.0;
    }
    return total;
}

void benchmarkTableHotReadWrite(Report& report) {
    std::cerr << "[bench] table hot read/write\n";
    constexpr Lua::StrView source = R"lua(
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

    const Lua::f64 expected = expectedTableChecksum(report.config.tableIterations);
    Lua::Vec<Lua::f64> rates;
    rates.reserve(report.config.samples);
    for (Lua::usize sample = 0; sample < report.config.samples; ++sample) {
        luaL_getref(state, resetReference);
        requireLuaStatus(state, lua_pcall(state, 0, 0, 0), "reset table fixture");
        const auto start = Clock::now();
        const Lua::f64 result = invokeNumberFunction(
            state, runReference, static_cast<Lua::f64>(report.config.tableIterations), "table hot read/write loop");
        const auto end = Clock::now();
        require(result == expected, "table hot read/write checksum mismatch");
        const Lua::f64 operations = static_cast<Lua::f64>(report.config.tableIterations) * 4.0;
        rates.push_back(operations / elapsedSeconds(start, end));
    }

    luaL_unref(state, LUA_REGISTRYINDEX, resetReference);
    luaL_unref(state, LUA_REGISTRYINDEX, runReference);
    addMetric(report, "table_operations_per_second", "operations/s", "higher", std::move(rates));
}

void benchmarkClosureLifecycle(Report& report) {
    std::cerr << "[bench] 100000 closure/upvalue lifecycle\n";
    constexpr Lua::StrView source = R"lua(
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
    const Lua::usize baselineObjects = gc.getObjectCount();

    Lua::Vec<Lua::f64> lifecycleRates;
    Lua::Vec<Lua::f64> allocationRates;
    lifecycleRates.reserve(report.config.closureSamples);
    allocationRates.reserve(report.config.closureSamples);

    for (Lua::usize sample = 0; sample < report.config.closureSamples; ++sample) {
        const Lua::usize grantedBefore = allocator.grantedBytes;
        allocator.resetPeak();
        const auto start = Clock::now();
        luaL_getref(state, functionReference);
        lua_pushnumber(state, static_cast<Lua::f64>(kRequiredClosureCount));
        requireLuaStatus(state, lua_pcall(state, 1, 4, 0), "create 100000 captured closures");
        const auto created = Clock::now();
        require(lua_istable(state, -4) != 0,
                "closure lifecycle did not return its retaining table (top=" + std::to_string(lua_gettop(state)) +
                    ", types=" + lua_typename(state, lua_type(state, -4)) + "," +
                    lua_typename(state, lua_type(state, -3)) + "," + lua_typename(state, lua_type(state, -2)) + "," +
                    lua_typename(state, lua_type(state, -1)) + ")");
        require(lua_tonumber(state, -3) == 1.0, "first closure captured the wrong upvalue");
        require(lua_tonumber(state, -2) == static_cast<Lua::f64>(kRequiredClosureCount / 2),
                "middle closure captured the wrong upvalue");
        require(lua_tonumber(state, -1) == static_cast<Lua::f64>(kRequiredClosureCount),
                "last closure captured the wrong upvalue");
        lua_pop(state, 4);
        (void)gc.collect(internal);
        const auto reclaimed = Clock::now();
        require(gc.getObjectCount() <= baselineObjects + 8,
                "100000 closure lifecycle left unreachable GC objects behind");
        require(!allocator.accountingError, "allocator accounting failed during closure lifecycle");

        const Lua::f64 lifecycleSeconds = elapsedSeconds(start, reclaimed);
        const Lua::f64 allocationSeconds = elapsedSeconds(start, created);
        lifecycleRates.push_back(static_cast<Lua::f64>(kRequiredClosureCount) / lifecycleSeconds);
        const Lua::f64 granted = static_cast<Lua::f64>(allocator.grantedBytes - grantedBefore);
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

int loadTransientFrameFunction(lua_State* state, Lua::StrView name) {
    constexpr Lua::StrView source = R"lua(
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

Lua::f64 expectedTransientChecksum(Lua::usize frame) {
    return static_cast<Lua::f64>(frame * 4 + 10);
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
    Lua::usize completedCycles = 0;
    for (Lua::usize frame = 1; frame <= report.config.gcPauseFrames; ++frame) {
        const Lua::f64 result =
            invokeNumberFunction(state, functionReference, static_cast<Lua::f64>(frame), "GC frame allocation fixture");
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

    const Lua::f64 p50 = nearestRankPercentile(report.gcPauseSamplesUs, 0.50);
    const Lua::f64 p95 = nearestRankPercentile(report.gcPauseSamplesUs, 0.95);
    const Lua::f64 p99 = nearestRankPercentile(report.gcPauseSamplesUs, 0.99);
    const Lua::f64 maximum = *std::max_element(report.gcPauseSamplesUs.begin(), report.gcPauseSamplesUs.end());
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
    constexpr Lua::StrView source = R"lua(
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

Lua::f64 expectedHeapChecksum(Lua::usize frame) {
    const Lua::usize slot = (frame % 128) + 1;
    return expectedTransientChecksum(frame) + static_cast<Lua::f64>(slot * 2);
}

HeapCheckpoint captureHeapCheckpoint(Lua::usize frame, const CountingAllocator& allocator,
                                     const Lua::GarbageCollector& gc) {
    return HeapCheckpoint{frame, allocator.liveBytes, gc.getTotalMemory(), gc.getObjectCount()};
}

Lua::f64 heapSlopeBytesPerMillionFrames(const Lua::Vec<HeapCheckpoint>& checkpoints) {
    require(checkpoints.size() >= 3, "heap stability needs at least three checkpoints");
    const Lua::usize begin = checkpoints.size() / 5;
    const Lua::usize count = checkpoints.size() - begin;
    Lua::f64 meanFrame = 0.0;
    Lua::f64 meanBytes = 0.0;
    for (Lua::usize i = begin; i < checkpoints.size(); ++i) {
        meanFrame += static_cast<Lua::f64>(checkpoints[i].frame);
        meanBytes += static_cast<Lua::f64>(checkpoints[i].allocatorLiveBytes);
    }
    meanFrame /= static_cast<Lua::f64>(count);
    meanBytes /= static_cast<Lua::f64>(count);

    Lua::f64 numerator = 0.0;
    Lua::f64 denominator = 0.0;
    for (Lua::usize i = begin; i < checkpoints.size(); ++i) {
        const Lua::f64 x = static_cast<Lua::f64>(checkpoints[i].frame) - meanFrame;
        const Lua::f64 y = static_cast<Lua::f64>(checkpoints[i].allocatorLiveBytes) - meanBytes;
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

    for (Lua::usize frame = 1; frame <= report.config.heapWarmupFrames; ++frame) {
        const Lua::f64 result =
            invokeNumberFunction(state, functionReference, static_cast<Lua::f64>(frame), "heap stability warmup");
        require(result == expectedHeapChecksum(frame), "heap warmup checksum mismatch");
        (void)gc.step(internal, report.config.gcStepSize);
    }
    (void)gc.collect(internal);
    report.heapBaselineBytes = allocator.liveBytes;

    const Lua::usize checkpointInterval = std::max<Lua::usize>(1, report.config.heapFrames / 100);
    report.heapCheckpoints.push_back(captureHeapCheckpoint(0, allocator, gc));
    Lua::usize completedCycles = 0;
    Lua::usize nextCheckpointFrame = checkpointInterval;
    for (Lua::usize frame = 1; frame <= report.config.heapFrames; ++frame) {
        const Lua::f64 result =
            invokeNumberFunction(state, functionReference, static_cast<Lua::f64>(frame), "heap stability frame");
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
        report.heapGrowthBytesPerMillionFrames <= static_cast<Lua::f64>(report.heapMaxGrowthBytesPerMillionFrames);
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

Lua::Str jsonEscape(Lua::StrView value) {
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

Lua::Str compilerName() {
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

Lua::Str operatingSystemName() {
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

Lua::Str gitSha() {
#if defined(_WIN32)
    char* sha = nullptr;
    Lua::usize length = 0;
    if (_dupenv_s(&sha, &length, "GITHUB_SHA") != 0 || sha == nullptr) {
        return "unknown";
    }
    Lua::Str result(sha);
    std::free(sha);
    return result;
#else
    Lua::CharPtr sha = std::getenv("GITHUB_SHA");
    return sha != nullptr ? sha : "unknown";
#endif
}

void writeDoubleArray(std::ostream& output, const Lua::Vec<Lua::f64>& values) {
    output << '[';
    for (Lua::usize i = 0; i < values.size(); ++i) {
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
    for (Lua::usize i = 0; i < report.metrics.size(); ++i) {
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
    for (Lua::usize i = 0; i < report.heapCheckpoints.size(); ++i) {
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

Config configForProfile(Lua::Str profile) {
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
    Lua::Str profile = "ci";
    std::filesystem::path jsonPath = "runtime-bench.json";
    Lua::usize samplesOverride = 0;
    for (int i = 1; i < argc; ++i) {
        const Lua::Str argument = argv[i];
        auto requireValue = [&](const Lua::Str& option) -> Lua::Str {
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
            samplesOverride = static_cast<Lua::usize>(std::stoull(requireValue(argument)));
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
