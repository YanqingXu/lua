/**
 * @file test_stack_inspector.cpp
 * @brief Owner-thread stack, scopes, variables, paging, and stale-handle tests.
 */

#include "../framework/test_framework.hpp"

#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/table.hpp"
#include "core/userdata.hpp"
#include "debugger/debug_runtime.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <thread>

using namespace Lua;
using namespace Lua::Debugger;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Debugger Stack Inspector";

Proto* compileInspectorChunk(RuntimeServices& services, StrView source, StrView sourceName) {
    Parser parser{Str(source), services};
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    CodeGenerator codegen(services);
    return codegen.generate(*parsed, sourceName);
}

Function* createInspectorFunction(RuntimeServices& services, LuaState* state, Proto* proto) {
    Function* function = new Function(proto);
    function->setEnv(state->getGlobalTable());
    services.gc.registerObject(function);
    return function;
}

const DebugScope* findScope(const Vec<DebugScope>& scopes, DebugScopeKind kind) {
    for (const DebugScope& scope : scopes) {
        if (scope.kind == kind) {
            return &scope;
        }
    }
    return nullptr;
}

const DebugVariable* findVariable(const Vec<DebugVariable>& variables, StrView name) {
    for (const DebugVariable& variable : variables) {
        if (variable.name == name) {
            return &variable;
        }
    }
    return nullptr;
}

std::atomic<i32> gInspectorMetamethodCalls = 0;

i32 inspectorIndexMetamethod(LuaState*) {
    gInspectorMetamethodCalls.fetch_add(1, std::memory_order_relaxed);
    return 0;
}

} // namespace

