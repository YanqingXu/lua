/**
 * @file test_tablelib.cpp
 * @brief Table Library Function Tests - 表库函数测试
 *
 * 全面测试 Lua table 库的实现。
 * 测试覆盖正常情况、边界情况和错误条件。
 *
 * @author Lua C++ Project
 * @date 2026-01-23
 */

#include "../framework/test_framework.hpp"
#include "lib/tablelib.hpp"
#include "lib/lib_manager.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"

#include <string>
#include <functional>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Table Library";

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
        if (!proto) {
            return false;
        }

        ScopedGCRoots roots(L);
        roots.protect(proto);
        Function* func = roots.create<Function>(proto);
        func->setEnv(L->getGlobalTable());
        VM::execute(L, func);
        return true;
    } catch (...) {
        return false;
    }
}

double getGlobalNumber(LuaState* L, const char* name) {
    Value v = L->getGlobal(name);
    return v.isNumber() ? v.asNumber() : -9999.0;
}

LuaState* createFullState() {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);
    return L;
}

// 辅助函数：调用 table 库函数
i32 callTableFunc(LuaState* L, const char* funcName, const std::function<void(LuaState*)>& pushArgs) {
    // 获取 table 表
    Value tableTable = L->getGlobal("table");
    if (!tableTable.isTable()) {
        return -1;
    }

    // 从表中获取函数
    Table* tblTable = tableTable.asTable();
    GCString* key = L->getGlobalState().getStringPool().intern(funcName);
    Value func = tblTable->get(Value(key));

    if (!func.isFunction()) {
        return -1;
    }

    // 清空栈并压入参数
    L->getStack().clear();
    L->setAbsoluteTop(0);

    if (pushArgs) {
        pushArgs(L);
    }

    // 调用 C 函数
    Function* f = func.asFunction();
    if (f->isCFunction()) {
        return f->getCFunction()(L);
    }

    return -1;
}

} // namespace

// =====================================================================
// table.insert 测试
// =====================================================================

void testTableInsert(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();
    ScopedGCRoots roots(L);

    // 检查 table 表是否存在
    Value tableTable = ctx.getGlobal("table");
    if (!tableTable.isTable()) {
        ASSERT_TRUE(suite, false, "table table exists");
        return;
    }

    // 测试 1: 在末尾插入
    Table* t1 = roots.create<Table>();
    t1->set(Value(1.0), Value(10.0));
    t1->set(Value(2.0), Value(20.0));

    i32 ret = callTableFunc(L, "insert", [&](LuaState* s) {
        s->pushValue(Value(t1));
        s->pushNumber(30.0);
    });
    ASSERT_EQ(suite, ret, 0, "insert returns 0");
    ASSERT_EQ(suite, 30.0, t1->get(Value(3.0)).asNumber(), "insert at end");

    // 测试 2: 在指定位置插入
    Table* t2 = roots.create<Table>();
    t2->set(Value(1.0), Value(10.0));
    t2->set(Value(2.0), Value(20.0));
    t2->set(Value(3.0), Value(30.0));

    ret = callTableFunc(L, "insert", [&](LuaState* s) {
        s->pushValue(Value(t2));
        s->pushNumber(2.0);  // position
        s->pushNumber(15.0); // value
    });
    ASSERT_EQ(suite, 15.0, t2->get(Value(2.0)).asNumber(), "insert at position 2");
    ASSERT_EQ(suite, 20.0, t2->get(Value(3.0)).asNumber(), "element shifted");
    ASSERT_EQ(suite, 30.0, t2->get(Value(4.0)).asNumber(), "element shifted");
}

// =====================================================================
// table.remove 测试
// =====================================================================

