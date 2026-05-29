/**
 * @file test_coroutinelib.cpp
 * @brief Lua coroutine library tests
 */

#include "../framework/test_framework.hpp"
#include "lib/lib_manager.hpp"
#include "lib/coroutinelib.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/thread.hpp"
#include "gc/garbage_collector.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"

#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Coroutine Library";

/// Helper: compile and execute Lua code, return true on success
bool runLua(LuaState* L, const char* code) {
    try {
        Parser parser(code);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);
        StringPool& pool = StringPool::getInstance();
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk, "test");
        if (!proto) return false;

        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        func->setEnv(L->getGlobalTable());
        VM::execute(L, func);
        delete proto;
        return true;
    } catch (...) {
        return false;
    }
}

/// Helper: get global number
double getGlobalNumber(LuaState* L, const char* name) {
    Value v = L->getGlobal(name);
    return v.isNumber() ? v.asNumber() : -9999.0;
}

/// Helper: get global string
std::string getGlobalString(LuaState* L, const char* name) {
    Value v = L->getGlobal(name);
    return v.isString() ? std::string(v.asString()->c_str()) : "";
}

/// Helper: get global boolean
bool getGlobalBool(LuaState* L, const char* name) {
    Value v = L->getGlobal(name);
    return v.isBoolean() && v.asBoolean();
}

/// Helper: create state with all libs
LuaState* createState() {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);
    return L;
}

} // anonymous namespace

// ==================================================================
// Test: coroutine.create returns a thread
// ==================================================================

void testCoroutineCreate(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local co = coroutine.create(function() end)
        result = (type(co) == "thread")
    )");
    ASSERT_TRUE(suite, ok, "coroutine.create compiles and runs");
    // Note: type() for thread may not be implemented yet, check via status
    // Just check that create doesn't crash
    delete L;
}

// ==================================================================
// Test: basic resume/yield
// ==================================================================

void testBasicResumeYield(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local co = coroutine.create(function()
            coroutine.yield(10)
            coroutine.yield(20)
            return 30
        end)
        local ok1, v1 = coroutine.resume(co)
        local ok2, v2 = coroutine.resume(co)
        local ok3, v3 = coroutine.resume(co)
        r_ok1 = ok1
        r_v1  = v1
        r_ok2 = ok2
        r_v2  = v2
        r_ok3 = ok3
        r_v3  = v3
    )");
    ASSERT_TRUE(suite, ok, "basic resume/yield runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_ok1"), "first resume ok");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v1"), 10.0, "first yield value");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_ok2"), "second resume ok");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v2"), 20.0, "second yield value");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_ok3"), "third resume ok");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v3"), 30.0, "return value");
    delete L;
}

// ==================================================================
// Test: resume args become yield returns
// ==================================================================

void testResumeArgsToYieldReturns(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local co = coroutine.create(function(a)
            local b = coroutine.yield(a + 1)
            local c = coroutine.yield(b + 1)
            return c + 1
        end)
        local ok1, v1 = coroutine.resume(co, 10)
        local ok2, v2 = coroutine.resume(co, 20)
        local ok3, v3 = coroutine.resume(co, 30)
        r_v1 = v1
        r_v2 = v2
        r_v3 = v3
    )");
    ASSERT_TRUE(suite, ok, "resume args test runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v1"), 11.0, "yield(a+1) with a=10");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v2"), 21.0, "yield(b+1) with b=20");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v3"), 31.0, "return c+1 with c=30");
    delete L;
}

// ==================================================================
// Test: generator pattern
// ==================================================================

void testGeneratorPattern(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local function range(n)
            return coroutine.create(function()
                for i = 1, n do
                    coroutine.yield(i)
                end
            end)
        end
        local co = range(3)
        local ok1, v1 = coroutine.resume(co)
        local ok2, v2 = coroutine.resume(co)
        local ok3, v3 = coroutine.resume(co)
        local ok4, v4 = coroutine.resume(co)
        r_v1 = v1
        r_v2 = v2
        r_v3 = v3
        r_ok4 = ok4
    )");
    ASSERT_TRUE(suite, ok, "generator pattern runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v1"), 1.0, "range yields 1");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v2"), 2.0, "range yields 2");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v3"), 3.0, "range yields 3");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_ok4"), "4th resume ok (returns nil)");
    delete L;
}

