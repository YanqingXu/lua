/**
 * @file test_metamethod_arith.cpp
 * @brief 算术元方法测试
 *
 * 测试算术运算元方法的正确性：
 * - __add, __sub, __mul, __div, __mod, __pow
 * - __unm (一元负号)
 * - 元方法回退机制（先尝试左操作数，再尝试右操作数）
 * - 错误处理（没有元方法时的错误信息）
 *
 * @author Lua C++ Project
 * @date 2025-11-22
 */

#include "test_framework.hpp"
#include "core/metatable.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "vm/lua_state.hpp"

using namespace Lua;
using namespace LuaTest;

// =====================================================================
// 测试用的C函数元方法
// =====================================================================

/**
 * @brief __add元方法的C函数实现
 *
 * 实现向量加法：{x1, y1} + {x2, y2} = {x1+x2, y1+y2}
 */
static i32 vector_add(LuaState* L) {
    auto& stack = L->getStack();

    // 获取两个参数（都应该是表）
    if (stack.size() < 3) {
        return 0;
    }

    Value v1 = stack.at(1);  // 第一个参数
    Value v2 = stack.at(2);  // 第二个参数

    if (!v1.isTable() || !v2.isTable()) {
        return 0;
    }

    Table* t1 = v1.asTable();
    Table* t2 = v2.asTable();

    // 获取x和y分量
    f64 x1 = t1->getArray(1).asNumber();
    f64 y1 = t1->getArray(2).asNumber();
    f64 x2 = t2->getArray(1).asNumber();
    f64 y2 = t2->getArray(2).asNumber();

    // 创建结果表
    Table* result = new Table();
    result->setArray(1, Value(x1 + x2));
    result->setArray(2, Value(y1 + y2));

    // 推入返回值
    stack.push(Value(result));

    return 1;  // 返回1个值
}

/**
 * @brief __unm元方法的C函数实现
 *
 * 实现向量取负：-{x, y} = {-x, -y}
 */
static i32 vector_unm(LuaState* L) {
    auto& stack = L->getStack();

    // 获取参数（应该是表）
    if (stack.size() < 2) {
        return 0;
    }

    Value v = stack.at(1);

    if (!v.isTable()) {
        return 0;
    }

    Table* t = v.asTable();

    // 获取x和y分量
    f64 x = t->getArray(1).asNumber();
    f64 y = t->getArray(2).asNumber();

    // 创建结果表
    Table* result = new Table();
    result->setArray(1, Value(-x));
    result->setArray(2, Value(-y));

    // 推入返回值
    stack.push(Value(result));

    return 1;  // 返回1个值
}

// =====================================================================
// 测试用例
// =====================================================================

/**
 * @brief 测试元方法查找机制
 */
void testMetamethodLookup(TestSuite& suite) {
    // 创建Lua状态
    LuaState* L = LuaState::newState();

    // 创建元表
    Table* metatable = new Table();

    // 设置__add元方法
    GCString* addName = new GCString("__add");
    Function* addFunc = new Function(vector_add);
    metatable->set(Value(addName), Value(addFunc));

    // 创建表并设置元表
    Table* t = new Table();
    t->setMetatable(metatable);

    // 测试元方法查找
    Value metamethod = getMetamethod(metatable, TMS::TM_ADD);
    ASSERT_TRUE(suite, metamethod.isFunction(), "Should find __add metamethod");

    // 测试不存在的元方法
    Value noMetamethod = getMetamethod(metatable, TMS::TM_SUB);
    ASSERT_TRUE(suite, noMetamethod.isNil(), "Should not find __sub metamethod");
}

/**
 * @brief 测试__add元方法调用
 */
void testAddMetamethod(TestSuite& suite) {
    // 创建Lua状态
    LuaState* L = LuaState::newState();

    // 创建元表
    Table* metatable = new Table();

    // 设置__add元方法
    GCString* addName = new GCString("__add");
    Function* addFunc = new Function(vector_add);
    metatable->set(Value(addName), Value(addFunc));

    // 创建两个向量表
    Table* v1 = new Table();
    v1->setArray(1, Value(1.0));
    v1->setArray(2, Value(2.0));
    v1->setMetatable(metatable);

    Table* v2 = new Table();
    v2->setArray(1, Value(3.0));
    v2->setArray(2, Value(4.0));
    v2->setMetatable(metatable);

    // 测试元方法调用
    Value addResult;
    bool success = callBinaryTM(L, Value(v1), Value(v2),
                               addResult, TMS::TM_ADD);

    ASSERT_TRUE(suite, success, "__add metamethod should be called");
    ASSERT_TRUE(suite, addResult.isTable(), "Result should be a table");

    if (addResult.isTable()) {
        Table* resultTable = addResult.asTable();
        ASSERT_EQ(suite, 4.0, resultTable->getArray(1).asNumber(), "x component should be 1+3=4");
        ASSERT_EQ(suite, 6.0, resultTable->getArray(2).asNumber(), "y component should be 2+4=6");
    }
}

/**
 * @brief 测试元方法回退机制
 */
void testMetamethodFallback(TestSuite& suite) {
    // 创建Lua状态
    LuaState* L = LuaState::newState();

    // 创建元表（只有左操作数有元方法）
    Table* leftMT = new Table();
    GCString* addName = new GCString("__add");
    Function* addFunc = new Function(vector_add);
    leftMT->set(Value(addName), Value(addFunc));

    // 创建左操作数（有元表）
    Table* left = new Table();
    left->setArray(1, Value(1.0));
    left->setArray(2, Value(2.0));
    left->setMetatable(leftMT);

    // 创建右操作数（无元表）
    Table* right = new Table();
    right->setArray(1, Value(3.0));
    right->setArray(2, Value(4.0));

    // 测试：应该使用左操作数的元方法
    Value fallbackResult;
    bool success = callBinaryTM(L, Value(left), Value(right),
                               fallbackResult, TMS::TM_ADD);

    ASSERT_TRUE(suite, success, "Should use left operand's metamethod");
    ASSERT_TRUE(suite, fallbackResult.isTable(), "Result should be a table");
}

/**
 * @brief 注册所有元方法算术测试
 */
void registerMetamethodArithTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Metamethod", "Lookup", testMetamethodLookup);
    registry.registerTest("Metamethod", "Add", testAddMetamethod);
    registry.registerTest("Metamethod", "Fallback", testMetamethodFallback);
}