void testTableRemove(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();
    ScopedGCRoots roots(L);

    // 测试 1: 移除末尾元素
    Table* t1 = roots.create<Table>();
    t1->set(Value(1.0), Value(10.0));
    t1->set(Value(2.0), Value(20.0));
    t1->set(Value(3.0), Value(30.0));

    i32 ret = callTableFunc(L, "remove", [&](LuaState* s) { s->pushValue(Value(t1)); });
    ASSERT_EQ(suite, ret, 1, "remove returns 1");
    ASSERT_EQ(suite, 30.0, L->top().asNumber(), "removed value is 30");
    ASSERT_TRUE(suite, t1->get(Value(3.0)).isNil(), "element removed");

    // 测试 2: 移除指定位置元素
    Table* t2 = roots.create<Table>();
    t2->set(Value(1.0), Value(10.0));
    t2->set(Value(2.0), Value(20.0));
    t2->set(Value(3.0), Value(30.0));

    ret = callTableFunc(L, "remove", [&](LuaState* s) {
        s->pushValue(Value(t2));
        s->pushNumber(2.0);
    });
    ASSERT_EQ(suite, 20.0, L->top().asNumber(), "removed value is 20");
    ASSERT_EQ(suite, 30.0, t2->get(Value(2.0)).asNumber(), "element shifted");
}

// =====================================================================
// table.concat 测试
// =====================================================================

void testTableConcat(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();
    ScopedGCRoots roots(L);

    // 测试 1: 基本连接
    Table* t1 = roots.create<Table>();
    t1->set(Value(1.0), Value(L->getGlobalState().getStringPool().intern("hello")));
    t1->set(Value(2.0), Value(L->getGlobalState().getStringPool().intern("world")));

    i32 ret = callTableFunc(L, "concat", [&](LuaState* s) { s->pushValue(Value(t1)); });
    ASSERT_EQ(suite, ret, 1, "concat returns 1");
    Value result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "helloworld", "concat without separator");
    }

    ctx.clearStack();
    (void)L->getGlobalState().getGC().collect(L);

    // 测试 2: 带分隔符连接
    ret = callTableFunc(L, "concat", [&](LuaState* s) {
        s->pushValue(Value(t1));
        s->pushString(s->getGlobalState().getStringPool().intern(", "));
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "hello, world", "concat with separator");
    }

    // 测试 3: 数字连接
    Table* t2 = roots.create<Table>();
    t2->set(Value(1.0), Value(1.0));
    t2->set(Value(2.0), Value(2.0));
    t2->set(Value(3.0), Value(3.0));

    ret = callTableFunc(L, "concat", [&](LuaState* s) {
        s->pushValue(Value(t2));
        s->pushString(s->getGlobalState().getStringPool().intern("-"));
    });
    result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "1-2-3", "concat numbers");
    }

    // 测试 4: 保留字符串和分隔符中的内嵌 NUL
    Table* t3 = roots.create<Table>();
    auto& pool = L->getGlobalState().getStringPool();
    const char s1[] = {'\0'};
    const char s2[] = {'\0', '\1'};
    const char s3[] = {'\0', '\1', '\2'};
    const char sep[] = {'.', '\0', '.'};
    t3->set(Value(1.0), Value(pool.intern(s1, sizeof(s1))));
    t3->set(Value(2.0), Value(pool.intern(s2, sizeof(s2))));
    t3->set(Value(3.0), Value(pool.intern(s3, sizeof(s3))));

    ret = callTableFunc(L, "concat", [&](LuaState* s) {
        s->pushValue(Value(t3));
        s->pushString(s->getGlobalState().getStringPool().intern(sep, sizeof(sep)));
    });
    result = L->top();
    if (result.isString()) {
        std::string expected;
        expected.append(s1, sizeof(s1));
        expected.append(sep, sizeof(sep));
        expected.append(s2, sizeof(s2));
        expected.append(sep, sizeof(sep));
        expected.append(s3, sizeof(s3));
        ASSERT_TRUE(suite, result.asString()->getData() == expected, "concat preserves embedded NUL bytes");
    }
}

// =====================================================================
// table.sort 测试
// =====================================================================