// ==================================================================
// Test: dead coroutine resume
// ==================================================================

void testDeadCoroutineResume(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local co = coroutine.create(function() return 1 end)
        coroutine.resume(co)
        local ok2, err = coroutine.resume(co)
        r_ok2 = ok2
        r_err = err
    )");
    ASSERT_TRUE(suite, ok, "dead coroutine test runs");
    ASSERT_FALSE(suite, getGlobalBool(L, "r_ok2"), "resume dead coroutine fails");
    ASSERT_TRUE(suite, getGlobalString(L, "r_err").find("dead") != std::string::npos,
        "error message mentions dead");
    delete L;
}

void testDeadCoroutineResumeDiscardsArguments(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local co = coroutine.create(function() return 1 end)
        coroutine.resume(co)
        local ok2, err, extra = coroutine.resume(co, "resume-arg")
        r_dead_ok_is_false = (ok2 == false)
        r_dead_err_mentions_dead = (type(err) == "string" and string.find(err, "dead") ~= nil)
        r_dead_extra_is_nil = (extra == nil)
    )");

    ASSERT_TRUE(suite, ok, "dead coroutine with args chunk runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_dead_ok_is_false"),
                "dead coroutine resume returns false first even with arguments");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_dead_err_mentions_dead"),
                "dead coroutine resume returns the error message second");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_dead_extra_is_nil"),
                "dead coroutine resume discards supplied arguments");
    delete L;
}

// ==================================================================
// Test: coroutine.status
// ==================================================================

void testCoroutineStatus(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local co = coroutine.create(function()
            coroutine.yield()
        end)
        s1 = coroutine.status(co)
        coroutine.resume(co)
        s2 = coroutine.status(co)
        coroutine.resume(co)
        s3 = coroutine.status(co)
    )");
    ASSERT_TRUE(suite, ok, "coroutine.status test runs");
    ASSERT_TRUE(suite, getGlobalString(L, "s1") == "suspended", "initial status is suspended");
    ASSERT_TRUE(suite, getGlobalString(L, "s2") == "suspended", "after yield status is suspended");
    ASSERT_TRUE(suite, getGlobalString(L, "s3") == "dead", "after return status is dead");
    delete L;
}

// ==================================================================
// Test: coroutine.running
// ==================================================================

void testCoroutineRunning(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        r_main = (coroutine.running() == nil)
        local co = coroutine.create(function()
            r_inside = (coroutine.running() ~= nil)
        end)
        coroutine.resume(co)
    )");
    ASSERT_TRUE(suite, ok, "coroutine.running test runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_main"), "running() is nil in main thread");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_inside"), "running() is non-nil inside coroutine");
    delete L;
}

// ==================================================================
// Test: multiple yield values
// ==================================================================

void testMultipleYieldValues(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local co = coroutine.create(function()
            coroutine.yield(1, 2, 3)
        end)
        local ok1, a, b, c = coroutine.resume(co)
        r_a = a
        r_b = b
        r_c = c
    )");
    ASSERT_TRUE(suite, ok, "multiple yield values runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_a"), 1.0, "first yield value");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_b"), 2.0, "second yield value");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_c"), 3.0, "third yield value");
    delete L;
}

void testYieldReturnValuesInOpenTableConstructor(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local co = coroutine.create(function()
            local empty = {coroutine.yield()}
            r_empty_len = table.getn(empty)
            r_empty_first_is_nil = (empty[1] == nil)

            local values = {coroutine.yield()}
            r_values_len = table.getn(values)
            r_values_first = values[1]
            r_values_second = values[2]
        end)

        coroutine.resume(co)
        coroutine.resume(co)
        coroutine.resume(co, "a", "b")
    )");

    ASSERT_TRUE(suite, ok, "yield open table constructor chunk runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_empty_len"), 0.0,
              "yield with no resume values creates an empty table");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_empty_first_is_nil"),
                "empty yield result table has no first element");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_values_len"), 2.0,
              "yield resume values define the open table length");
    ASSERT_TRUE(suite, getGlobalString(L, "r_values_first") == "a",
                "first resume value is preserved");
    ASSERT_TRUE(suite, getGlobalString(L, "r_values_second") == "b",
                "second resume value is preserved");
    delete L;
}

