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

#include "../framework/test_framework.hpp"
#include "core/metatable.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/state/global_state.hpp"
#include "vm/vm.hpp"
#include "lib/lib_manager.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

Table* createTestTable(ScopedGCRoots& roots) {
    return roots.create<Table>();
}

Function* createTestFunction(ScopedGCRoots& roots, CFunction function) {
    return roots.create<Function>(function);
}

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
        Proto* proto = codegen.generate(chunk, "metamethod_test");
        if (!proto)
            return false;

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

} // namespace

// =====================================================================
// 测试用的C函数元方法
// =====================================================================

/**
 * @brief __add元方法的C函数实现
 *
 * 实现向量加法：{x1, y1} + {x2, y2} = {x1+x2, y1+y2}
 *
 * 注意：在当前实现中，callTMWithResult会推入[func][arg1][arg2]到栈
 * 所以参数实际在savedTop+1和savedTop+2位置
 * 但由于我们不知道savedTop，需要从栈顶往回找参数
 */
static i32 vector_add(LuaState* L) {
    if (L->getTop() < 2) {
        return 0;
    }

    Value v1 = L->at(1);
    Value v2 = L->at(2);

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
    ScopedGCRoots roots(L);
    Table* result = createTestTable(roots);
    result->setArray(1, Value(x1 + x2));
    result->setArray(2, Value(y1 + y2));

    // 推入返回值
    L->pushTable(result);

    return 1; // 返回1个值
}

// =====================================================================
// 测试用例
// =====================================================================

/**
 * @brief 测试元方法查找机制
 */
void testMetamethodLookup(TestSuite& suite) {
    UPtr<LuaState> state = LuaState::create();
    LuaState* L = state.get();
    ScopedGCRoots roots(L);

    // 创建元表
    Table* metatable = createTestTable(roots);

    // 设置__add元方法
    // 使用StringPool获取内部化字符串，确保与 getMetamethod 中的查找使用相同的指针
    StringPool& pool = GlobalState::getInstance().getStringPool();
    GCString* addName = pool.intern("__add");
    Function* addFunc = createTestFunction(roots, vector_add);
    metatable->set(Value(addName), Value(addFunc));

    // 创建表并设置元表
    Table* t = createTestTable(roots);
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
    UPtr<LuaState> state = LuaState::create();
    LuaState* L = state.get();
    ScopedGCRoots roots(L);

    // 创建元表
    Table* metatable = createTestTable(roots);

    // 设置__add元方法
    // 使用StringPool获取内部化字符串
    StringPool& pool = GlobalState::getInstance().getStringPool();
    GCString* addName = pool.intern("__add");
    Function* addFunc = createTestFunction(roots, vector_add);
    metatable->set(Value(addName), Value(addFunc));

    // 创建两个向量表
    Table* v1 = createTestTable(roots);
    v1->setArray(1, Value(1.0));
    v1->setArray(2, Value(2.0));
    v1->setMetatable(metatable);

    Table* v2 = createTestTable(roots);
    v2->setArray(1, Value(3.0));
    v2->setArray(2, Value(4.0));
    v2->setMetatable(metatable);

    (void)L->getGlobalState().getGC().collect(L);

    // 测试元方法调用
    Value addResult;
    bool success = callBinaryTM(L, Value(v1), Value(v2), addResult, TMS::TM_ADD);

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
    UPtr<LuaState> state = LuaState::create();
    LuaState* L = state.get();
    ScopedGCRoots roots(L);

    // 创建元表（只有左操作数有元方法）
    Table* leftMT = createTestTable(roots);
    // 使用StringPool获取内部化字符串
    StringPool& pool = GlobalState::getInstance().getStringPool();
    GCString* addName = pool.intern("__add");
    Function* addFunc = createTestFunction(roots, vector_add);
    leftMT->set(Value(addName), Value(addFunc));

    // 创建左操作数（有元表）
    Table* left = createTestTable(roots);
    left->setArray(1, Value(1.0));
    left->setArray(2, Value(2.0));
    left->setMetatable(leftMT);

    // 创建右操作数（无元表）
    Table* right = createTestTable(roots);
    right->setArray(1, Value(3.0));
    right->setArray(2, Value(4.0));

    // 测试：应该使用左操作数的元方法
    Value fallbackResult;
    bool success = callBinaryTM(L, Value(left), Value(right), fallbackResult, TMS::TM_ADD);

    ASSERT_TRUE(suite, success, "Should use left operand's metamethod");
    ASSERT_TRUE(suite, fallbackResult.isTable(), "Result should be a table");
}

