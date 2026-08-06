/**
 * @file debugger_bench.cpp
 * @brief Median-based debugger disabled/attached/breakpoint benchmark contract.
 */

#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "debugger/debug_runtime.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

using namespace Lua;
using namespace Lua::Debugger;
using Clock = std::chrono::steady_clock;

enum class Profile {
    Disabled,
    Attached,
    LineBreakpoint,
};

struct Config {
    std::string profile = "all";
    std::filesystem::path jsonPath = "debugger-bench.json";
    usize iterations = 200000;
    usize samples = 7;
};

struct Sample {
    double nsPerIteration = 0.0;
    double variablesPageNs = 0.0;
    usize variablesPayloadBytes = 0;
    usize variablesReturned = 0;
    usize breakpointHits = 0;
};

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

double median(std::vector<double> values) {
    require(!values.empty(), "cannot compute median of an empty sample set");
    std::sort(values.begin(), values.end());
    const usize middle = values.size() / 2;
    return values.size() % 2 == 0 ? (values[middle - 1] + values[middle]) / 2.0 : values[middle];
}

double expectedChecksum(usize count) {
    const usize cycles = count / 7;
    const usize remainder = count % 7;
    usize total = cycles * 21;
    for (usize value = 1; value <= remainder; ++value) {
        total += value;
    }
    return static_cast<double>(total);
}

Proto* compileFixture(RuntimeServices& services) {
    constexpr StrView source = "local values = {}\n"
                               "for i = 1, 150 do\n"
                               "    values[i] = i\n"
                               "end\n"
                               "local total = 0\n"
                               "for i = 1, count do\n"
                               "    total = total + (i % 7)\n"
                               "end\n"
                               "return total\n";
    Parser parser{Str(source), services};
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    CodeGenerator codegen(services);
    return codegen.generate(*parsed, "@debugger_bench.lua");
}

Function* createFixtureFunction(RuntimeServices& services, LuaState& state, Proto& proto) {
    Function* function = new Function(&proto);
    function->setEnv(state.getGlobalTable());
    services.gc.registerObject(function);
    return function;
}

const DebugScope* localScope(const Vec<DebugScope>& scopes) {
    for (const DebugScope& scope : scopes) {
        if (scope.kind == DebugScopeKind::Locals) {
            return &scope;
        }
    }
    return nullptr;
}

const DebugVariable* namedVariable(const Vec<DebugVariable>& variables, StrView name) {
    for (const DebugVariable& variable : variables) {
        if (variable.name == name) {
            return &variable;
        }
    }
    return nullptr;
}