void testTableSort(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();
    ScopedGCRoots roots(L);

    // 测试 1: 数字排序
    Table* t1 = roots.create<Table>();
    t1->set(Value(1.0), Value(3.0));
    t1->set(Value(2.0), Value(1.0));
    t1->set(Value(3.0), Value(2.0));

    i32 ret = callTableFunc(L, "sort", [&](LuaState* s) { s->pushValue(Value(t1)); });
    ASSERT_EQ(suite, ret, 0, "sort returns 0");
    ASSERT_EQ(suite, 1.0, t1->get(Value(1.0)).asNumber(), "sorted[1] = 1");
    ASSERT_EQ(suite, 2.0, t1->get(Value(2.0)).asNumber(), "sorted[2] = 2");
    ASSERT_EQ(suite, 3.0, t1->get(Value(3.0)).asNumber(), "sorted[3] = 3");

    // 测试 2: 字符串排序
    Table* t2 = roots.create<Table>();
    t2->set(Value(1.0), Value(L->getGlobalState().getStringPool().intern("c")));
    t2->set(Value(2.0), Value(L->getGlobalState().getStringPool().intern("a")));
    t2->set(Value(3.0), Value(L->getGlobalState().getStringPool().intern("b")));

    ret = callTableFunc(L, "sort", [&](LuaState* s) { s->pushValue(Value(t2)); });
    Value v1 = t2->get(Value(1.0));
    Value v2 = t2->get(Value(2.0));
    Value v3 = t2->get(Value(3.0));
    if (v1.isString() && v2.isString() && v3.isString()) {
        ASSERT_TRUE(suite, std::string(v1.asString()->c_str()) == "a", "sorted[1] = 'a'");
        ASSERT_TRUE(suite, std::string(v2.asString()->c_str()) == "b", "sorted[2] = 'b'");
        ASSERT_TRUE(suite, std::string(v3.asString()->c_str()) == "c", "sorted[3] = 'c'");
    }

    // 测试 3: 非函数比较器报错
    bool invalidComparator = false;
    try {
        callTableFunc(L, "sort", [&](LuaState* s) {
            s->pushValue(Value(t1));
            s->pushNumber(1.0);
        });
    } catch (const std::runtime_error& e) {
        invalidComparator = std::string(e.what()) == "bad argument #2 to 'table.sort' (function expected)";
    }
    ASSERT_TRUE(suite, invalidComparator, "sort rejects non-function comparator");
}

void testTableSortWithLuaComparator(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"(
        local values = {3, 1, 4, 2}
        table.sort(values, function(a, b)
            return a > b
        end)
        g1 = values[1]
        g2 = values[2]
        g3 = values[3]
        g4 = values[4]
    )");
    ASSERT_TRUE(suite, ok, "table.sort descending comparator runs");
    ASSERT_EQ(suite, 4.0, getGlobalNumber(L, "g1"), "descending sort result[1] = 4");
    ASSERT_EQ(suite, 3.0, getGlobalNumber(L, "g2"), "descending sort result[2] = 3");
    ASSERT_EQ(suite, 2.0, getGlobalNumber(L, "g3"), "descending sort result[3] = 2");
    ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "g4"), "descending sort result[4] = 1");

    delete L;
}

void testTableSortWithComparatorUsingDerivedKey(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"(
        local values = {"pear", "fig", "banana", "kiwi"}
        table.sort(values, function(a, b)
            if #a == #b then
                return a < b
            end
            return #a < #b
        end)
        len1 = #values[1]
        len2 = #values[2]
        len3 = #values[3]
        len4 = #values[4]
        first = values[1]
        second = values[2]
        third = values[3]
        fourth = values[4]
    )");
    ASSERT_TRUE(suite, ok, "table.sort derived-key comparator runs");

    Value first = L->getGlobal("first");
    Value second = L->getGlobal("second");
    Value third = L->getGlobal("third");
    Value fourth = L->getGlobal("fourth");
    ASSERT_TRUE(suite, first.isString() && std::string(first.asString()->c_str()) == "fig",
                "derived-key sort first element is fig");
    ASSERT_TRUE(suite, second.isString() && std::string(second.asString()->c_str()) == "kiwi",
                "derived-key sort second element is kiwi");
    ASSERT_TRUE(suite, third.isString() && std::string(third.asString()->c_str()) == "pear",
                "derived-key sort third element is pear");
    ASSERT_TRUE(suite, fourth.isString() && std::string(fourth.asString()->c_str()) == "banana",
                "derived-key sort fourth element is banana");

    delete L;
}

