#include "../../lib/base/base_lib.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../test_framework/core/test_macros.hpp"
#include <iostream>
#include <cassert>
#include <sstream>

namespace Lua::Tests {

/**
 * 新 Base Library 测试套件
 * 展示统一架构的使用方式和测试覆盖
 */
class BaseLibTestSuite {
public:
    /**
     * 运行所有测试
     */
    static void runAllTests();
    
private:
    // 测试组
    static void runCoreTests();
    static void runTableTests();
    static void runMetatableTests();
    static void runRawAccessTests();
    static void runErrorHandlingTests();
    static void runUtilityTests();
    
    // 核心函数测试
    static void testPrint();
    static void testType();
    static void testTostring();
    static void testTonumber();
    static void testError();
    static void testAssert();
    
    // 表操作测试
    static void testPairs();
    static void testIpairs();
    static void testNext();
    
    // 元表操作测试
    static void testGetmetatable();
    static void testSetmetatable();
    
    // 原始访问测试
    static void testRawget();
    static void testRawset();
    static void testRawlen();
    static void testRawequal();
    
    // 错误处理测试
    static void testPcall();
    static void testXpcall();
    
    // 工具函数测试
    static void testSelect();
    static void testUnpack();
    
    // 辅助方法
    static std::unique_ptr<State> createTestState();
    static void setupBaseLib(State* state);
};

// ===================================================================
// 测试实现
// ===================================================================

void BaseLibNewTestSuite::runAllTests() {
    std::cout << "\n=== BaseLib New Architecture Tests ===" << std::endl;
    
    RUN_TEST_GROUP("Core Functions", runCoreTests);
    RUN_TEST_GROUP("Table Operations", runTableTests);
    RUN_TEST_GROUP("Metatable Operations", runMetatableTests);
    RUN_TEST_GROUP("Raw Access", runRawAccessTests);
    RUN_TEST_GROUP("Error Handling", runErrorHandlingTests);
    RUN_TEST_GROUP("Utility Functions", runUtilityTests);
    
    std::cout << "=== All BaseLib New Tests Completed ===" << std::endl;
}

void BaseLibNewTestSuite::runCoreTests() {
    RUN_TEST(BaseLibNewTestSuite, testPrint);
    RUN_TEST(BaseLibNewTestSuite, testType);
    RUN_TEST(BaseLibNewTestSuite, testTostring);
    RUN_TEST(BaseLibNewTestSuite, testTonumber);
    RUN_TEST(BaseLibNewTestSuite, testError);
    RUN_TEST(BaseLibNewTestSuite, testAssert);
}

void BaseLibNewTestSuite::runTableTests() {
    RUN_TEST(BaseLibNewTestSuite, testPairs);
    RUN_TEST(BaseLibNewTestSuite, testIpairs);
    RUN_TEST(BaseLibNewTestSuite, testNext);
}

void BaseLibNewTestSuite::runMetatableTests() {
    RUN_TEST(BaseLibNewTestSuite, testGetmetatable);
    RUN_TEST(BaseLibNewTestSuite, testSetmetatable);
}

void BaseLibNewTestSuite::runRawAccessTests() {
    RUN_TEST(BaseLibNewTestSuite, testRawget);
    RUN_TEST(BaseLibNewTestSuite, testRawset);
    RUN_TEST(BaseLibNewTestSuite, testRawlen);
    RUN_TEST(BaseLibNewTestSuite, testRawequal);
}

void BaseLibNewTestSuite::runErrorHandlingTests() {
    RUN_TEST(BaseLibNewTestSuite, testPcall);
    RUN_TEST(BaseLibNewTestSuite, testXpcall);
}

void BaseLibNewTestSuite::runUtilityTests() {
    RUN_TEST(BaseLibNewTestSuite, testSelect);
    RUN_TEST(BaseLibNewTestSuite, testUnpack);
}

// ===================================================================
// 核心函数测试实现
// ===================================================================

void BaseLibNewTestSuite::testPrint() {
    auto state = createTestState();
    setupBaseLib(state.get());
    
    // 重定向 cout 来测试 print 输出
    std::ostringstream output;
    auto oldCout = std::cout.rdbuf();
    std::cout.rdbuf(output.rdbuf());
    
    try {
        // 测试 print 函数
        BaseLib baseLib;
        
        // 测试单个参数
        state->push(Value("Hello World"));
        Value result = baseLib.print(state.get(), 1);
        
        assert(result.isNil());
        assert(output.str() == "Hello World\n");
        
        // 测试多个参数
        output.str("");
        state->push(Value("Hello"));
        state->push(Value(42.0));
        state->push(Value(true));
        result = baseLib.print(state.get(), 3);
        
        assert(output.str() == "Hello\t42\ttrue\n");
        
        std::cout.rdbuf(oldCout);
        std::cout << "✓ print() test passed" << std::endl;
        
    } catch (...) {
        std::cout.rdbuf(oldCout);
        throw;
    }
}

void BaseLibNewTestSuite::testType() {
    auto state = createTestState();
    setupBaseLib(state.get());
    
    BaseLib baseLib;
    
    // 测试不同类型
    state->push(Value());
    Value result = baseLib.type(state.get(), 1);
    assert(result.isString() && result.asString() == "nil");
    
    state->push(Value(true));
    result = baseLib.type(state.get(), 1);
    assert(result.isString() && result.asString() == "boolean");
    
    state->push(Value(42.0));
    result = baseLib.type(state.get(), 1);
    assert(result.isString() && result.asString() == "number");
    
    state->push(Value("hello"));
    result = baseLib.type(state.get(), 1);
    assert(result.isString() && result.asString() == "string");
    
    std::cout << "✓ type() test passed" << std::endl;
}

void BaseLibNewTestSuite::testTostring() {
    auto state = createTestState();
    setupBaseLib(state.get());
    
    BaseLib baseLib;
    
    // 测试基本类型转换
    state->push(Value());
    Value result = baseLib.tostring(state.get(), 1);
    assert(result.isString() && result.asString() == "nil");
    
    state->push(Value(true));
    result = baseLib.tostring(state.get(), 1);
    assert(result.isString() && result.asString() == "true");
    
    state->push(Value(42.5));
    result = baseLib.tostring(state.get(), 1);
    assert(result.isString() && result.asString() == "42.5");
    
    state->push(Value("hello"));
    result = baseLib.tostring(state.get(), 1);
    assert(result.isString() && result.asString() == "hello");
    
    std::cout << "✓ tostring() test passed" << std::endl;
}

void BaseLibNewTestSuite::testTonumber() {
    auto state = createTestState();
    setupBaseLib(state.get());
    
    BaseLib baseLib;
    
    // 测试数字转换
    state->push(Value(42.0));
    Value result = baseLib.tonumber(state.get(), 1);
    assert(result.isNumber() && result.asNumber() == 42.0);
    
    // 测试字符串转换
    state->push(Value("123.45"));
    result = baseLib.tonumber(state.get(), 1);
    assert(result.isNumber() && result.asNumber() == 123.45);
    
    // 测试无效转换
    state->push(Value("not a number"));
    result = baseLib.tonumber(state.get(), 1);
    assert(result.isNil());
    
    // 测试进制转换
    state->push(Value("FF"));
    state->push(Value(16.0));
    result = baseLib.tonumber(state.get(), 2);
    assert(result.isNumber() && result.asNumber() == 255.0);
    
    std::cout << "✓ tonumber() test passed" << std::endl;
}

void BaseLibNewTestSuite::testAssert() {
    auto state = createTestState();
    setupBaseLib(state.get());
    
    BaseLib baseLib;
    
    // 测试成功断言
    state->push(Value(true));
    Value result = baseLib.assert_func(state.get(), 1);
    assert(result.isBoolean() && result.asBoolean() == true);
    
    state->push(Value(42.0));
    result = baseLib.assert_func(state.get(), 1);
    assert(result.isNumber() && result.asNumber() == 42.0);
    
    // 测试失败断言（这里需要捕获异常）
    try {
        state->push(Value(false));
        baseLib.assert_func(state.get(), 1);
        assert(false); // 不应该到达这里
    } catch (const std::exception&) {
        // 预期的异常
    }
    
    std::cout << "✓ assert() test passed" << std::endl;
}

// ===================================================================
// 表操作测试实现（示例）
// ===================================================================

void BaseLibNewTestSuite::testNext() {
    auto state = createTestState();
    setupBaseLib(state.get());
    
    BaseLib baseLib;
    
    // 创建测试表
    auto table = Table::create();
    table->set(Value("key1"), Value("value1"));
    table->set(Value("key2"), Value("value2"));
    table->set(Value(1.0), Value("array1"));
    
    // 测试从头开始迭代
    state->push(Value(table));
    Value result = baseLib.next(state.get(), 1);
    
    // next 应该返回第一个键值对
    assert(!result.isNil()); // 应该有键返回
    
    std::cout << "✓ next() test passed" << std::endl;
}

// ===================================================================
// 辅助方法实现
// ===================================================================

std::unique_ptr<State> BaseLibNewTestSuite::createTestState() {
    return std::make_unique<State>();
}

void BaseLibNewTestSuite::setupBaseLib(State* state) {
    // 设置测试环境中的基础库
    registerBaseLib(state);
}

// 其他测试方法的实现...
// 为了简洁，这里只展示几个关键测试的实现

void BaseLibNewTestSuite::testPairs() {
    std::cout << "✓ pairs() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testIpairs() {
    std::cout << "✓ ipairs() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testGetmetatable() {
    std::cout << "✓ getmetatable() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testSetmetatable() {
    std::cout << "✓ setmetatable() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testRawget() {
    std::cout << "✓ rawget() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testRawset() {
    std::cout << "✓ rawset() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testRawlen() {
    std::cout << "✓ rawlen() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testRawequal() {
    std::cout << "✓ rawequal() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testPcall() {
    std::cout << "✓ pcall() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testXpcall() {
    std::cout << "✓ xpcall() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testSelect() {
    std::cout << "✓ select() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testUnpack() {
    std::cout << "✓ unpack() test placeholder" << std::endl;
}

void BaseLibNewTestSuite::testError() {
    std::cout << "✓ error() test placeholder" << std::endl;
}

} // namespace Lua::Tests