Sample runSample(Profile profile, usize iterations) {
    EngineContext context;
    DebugController* controller = nullptr;
    Opt<DebugSession> session;
    if (profile != Profile::Disabled) {
        controller = &context.globalState().enableDebugger();
    }

    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);
    state->setGlobal("count", Value(static_cast<LuaNumber>(iterations)));
    Proto* proto = compileFixture(services);
    Function* function = createFixtureFunction(services, *state, *proto);

    if (controller != nullptr) {
        controller->registerProto(*proto);
        if (profile == Profile::LineBreakpoint) {
            const SourceId source = controller->registerFilePath("debugger_bench.lua");
            const std::array requested{SourceBreakpoint{9}};
            const auto bindings = controller->setBreakpoints(source, requested);
            require(bindings && bindings->size() == 1 && bindings->front().verified,
                    "benchmark line breakpoint did not bind");
        }
        auto attached = controller->attachSession();
        require(attached.has_value(), "benchmark debugger session failed to attach");
        session.emplace(std::move(*attached));
        require(controller->configurationDone().has_value(), "benchmark debugger session failed to start");
    }

    Sample sample;
    std::atomic<bool> executionDone = false;
    bool controlTimedOut = false;
    std::exception_ptr controlError;
    std::thread control;
    if (profile == Profile::LineBreakpoint) {
        control = std::thread([&]() {
            try {
                const auto deadline = Clock::now() + std::chrono::seconds(10);
                while (!executionDone.load(std::memory_order_acquire) && Clock::now() < deadline) {
                    const DebugSessionSnapshot snapshot = controller->snapshot();
                    if (snapshot.state != DebugSessionState::Suspended) {
                        std::this_thread::yield();
                        continue;
                    }

                    ++sample.breakpointHits;
                    const auto inspectStart = Clock::now();
                    auto frames = controller->stackTrace(DebugController::mainThreadId(), 0, 1);
                    require(frames && frames->size() == 1, "benchmark stack inspection failed");
                    auto scopes = controller->scopes(frames->front().id);
                    require(scopes.has_value(), "benchmark scope inspection failed");
                    const DebugScope* locals = localScope(*scopes);
                    require(locals != nullptr, "benchmark locals scope is missing");
                    auto localVariables = controller->variables(locals->variablesReference, 0, 100);
                    require(localVariables.has_value(), "benchmark locals inspection failed");
                    const DebugVariable* values = namedVariable(*localVariables, "values");
                    require(values != nullptr && values->variablesReference.valid(),
                            "benchmark expandable table local is missing");
                    auto page = controller->variables(values->variablesReference, 0, 100);
                    const auto inspectEnd = Clock::now();
                    require(page && page->size() == 100, "benchmark variable page did not honor the default limit");
                    sample.variablesReturned = page->size();
                    for (const DebugVariable& variable : *page) {
                        sample.variablesPayloadBytes +=
                            variable.name.size() + variable.value.size() + variable.type.size();
                    }
                    sample.variablesPageNs =
                        std::chrono::duration<double, std::nano>(inspectEnd - inspectStart).count();
                    require(controller->continueExecution(DebugController::mainThreadId()).has_value(),
                            "benchmark breakpoint could not continue");
                }
                if (!executionDone.load(std::memory_order_acquire)) {
                    controlTimedOut = true;
                    (void)controller->terminateExecution();
                }
            } catch (...) {
                controlError = std::current_exception();
                (void)controller->terminateExecution();
            }
        });
    }

    const auto start = Clock::now();
    bool executed = true;
    try {
        VM::execute(services, state.get(), function);
    } catch (const RuntimeError&) {
        executed = false;
    }
    const auto end = Clock::now();
    executionDone.store(true, std::memory_order_release);
    if (control.joinable()) {
        control.join();
    }
    if (controlError != nullptr) {
        std::rethrow_exception(controlError);
    }

    require(executed && !controlTimedOut, "debugger benchmark execution failed or timed out");
    require(state->top().isNumber() && state->top().asNumber() == expectedChecksum(iterations),
            "debugger benchmark checksum mismatch");
    if (profile == Profile::LineBreakpoint) {
        require(sample.breakpointHits == 1, "line breakpoint profile must stop exactly once");
    }
    sample.nsPerIteration =
        std::chrono::duration<double, std::nano>(end - start).count() / static_cast<double>(iterations);

    if (session) {
        session->disconnect(DisconnectAction::ContinueExecution);
        session.reset();
    }
    if (controller != nullptr) {
        context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    }
    state.reset();
    context.gc().clearAll(context.strings());
    return sample;
}

std::vector<Sample> runProfile(Profile profile, const Config& config) {
    std::vector<Sample> samples;
    samples.reserve(config.samples);
    for (usize index = 0; index < config.samples; ++index) {
        samples.push_back(runSample(profile, config.iterations));
    }
    return samples;
}

std::vector<double> timingSamples(const std::vector<Sample>& samples) {
    std::vector<double> values;
    values.reserve(samples.size());
    for (const Sample& sample : samples) {
        values.push_back(sample.nsPerIteration);
    }
    return values;
}

void writeSamples(std::ostream& output, const char* name, const std::vector<Sample>& samples, bool comma) {
    const std::vector<double> timings = timingSamples(samples);
    output << "    \"" << name << "\": {\"median_ns_per_iteration\": " << std::setprecision(17) << median(timings)
           << ", \"samples_ns_per_iteration\": [";
    for (usize index = 0; index < timings.size(); ++index) {
        output << std::setprecision(17) << timings[index];
        if (index + 1 != timings.size()) {
            output << ',';
        }
    }
    output << "]}" << (comma ? "," : "") << '\n';
}