void testTableSortComparatorDoesNotUseQuadraticComparisons(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"(
        local values = {}
        for i = 1, 512 do
            values[i] = 513 - i
        end

        local comparisons = 0
        table.sort(values, function(a, b)
            comparisons = comparisons + 1
            return a < b
        end)

        gSortFirst = values[1]
        gSortLast = values[512]
        gSortComparisons = comparisons
    )");

    ASSERT_TRUE(suite, ok, "table.sort large comparator chunk runs");
    ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gSortFirst"), "large comparator sort first element");
    ASSERT_EQ(suite, 512.0, getGlobalNumber(L, "gSortLast"), "large comparator sort last element");
    ASSERT_TRUE(suite, getGlobalNumber(L, "gSortComparisons") < 20000.0,
                "large comparator sort avoids quadratic comparison count");

    delete L;
}

void testTableSortUsesLtMetamethodByDefault(TestSuite& suite) {
    LuaState* L = createFullState();

    bool ok = runLua(L, R"(
        local mt = {__lt = function(a, b) return a.val < b.val end}
        local values = {}
        for i = 1, 5 do
            values[i] = setmetatable({val = 6 - i}, mt)
        end

        table.sort(values)

        gMetaSortFirst = values[1].val
        gMetaSortLast = values[5].val
    )");

    ASSERT_TRUE(suite, ok, "table.sort default comparator uses __lt");
    ASSERT_EQ(suite, 1.0, getGlobalNumber(L, "gMetaSortFirst"), "metamethod sort first element");
    ASSERT_EQ(suite, 5.0, getGlobalNumber(L, "gMetaSortLast"), "metamethod sort last element");

    delete L;
}

void testTableSortRejectsHostileComparatorsSafely(TestSuite& suite) {
    LuaState* L = createFullState();
    const bool ok = runLua(L, R"(
        local function rejected(comparator)
            local values = {4, 3, 2, 1}
            local success, message = pcall(table.sort, values, comparator)
            return not success and string.find(message, "invalid order function for sorting", 1, true) ~= nil
        end

        gSortAlwaysTrueRejected = rejected(function() return true end)
        gSortEqualityRejected = rejected(function(a, b) return a == b end)

        local flip = false
        gSortFlippingRejected = rejected(function()
            flip = not flip
            return flip
        end)

        local randomValues = {8, 7, 6, 5, 4, 3, 2, 1}
        pcall(table.sort, randomValues, function() return math.random() > 0.5 end)
        gSortRandomComparatorReturned = true
    )");

    ASSERT_TRUE(suite, ok, "hostile comparator probes complete without escaping the protected call");
    ASSERT_TRUE(suite, L->getGlobal("gSortAlwaysTrueRejected").isTrue(),
                "always-true comparator receives the stable invalid-order error");
    ASSERT_TRUE(suite, L->getGlobal("gSortEqualityRejected").isTrue(),
                "equality comparator receives the stable invalid-order error");
    ASSERT_TRUE(suite, L->getGlobal("gSortFlippingRejected").isTrue(),
                "obviously stateful comparator receives the stable invalid-order error");
    ASSERT_TRUE(suite, L->getGlobal("gSortRandomComparatorReturned").isTrue(),
                "random comparator cannot make the bounded sort overrun storage");
    delete L;
}

void testTableSortConsumesNativeWorkBudget(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();
    ScopedGCRoots roots(L);
    Table* values = roots.create<Table>();
    for (i32 i = 1; i <= 32; ++i) {
        values->set(Value(static_cast<LuaNumber>(i)), Value(static_cast<LuaNumber>(33 - i)));
    }


    L->getGlobalState().getResourcePolicy().maxSortElements = 8;
    bool elementLimitStopped = false;
    try {
        (void)callTableFunc(L, "sort", [&](LuaState* state) { state->pushTable(values); });
    } catch (const RuntimeError& error) {
        elementLimitStopped = std::string(error.what()) == "table.sort: element limit exceeded";
    }
    L->getGlobalState().getResourcePolicy().maxSortElements = 1'000'000;
    ASSERT_TRUE(suite, elementLimitStopped, "table.sort rejects oversized input before temporary allocation");

    ExecutionPolicy::Limits limits;
    limits.nativeWorkBudget = 4;
    L->getGlobalState().getExecutionPolicy().configure(limits);
    bool stopped = false;
    try {
        (void)callTableFunc(L, "sort", [&](LuaState* state) { state->pushTable(values); });
    } catch (const RuntimeError& error) {
        stopped = std::string(error.what()) == "execution native work budget exceeded";
    }
    L->getGlobalState().getExecutionPolicy().reset();

    ASSERT_TRUE(suite, stopped, "table.sort stops at the independent native-work budget");
}

