#include "../framework/test_framework.hpp"
#include "lib/oslib.hpp"
#include "vm/lua_state.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include <ctime>
#include <cstdlib>
#include <functional>
#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "OS Library";

// Helper function to call OS library functions from the os table
i32 callOSFunc(LuaState* L, const char* funcName, const std::function<void(LuaState*)>& pushArgs) {
    // 获取 os 表
    Value osTable = L->getGlobal("os");
    if (!osTable.isTable()) {
        return -1;
    }

    // 从表中获取函数
    Table* table = osTable.asTable();
    GCString* key = L->getGlobalState().getStringPool().intern(funcName);
    Value func = table->get(Value(key));

    if (!func.isFunction()) {
        return -1;
    }

    // 清空栈并推送参数
    L->getStack().clear();
    L->setAbsoluteTop(0);

    if (pushArgs) {
        pushArgs(L);
    }

    // 调用C函数
    Function* f = func.asFunction();
    if (f->isCFunction()) {
        return f->getCFunction()(L);
    }

    return -1;
}

} // namespace

void testClockWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openOSLib);
    LuaState* L = ctx.getState();
    
    i32 ret = callOSFunc(L, "clock", nullptr);
    ASSERT_EQ(suite, ret, 1, "clock returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber(), "clock returns number");
    ASSERT_TRUE(suite, L->top().asNumber() >= 0.0, "clock returns non-negative value");
}

void testDifftimeWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openOSLib);
    LuaState* L = ctx.getState();
    
    // 测试时间差计算
    i32 ret = callOSFunc(L, "difftime", [](LuaState* s) {
        s->pushNumber(1000.0);
        s->pushNumber(500.0);
    });
    ASSERT_EQ(suite, ret, 1, "difftime returns 1 value");
    ASSERT_EQ(suite, 500.0, L->top().asNumber(), "difftime(1000, 500) == 500");
    
    // 测试负数差值
    ret = callOSFunc(L, "difftime", [](LuaState* s) {
        s->pushNumber(100.0);
        s->pushNumber(200.0);
    });
    ASSERT_EQ(suite, -100.0, L->top().asNumber(), "difftime(100, 200) == -100");
}

void testTimeWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openOSLib);
    LuaState* L = ctx.getState();
    auto& pool = L->getGlobalState().getStringPool();
    
    // 测试无参数调用（当前时间）
    i32 ret = callOSFunc(L, "time", nullptr);
    ASSERT_EQ(suite, ret, 1, "time() returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber(), "time() returns number");
    ASSERT_TRUE(suite, L->top().asNumber() > 0.0, "time() returns positive value");
    
    // 测试时间表转换
    ret = callOSFunc(L, "time", [&pool](LuaState* s) {
        Table* t = new Table();
        s->getGlobalState().getGC().registerObject(t);
        
        t->set(Value(pool.intern("year")), Value(2023.0));
        t->set(Value(pool.intern("month")), Value(12.0));
        t->set(Value(pool.intern("day")), Value(25.0));
        t->set(Value(pool.intern("hour")), Value(10.0));
        t->set(Value(pool.intern("min")), Value(30.0));
        t->set(Value(pool.intern("sec")), Value(0.0));
        
        s->pushTable(t);
    });
    ASSERT_EQ(suite, ret, 1, "time(table) returns 1 value");
    ASSERT_TRUE(suite, L->top().isNumber(), "time(table) returns number");
}

void testDateWrapper(TestSuite& suite) {
    LuaStdLibTestContext ctx(openOSLib);
    LuaState* L = ctx.getState();
    auto& pool = L->getGlobalState().getStringPool();
    
    // 测试默认格式
    i32 ret = callOSFunc(L, "date", nullptr);
    ASSERT_EQ(suite, ret, 1, "date() returns 1 value");
    ASSERT_TRUE(suite, L->top().isString(), "date() returns string");
    
    // 测试自定义格式
    ret = callOSFunc(L, "date", [&pool](LuaState* s) {
        s->pushString(pool.intern("%Y-%m-%d"));
    });
    ASSERT_EQ(suite, ret, 1, "date('%Y-%m-%d') returns 1 value");
    ASSERT_TRUE(suite, L->top().isString(), "date('%Y-%m-%d') returns string");
    
    // 测试返回日期表
    ret = callOSFunc(L, "date", [&pool](LuaState* s) {
        s->pushString(pool.intern("*t"));
    });
    ASSERT_EQ(suite, ret, 1, "date('*t') returns 1 value");
    ASSERT_TRUE(suite, L->top().isTable(), "date('*t') returns table");
    
    // 验证日期表字段
    Table* dateTable = L->top().asTable();
    Value year = dateTable->get(Value(pool.intern("year")));
    Value month = dateTable->get(Value(pool.intern("month")));
    Value day = dateTable->get(Value(pool.intern("day")));
    
    ASSERT_TRUE(suite, year.isNumber(), "date table has 'year' field");
    ASSERT_TRUE(suite, month.isNumber(), "date table has 'month' field");
    ASSERT_TRUE(suite, day.isNumber(), "date table has 'day' field");
    ASSERT_TRUE(suite, year.asNumber() >= 2020.0, "year is reasonable");
    ASSERT_TRUE(suite, month.asNumber() >= 1.0 && month.asNumber() <= 12.0, "month is 1-12");
    ASSERT_TRUE(suite, day.asNumber() >= 1.0 && day.asNumber() <= 31.0, "day is 1-31");
}

void registerOSlibTests() {
    auto& registry = TestRegistry::getInstance();
    
    registry.registerTest(kSuiteName, "clock", testClockWrapper);
    registry.registerTest(kSuiteName, "difftime", testDifftimeWrapper);
    registry.registerTest(kSuiteName, "time", testTimeWrapper);
    registry.registerTest(kSuiteName, "date", testDateWrapper);
}

