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
#include "vm/lua_state.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/function.hpp"

#include <string>
#include <functional>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Table Library";

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
    
    // 检查 table 表是否存在
    Value tableTable = ctx.getGlobal("table");
    if (!tableTable.isTable()) {
        ASSERT_TRUE(suite, false, "table table exists");
        return;
    }
    
    // 测试 1: 在末尾插入
    Table* t1 = new Table();
    t1->set(Value(1.0), Value(10.0));
    t1->set(Value(2.0), Value(20.0));
    
    i32 ret = callTableFunc(L, "insert", [&](LuaState* s) {
        s->pushValue(Value(t1));
        s->pushNumber(30.0);
    });
    ASSERT_EQ(suite, ret, 0, "insert returns 0");
    ASSERT_EQ(suite, 30.0, t1->get(Value(3.0)).asNumber(), "insert at end");
    
    // 测试 2: 在指定位置插入
    Table* t2 = new Table();
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
    
    // 测试 1: 移除末尾元素
    Table* t1 = new Table();
    t1->set(Value(1.0), Value(10.0));
    t1->set(Value(2.0), Value(20.0));
    t1->set(Value(3.0), Value(30.0));
    
    i32 ret = callTableFunc(L, "remove", [&](LuaState* s) {
        s->pushValue(Value(t1));
    });
    ASSERT_EQ(suite, ret, 1, "remove returns 1");
    ASSERT_EQ(suite, 30.0, L->top().asNumber(), "removed value is 30");
    ASSERT_TRUE(suite, t1->get(Value(3.0)).isNil(), "element removed");
    
    // 测试 2: 移除指定位置元素
    Table* t2 = new Table();
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

    // 测试 1: 基本连接
    Table* t1 = new Table();
    t1->set(Value(1.0), Value(L->getGlobalState().getStringPool().intern("hello")));
    t1->set(Value(2.0), Value(L->getGlobalState().getStringPool().intern("world")));

    i32 ret = callTableFunc(L, "concat", [&](LuaState* s) {
        s->pushValue(Value(t1));
    });
    ASSERT_EQ(suite, ret, 1, "concat returns 1");
    Value result = L->top();
    if (result.isString()) {
        std::string str = result.asString()->c_str();
        ASSERT_TRUE(suite, str == "helloworld", "concat without separator");
    }

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
    Table* t2 = new Table();
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
}

// =====================================================================
// table.sort 测试
// =====================================================================