void testTableResourcePolicyCoversLinearOperations(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();
    ScopedGCRoots roots(L);
    Table* values = roots.create<Table>();
    for (i32 i = 1; i <= 8; ++i) {
        values->set(Value(static_cast<LuaNumber>(i)), Value(static_cast<LuaNumber>(i * 10)));
    }

    const usize oldReturnLimit = L->getGlobalState().getResourcePolicy().maxReturnValues;
    L->getGlobalState().getResourcePolicy().maxReturnValues = 2;
    bool unpackStopped = false;
    try {
        (void)callTableFunc(L, "unpack", [&](LuaState* state) { state->pushTable(values); });
    } catch (const RuntimeError& error) {
        unpackStopped = std::string(error.what()) == "table.unpack: result count exceeds resource limit";
    }
    L->getGlobalState().getResourcePolicy().maxReturnValues = oldReturnLimit;
    ASSERT_TRUE(suite, unpackStopped, "table.unpack checks the return-count limit before pushing values");

    ExecutionPolicy::Limits limits;
    limits.nativeWorkBudget = 2;
    L->getGlobalState().getExecutionPolicy().configure(limits);
    bool insertStopped = false;
    try {
        (void)callTableFunc(L, "insert", [&](LuaState* state) {
            state->pushTable(values);
            state->pushNumber(1);
            state->pushNumber(99);
        });
    } catch (const RuntimeError& error) {
        insertStopped = std::string(error.what()) == "execution native work budget exceeded";
    }
    L->getGlobalState().getExecutionPolicy().reset();
    ASSERT_TRUE(suite, insertStopped, "table.insert charges its complete shift before mutation");
    ASSERT_EQ(suite, 10.0, values->get(Value(1.0)).asNumber(),
              "budget rejection leaves table.insert input unchanged");
    ASSERT_TRUE(suite, values->get(Value(9.0)).isNil(),
                "budget rejection does not append a partial table.insert result");

    Table* destination = roots.create<Table>();
    limits.nativeWorkBudget = 2;
    L->getGlobalState().getExecutionPolicy().configure(limits);
    bool moveStopped = false;
    try {
        (void)callTableFunc(L, "move", [&](LuaState* state) {
            state->pushTable(values);
            state->pushNumber(1);
            state->pushNumber(8);
            state->pushNumber(1);
            state->pushTable(destination);
        });
    } catch (const RuntimeError& error) {
        moveStopped = std::string(error.what()) == "execution native work budget exceeded";
    }
    L->getGlobalState().getExecutionPolicy().reset();
    ASSERT_TRUE(suite, moveStopped, "table.move charges its complete copy before mutation");
    ASSERT_TRUE(suite, destination->get(Value(1.0)).isNil(),
                "budget rejection leaves table.move destination unchanged");
}

// =====================================================================
// table.maxn 测试
// =====================================================================