void testStackScopesVariablesAndStaleHandles(TestSuite& suite) {
    constexpr StrView source = "local outerValue = 10\n"
                               "local shared = {11, 22, 33, name = 'fixture'}\n"
                               "shared.self = shared\n"
                               "local function outer(arg)\n"
                               "    local shadow = 'outer'\n"
                               "    local function inner(extra)\n"
                               "        local shadow = 'inner'\n"
                               "        local sum = outerValue + arg + extra\n"
                               "        return sum, shared, shadow\n"
                               "    end\n"
                               "    return inner(5)\n"
                               "end\n"
                               "local first = outer(7)\n"
                               "local second = outer(8)\n"
                               "return second\n";
    EngineContext context;
    DebugController& controller = context.globalState().enableDebugger();
    IDebugRuntime& runtime = controller;
    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);
    Table* rawTable = services.gc.create<Table>();
    for (i32 index = 1; index <= 150; ++index) {
        rawTable->setArray(index, Value(static_cast<LuaNumber>(index)));
    }
    Table* metatable = services.gc.create<Table>();
    Function* indexMetamethod = new Function(inspectorIndexMetamethod);
    services.gc.registerObject(indexMetamethod);
    metatable->set(Value(services.strings.intern("__index")), Value(indexMetamethod));
    rawTable->setMetatable(metatable);
    Userdata* userdata = services.gc.create<Userdata>(16);
    Str longText(300, 'x');
    longText += "\nend";
    state->setGlobal("debug_bool", Value(true));
    state->setGlobal("debug_number", Value(3.5));
    state->setGlobal("debug_string", Value(services.strings.intern(longText)));
    state->setGlobal("debug_table", Value(rawTable));
    state->setGlobal("debug_function", Value(indexMetamethod));
    state->setGlobal("debug_userdata", Value(userdata));
    state->setGlobal("debug_thread", Value(state->getMainThreadFacade()));
    state->setGlobal("debug_lightuserdata", Value(static_cast<void*>(&gInspectorMetamethodCalls)));
    state->setGlobal("debug_resolution", Value(services.strings.intern("global")));
    gInspectorMetamethodCalls.store(0, std::memory_order_relaxed);
    Proto* proto = compileInspectorChunk(services, source, "@debugger/stack_and_values.lua");
    Function* function = createInspectorFunction(services, state.get(), proto);
    Table* functionEnvironment = services.gc.create<Table>();
    functionEnvironment->set(Value(services.strings.intern("debug_resolution")),
                             Value(services.strings.intern("fenv")));
    function->setEnv(functionEnvironment);

    const SourceId sourceId = runtime.registerFilePath("debugger/stack_and_values.lua");
    const std::array breakpoints{SourceBreakpoint{9}};
    auto breakpointResult = runtime.setBreakpoints(sourceId, breakpoints);
    auto attached = runtime.attachSession();
    DebugSession session = std::move(*attached);
    const bool configured = runtime.configurationDone().has_value();

    std::atomic<bool> executionDone = false;
    bool timedOut = false;
    bool threadsValid = false;
    bool stackOrderValid = false;
    bool framePagingStable = false;
    bool scopesStable = false;
    bool localsValid = false;
    bool upvaluesValid = false;
    bool tablePagingValid = false;
    bool tableFiltersValid = false;
    bool selfReferenceStable = false;
    bool formatterCoverage = false;
    bool basicFormatCoverage = false;
    bool stringFormatCoverage = false;
    bool objectFormatCoverage = false;
    bool hardPageLimitApplied = false;
    bool inspectionHadNoMetamethodSideEffect = false;
    bool oldFrameStale = false;
    bool oldVariableStale = false;
    bool evaluationSubsetValid = false;
    bool evaluationRejectedSideEffects = false;
    usize stopCount = 0;

    std::thread control([&]() {
        PauseGeneration handled;
        FrameId oldFrame;
        VariableReference oldVariables;
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (!executionDone.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
            const DebugSessionSnapshot snapshot = runtime.snapshot();
            if (snapshot.state != DebugSessionState::Suspended || snapshot.pauseGeneration == handled) {
                std::this_thread::yield();
                continue;
            }
            handled = snapshot.pauseGeneration;
            ++stopCount;

            if (stopCount == 1) {
                auto threadList = runtime.threads();
                threadsValid = threadList && threadList->size() == 1 && threadList->front().id == ThreadId{1} &&
                               threadList->front().state == DebugThreadState::Paused;

                auto firstPage = runtime.stackTrace(ThreadId{1}, 0, 2);
                auto secondPage = runtime.stackTrace(ThreadId{1}, 2, 100);
                auto fullStack = runtime.stackTrace(ThreadId{1}, 0, 100);
                stackOrderValid = fullStack && fullStack->size() >= 3 && !fullStack->at(0).native &&
                                  fullStack->at(0).location.line == 9;
                framePagingStable = firstPage && secondPage && fullStack && firstPage->size() == 2 &&
                                    firstPage->front().id == fullStack->front().id &&
                                    firstPage->size() + secondPage->size() == fullStack->size();
                if (!fullStack || fullStack->empty()) {
                    (void)runtime.terminateExecution();
                    break;
                }

                oldFrame = fullStack->front().id;
                auto evaluatedLocal = runtime.evaluate(oldFrame, "shadow");
                auto evaluatedUpvalue = runtime.evaluate(oldFrame, "shared[2]");
                auto evaluatedField = runtime.evaluate(oldFrame, "shared.name");
                auto evaluatedEnvironment = runtime.evaluate(oldFrame, "debug_resolution");
                auto rejectedCall = runtime.evaluate(oldFrame, "setmetatable(shared, {})");
                auto rejectedAssignment = runtime.evaluate(oldFrame, "shared.name = 'changed'");
                evaluationSubsetValid = evaluatedLocal && evaluatedUpvalue && evaluatedField && evaluatedEnvironment &&
                                        evaluatedLocal->value == "\"inner\"" && evaluatedUpvalue->value == "22" &&
                                        evaluatedField->value == "\"fixture\"" &&
                                        evaluatedEnvironment->value == "\"fenv\"";
                evaluationRejectedSideEffects =
                    !rejectedCall && !rejectedAssignment && rejectedCall.error().code == DebugErrorCode::Unsupported &&
                    rejectedAssignment.error().code == DebugErrorCode::Unsupported;
                auto frameScopes = runtime.scopes(oldFrame);
                auto repeatedScopes = runtime.scopes(oldFrame);
                scopesStable = frameScopes && repeatedScopes && frameScopes->size() == 3 &&
                               frameScopes->at(0).variablesReference == repeatedScopes->at(0).variablesReference;
                if (frameScopes) {
                    const DebugScope* localsScope = findScope(*frameScopes, DebugScopeKind::Locals);
                    const DebugScope* upvaluesScope = findScope(*frameScopes, DebugScopeKind::Upvalues);
                    auto locals = localsScope != nullptr
                                      ? runtime.variables(localsScope->variablesReference, 0, 100)
                                      : DebugResult<Vec<DebugVariable>>(std::unexpected(DebugError{}));
                    auto upvalues = upvaluesScope != nullptr
                                        ? runtime.variables(upvaluesScope->variablesReference, 0, 100)
                                        : DebugResult<Vec<DebugVariable>>(std::unexpected(DebugError{}));

                    const DebugVariable* extra = locals ? findVariable(*locals, "extra") : nullptr;
                    const DebugVariable* shadow = locals ? findVariable(*locals, "shadow") : nullptr;
                    const DebugVariable* sum = locals ? findVariable(*locals, "sum") : nullptr;
                    localsValid = extra != nullptr && extra->value == "5" && shadow != nullptr &&
                                  shadow->value == "\"inner\"" && sum != nullptr && sum->value == "22";

                    const DebugVariable* outerValue = upvalues ? findVariable(*upvalues, "outerValue") : nullptr;
                    const DebugVariable* argument = upvalues ? findVariable(*upvalues, "arg") : nullptr;
                    const DebugVariable* shared = upvalues ? findVariable(*upvalues, "shared") : nullptr;
                    upvaluesValid = outerValue != nullptr && outerValue->value == "10" && argument != nullptr &&
                                    argument->value == "7" && shared != nullptr && shared->variablesReference.valid();
                    if (shared != nullptr) {
                        oldVariables = shared->variablesReference;
                        auto firstVariables = runtime.variables(oldVariables, 0, 2);
                        auto nextVariables = runtime.variables(oldVariables, 2, 2);
                        tablePagingValid = firstVariables && nextVariables && firstVariables->size() == 2 &&
                                           nextVariables->size() == 2;
                        auto allVariables = runtime.variables(oldVariables, 0, 100);
                        const DebugVariable* self = allVariables ? findVariable(*allVariables, "self") : nullptr;
                        selfReferenceStable = self != nullptr && self->variablesReference == oldVariables;
                        auto indexedVariables =
                            runtime.variables(oldVariables, 0, 100, DebugVariableFilter::Indexed);
                        auto namedVariables = runtime.variables(oldVariables, 0, 100, DebugVariableFilter::Named);
                        tableFiltersValid = indexedVariables && namedVariables && indexedVariables->size() == 3 &&
                                            namedVariables->size() == 2 &&
                                            findVariable(*namedVariables, "name") != nullptr &&
                                            findVariable(*namedVariables, "self") != nullptr;
                    }

                    const DebugScope* globalsScope = findScope(*frameScopes, DebugScopeKind::Globals);
                    auto globals = globalsScope != nullptr
                                       ? runtime.variables(globalsScope->variablesReference, 0, 100)
                                       : DebugResult<Vec<DebugVariable>>(std::unexpected(DebugError{}));
                    const DebugVariable* boolean = globals ? findVariable(*globals, "debug_bool") : nullptr;
                    const DebugVariable* number = globals ? findVariable(*globals, "debug_number") : nullptr;
                    const DebugVariable* string = globals ? findVariable(*globals, "debug_string") : nullptr;
                    const DebugVariable* table = globals ? findVariable(*globals, "debug_table") : nullptr;
                    const DebugVariable* cfunction = globals ? findVariable(*globals, "debug_function") : nullptr;
                    const DebugVariable* fullUserdata = globals ? findVariable(*globals, "debug_userdata") : nullptr;
                    const DebugVariable* thread = globals ? findVariable(*globals, "debug_thread") : nullptr;
                    const DebugVariable* light = globals ? findVariable(*globals, "debug_lightuserdata") : nullptr;
                    basicFormatCoverage =
                        boolean != nullptr && boolean->value == "true" && number != nullptr && number->value == "3.5";
                    stringFormatCoverage = string != nullptr && string->value.find("…") != Str::npos;
                    objectFormatCoverage = cfunction != nullptr && cfunction->value == "C function" &&
                                           fullUserdata != nullptr && fullUserdata->value == "userdata (16 bytes)" &&
                                           thread != nullptr && thread->value.starts_with("thread") &&
                                           light != nullptr && light->value == "lightuserdata" && table != nullptr;
                    formatterCoverage = basicFormatCoverage && stringFormatCoverage && objectFormatCoverage;
                    if (table != nullptr) {
                        auto oversizedPage = runtime.variables(table->variablesReference, 0, 1000);
                        hardPageLimitApplied = oversizedPage && oversizedPage->size() == 100;
                    }
                    inspectionHadNoMetamethodSideEffect =
                        gInspectorMetamethodCalls.load(std::memory_order_relaxed) == 0;
                }
            } else if (stopCount == 2) {
                auto staleFrameResult = runtime.scopes(oldFrame);
                auto staleVariableResult = runtime.variables(oldVariables, 0, 1);
                oldFrameStale = !staleFrameResult && staleFrameResult.error().code == DebugErrorCode::StaleReference;
                oldVariableStale =
                    !staleVariableResult && staleVariableResult.error().code == DebugErrorCode::StaleReference;
            }

            (void)runtime.continueExecution(ThreadId{1});
        }
        if (!executionDone.load(std::memory_order_acquire)) {
            timedOut = true;
            (void)runtime.terminateExecution();
        }
    });

    bool executed = true;
    try {
        VM::execute(services, state.get(), function);
    } catch (const RuntimeError&) {
        executed = false;
    }
    executionDone.store(true, std::memory_order_release);
    control.join();

    ASSERT_TRUE(suite, configured && breakpointResult && !breakpointResult->front().verified,
                "Inspector fixture configures a pending breakpoint before VM entry");
    ASSERT_FALSE(suite, timedOut, "Owner-thread inspection commands complete within timeout");
    ASSERT_EQ(suite, usize{2}, stopCount, "Loop-free repeated call produces two breakpoint generations");
    ASSERT_TRUE(suite, threadsValid, "threads returns one stable paused main thread");
    ASSERT_TRUE(suite, stackOrderValid, "stackTrace is newest-first and uses savedpc minus one");
    ASSERT_TRUE(suite, framePagingStable, "stackTrace pagination preserves stable frame IDs");
    ASSERT_TRUE(suite, scopesStable, "Repeated scopes calls preserve pause-generation references");
    ASSERT_TRUE(suite, localsValid, "Locals honor PC lifetimes and expose expected values");
    ASSERT_TRUE(suite, upvaluesValid, "Upvalues use Proto names and closure slot values");
    ASSERT_TRUE(suite, tablePagingValid, "Raw table expansion enforces requested pages");
    ASSERT_TRUE(suite, tableFiltersValid, "Indexed and named table views have independent deterministic pages");
    ASSERT_TRUE(suite, selfReferenceStable, "Self-referential table reuses its existing object handle");
    ASSERT_TRUE(
        suite, formatterCoverage,
        "Value formatter covers booleans, numbers, truncated strings, functions, userdata, threads, and pointers");
    ASSERT_TRUE(suite, basicFormatCoverage, "Primitive value formatting is deterministic");
    ASSERT_TRUE(suite, stringFormatCoverage, "Long strings are truncated with an explicit marker");
    ASSERT_TRUE(suite, objectFormatCoverage, "Object formatting avoids exposing raw pointer addresses");
    ASSERT_TRUE(suite, hardPageLimitApplied, "Oversized variables request is capped by the hard page limit");
    ASSERT_TRUE(suite, inspectionHadNoMetamethodSideEffect,
                "Raw table inspection never invokes the table's __index metamethod");
    ASSERT_TRUE(suite, evaluationSubsetValid,
                "Read-only evaluation resolves shadowed locals, upvalues, raw indexes, and function environments");
    ASSERT_TRUE(suite, evaluationRejectedSideEffects,
                "Read-only evaluation rejects function calls and assignments without changing the VM");
    ASSERT_TRUE(suite, oldFrameStale && oldVariableStale,
                "Prior-generation frame and variable handles return staleReference");
    ASSERT_TRUE(suite, executed && state->top().isNumber() && state->top().asNumber() == 23,
                "Inspection has no Lua-visible side effects");

    session.disconnect(DisconnectAction::ContinueExecution);
    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    state.reset();
    context.gc().clearAll(context.strings());
}