void testTableSort(TestSuite& suite) {
    LuaStdLibTestContext ctx(openTableLib);
    LuaState* L = ctx.getState();

    // 测试 1: 数字排序
    Table* t1 = new Table();
    t1->set(Value(1.0), Value(3.0));
    t1->set(Value(2.0), Value(1.0));
    t1->set(Value(3.0), Value(2.0));

    i32 ret = callTableFunc(L, "sort", [&](LuaState* s) {
        s->pushValue(Value(t1));
    });
    ASSERT_EQ(suite, ret, 0, "sort returns 0");
    ASSERT_EQ(suite, 1.0, t1->get(Value(1.0)).asNumber(), "sorted[1] = 1");
    ASSERT_EQ(suite, 2.0, t1->get(Value(2.0)).asNumber(), "sorted[2] = 2");
    ASSERT_EQ(suite, 3.0, t1->get(Value(3.0)).asNumber(), "sorted[3] = 3");

    // 测试 2: 字符串排序
    Table* t2 = new Table();
    t2->set(Value(1.0), Value(L->getGlobalState().getStringPool().intern("c")));
    t2->set(Value(2.0), Value(L->getGlobalState().getStringPool().intern("a")));
    t2->set(Value(3.0), Value(L->getGlobalState().getStringPool().intern("b")));

    ret = callTableFunc(L, "sort", [&](LuaState* s) {
        s->pushValue(Value(t2));
    });
    Value v1 = t2->get(Value(1.0));
    Value v2 = t2->get(Value(2.0));
    Value v3 = t2->get(Value(3.0));
    if (v1.isString() && v2.isString() && v3.isString()) {
        ASSERT_TRUE(suite, std::string(v1.asString()->c_str()) == "a", "sorted[1] = 'a'");
        ASSERT_TRUE(suite, std::string(v2.asString()->c_str()) == "b", "sorted[2] = 'b'");
        ASSERT_TRUE(suite, std::string(v3.asString()->c_str()) == "c", "sorted[3] = 'c'");
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

    // 测试 1: 解包整个表
    Table* t1 = new Table();
    t1->set(Value(1.0), Value(10.0));
    t1->set(Value(2.0), Value(20.0));
    t1->set(Value(3.0), Value(30.0));

    i32 ret = callTableFunc(L, "unpack", [&](LuaState* s) {
        s->pushValue(Value(t1));
    });
    ASSERT_EQ(suite, ret, 3, "unpack returns 3 values");
    ASSERT_EQ(suite, 30.0, L->at(-1).asNumber(), "unpack[3] = 30");
    ASSERT_EQ(suite, 20.0, L->at(-2).asNumber(), "unpack[2] = 20");
    ASSERT_EQ(suite, 10.0, L->at(-3).asNumber(), "unpack[1] = 10");

    // 测试 2: 解包指定范围
    ret = callTableFunc(L, "unpack", [&](LuaState* s) {
        s->pushValue(Value(t1));
        s->pushNumber(2.0);  // start
        s->pushNumber(3.0);  // end
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

    // 测试 1: 在同一个表内向前移动
    Table* t1 = new Table();
    t1->set(Value(1.0), Value(10.0));
    t1->set(Value(2.0), Value(20.0));
    t1->set(Value(3.0), Value(30.0));

    i32 ret = callTableFunc(L, "move", [&](LuaState* s) {
        s->pushValue(Value(t1));
        s->pushNumber(1.0);  // from
        s->pushNumber(2.0);  // to
        s->pushNumber(3.0);  // target position
    });
    ASSERT_EQ(suite, ret, 1, "move returns 1");
    ASSERT_EQ(suite, 10.0, t1->get(Value(3.0)).asNumber(), "move[3] = 10");
    ASSERT_EQ(suite, 20.0, t1->get(Value(4.0)).asNumber(), "move[4] = 20");

    // 测试 2: 在同一个表内向后移动
    Table* t2 = new Table();
    t2->set(Value(1.0), Value(10.0));
    t2->set(Value(2.0), Value(20.0));
    t2->set(Value(3.0), Value(30.0));
    t2->set(Value(4.0), Value(40.0));

    ret = callTableFunc(L, "move", [&](LuaState* s) {
        s->pushValue(Value(t2));
        s->pushNumber(3.0);  // from
        s->pushNumber(4.0);  // to
        s->pushNumber(1.0);  // target position
    });
    ASSERT_EQ(suite, 30.0, t2->get(Value(1.0)).asNumber(), "move backward[1] = 30");
    ASSERT_EQ(suite, 40.0, t2->get(Value(2.0)).asNumber(), "move backward[2] = 40");

    // 测试 3: 移动到不同的表
    Table* t3 = new Table();
    t3->set(Value(1.0), Value(100.0));
    t3->set(Value(2.0), Value(200.0));

    Table* t4 = new Table();

    ret = callTableFunc(L, "move", [&](LuaState* s) {
        s->pushValue(Value(t3));
        s->pushNumber(1.0);  // from
        s->pushNumber(2.0);  // to
        s->pushNumber(1.0);  // target position
        s->pushValue(Value(t4));  // target table
    });
    ASSERT_EQ(suite, 100.0, t4->get(Value(1.0)).asNumber(), "move to different table[1] = 100");
    ASSERT_EQ(suite, 200.0, t4->get(Value(2.0)).asNumber(), "move to different table[2] = 200");
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
    registry.registerTest(kSuiteName, "table.pack", testTablePack);
    registry.registerTest(kSuiteName, "table.unpack", testTableUnpack);
    registry.registerTest(kSuiteName, "table.move", testTableMove);
}