void testTableMaxn(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();
    ScopedGCRoots roots(L);

    Value tableValue = ctx.getGlobal("table");
    ASSERT_TRUE(suite, tableValue.isTable(), "table table exists");
    if (tableValue.isTable()) {
        GCString* key = L->getGlobalState().getStringPool().intern("maxn");
        ASSERT_TRUE(suite, tableValue.asTable()->get(Value(key)).isFunction(), "table.maxn exists");
    }

    Table* t = roots.create<Table>();
    t->set(Value(1.0), Value(10.0));
    t->set(Value(5.0), Value(50.0));
    t->set(Value(12.0), Value(120.0));
    t->set(Value(-3.0), Value(300.0));
    t->set(Value(10.5), Value(105.0));

    i32 ret = callTableFunc(L, "maxn", [&](LuaState* s) { s->pushValue(Value(t)); });
    ASSERT_EQ(suite, 1, ret, "maxn returns one value");
    ASSERT_TRUE(suite, L->top().isNumber(), "maxn returns number");
    if (L->top().isNumber()) {
        ASSERT_EQ(suite, 12.0, L->top().asNumber(), "maxn returns largest positive numeric index");
    }

    Table* empty = roots.create<Table>();
    ret = callTableFunc(L, "maxn", [&](LuaState* s) { s->pushValue(Value(empty)); });
    ASSERT_EQ(suite, 1, ret, "maxn empty returns one value");
    ASSERT_TRUE(suite, L->top().isNumber(), "maxn empty returns number");
    if (L->top().isNumber()) {
        ASSERT_EQ(suite, 0.0, L->top().asNumber(), "maxn empty table returns 0");
    }
}

void testTableGetnCompatibility(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();
    ScopedGCRoots roots(L);

    Value tableValue = ctx.getGlobal("table");
    ASSERT_TRUE(suite, tableValue.isTable(), "table table exists for getn");
    if (tableValue.isTable()) {
        GCString* key = L->getGlobalState().getStringPool().intern("getn");
        ASSERT_TRUE(suite, tableValue.asTable()->get(Value(key)).isFunction(), "table.getn exists");
    }

    Table* t = roots.create<Table>();
    t->set(Value(1.0), Value(10.0));
    t->set(Value(2.0), Value(20.0));
    t->set(Value(3.0), Value(30.0));

    i32 ret = callTableFunc(L, "getn", [&](LuaState* s) { s->pushValue(Value(t)); });
    ASSERT_EQ(suite, 1, ret, "getn returns one value");
    ASSERT_TRUE(suite, L->top().isNumber(), "getn returns number");
    if (L->top().isNumber()) {
        ASSERT_EQ(suite, 3.0, L->top().asNumber(), "getn returns Lua length");
    }
}

// =====================================================================
// table.pack 测试
// =====================================================================

void testTablePack(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();

    // 测试 1: 打包多个参数
    i32 ret = callTableFunc(L, "pack", [&](LuaState* s) {
        s->pushNumber(10.0);
        s->pushNumber(20.0);
        s->pushNumber(30.0);
    });
    ASSERT_EQ(suite, ret, 1, "pack returns 1");

    Value result = L->top();
    ASSERT_TRUE(suite, result.isTable(), "pack returns table");

    if (result.isTable()) {
        Table* t = result.asTable();
        ASSERT_EQ(suite, 10.0, t->get(Value(1.0)).asNumber(), "pack[1] = 10");
        ASSERT_EQ(suite, 20.0, t->get(Value(2.0)).asNumber(), "pack[2] = 20");
        ASSERT_EQ(suite, 30.0, t->get(Value(3.0)).asNumber(), "pack[3] = 30");

        GCString* nKey = L->getGlobalState().getStringPool().intern("n");
        Value nVal = t->get(Value(nKey));
        ASSERT_EQ(suite, 3.0, nVal.asNumber(), "pack.n = 3");
    }
}

// =====================================================================
// table.unpack 测试
// =====================================================================