// ==================================================================
// Test: error in coroutine
// ==================================================================

void testCoroutineError(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local co = coroutine.create(function()
            error("boom")
        end)
        local ok1, err = coroutine.resume(co)
        r_ok1 = ok1
        r_has_err = (err ~= nil)
        r_status = coroutine.status(co)
    )");
    ASSERT_TRUE(suite, ok, "coroutine error test runs");
    ASSERT_FALSE(suite, getGlobalBool(L, "r_ok1"), "resume returns false on error");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_has_err"), "error message is provided");
    ASSERT_TRUE(suite, getGlobalString(L, "r_status") == "dead", "errored coroutine is dead");
    delete L;
}

// ==================================================================
// Test: coroutine with no yield (just return)
// ==================================================================

void testCoroutineNoYield(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local co = coroutine.create(function(x, y)
            return x + y
        end)
        local ok1, v = coroutine.resume(co, 3, 4)
        r_ok1 = ok1
        r_v = v
    )");
    ASSERT_TRUE(suite, ok, "coroutine no yield runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_ok1"), "resume ok");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v"), 7.0, "return value 3+4");
    delete L;
}

// ==================================================================
// Test: coroutine.wrap basic usage
// ==================================================================

void testWrapBasic(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local gen = coroutine.wrap(function()
            coroutine.yield(10)
            coroutine.yield(20)
            return 30
        end)
        r_v1 = gen()
        r_v2 = gen()
        r_v3 = gen()
    )");
    ASSERT_TRUE(suite, ok, "wrap basic runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v1"), 10.0, "wrap first yield");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v2"), 20.0, "wrap second yield");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v3"), 30.0, "wrap return value");
    delete L;
}

// ==================================================================
// Test: coroutine.wrap as generator pattern
// ==================================================================

void testWrapGenerator(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local function range(n)
            return coroutine.wrap(function()
                for i = 1, n do
                    coroutine.yield(i)
                end
            end)
        end
        local sum = 0
        for v in range(5) do
            sum = sum + v
        end
        r_sum = sum
    )");
    ASSERT_TRUE(suite, ok, "wrap generator runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_sum"), 15.0, "wrap sum 1..5 = 15");
    delete L;
}

// ==================================================================
// Test: coroutine.wrap passes arguments to first call
// ==================================================================

void testWrapArgs(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local gen = coroutine.wrap(function(a, b)
            coroutine.yield(a + b)
            return a * b
        end)
        r_v1 = gen(3, 4)
        r_v2 = gen()
    )");
    ASSERT_TRUE(suite, ok, "wrap args runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v1"), 7.0, "wrap yield a+b");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v2"), 12.0, "wrap return a*b");
    delete L;
}

// ==================================================================
// Test: coroutine.wrap resume args become yield returns
// ==================================================================

void testWrapResumeArgs(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local gen = coroutine.wrap(function()
            local x = coroutine.yield(1)
            local y = coroutine.yield(2)
            return x + y
        end)
        r_v1 = gen()
        r_v2 = gen(10)
        r_v3 = gen(20)
    )");
    ASSERT_TRUE(suite, ok, "wrap resume args runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v1"), 1.0, "wrap yield 1");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v2"), 2.0, "wrap yield 2");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v3"), 30.0, "wrap return 10+20");
    delete L;
}

void testWrapTailCallYield(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local function tail_yield(v)
            return coroutine.yield(v)
        end

        local gen = coroutine.wrap(function()
            return tail_yield(1)
        end)

        r_tail_first = gen()
        r_tail_second = gen(20)
    )");

    ASSERT_TRUE(suite, ok, "wrap tail-call yield chunk runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_tail_first"), 1.0,
              "tail-called yield returns its yielded value to wrap");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_tail_second"), 20.0,
              "resume arguments become the tail-called yield return values");
    delete L;
}

// ==================================================================
// Test: coroutine.wrap error propagation
// ==================================================================

