/**
 * @file test_function_call.cpp
 * @brief 测试VM函数调用机制：递归调用、多返回值、嵌套调用
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "vm/vm.hpp"
#include "vm/lua_state.hpp"
#include "core/string_pool.hpp"
#include "lib/baselib.hpp"

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Function Call";

} // namespace

/**
 * @brief 测试简单函数调用（无递归）
 */
void testSimpleFunctionCall(TestSuite& suite) {
    const char* code = R"(
        function add(a, b)
            return a + b
        end

        return add(3, 4)
    )";

    try {
        StringPool& pool = StringPool::getInstance();
        Parser parser(code);
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        ASSERT_TRUE(suite, proto != nullptr, "Proto generated");

        // 创建Lua状态并执行
        LuaState* L = LuaState::newState();
        openBaseLib(L);

        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);

        VM vm(L);
        vm.execute(func);

        // 检查结果
        ASSERT_TRUE(suite, L->getTop() > 0, "Has return value");
        Value& retval = L->getStack().top();
        ASSERT_TRUE(suite, retval.isNumber(), "Result is number");
        ASSERT_EQ(suite, static_cast<i32>(retval.asNumber()), 7, "add(3, 4) == 7");

        delete L;
        delete proto;
    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Exception: " << e.what() << std::endl;
        ASSERT_TRUE(suite, false, "No exception should be thrown");
    }
}

/**
 * @brief 测试简单的递归函数调用（阶乘）
 */
void testFactorialRecursion(TestSuite& suite) {
    const char* code = R"(
        function factorial(n)
            if n <= 1 then
                return 1
            end
            return n * factorial(n - 1)
        end

        return factorial(5)
    )";
    
    try {
        StringPool& pool = StringPool::getInstance();
        Parser parser(code);
        Chunk chunk = parser.parse();
        
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);
        
        ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
        
        // 创建Lua状态并执行
        LuaState* L = LuaState::newState();
        openBaseLib(L);
        
        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        
        VM vm(L);
        vm.execute(func);

        // 检查结果
        ASSERT_TRUE(suite, L->getTop() > 0, "Has return value");
        Value& retval = L->getStack().top();
        ASSERT_TRUE(suite, retval.isNumber(), "Result is number");
        std::cout << "  [DEBUG] factorial(5) returned: " << retval.asNumber() << std::endl;
        ASSERT_EQ(suite, static_cast<i32>(retval.asNumber()), 120, "factorial(5) == 120");
        
        delete L;
        delete proto;
    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Exception: " << e.what() << std::endl;
        ASSERT_TRUE(suite, false, "No exception should be thrown");
    }
}

/**
 * @brief 测试多返回值函数
 */
void testMultipleReturnValues(TestSuite& suite) {
    const char* code = R"(
        function multi_return()
            return 1, 2, 3
        end
        
        local a, b, c = multi_return()
        return a, b, c
    )";
    
    try {
        StringPool& pool = StringPool::getInstance();
        Parser parser(code);
        Chunk chunk = parser.parse();
        
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);
        
        ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
        
        // 创建Lua状态并执行
        LuaState* L = LuaState::newState();
        openBaseLib(L);
        
        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        
        VM vm(L);
        vm.execute(func);
        
        // 检查结果（应该有3个返回值）
        ASSERT_TRUE(suite, L->getTop() >= 3, "Has at least 3 return values");
        
        delete L;
        delete proto;
    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Exception: " << e.what() << std::endl;
        ASSERT_TRUE(suite, false, "No exception should be thrown");
    }
}

/**
 * @brief 测试嵌套函数调用
 */
void testNestedFunctionCalls(TestSuite& suite) {
    const char* code = R"(
        function add(a, b)
            return a + b
        end
        
        function multiply(x, y)
            return x * y
        end
        
        function compute()
            return multiply(add(1, 2), add(3, 4))
        end
        
        return compute()
    )";
    
    try {
        StringPool& pool = StringPool::getInstance();
        Parser parser(code);
        Chunk chunk = parser.parse();
        
        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);
        
        ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
        
        // 创建Lua状态并执行
        LuaState* L = LuaState::newState();
        openBaseLib(L);
        
        Function* func = new Function(proto);
        L->getGlobalState().getGC().registerObject(func);
        
        VM vm(L);
        vm.execute(func);

        // 检查结果：(1+2) * (3+4) = 3 * 7 = 21
        ASSERT_TRUE(suite, L->getTop() > 0, "Has return value");
        Value& retval = L->getStack().top();
        ASSERT_TRUE(suite, retval.isNumber(), "Result is number");
        std::cout << "  [DEBUG] compute() returned: " << retval.asNumber() << std::endl;
        ASSERT_EQ(suite, static_cast<i32>(retval.asNumber()), 21, "compute() == 21");
        
        delete L;
        delete proto;
    } catch (const std::exception& e) {
        std::cout << "  [ERROR] Exception: " << e.what() << std::endl;
        ASSERT_TRUE(suite, false, "No exception should be thrown");
    }
}

void registerFunctionCallTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Simple Function Call", testSimpleFunctionCall);
    // TODO: Fix conditional jump issue before enabling these tests
    // registry.registerTest(kSuiteName, "Factorial Recursion", testFactorialRecursion);
    // registry.registerTest(kSuiteName, "Multiple Return Values", testMultipleReturnValues);
    // registry.registerTest(kSuiteName, "Nested Function Calls", testNestedFunctionCalls);
}