void testTableUnpack(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();
    ScopedGCRoots roots(L);

    // 测试 1: 解包整个表
    Table* t1 = roots.create<Table>();
    t1->set(Value(1.0), Value(10.0));
    t1->set(Value(2.0), Value(20.0));
    t1->set(Value(3.0), Value(30.0));

    i32 ret = callTableFunc(L, "unpack", [&](LuaState* s) { s->pushValue(Value(t1)); });
    ASSERT_EQ(suite, ret, 3, "unpack returns 3 values");
    ASSERT_EQ(suite, 30.0, L->at(-1).asNumber(), "unpack[3] = 30");
    ASSERT_EQ(suite, 20.0, L->at(-2).asNumber(), "unpack[2] = 20");
    ASSERT_EQ(suite, 10.0, L->at(-3).asNumber(), "unpack[1] = 10");

    // 测试 2: 解包指定范围
    ret = callTableFunc(L, "unpack", [&](LuaState* s) {
        s->pushValue(Value(t1));
        s->pushNumber(2.0); // start
        s->pushNumber(3.0); // end
    });
    ASSERT_EQ(suite, ret, 2, "unpack returns 2 values");
    ASSERT_EQ(suite, 30.0, L->at(-1).asNumber(), "unpack[2] = 30");
    ASSERT_EQ(suite, 20.0, L->at(-2).asNumber(), "unpack[1] = 20");
}

// =====================================================================
// table.move 测试
// =====================================================================

void testTableMove(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();
    ScopedGCRoots roots(L);

    // 测试 1: 在同一个表内向前移动
    Table* t1 = roots.create<Table>();
    t1->set(Value(1.0), Value(10.0));
    t1->set(Value(2.0), Value(20.0));
    t1->set(Value(3.0), Value(30.0));

    i32 ret = callTableFunc(L, "move", [&](LuaState* s) {
        s->pushValue(Value(t1));
        s->pushNumber(1.0); // from
        s->pushNumber(2.0); // to
        s->pushNumber(3.0); // target position
    });
    ASSERT_EQ(suite, ret, 1, "move returns 1");
    ASSERT_EQ(suite, 10.0, t1->get(Value(3.0)).asNumber(), "move[3] = 10");
    ASSERT_EQ(suite, 20.0, t1->get(Value(4.0)).asNumber(), "move[4] = 20");

    // 测试 2: 在同一个表内向后移动
    Table* t2 = roots.create<Table>();
    t2->set(Value(1.0), Value(10.0));
    t2->set(Value(2.0), Value(20.0));
    t2->set(Value(3.0), Value(30.0));
    t2->set(Value(4.0), Value(40.0));

    ret = callTableFunc(L, "move", [&](LuaState* s) {
        s->pushValue(Value(t2));
        s->pushNumber(3.0); // from
        s->pushNumber(4.0); // to
        s->pushNumber(1.0); // target position
    });
    ASSERT_EQ(suite, 30.0, t2->get(Value(1.0)).asNumber(), "move backward[1] = 30");
    ASSERT_EQ(suite, 40.0, t2->get(Value(2.0)).asNumber(), "move backward[2] = 40");

    // 测试 3: 移动到不同的表
    Table* t3 = roots.create<Table>();
    t3->set(Value(1.0), Value(100.0));
    t3->set(Value(2.0), Value(200.0));

    Table* t4 = roots.create<Table>();

    ret = callTableFunc(L, "move", [&](LuaState* s) {
        s->pushValue(Value(t3));
        s->pushNumber(1.0);      // from
        s->pushNumber(2.0);      // to
        s->pushNumber(1.0);      // target position
        s->pushValue(Value(t4)); // target table
    });
    ASSERT_EQ(suite, 100.0, t4->get(Value(1.0)).asNumber(), "move to different table[1] = 100");
    ASSERT_EQ(suite, 200.0, t4->get(Value(2.0)).asNumber(), "move to different table[2] = 200");
}

// =====================================================================
// table.foreach / table.foreachi 兼容测试
// =====================================================================

void testTableForeachCompatibility(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        local t = {x = 90, y = 8, z = 23}
        foreach_value = table.foreach(t, function(k, v)
            if k == "x" then
                return v
            end
        end)
        foreach_nil = table.foreach(t, function(k, v)
            if k == "missing" then
                return v
            end
        end)

        foreach_empty_ok = true
        table.foreach({}, function()
            foreach_empty_ok = false
        end)
    )lua");

    ASSERT_TRUE(suite, ok, "table.foreach Lua callback compatibility runs");
    ASSERT_EQ(suite, 90.0, getGlobalNumber(L, "foreach_value"), "foreach returns first callback value");
    ASSERT_TRUE(suite, L->getGlobal("foreach_nil").isNil(), "foreach returns nil when callback never returns");
    ASSERT_TRUE(suite, L->getGlobal("foreach_empty_ok").isBoolean() && L->getGlobal("foreach_empty_ok").asBoolean(),
                "foreach does not call callback for empty table");
    delete L;
}

