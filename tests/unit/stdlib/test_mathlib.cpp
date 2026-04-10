#include "../framework/test_framework.hpp"
#include "lib/mathlib.hpp"
#include "vm/lua_state.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/function.hpp"

#include <cmath>
#include <functional>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Math Library";

i32 callMathFunc(LuaState* L, const char* funcName, const std::function<void(LuaState*)>& pushArgs) {
    Value mathTable = L->getGlobal("math");
    if (!mathTable.isTable()) {
        return -1;
    }

    Table* table = mathTable.asTable();
    GCString* key = L->getGlobalState().getStringPool().intern(funcName);
    Value func = table->get(Value(key));
    if (!func.isFunction()) {
        return -1;
    }

    L->getStack().clear();
    L->setAbsoluteTop(0);

    if (pushArgs) {
        pushArgs(L);
    }

    Function* f = func.asFunction();
    if (f->isCFunction()) {
        return f->getCFunction()(L);
    }

    return -1;
}

bool nearlyEqual(f64 left, f64 right, f64 epsilon = 1e-12) {
    return std::fabs(left - right) <= epsilon;
}

} // namespace

void testMathConstants(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathLib);
    LuaState* L = ctx.getState();

    Value mathTable = L->getGlobal("math");
    ASSERT_TRUE(suite, mathTable.isTable(), "math table exists");
    if (!mathTable.isTable()) {
        return;
    }

    Table* table = mathTable.asTable();
    auto& pool = L->getGlobalState().getStringPool();
    Value pi = table->get(Value(pool.intern("pi")));
    Value huge = table->get(Value(pool.intern("huge")));

    ASSERT_TRUE(suite, pi.isNumber(), "math.pi is number");
    ASSERT_TRUE(suite, huge.isNumber(), "math.huge is number");
    if (pi.isNumber()) {
        ASSERT_TRUE(suite, nearlyEqual(pi.asNumber(), std::acos(-1.0)), "math.pi has expected value");
    }
}

void testMathHyperbolicFunctions(TestSuite& suite) {
    LuaStdLibTestContext ctx(openMathLib);
    LuaState* L = ctx.getState();

    i32 ret = callMathFunc(L, "sinh", [](LuaState* s) {
        s->pushNumber(0.0);
    });
    ASSERT_EQ(suite, ret, 1, "math.sinh returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber(), "math.sinh returns number");
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), 0.0), "math.sinh(0) == 0");

    ret = callMathFunc(L, "cosh", [](LuaState* s) {
        s->pushNumber(0.0);
    });
    ASSERT_EQ(suite, ret, 1, "math.cosh returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber(), "math.cosh returns number");
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), 1.0), "math.cosh(0) == 1");

    ret = callMathFunc(L, "tanh", [](LuaState* s) {
        s->pushNumber(0.0);
    });
    ASSERT_EQ(suite, ret, 1, "math.tanh returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber(), "math.tanh returns number");
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), 0.0), "math.tanh(0) == 0");

    ret = callMathFunc(L, "sinh", [](LuaState* s) {
        s->pushNumber(1.0);
    });
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), std::sinh(1.0)), "math.sinh(1) matches std::sinh");

    ret = callMathFunc(L, "cosh", [](LuaState* s) {
        s->pushNumber(1.0);
    });
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), std::cosh(1.0)), "math.cosh(1) matches std::cosh");

    ret = callMathFunc(L, "tanh", [](LuaState* s) {
        s->pushNumber(1.0);
    });
    ASSERT_TRUE(suite, nearlyEqual(L->top().asNumber(), std::tanh(1.0)), "math.tanh(1) matches std::tanh");
}

void registerMathLibTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "constants", testMathConstants);
    registry.registerTest(kSuiteName, "hyperbolic functions", testMathHyperbolicFunctions);
}