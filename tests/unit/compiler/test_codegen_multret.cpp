/**
 * @file test_codegen_multret.cpp
 * @brief Guardrail tests for multi-return semantics before removing expdesc.
 */

#include "../framework/test_framework.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "lib/lib_manager.hpp"
#include "vm/lua_state.hpp"
#include "vm/vm.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Codegen MultiRet";

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
        Proto* proto = codegen.generate(chunk, "test_codegen_multret");
        if (proto == nullptr) {
            return false;
        }

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

LuaState* createFullState() {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);
    return L;
}

} // namespace

void testParenthesizedCallCollapsesToSingleValue(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local function pack3()
            return 10, 20, 30
        end

        local single = (pack3())
        assert(single == 10, "parenthesized call should collapse to first value")

        local function collapse()
            return (pack3())
        end

        local a, b, c = collapse()
        assert(a == 10, "collapsed return should keep first value")
        assert(b == nil and c == nil, "collapsed return should discard trailing values")
    )lua");

    ASSERT_TRUE(suite, ok, "Parenthesized call collapses to single value");

    delete L;
}

void testOpenMultretPropagation(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local function pack3()
            return 10, 20, 30
        end

        local a, b, c = pack3()
        assert(a == 10 and b == 20 and c == 30, "assignment should keep all returned values")

        local p, q, r, s = pack3()
        assert(p == 10 and q == 20 and r == 30, "assignment should preserve the first three values")
        assert(s == nil, "assignment should pad extra targets with nil")
    )lua");

    ASSERT_TRUE(suite, ok, "Open multret propagation semantics");

    delete L;
}

void testTableConstructorLastFieldMultret(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"lua(
        local function pack3()
            return 10, 20, 30
        end

        local t = { pack3() }
        assert(t[1] == 10 and t[2] == 20 and t[3] == 30, "last table field should expand multret")
        assert(t[4] == nil, "expanded table field should stop after returned values")

        local t2 = { (pack3()) }
        assert(t2[1] == 10, "parenthesized call should still keep first table value")
        assert(t2[2] == nil and t2[3] == nil, "parenthesized table field should collapse multret")

        local t3 = { "head", pack3() }
        assert(t3[1] == "head", "fixed leading field should stay in place")
        assert(t3[2] == 10 and t3[3] == 20 and t3[4] == 30, "last field should expand after leading field")
    )lua");

    ASSERT_TRUE(suite, ok, "Table constructor last field multret semantics");

    delete L;
}

void registerCodegenMultiRetTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Parenthesized Call Collapses To Single Value", testParenthesizedCallCollapsesToSingleValue);
    registry.registerTest(kSuiteName, "Open MultiRet Propagation", testOpenMultretPropagation);
    registry.registerTest(kSuiteName, "Table Constructor Last Field MultiRet", testTableConstructorLastFieldMultret);
}