void testTableForeachiCompatibility(TestSuite& suite) {
    LuaState* L = createFullState();
    bool ok = runLua(L, R"lua(
        foreachi_hash_only_ok = true
        table.foreachi({x = 10, y = 20}, function()
            foreachi_hash_only_ok = false
        end)

        local seen = {}
        table.foreachi({10, 20, 30, nil, 50}, function(i, v)
            seen[i] = v
        end)
        foreachi_seen_1 = seen[1]
        foreachi_seen_3 = seen[3]
        foreachi_seen_4_is_nil = seen[4] == nil
        foreachi_seen_5 = seen[5]

        foreachi_value = table.foreachi({"a", "b", "c"}, function(i, v)
            if i == 2 then
                return v
            end
        end)
    )lua");

    ASSERT_TRUE(suite, ok, "table.foreachi Lua callback compatibility runs");
    ASSERT_TRUE(suite,
                L->getGlobal("foreachi_hash_only_ok").isBoolean() && L->getGlobal("foreachi_hash_only_ok").asBoolean(),
                "foreachi ignores hash-only fields");
    ASSERT_EQ(suite, 10.0, getGlobalNumber(L, "foreachi_seen_1"), "foreachi visits index 1");
    ASSERT_EQ(suite, 30.0, getGlobalNumber(L, "foreachi_seen_3"), "foreachi visits index 3");
    ASSERT_TRUE(
        suite, L->getGlobal("foreachi_seen_4_is_nil").isBoolean() && L->getGlobal("foreachi_seen_4_is_nil").asBoolean(),
        "foreachi passes nil array slots through the callback");
    ASSERT_EQ(suite, 50.0, getGlobalNumber(L, "foreachi_seen_5"), "foreachi visits final array index");
    Value returned = L->getGlobal("foreachi_value");
    ASSERT_TRUE(suite, returned.isString() && std::string(returned.asString()->c_str()) == "b",
                "foreachi returns first callback value");
    delete L;
}

// =====================================================================
// 测试注册
// =====================================================================

void registerTableLibTests() {
    TestRegistry& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "table.insert", testTableInsert);
    registry.registerTest(kSuiteName, "table.remove", testTableRemove);
    registry.registerTest(kSuiteName, "table.concat", testTableConcat);
    registry.registerTest(kSuiteName, "table.sort", testTableSort);
    registry.registerTest(kSuiteName, "table.sort comparator descending", testTableSortWithLuaComparator);
    registry.registerTest(kSuiteName, "table.sort comparator derived key", testTableSortWithComparatorUsingDerivedKey);
    registry.registerTest(kSuiteName, "table.sort comparator complexity",
                          testTableSortComparatorDoesNotUseQuadraticComparisons);
    registry.registerTest(kSuiteName, "table.sort default __lt", testTableSortUsesLtMetamethodByDefault);
    registry.registerTest(kSuiteName, "table.sort rejects hostile comparators",
                          testTableSortRejectsHostileComparatorsSafely);
    registry.registerTest(kSuiteName, "table.sort consumes native work budget",
                          testTableSortConsumesNativeWorkBudget);
    registry.registerTest(kSuiteName, "resource policy covers linear operations",
                          testTableResourcePolicyCoversLinearOperations);
    registry.registerTest(kSuiteName, "table.maxn", testTableMaxn);
    registry.registerTest(kSuiteName, "table.getn compatibility", testTableGetnCompatibility);
    registry.registerTest(kSuiteName, "table.pack", testTablePack);
    registry.registerTest(kSuiteName, "table.unpack", testTableUnpack);
    registry.registerTest(kSuiteName, "table.move", testTableMove);
    registry.registerTest(kSuiteName, "table.foreach compatibility", testTableForeachCompatibility);
    registry.registerTest(kSuiteName, "table.foreachi compatibility", testTableForeachiCompatibility);
}