void testLuaFunctionMetamethodsAndBasicTypeMetatable(TestSuite& suite) {
    UPtr<LuaState> state = LuaState::create();
    LuaState* L = state.get();
    StandardLibrary::openAll(L);

    bool ok = runLua(L, R"lua(
        local mt = {
            __add = function(a, b)
                return { value = a.value + b.value }
            end,
            __index = function(_, key)
                return "fallback:" .. key
            end,
            __newindex = function(t, key, value)
                rawset(t, "set_" .. key, value * 2)
            end,
            __call = function(self, value)
                return self.value + value
            end
        }

        local a = setmetatable({ value = 10 }, mt)
        local b = setmetatable({ value = 32 }, mt)

        _sum = (a + b).value
        _indexed = a.missing
        a.answer = 21
        _newindex = a.set_answer
        _call = a(5)
        _string_method = ("abcdef"):sub(2, 4)
    )lua");

    ASSERT_TRUE(suite, ok, "Lua function metamethods should execute");
    ASSERT_EQ(suite, 42.0, L->getGlobal("_sum").asNumber(), "Lua __add result");
    ASSERT_TRUE(suite, L->getGlobal("_indexed").isString(), "Lua __index result is string");
    ASSERT_EQ(suite, std::string("fallback:missing"), std::string(L->getGlobal("_indexed").asString()->c_str()),
              "Lua __index fallback result");
    ASSERT_EQ(suite, 42.0, L->getGlobal("_newindex").asNumber(), "Lua __newindex side effect");
    ASSERT_EQ(suite, 15.0, L->getGlobal("_call").asNumber(), "Lua __call result");
    ASSERT_EQ(suite, std::string("bcd"), std::string(L->getGlobal("_string_method").asString()->c_str()),
              "string metatable __index enables method syntax");
}

void testRuntimeMetamethodOpcodeExecution(TestSuite& suite) {
    UPtr<LuaState> state = LuaState::create();
    LuaState* L = state.get();
    StandardLibrary::openAll(L);

    bool ok = runLua(L, R"lua(
        local mt = {
            __unm = function(a)
                return -a.value * 2
            end,
            __mod = function(a, b)
                return a.value - b.value
            end,
            __pow = function(a, b)
                return a.value * 10 + b.value
            end,
            __concat = function(a, b)
                return "cat:" .. a.value .. ":" .. b.value
            end
        }

        local a = setmetatable({ value = 5 }, mt)
        local b = setmetatable({ value = 3 }, mt)

        _runtime_unm = -a
        _runtime_mod = a % b
        _runtime_pow = a ^ b
        _runtime_concat = a .. b

        local callable = setmetatable({ base = 40 }, {
            __call = function(self, value)
                return self.base + value
            end
        })

        local function tail_call_target(obj)
            return obj(2)
        end

        _runtime_tailcall_call = tail_call_target(callable)
    )lua");

    ASSERT_TRUE(suite, ok, "runtime metamethod opcode script should execute");
    ASSERT_EQ(suite, -10.0, L->getGlobal("_runtime_unm").asNumber(), "runtime __unm opcode result");
    ASSERT_EQ(suite, 2.0, L->getGlobal("_runtime_mod").asNumber(), "runtime __mod opcode result");
    ASSERT_EQ(suite, 53.0, L->getGlobal("_runtime_pow").asNumber(), "runtime __pow opcode result");
    ASSERT_TRUE(suite, L->getGlobal("_runtime_concat").isString(), "runtime __concat result should be string");
    ASSERT_EQ(suite, std::string("cat:5:3"), std::string(L->getGlobal("_runtime_concat").asString()->c_str()),
              "runtime __concat opcode result");
    ASSERT_EQ(suite, 42.0, L->getGlobal("_runtime_tailcall_call").asNumber(),
              "tailcall through __call metamethod should return the metamethod result");
}

/**
 * @brief 注册所有元方法算术测试
 */
void registerMetamethodArithTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Metamethod", "Lookup", testMetamethodLookup);
    registry.registerTest("Metamethod", "Add", testAddMetamethod);
    registry.registerTest("Metamethod", "Fallback", testMetamethodFallback);
    registry.registerTest("Metamethod", "Lua function metamethods and basic type metatable",
                          testLuaFunctionMetamethodsAndBasicTypeMetatable);
    registry.registerTest("Metamethod", "Runtime metamethod opcode execution", testRuntimeMetamethodOpcodeExecution);
}