void testTailCallPlaceholders(TestSuite& suite) {
    constexpr StrView source = "local function tail(n)\n"
                               "    if n == 0 then\n"
                               "        local marker = 'stop'\n"
                               "        return marker\n"
                               "    end\n"
                               "    return tail(n - 1)\n"
                               "end\n"
                               "return tail(3)\n";
    EngineContext context;
    DebugController& controller = context.globalState().enableDebugger();
    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);
    Proto* proto = compileInspectorChunk(services, source, "@debugger/tail_stack.lua");
    Function* function = createInspectorFunction(services, state.get(), proto);
    const SourceId sourceId = controller.registerFilePath("debugger/tail_stack.lua");
    const std::array breakpoints{SourceBreakpoint{4}};
    (void)controller.setBreakpoints(sourceId, breakpoints);
    auto attached = controller.attachSession();
    DebugSession session = std::move(*attached);
    (void)controller.configurationDone();

    bool sawTailPlaceholder = false;
    bool controlFinished = false;
    std::thread control([&]() {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (std::chrono::steady_clock::now() < deadline) {
            if (controller.snapshot().state == DebugSessionState::Suspended) {
                auto frames = controller.stackTrace(ThreadId{1}, 0, 100);
                if (frames) {
                    for (const DebugStackFrame& frame : *frames) {
                        sawTailPlaceholder = sawTailPlaceholder || frame.name == "[tail call]";
                    }
                }
                controlFinished = controller.continueExecution(ThreadId{1}).has_value();
                return;
            }
            std::this_thread::yield();
        }
        (void)controller.terminateExecution();
    });

    bool executed = true;
    try {
        VM::execute(services, state.get(), function);
    } catch (const RuntimeError&) {
        executed = false;
    }
    control.join();

    ASSERT_TRUE(suite, controlFinished && executed, "Tail-call stack fixture pauses, inspects, and resumes");
    ASSERT_TRUE(suite, sawTailPlaceholder, "Optimized tail calls are represented by deterministic placeholders");

    session.disconnect(DisconnectAction::ContinueExecution);
    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    state.reset();
    context.gc().clearAll(context.strings());
}