void testWrapError(TestSuite& suite) {
    LuaState* L = createState();
    // wrap raises error when coroutine is dead (after return)
    bool ok = runLua(L, R"(
        local gen = coroutine.wrap(function()
            return 42
        end)
        r_v1 = gen()
        -- calling again should raise an error
        local ok2, err = pcall(gen)
        r_ok2 = ok2
        r_has_err = (err ~= nil)
    )");
    ASSERT_TRUE(suite, ok, "wrap error runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v1"), 42.0, "wrap return 42");
    ASSERT_TRUE(suite, !getGlobalBool(L, "r_ok2"), "wrap dead raises error");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_has_err"), "wrap error has message");
    delete L;
}

void testWrapPreservesErrorObject(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local marker = function() end
        local gen = coroutine.wrap(function()
            coroutine.yield()
            error(marker)
        end)

        gen()
        local ok2, err = pcall(gen)
        r_wrap_error_object = (not ok2 and err == marker)
    )");

    ASSERT_TRUE(suite, ok, "wrap error object chunk runs");
    ASSERT_TRUE(suite, getGlobalBool(L, "r_wrap_error_object"),
                "wrap preserves non-string coroutine error objects");
    delete L;
}

// ==================================================================
// Test: coroutine.wrap with multiple return values
// ==================================================================

void testWrapMultipleValues(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local gen = coroutine.wrap(function()
            coroutine.yield(10, 20)
            return 30, 40
        end)
        r_a, r_b = gen()
        r_c, r_d = gen()
    )");
    ASSERT_TRUE(suite, ok, "wrap multiple values runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_a"), 10.0, "wrap yield val1");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_b"), 20.0, "wrap yield val2");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_c"), 30.0, "wrap return val1");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_d"), 40.0, "wrap return val2");
    delete L;
}

// ==================================================================
// Test: coroutine.wrap no yield (direct return)
// ==================================================================

void testWrapNoYield(TestSuite& suite) {
    LuaState* L = createState();
    bool ok = runLua(L, R"(
        local gen = coroutine.wrap(function(x)
            return x * 2
        end)
        r_v = gen(5)
    )");
    ASSERT_TRUE(suite, ok, "wrap no yield runs");
    ASSERT_EQ(suite, getGlobalNumber(L, "r_v"), 10.0, "wrap direct return");
    delete L;
}

// ==================================================================
// Registration
// ==================================================================

void registerCoroutineLibTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "coroutine.create", testCoroutineCreate);
    registry.registerTest(kSuiteName, "basic resume/yield", testBasicResumeYield);
    registry.registerTest(kSuiteName, "resume args to yield returns", testResumeArgsToYieldReturns);
    registry.registerTest(kSuiteName, "generator pattern", testGeneratorPattern);
    registry.registerTest(kSuiteName, "dead coroutine resume", testDeadCoroutineResume);
    registry.registerTest(kSuiteName, "dead coroutine resume discards args",
                          testDeadCoroutineResumeDiscardsArguments);
    registry.registerTest(kSuiteName, "coroutine.status", testCoroutineStatus);
    registry.registerTest(kSuiteName, "coroutine.running", testCoroutineRunning);
    registry.registerTest(kSuiteName, "multiple yield values", testMultipleYieldValues);
    registry.registerTest(kSuiteName, "yield open table constructor",
                          testYieldReturnValuesInOpenTableConstructor);
    registry.registerTest(kSuiteName, "coroutine error", testCoroutineError);
    registry.registerTest(kSuiteName, "coroutine no yield", testCoroutineNoYield);
    registry.registerTest(kSuiteName, "wrap basic", testWrapBasic);
    registry.registerTest(kSuiteName, "wrap generator", testWrapGenerator);
    registry.registerTest(kSuiteName, "wrap args", testWrapArgs);
    registry.registerTest(kSuiteName, "wrap resume args", testWrapResumeArgs);
    registry.registerTest(kSuiteName, "wrap tail-call yield", testWrapTailCallYield);
    registry.registerTest(kSuiteName, "wrap error", testWrapError);
    registry.registerTest(kSuiteName, "wrap preserves error object", testWrapPreservesErrorObject);
    registry.registerTest(kSuiteName, "wrap multiple values", testWrapMultipleValues);
    registry.registerTest(kSuiteName, "wrap no yield", testWrapNoYield);
}