Config parseArguments(int argc, char** argv) {
    Config config;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        auto value = [&]() -> std::string {
            if (index + 1 >= argc) {
                fail(argument + " requires a value");
            }
            return argv[++index];
        };
        if (argument == "--profile") {
            config.profile = value();
        } else if (argument == "--json") {
            config.jsonPath = value();
        } else if (argument == "--iterations") {
            config.iterations = static_cast<usize>(std::stoull(value()));
        } else if (argument == "--samples") {
            config.samples = static_cast<usize>(std::stoull(value()));
        } else if (argument == "--help") {
            std::cout << "usage: lua_debugger_bench [--profile all|debugger-disabled|attached-no-breakpoint|"
                         "line-breakpoint] [--samples N] [--iterations N] [--json PATH]\n";
            std::exit(0);
        } else {
            fail("unknown argument: " + argument);
        }
    }
    require(config.samples >= 3, "debugger benchmark requires at least three samples");
    require(config.iterations >= 1000, "debugger benchmark requires at least 1000 loop iterations");
    return config;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Config config = parseArguments(argc, argv);
        const bool runDisabled = config.profile == "all" || config.profile == "debugger-disabled";
        const bool runAttached = config.profile == "all" || config.profile == "attached-no-breakpoint";
        const bool runBreakpoint = config.profile == "all" || config.profile == "line-breakpoint";
        require(runDisabled || runAttached || runBreakpoint, "unknown debugger benchmark profile");

        std::vector<Sample> disabled;
        std::vector<Sample> attached;
        std::vector<Sample> breakpoint;
        if (runDisabled && runAttached) {
            disabled.reserve(config.samples);
            attached.reserve(config.samples);
            for (usize index = 0; index < config.samples; ++index) {
                if (index % 2 == 0) {
                    disabled.push_back(runSample(Profile::Disabled, config.iterations));
                    attached.push_back(runSample(Profile::Attached, config.iterations));
                } else {
                    attached.push_back(runSample(Profile::Attached, config.iterations));
                    disabled.push_back(runSample(Profile::Disabled, config.iterations));
                }
            }
        } else if (runDisabled) {
            disabled = runProfile(Profile::Disabled, config);
        } else if (runAttached) {
            attached = runProfile(Profile::Attached, config);
        }
        if (runBreakpoint) {
            breakpoint = runProfile(Profile::LineBreakpoint, config);
        }

        double attachedRatio = 0.0;
        if (!disabled.empty() && !attached.empty()) {
            attachedRatio = median(timingSamples(attached)) / median(timingSamples(disabled));
            require(attachedRatio <= 1.05, "attached-no-breakpoint median overhead exceeds the 5% budget");
        }

        std::ofstream output(config.jsonPath, std::ios::binary | std::ios::trunc);
        require(output.is_open(), "failed to open debugger benchmark JSON output");
        output << "{\n  \"schema_version\": 1,\n  \"profile\": \"" << config.profile << "\",\n"
               << "  \"iterations\": " << config.iterations << ",\n  \"sample_count\": " << config.samples
               << ",\n  \"profiles\": {\n";
        usize remaining =
            static_cast<usize>(runDisabled) + static_cast<usize>(runAttached) + static_cast<usize>(runBreakpoint);
        if (runDisabled) {
            writeSamples(output, "debugger-disabled", disabled, --remaining != 0);
        }
        if (runAttached) {
            writeSamples(output, "attached-no-breakpoint", attached, --remaining != 0);
        }
        if (runBreakpoint) {
            writeSamples(output, "line-breakpoint", breakpoint, --remaining != 0);
        }
        output << "  },\n  \"attached_overhead_ratio\": " << std::setprecision(17) << attachedRatio
               << ",\n  \"attached_overhead_budget_ratio\": 1.05,\n"
               << "  \"resource_limits\": {\"max_stack_frames\": 256, \"variable_page_size\": 100, "
                  "\"max_string_length\": 256, \"max_object_handles\": 4096},\n";
        if (!breakpoint.empty()) {
            std::vector<double> variableTimes;
            usize maxPayload = 0;
            for (const Sample& sample : breakpoint) {
                variableTimes.push_back(sample.variablesPageNs);
                maxPayload = std::max(maxPayload, sample.variablesPayloadBytes);
            }
            output << "  \"variables_page_median_ns\": " << median(variableTimes)
                   << ",\n  \"variables_page_max_payload_bytes\": " << maxPayload << ",\n";
        }
        output << "  \"passed\": true\n}\n";
        require(output.good(), "failed to write debugger benchmark JSON output");

        if (!disabled.empty()) {
            std::cout << "debugger-disabled: " << median(timingSamples(disabled)) << " ns/iteration\n";
        }
        if (!attached.empty()) {
            std::cout << "attached-no-breakpoint: " << median(timingSamples(attached)) << " ns/iteration\n";
        }
        if (!breakpoint.empty()) {
            std::cout << "line-breakpoint: " << median(timingSamples(breakpoint)) << " ns/iteration\n";
        }
        std::cout << "attached overhead ratio: " << attachedRatio << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "debugger benchmark failed: " << error.what() << '\n';
        return 1;
    }
}