void testInspectorResourceLimitIsStructured(TestSuite& suite) {
    EngineContext context;
    DebugResourceLimits limits;
    limits.maxStackFrames = 1;
    limits.maxObjectHandles = 2;
    DebugController& controller = context.globalState().enableDebugger(limits);
    RuntimeServices services = context.services();
    UPtr<LuaState> state = LuaState::create(context);
    Proto* proto = compileInspectorChunk(services, "local value = 1\nreturn value\n", "@debugger/limits.lua");
    Function* function = createInspectorFunction(services, state.get(), proto);
    const SourceId sourceId = controller.registerFilePath("debugger/limits.lua");
    const std::array breakpoints{SourceBreakpoint{2}};
    (void)controller.setBreakpoints(sourceId, breakpoints);
    auto attached = controller.attachSession();
    DebugSession session = std::move(*attached);
    (void)controller.configurationDone();

    bool stackWasCapped = false;
    bool resourceErrorWasStructured = false;
    std::thread control([&]() {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (std::chrono::steady_clock::now() < deadline) {
            if (controller.snapshot().state == DebugSessionState::Suspended) {
                auto frames = controller.stackTrace(ThreadId{1}, 0, 100);
                stackWasCapped = frames && frames->size() == 1;
                if (frames && !frames->empty()) {
                    auto frameScopes = controller.scopes(frames->front().id);
                    resourceErrorWasStructured =
                        !frameScopes && frameScopes.error().code == DebugErrorCode::ResourceLimit;
                }
                (void)controller.continueExecution(ThreadId{1});
                return;
            }
            std::this_thread::yield();
        }
        (void)controller.terminateExecution();
    });

    bool executed = true;
    try {
        VM::execute(services, state.get(), function);
    } catch (const RuntimeError&) {
        executed = false;
    }
    control.join();

    ASSERT_TRUE(suite, executed && stackWasCapped, "Configured stack frame limit caps the inspection response");
    ASSERT_TRUE(suite, resourceErrorWasStructured,
                "Handle exhaustion returns a structured resourceLimit error without crashing the VM");

    session.disconnect(DisconnectAction::ContinueExecution);
    context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
    state.reset();
    context.gc().clearAll(context.strings());
}

void registerDebuggerStackInspectorTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "Scopes Variables And Stale Handles", testStackScopesVariablesAndStaleHandles);
    registry.registerTest(kSuiteName, "Tail Call Placeholders", testTailCallPlaceholders);
    registry.registerTest(kSuiteName, "Structured Resource Limits", testInspectorResourceLimitIsStructured);
}
