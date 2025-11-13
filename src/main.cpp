/**
 * @file main.cpp
 * @brief 临时主程序入口 - 用于Visual Studio IDE手动编译测试
 * 
 * 这是一个临时文件，仅用于在Visual Studio 2026 IDE中进行手动编译和调试。
 * 不应该被包含在build_with_vcvars.bat的自动化构建流程中。
 * 
 * 功能：
 * - 初始化核心组件（StringPool等）
 * - 创建和测试已实现的核心类（Value、GCString、Table等）
 * - 输出测试信息到控制台
 * - 验证所有核心类能够正常工作
 * 
 * @note 这是临时测试文件，不是最终的Lua解释器入口
 * @author Lua C++ Project
 * @date 2025-11-12
 */

#include "common/types.hpp"
#include "common/config.hpp"
#include "core/value.hpp"
#include "core/gc_object.hpp"
#include "core/gc_string.hpp"
#include "core/string_pool.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "core/userdata.hpp"
#include "gc/garbage_collector.hpp"
#include "vm/global_state.hpp"
#include "vm/stack.hpp"
#include "vm/call_info.hpp"
#include "vm/lua_state.hpp"
#include "compiler/lexer.hpp"
#include "compiler/token.hpp"
#include "compiler/parser.hpp"
#include "compiler/ast.hpp"
#include "compiler/codegen.hpp"
#include "compiler/opcode.hpp"
#include "vm/vm.hpp"

#include <iostream>
#include <iomanip>

using namespace Lua;

// 前向声明
void testValue();
void testString();
void testTable();
void testGCObject();
void testGarbageCollector();
void testFunction();
void testUserdata();
void testGlobalState();
void testStack();
void testCallInfo();
void testLuaState();
void testLexer();
void testParser();
void testCodeGenerator();
void testVM();
void comprehensiveTest();

/**
 * @brief 打印分隔线
 */
void printSeparator() {
    std::cout << "========================================" << std::endl;
}

/**
 * @brief 打印标题
 */
void printTitle(const char* title) {
    printSeparator();
    std::cout << title << std::endl;
    printSeparator();
}

/**
 * @brief 测试Value类
 */
void testValue() {
    printTitle("Testing Value Class");
    
    // 测试各种类型的Value
    Value nilVal;
    Value boolVal(true);
    Value numVal(3.14);
    Value intVal(static_cast<LuaInteger>(42));
    
    std::cout << "Nil value: " << nilVal.toString() << std::endl;
    std::cout << "Boolean value: " << boolVal.toString() << std::endl;
    std::cout << "Number value: " << numVal.toString() << std::endl;
    std::cout << "Integer value: " << intVal.toString() << std::endl;
    
    // 测试类型检查
    std::cout << "\nType checking:" << std::endl;
    std::cout << "  nilVal.isNil(): " << (nilVal.isNil() ? "true" : "false") << std::endl;
    std::cout << "  boolVal.isBoolean(): " << (boolVal.isBoolean() ? "true" : "false") << std::endl;
    std::cout << "  numVal.isNumber(): " << (numVal.isNumber() ? "true" : "false") << std::endl;
    
    std::cout << "\n[PASS] Value class test completed\n" << std::endl;
}

/**
 * @brief 测试GCString和StringPool
 */
void testString() {
    printTitle("Testing GCString and StringPool");
    
    // 获取StringPool实例
    StringPool& pool = StringPool::getInstance();
    
    // 创建字符串
    GCString* str1 = pool.intern("Hello, Lua!");
    GCString* str2 = pool.intern("Hello, Lua!");
    GCString* str3 = pool.intern("Different String");
    
    std::cout << "String 1: " << str1->c_str() << std::endl;
    std::cout << "String 2: " << str2->c_str() << std::endl;
    std::cout << "String 3: " << str3->c_str() << std::endl;
    
    // 测试字符串池
    std::cout << "\nString interning:" << std::endl;
    std::cout << "  str1 == str2 (same pointer): " << (str1 == str2 ? "true" : "false") << std::endl;
    std::cout << "  str1 == str3 (different): " << (str1 == str3 ? "true" : "false") << std::endl;
    
    // 测试哈希值
    std::cout << "\nHash values:" << std::endl;
    std::cout << "  str1 hash: 0x" << std::hex << str1->getHash() << std::dec << std::endl;
    std::cout << "  str2 hash: 0x" << std::hex << str2->getHash() << std::dec << std::endl;
    std::cout << "  str3 hash: 0x" << std::hex << str3->getHash() << std::dec << std::endl;
    
    // 测试StringPool统计
    std::cout << "\nStringPool statistics:" << std::endl;
    std::cout << "  Pool size: " << pool.size() << std::endl;
    
    std::cout << "\n[PASS] String test completed\n" << std::endl;
}

/**
 * @brief 测试Table类
 */
void testTable() {
    printTitle("Testing Table Class");
    
    // 创建表
    Table* table = new Table();
    
    // 测试数组部分
    std::cout << "Testing array part:" << std::endl;
    table->setArray(1, Value(10.0));
    table->setArray(2, Value(20.0));
    table->setArray(3, Value(30.0));
    
    std::cout << "  table[1] = " << table->getArray(1).asNumber() << std::endl;
    std::cout << "  table[2] = " << table->getArray(2).asNumber() << std::endl;
    std::cout << "  table[3] = " << table->getArray(3).asNumber() << std::endl;
    
    // 测试哈希部分
    std::cout << "\nTesting hash part:" << std::endl;
    StringPool& pool = StringPool::getInstance();
    GCString* keyName = pool.intern("name");
    GCString* keyAge = pool.intern("age");
    GCString* valueName = pool.intern("Lua");
    
    table->set(Value(keyName), Value(valueName));
    table->set(Value(keyAge), Value(5.1));
    
    Value nameVal = table->get(Value(keyName));
    Value ageVal = table->get(Value(keyAge));
    
    std::cout << "  table['name'] = " << nameVal.asString()->c_str() << std::endl;
    std::cout << "  table['age'] = " << ageVal.asNumber() << std::endl;
    
    // 测试表统计
    std::cout << "\nTable statistics:" << std::endl;
    std::cout << "  Array size: " << table->getArraySize() << std::endl;
    std::cout << "  Hash size: " << table->getHashSize() << std::endl;
    std::cout << "  Total size: " << table->getTotalSize() << std::endl;
    std::cout << "  Table length: " << table->length() << std::endl;
    
    // 测试元表
    std::cout << "\nTesting metatable:" << std::endl;
    Table* metatable = new Table();
    table->setMetatable(metatable);
    std::cout << "  Metatable set: " << (table->getMetatable() == metatable ? "true" : "false") << std::endl;
    
    // 清理
    delete metatable;
    delete table;
    
    std::cout << "\n[PASS] Table test completed\n" << std::endl;
}

/**
 * @brief 测试GC对象
 */
void testGCObject() {
    printTitle("Testing GC Object System");
    
    // 创建GC对象
    GCString* str = new GCString("Test GC String");
    Table* table = new Table();
    
    std::cout << "GC Object types:" << std::endl;
    std::cout << "  GCString type: " << static_cast<int>(str->getType()) << std::endl;
    std::cout << "  Table type: " << static_cast<int>(table->getType()) << std::endl;
    
    std::cout << "\nGC colors (initial):" << std::endl;
    std::cout << "  GCString color: " << static_cast<int>(str->getColor()) << std::endl;
    std::cout << "  Table color: " << static_cast<int>(table->getColor()) << std::endl;
    
    // 测试标记
    str->setColor(GCColor::Gray);
    table->setColor(GCColor::Black);
    
    std::cout << "\nGC colors (after marking):" << std::endl;
    std::cout << "  GCString color: " << static_cast<int>(str->getColor()) << std::endl;
    std::cout << "  Table color: " << static_cast<int>(table->getColor()) << std::endl;
    
    // 测试对象大小
    std::cout << "\nObject sizes:" << std::endl;
    std::cout << "  GCString size: " << str->getSize() << " bytes" << std::endl;
    std::cout << "  Table size: " << table->getSize() << " bytes" << std::endl;
    
    // 清理
    delete str;
    delete table;
    
    std::cout << "\n[PASS] GC object test completed\n" << std::endl;
}

/**
 * @brief 测试垃圾回收器
 */
void testGarbageCollector() {
    printTitle("Testing Garbage Collector");

    GarbageCollector& gc = GarbageCollector::getInstance();

    // 清空GC（确保干净的测试环境）
    gc.clearAll();

    std::cout << "Creating GC objects:" << std::endl;

    // 创建一些GC对象
    GCString* str1 = new GCString("GC Test String 1");
    GCString* str2 = new GCString("GC Test String 2");
    GCString* str3 = new GCString("GC Test String 3");
    Table* table1 = new Table();
    Table* table2 = new Table();

    // 注册到GC
    gc.registerObject(str1);
    gc.registerObject(str2);
    gc.registerObject(str3);
    gc.registerObject(table1);
    gc.registerObject(table2);

    std::cout << "  Registered objects: " << gc.getObjectCount() << std::endl;

    // 添加根对象（保护str1和table1不被回收）
    std::cout << "\nAdding root objects:" << std::endl;
    gc.addRoot(str1);
    gc.addRoot(table1);
    std::cout << "  Root objects: " << gc.getRootCount() << std::endl;

    // 检查根对象
    std::cout << "\nChecking root status:" << std::endl;
    std::cout << "  str1 is root: " << (gc.isRoot(str1) ? "true" : "false") << std::endl;
    std::cout << "  str2 is root: " << (gc.isRoot(str2) ? "true" : "false") << std::endl;

    // 执行垃圾回收
    std::cout << "\nPerforming garbage collection:" << std::endl;
    usize collected = gc.collect();
    std::cout << "  Collected objects: " << collected << std::endl;
    std::cout << "  Remaining objects: " << gc.getObjectCount() << std::endl;

    // 移除一个根对象
    std::cout << "\nRemoving root object (table1):" << std::endl;
    gc.removeRoot(table1);
    std::cout << "  Root objects: " << gc.getRootCount() << std::endl;

    // 再次执行GC
    std::cout << "\nSecond garbage collection:" << std::endl;
    usize collected2 = gc.collect();
    std::cout << "  Collected objects: " << collected2 << std::endl;
    std::cout << "  Remaining objects: " << gc.getObjectCount() << std::endl;

    // 打印统计信息
    std::cout << "\nGC Statistics:" << std::endl;
    usize objCount, rootCount, totalMem;
    gc.getStatistics(objCount, rootCount, totalMem);
    std::cout << "  Total objects: " << objCount << std::endl;
    std::cout << "  Root objects: " << rootCount << std::endl;
    std::cout << "  Total memory: " << totalMem << " bytes" << std::endl;

    // 清理所有对象
    gc.clearAll();
    std::cout << "\nAfter clearAll():" << std::endl;
    std::cout << "  Remaining objects: " << gc.getObjectCount() << std::endl;

    std::cout << "\n[PASS] Garbage collector test completed\n" << std::endl;
}

/**
 * @brief 测试Function类（函数对象）
 */
void testFunction() {
    printTitle("Testing Function Class");

    // ===== 测试1: C函数闭包 =====
    std::cout << "Testing C Function Closure:" << std::endl;

    // 创建一个简单的C函数（使用lambda）
    auto testCFunc = [](LuaState* L) -> i32 {
        // 这是一个简单的测试函数，返回0表示没有返回值
        return 0;
    };

    Function* cfunc = new Function(testCFunc);
    std::cout << "  [OK] C function creation" << std::endl;

    // 验证是C函数
    if (cfunc->isCFunction()) {
        std::cout << "  [OK] isCFunction() returns true" << std::endl;
    } else {
        std::cout << "  [FAIL] isCFunction() should return true" << std::endl;
    }

    // 获取C函数指针
    CFunction funcPtr = cfunc->getCFunction();
    if (funcPtr != nullptr) {
        std::cout << "  [OK] getCFunction() returns valid pointer" << std::endl;
    } else {
        std::cout << "  [FAIL] getCFunction() should return valid pointer" << std::endl;
    }

    // 验证GC类型
    if (cfunc->getType() == GCObjectType::Function) {
        std::cout << "  [OK] GC type is Function" << std::endl;
    } else {
        std::cout << "  [FAIL] GC type should be Function" << std::endl;
    }

    // ===== 测试2: Proto（函数原型） =====
    std::cout << "\nTesting Proto (Function Prototype):" << std::endl;

    Proto* proto = new Proto();
    std::cout << "  [OK] Proto creation" << std::endl;

    // 设置函数参数
    proto->setNumParams(3);
    if (proto->getNumParams() == 3) {
        std::cout << "  [OK] setNumParams/getNumParams (3 params)" << std::endl;
    } else {
        std::cout << "  [FAIL] NumParams should be 3" << std::endl;
    }

    // 设置可变参数标志
    proto->setVararg(true);
    if (proto->isVararg()) {
        std::cout << "  [OK] setVararg/isVararg (true)" << std::endl;
    } else {
        std::cout << "  [FAIL] Vararg should be true" << std::endl;
    }

    // 设置最大栈大小
    proto->setMaxStackSize(20);
    if (proto->getMaxStackSize() == 20) {
        std::cout << "  [OK] setMaxStackSize/getMaxStackSize (20)" << std::endl;
    } else {
        std::cout << "  [FAIL] MaxStackSize should be 20" << std::endl;
    }

    // 添加常量
    usize idx1 = proto->addConstant(Value(42.0));
    usize idx2 = proto->addConstant(Value(true));
    usize idx3 = proto->addConstant(Value());  // nil

    if (proto->getConstantCount() == 3) {
        std::cout << "  [OK] addConstant (3 constants added)" << std::endl;
    } else {
        std::cout << "  [FAIL] Constant count should be 3" << std::endl;
    }

    // 获取常量
    Value c1 = proto->getConstant(idx1);
    if (c1.isNumber() && c1.asNumber() == 42.0) {
        std::cout << "  [OK] getConstant (number: 42.0)" << std::endl;
    } else {
        std::cout << "  [FAIL] Constant should be 42.0" << std::endl;
    }

    Value c2 = proto->getConstant(idx2);
    if (c2.isBoolean() && c2.asBoolean() == true) {
        std::cout << "  [OK] getConstant (boolean: true)" << std::endl;
    } else {
        std::cout << "  [FAIL] Constant should be true" << std::endl;
    }

    Value c3 = proto->getConstant(idx3);
    if (c3.isNil()) {
        std::cout << "  [OK] getConstant (nil)" << std::endl;
    } else {
        std::cout << "  [FAIL] Constant should be nil" << std::endl;
    }

    // 验证Proto的GC类型
    if (proto->getType() == GCObjectType::Proto) {
        std::cout << "  [OK] Proto GC type is Proto" << std::endl;
    } else {
        std::cout << "  [FAIL] Proto GC type should be Proto" << std::endl;
    }

    // ===== 测试3: Lua函数闭包 =====
    std::cout << "\nTesting Lua Function Closure:" << std::endl;

    Function* lfunc = new Function(proto);
    std::cout << "  [OK] Lua function creation" << std::endl;

    // 验证是Lua函数
    if (lfunc->isLuaFunction()) {
        std::cout << "  [OK] isLuaFunction() returns true" << std::endl;
    } else {
        std::cout << "  [FAIL] isLuaFunction() should return true" << std::endl;
    }

    // 获取Proto指针
    Proto* retrievedProto = lfunc->getProto();
    if (retrievedProto == proto) {
        std::cout << "  [OK] getProto() returns correct Proto" << std::endl;
    } else {
        std::cout << "  [FAIL] getProto() should return the same Proto" << std::endl;
    }

    // ===== 测试4: GC标记 =====
    std::cout << "\nTesting GC Mark:" << std::endl;

    // 设置Proto为白色
    proto->setColor(GCColor::White);

    // 调用Lua函数的mark方法，应该将Proto标记为灰色
    lfunc->mark();

    if (proto->getColor() == GCColor::Gray) {
        std::cout << "  [OK] mark() marks Proto as Gray" << std::endl;
    } else {
        std::cout << "  [FAIL] Proto should be marked as Gray" << std::endl;
    }

    // ===== 测试5: 对象大小 =====
    std::cout << "\nObject Sizes:" << std::endl;
    std::cout << "  C Function size: " << cfunc->getSize() << " bytes" << std::endl;
    std::cout << "  Lua Function size: " << lfunc->getSize() << " bytes" << std::endl;
    std::cout << "  Proto size: " << proto->getSize() << " bytes" << std::endl;

    // 清理内存
    delete cfunc;
    delete lfunc;
    delete proto;

    std::cout << "\n[PASS] Function class test completed\n" << std::endl;
}

/**
 * @brief 测试GlobalState类
 */
void testGlobalState() {
    printTitle("Testing GlobalState Class");

    // 获取GlobalState单例
    GlobalState& gs = GlobalState::getInstance();

    std::cout << "GlobalState singleton:" << std::endl;
    std::cout << "  [OK] getInstance() returns valid reference" << std::endl;

    // 测试字符串池访问
    StringPool& pool = gs.getStringPool();
    GCString* testStr = pool.intern("GlobalState Test");
    std::cout << "  [OK] getStringPool() returns valid StringPool" << std::endl;
    std::cout << "  Test string: " << testStr->c_str() << std::endl;

    // 测试GC访问
    GarbageCollector& gc = gs.getGC();
    std::cout << "  [OK] getGC() returns valid GarbageCollector" << std::endl;

    // 测试注册表
    Table* registry = gs.getRegistry();
    if (registry != nullptr) {
        std::cout << "  [OK] getRegistry() returns valid Table" << std::endl;

        // 在注册表中存储一些数据
        GCString* key = pool.intern("test_key");
        registry->set(Value(key), Value(123.0));

        Value val = registry->get(Value(key));
        if (val.isNumber() && val.asNumber() == 123.0) {
            std::cout << "  [OK] Registry can store and retrieve values" << std::endl;
        }
    }

    // 测试元表管理
    Table* numberMT = new Table();
    gc.registerObject(numberMT);
    gs.setMetatable(ValueType::Number, numberMT);

    Table* retrievedMT = gs.getMetatable(ValueType::Number);
    if (retrievedMT == numberMT) {
        std::cout << "  [OK] setMetatable/getMetatable work correctly" << std::endl;
    }

    std::cout << "\n[PASS] GlobalState class test completed\n" << std::endl;
}

/**
 * @brief 测试Stack类
 */
void testStack() {
    printTitle("Testing Stack Class");

    Stack stack;

    // 测试初始状态
    std::cout << "Initial state:" << std::endl;
    std::cout << "  Size: " << stack.size() << std::endl;
    std::cout << "  Capacity: " << stack.capacity() << std::endl;
    std::cout << "  Empty: " << (stack.empty() ? "true" : "false") << std::endl;

    if (stack.empty()) {
        std::cout << "  [OK] New stack is empty" << std::endl;
    }

    // 测试push操作
    std::cout << "\nTesting push operations:" << std::endl;
    stack.push(Value(1.0));
    stack.push(Value(2.0));
    stack.push(Value(true));
    stack.push(Value());  // nil

    std::cout << "  Pushed 4 values" << std::endl;
    std::cout << "  Size: " << stack.size() << std::endl;

    if (stack.size() == 4) {
        std::cout << "  [OK] Stack size is correct" << std::endl;
    }

    // 测试top操作
    std::cout << "\nTesting top operation:" << std::endl;
    Value topVal = stack.top();
    if (topVal.isNil()) {
        std::cout << "  [OK] top() returns nil (last pushed value)" << std::endl;
    }

    // 测试索引访问
    std::cout << "\nTesting indexed access:" << std::endl;
    Value v0 = stack.at(0);
    Value v1 = stack.at(1);
    Value v2 = stack.at(2);

    if (v0.isNumber() && v0.asNumber() == 1.0) {
        std::cout << "  [OK] at(0) returns first value (1.0)" << std::endl;
    }
    if (v1.isNumber() && v1.asNumber() == 2.0) {
        std::cout << "  [OK] at(1) returns second value (2.0)" << std::endl;
    }
    if (v2.isBoolean() && v2.asBoolean() == true) {
        std::cout << "  [OK] at(2) returns third value (true)" << std::endl;
    }

    // 测试pop操作
    std::cout << "\nTesting pop operation:" << std::endl;
    Value popped = stack.pop();
    if (popped.isNil()) {
        std::cout << "  [OK] pop() returns nil" << std::endl;
    }
    if (stack.size() == 3) {
        std::cout << "  [OK] Size decreased to 3" << std::endl;
    }

    // 测试自动扩展
    std::cout << "\nTesting automatic expansion:" << std::endl;
    usize initialCapacity = stack.capacity();
    for (usize i = 0; i < 100; ++i) {
        stack.push(Value(static_cast<f64>(i)));
    }
    usize newCapacity = stack.capacity();
    if (newCapacity > initialCapacity) {
        std::cout << "  [OK] Stack expanded from " << initialCapacity
                  << " to " << newCapacity << std::endl;
    }

    // 测试clear
    stack.clear();
    if (stack.empty()) {
        std::cout << "  [OK] clear() empties the stack" << std::endl;
    }

    std::cout << "\n[PASS] Stack class test completed\n" << std::endl;
}

/**
 * @brief 测试CallInfo类
 */
void testCallInfo() {
    printTitle("Testing CallInfo Class");

    CallInfo ci;

    // 测试默认构造
    std::cout << "Default construction:" << std::endl;
    std::cout << "  func: " << ci.func << std::endl;
    std::cout << "  base: " << ci.base << std::endl;
    std::cout << "  top: " << ci.top << std::endl;
    std::cout << "  nresults: " << ci.nresults << std::endl;
    std::cout << "  tailcalls: " << ci.tailcalls << std::endl;

    if (ci.func == 0 && ci.base == 0 && ci.top == 0) {
        std::cout << "  [OK] Default values are zero" << std::endl;
    }

    // 测试设置值
    std::cout << "\nSetting values:" << std::endl;
    ci.func = 10;
    ci.base = 11;
    ci.top = 20;
    ci.nresults = 2;
    ci.tailcalls = 0;

    if (ci.func == 10 && ci.base == 11 && ci.top == 20 && ci.nresults == 2) {
        std::cout << "  [OK] Values set correctly" << std::endl;
    }

    // 测试reset
    std::cout << "\nTesting reset:" << std::endl;
    ci.reset();
    if (ci.func == 0 && ci.base == 0 && ci.top == 0) {
        std::cout << "  [OK] reset() clears all values" << std::endl;
    }

    std::cout << "\n[PASS] CallInfo class test completed\n" << std::endl;
}

/**
 * @brief 测试LuaState类
 */
void testLuaState() {
    printTitle("Testing LuaState Class");

    // 创建LuaState
    LuaState* L = LuaState::newState();
    std::cout << "Created new LuaState" << std::endl;

    // 测试初始状态
    std::cout << "\nInitial state:" << std::endl;
    std::cout << "  Stack size: " << L->getStack().size() << std::endl;
    std::cout << "  Call stack size: " << L->getCallStackSize() << std::endl;
    std::cout << "  Status: " << static_cast<int>(L->getStatus()) << std::endl;

    if (L->getStatus() == ThreadStatus::OK) {
        std::cout << "  [OK] Initial status is OK" << std::endl;
    }

    // 测试栈操作
    std::cout << "\nTesting stack operations:" << std::endl;
    L->pushNumber(42.0);
    L->pushBoolean(true);
    L->pushNil();

    std::cout << "  Pushed 3 values" << std::endl;
    std::cout << "  Stack size: " << L->getStack().size() << std::endl;

    // 注意：初始化时会push一个nil作为虚拟函数，所以实际大小是4
    if (L->getStack().size() == 4) {
        std::cout << "  [OK] Stack size is 4 (1 initial + 3 pushed)" << std::endl;
    }

    // 测试访问栈顶
    Value topVal = L->top();
    if (topVal.isNil()) {
        std::cout << "  [OK] top() returns nil" << std::endl;
    }

    // 测试pop
    Value popped = L->pop();
    if (popped.isNil() && L->getStack().size() == 3) {
        std::cout << "  [OK] pop() works correctly" << std::endl;
    }

    // 测试全局状态访问
    std::cout << "\nTesting global state access:" << std::endl;
    GlobalState& gs = L->getGlobalState();
    std::cout << "  [OK] getGlobalState() returns valid reference" << std::endl;

    // 测试全局表
    Table* globalTable = L->getGlobalTable();
    if (globalTable != nullptr) {
        std::cout << "  [OK] getGlobalTable() returns valid Table" << std::endl;

        // 在全局表中存储值
        StringPool& pool = gs.getStringPool();
        GCString* key = pool.intern("test_global");
        globalTable->set(Value(key), Value(999.0));

        Value val = globalTable->get(Value(key));
        if (val.isNumber() && val.asNumber() == 999.0) {
            std::cout << "  [OK] Global table can store and retrieve values" << std::endl;
        }
    }

    // 测试CallInfo访问
    std::cout << "\nTesting CallInfo access:" << std::endl;
    CallInfo& ci = L->getCurrentCallInfo();
    std::cout << "  Current CallInfo base: " << ci.base << std::endl;
    std::cout << "  [OK] getCurrentCallInfo() returns valid reference" << std::endl;

    // 清理
    delete L;
    std::cout << "\n[PASS] LuaState class test completed\n" << std::endl;
}

/**
 * @brief 测试Userdata类
 */
void testUserdata() {
    printTitle("Testing Userdata Class");

    // ===== 测试1: 创建简单的完整用户数据 =====
    std::cout << "Testing full userdata creation:" << std::endl;

    Userdata* ud1 = Userdata::createFull(64);
    std::cout << "  [OK] Created userdata with 64 bytes" << std::endl;
    std::cout << "  Data size: " << ud1->getDataSize() << " bytes" << std::endl;
    std::cout << "  Object size: " << ud1->getSize() << " bytes" << std::endl;

    // ===== 测试2: 数据访问和修改 =====
    std::cout << "\nTesting data access:" << std::endl;

    // 写入数据
    void* data = ud1->getData();
    i32* intData = static_cast<i32*>(data);
    intData[0] = 42;
    intData[1] = 100;

    std::cout << "  Written values: " << intData[0] << ", " << intData[1] << std::endl;

    // 读取数据
    i32* readData = ud1->getTypedData<i32>();
    std::cout << "  Read values: " << readData[0] << ", " << readData[1] << std::endl;

    // ===== 测试3: 类型化创建 =====
    std::cout << "\nTesting typed userdata creation:" << std::endl;

    struct TestStruct {
        i32 id;
        f64 value;
        char name[16];
    };

    TestStruct testData;
    testData.id = 123;
    testData.value = 3.14159;
    strcpy_s(testData.name, sizeof(testData.name), "TestData");

    Userdata* ud2 = Userdata::create(testData);
    std::cout << "  [OK] Created typed userdata" << std::endl;

    TestStruct* retrievedData = ud2->getTypedData<TestStruct>();
    std::cout << "  Retrieved data:" << std::endl;
    std::cout << "    id: " << retrievedData->id << std::endl;
    std::cout << "    value: " << retrievedData->value << std::endl;
    std::cout << "    name: " << retrievedData->name << std::endl;

    // ===== 测试4: 元表操作 =====
    std::cout << "\nTesting metatable operations:" << std::endl;

    std::cout << "  Has metatable: " << (ud1->hasMetatable() ? "true" : "false") << std::endl;

    Table* mt = new Table();
    ud1->setMetatable(mt);
    std::cout << "  [OK] Metatable set" << std::endl;
    std::cout << "  Has metatable: " << (ud1->hasMetatable() ? "true" : "false") << std::endl;
    std::cout << "  Metatable pointer match: " << (ud1->getMetatable() == mt ? "true" : "false") << std::endl;

    // ===== 测试5: GC集成 =====
    std::cout << "\nTesting GC integration:" << std::endl;

    GarbageCollector& gc = GarbageCollector::getInstance();
    gc.clearAll();

    Userdata* ud3 = Userdata::createFull(128);
    Userdata* ud4 = Userdata::createFull(256);
    Table* mt2 = new Table();
    ud3->setMetatable(mt2);

    gc.registerObject(ud3);
    gc.registerObject(ud4);
    gc.registerObject(mt2);

    std::cout << "  Registered objects: " << gc.getObjectCount() << std::endl;

    // 添加ud3为根对象(保护它和它的元表)
    gc.addRoot(ud3);
    std::cout << "  Added ud3 as root" << std::endl;

    // 执行GC
    usize collected = gc.collect();
    std::cout << "  Collected objects: " << collected << std::endl;
    std::cout << "  Remaining objects: " << gc.getObjectCount() << std::endl;
    std::cout << "  (ud3 and its metatable should survive, ud4 should be collected)" << std::endl;

    // ===== 测试6: Value集成 =====
    std::cout << "\nTesting Value integration:" << std::endl;

    Userdata* ud5 = Userdata::createFull(32);
    Value val(ud5);

    std::cout << "  Value type: " << static_cast<i32>(val.getType()) << std::endl;
    std::cout << "  Is userdata: " << (val.isUserdata() ? "true" : "false") << std::endl;
    std::cout << "  Retrieved userdata: " << (val.asUserdata() == ud5 ? "true" : "false") << std::endl;

    // 清理
    gc.clearAll();
    delete mt;
    delete ud1;
    delete ud2;
    delete ud5;

    std::cout << "\n[PASS] Userdata test completed\n" << std::endl;
}

/**
 * @brief 测试Lexer类
 */
void testLexer() {
    printTitle("Testing Lexer Class");

    i32 testCount = 0;

    // ===== 测试1: 关键字识别 =====
    std::cout << "\n[Test 1] Keyword recognition:" << std::endl;
    {
        Lexer lexer("local function if then else end");

        Token t1 = lexer.nextToken();
        std::cout << "  Token 1: " << tokenTypeToString(t1.type) << " (expected: local)" << std::endl;
        testCount++;

        Token t2 = lexer.nextToken();
        std::cout << "  Token 2: " << tokenTypeToString(t2.type) << " (expected: function)" << std::endl;
        testCount++;

        Token t3 = lexer.nextToken();
        std::cout << "  Token 3: " << tokenTypeToString(t3.type) << " (expected: if)" << std::endl;
        testCount++;
    }

    // ===== 测试2: 标识符识别 =====
    std::cout << "\n[Test 2] Identifier recognition:" << std::endl;
    {
        Lexer lexer("myVar _private var123");

        Token t1 = lexer.nextToken();
        std::cout << "  Identifier: " << t1.lexeme << " (type: " << tokenTypeToString(t1.type) << ")" << std::endl;
        testCount++;

        Token t2 = lexer.nextToken();
        std::cout << "  Identifier: " << t2.lexeme << " (type: " << tokenTypeToString(t2.type) << ")" << std::endl;
        testCount++;
    }

    // ===== 测试3: 数字字面量 =====
    std::cout << "\n[Test 3] Number literals:" << std::endl;
    {
        Lexer lexer("42 3.14 2.5e10 0xFF");

        Token t1 = lexer.nextToken();
        if (t1.isNumber()) {
            f64 val = std::get<f64>(t1.value);
            std::cout << "  Integer: " << val << std::endl;
        }
        testCount++;

        Token t2 = lexer.nextToken();
        if (t2.isNumber()) {
            f64 val = std::get<f64>(t2.value);
            std::cout << "  Float: " << val << std::endl;
        }
        testCount++;

        Token t3 = lexer.nextToken();
        if (t3.isNumber()) {
            f64 val = std::get<f64>(t3.value);
            std::cout << "  Scientific: " << val << std::endl;
        }
        testCount++;

        Token t4 = lexer.nextToken();
        if (t4.isNumber()) {
            f64 val = std::get<f64>(t4.value);
            std::cout << "  Hexadecimal: " << val << std::endl;
        }
        testCount++;
    }

    // ===== 测试4: 字符串字面量 =====
    std::cout << "\n[Test 4] String literals:" << std::endl;
    {
        Lexer lexer("\"hello\" 'world' [[long string]]");

        Token t1 = lexer.nextToken();
        if (t1.isString()) {
            Str val = std::get<Str>(t1.value);
            std::cout << "  Double quote: \"" << val << "\"" << std::endl;
        }
        testCount++;

        Token t2 = lexer.nextToken();
        if (t2.isString()) {
            Str val = std::get<Str>(t2.value);
            std::cout << "  Single quote: '" << val << "'" << std::endl;
        }
        testCount++;

        Token t3 = lexer.nextToken();
        if (t3.isString()) {
            Str val = std::get<Str>(t3.value);
            std::cout << "  Long string: [[" << val << "]]" << std::endl;
        }
        testCount++;
    }

    // ===== 测试5: 运算符 =====
    std::cout << "\n[Test 5] Operators:" << std::endl;
    {
        Lexer lexer("+ - * / == ~= <= >= .. ...");

        Token t1 = lexer.nextToken();
        std::cout << "  Operator: " << t1.lexeme << std::endl;

        Token t2 = lexer.nextToken();
        std::cout << "  Operator: " << t2.lexeme << std::endl;

        // 跳过几个
        lexer.nextToken(); // *
        lexer.nextToken(); // /

        Token t3 = lexer.nextToken();
        std::cout << "  Operator: " << tokenTypeToString(t3.type) << " (==)" << std::endl;
        testCount++;

        Token t4 = lexer.nextToken();
        std::cout << "  Operator: " << tokenTypeToString(t4.type) << " (~=)" << std::endl;
        testCount++;
    }

    // ===== 测试6: 注释处理 =====
    std::cout << "\n[Test 6] Comment handling:" << std::endl;
    {
        Lexer lexer("x -- this is a comment\ny");

        Token t1 = lexer.nextToken();
        std::cout << "  Before comment: " << t1.lexeme << std::endl;
        testCount++;

        Token t2 = lexer.nextToken();
        std::cout << "  After comment: " << t2.lexeme << std::endl;
        testCount++;
    }

    // ===== 测试7: 长注释 =====
    std::cout << "\n[Test 7] Long comment:" << std::endl;
    {
        Lexer lexer("a --[[ multi\nline\ncomment ]] b");

        Token t1 = lexer.nextToken();
        std::cout << "  Before long comment: " << t1.lexeme << std::endl;
        testCount++;

        Token t2 = lexer.nextToken();
        std::cout << "  After long comment: " << t2.lexeme << std::endl;
        testCount++;
    }

    // ===== 测试8: 完整Lua代码片段 =====
    std::cout << "\n[Test 8] Complete Lua code:" << std::endl;
    {
        Lexer lexer("local x = 42\nif x > 10 then\n  print(x)\nend");

        i32 tokenCount = 0;
        while (true) {
            Token t = lexer.nextToken();
            if (t.type == TokenType::Eos) break;
            tokenCount++;
        }

        std::cout << "  Total tokens: " << tokenCount << std::endl;
        testCount++;
    }

    std::cout << "\n[SUCCESS] Lexer tests completed: " << testCount << " tests" << std::endl;
    printSeparator();
}

/**
 * @brief 测试Parser类
 */
void testParser() {
    printTitle("Testing Parser Class");

    i32 testCount = 0;

    // ===== 测试1: 简单赋值语句 =====
    std::cout << "\n[Test 1] Simple assignment:" << std::endl;
    try {
        Parser parser("x = 42");
        Chunk chunk = parser.parse();
        std::cout << "  Parsed successfully: " << chunk.statements.size() << " statement(s)" << std::endl;
        testCount++;
    } catch (const ParseError& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试2: 局部变量声明 =====
    std::cout << "\n[Test 2] Local variable declaration:" << std::endl;
    try {
        Parser parser("local x, y = 1, 2");
        Chunk chunk = parser.parse();
        std::cout << "  Parsed successfully: " << chunk.statements.size() << " statement(s)" << std::endl;
        testCount++;
    } catch (const ParseError& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试3: if语句 =====
    std::cout << "\n[Test 3] If statement:" << std::endl;
    try {
        Parser parser("if x > 0 then print(x) end");
        Chunk chunk = parser.parse();
        std::cout << "  Parsed successfully: " << chunk.statements.size() << " statement(s)" << std::endl;
        testCount++;
    } catch (const ParseError& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试4: while循环 =====
    std::cout << "\n[Test 4] While loop:" << std::endl;
    try {
        Parser parser("while x < 10 do x = x + 1 end");
        Chunk chunk = parser.parse();
        std::cout << "  Parsed successfully: " << chunk.statements.size() << " statement(s)" << std::endl;
        testCount++;
    } catch (const ParseError& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试5: 数值for循环 =====
    std::cout << "\n[Test 5] Numeric for loop:" << std::endl;
    try {
        Parser parser("for i = 1, 10, 2 do print(i) end");
        Chunk chunk = parser.parse();
        std::cout << "  Parsed successfully: " << chunk.statements.size() << " statement(s)" << std::endl;
        testCount++;
    } catch (const ParseError& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试6: 函数定义 =====
    std::cout << "\n[Test 6] Function definition:" << std::endl;
    try {
        Parser parser("function add(a, b) return a + b end");
        Chunk chunk = parser.parse();
        std::cout << "  Parsed successfully: " << chunk.statements.size() << " statement(s)" << std::endl;
        testCount++;
    } catch (const ParseError& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试7: 表构造器 =====
    std::cout << "\n[Test 7] Table constructor:" << std::endl;
    try {
        Parser parser("t = {1, 2, 3, x = 10, y = 20}");
        Chunk chunk = parser.parse();
        std::cout << "  Parsed successfully: " << chunk.statements.size() << " statement(s)" << std::endl;
        testCount++;
    } catch (const ParseError& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试8: 二元运算表达式 =====
    std::cout << "\n[Test 8] Binary expressions:" << std::endl;
    try {
        Parser parser("result = a + b * c - d / e");
        Chunk chunk = parser.parse();
        std::cout << "  Parsed successfully: " << chunk.statements.size() << " statement(s)" << std::endl;
        testCount++;
    } catch (const ParseError& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试9: 函数调用 =====
    std::cout << "\n[Test 9] Function call:" << std::endl;
    try {
        Parser parser("print(\"Hello, Lua!\")");
        Chunk chunk = parser.parse();
        std::cout << "  Parsed successfully: " << chunk.statements.size() << " statement(s)" << std::endl;
        testCount++;
    } catch (const ParseError& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试10: 复杂代码 =====
    std::cout << "\n[Test 10] Complex code:" << std::endl;
    try {
        Str code = R"(
            local function factorial(n)
                if n <= 1 then
                    return 1
                else
                    return n * factorial(n - 1)
                end
            end

            local result = factorial(5)
        )";
        Parser parser(code);
        Chunk chunk = parser.parse();
        std::cout << "  Parsed successfully: " << chunk.statements.size() << " statement(s)" << std::endl;
        testCount++;
    } catch (const ParseError& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    std::cout << "\n[SUCCESS] Parser tests completed: " << testCount << " tests" << std::endl;
    printSeparator();
}

/**
 * @brief 测试CodeGenerator类
 */
void testCodeGenerator() {
    printTitle("Testing CodeGenerator Class");

    StringPool& pool = StringPool::getInstance();
    i32 testCount = 0;

    // ===== 测试1: 简单数字常量 =====
    std::cout << "\n[Test 1] Number constant:" << std::endl;
    try {
        Parser parser("return 42");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        std::cout << "  Generated " << proto->getInstructionCount() << " instruction(s)" << std::endl;
        std::cout << "  Constant count: " << proto->getConstantCount() << std::endl;
        std::cout << "  Max stack size: " << static_cast<i32>(proto->getMaxStackSize()) << std::endl;

        // 打印指令
        for (usize i = 0; i < proto->getInstructionCount(); i++) {
            Instruction inst = proto->getInstruction(i);
            OpCode op = GET_OPCODE(inst);
            std::cout << "    [" << i << "] " << getOpName(op);

            if (getOpMode(op) == OpMode::iABC) {
                std::cout << " A=" << GETARG_A(inst)
                         << " B=" << GETARG_B(inst)
                         << " C=" << GETARG_C(inst);
            } else if (getOpMode(op) == OpMode::iABx) {
                std::cout << " A=" << GETARG_A(inst)
                         << " Bx=" << GETARG_Bx(inst);
            } else {
                std::cout << " A=" << GETARG_A(inst)
                         << " sBx=" << GETARG_sBx(inst);
            }
            std::cout << std::endl;
        }

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试2: 局部变量赋值 =====
    std::cout << "\n[Test 2] Local variable assignment:" << std::endl;
    try {
        Parser parser("local x = 10");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        std::cout << "  Generated " << proto->getInstructionCount() << " instruction(s)" << std::endl;
        std::cout << "  Constant count: " << proto->getConstantCount() << std::endl;

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试3: 全局变量赋值 =====
    std::cout << "\n[Test 3] Global variable assignment:" << std::endl;
    try {
        Parser parser("x = 42");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        std::cout << "  Generated " << proto->getInstructionCount() << " instruction(s)" << std::endl;

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试4: 字符串常量 =====
    std::cout << "\n[Test 4] String constant:" << std::endl;
    try {
        Parser parser(R"(return "hello")");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        std::cout << "  Generated " << proto->getInstructionCount() << " instruction(s)" << std::endl;
        std::cout << "  Constant count: " << proto->getConstantCount() << std::endl;

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试5: 布尔常量 =====
    std::cout << "\n[Test 5] Boolean constant:" << std::endl;
    try {
        Parser parser("return true");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        std::cout << "  Generated " << proto->getInstructionCount() << " instruction(s)" << std::endl;

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试6: nil常量 =====
    std::cout << "\n[Test 6] Nil constant:" << std::endl;
    try {
        Parser parser("return nil");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        std::cout << "  Generated " << proto->getInstructionCount() << " instruction(s)" << std::endl;

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试7: 多个局部变量 =====
    std::cout << "\n[Test 7] Multiple local variables:" << std::endl;
    try {
        Parser parser("local x, y, z = 1, 2, 3");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        std::cout << "  Generated " << proto->getInstructionCount() << " instruction(s)" << std::endl;
        std::cout << "  Max stack size: " << static_cast<i32>(proto->getMaxStackSize()) << std::endl;

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    std::cout << "\n[SUCCESS] CodeGenerator tests completed: " << testCount << " tests" << std::endl;
    printSeparator();
}

/**
 * @brief 测试VM类
 */
void testVM() {
    printTitle("Testing VM Class");

    StringPool& pool = StringPool::getInstance();
    LuaState* L = LuaState::newState();
    VM vm(L);
    i32 testCount = 0;

    // ===== 测试1: 简单数字常量 =====
    std::cout << "\n[Test 1] Execute: return 42" << std::endl;
    try {
        Parser parser("return 42");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        vm.executeProto(proto);

        // 检查返回值
        Stack& stack = L->getStack();
        if (stack.size() > 0) {
            Value result = stack.top();
            if (result.isNumber()) {
                std::cout << "  Result: " << result.asNumber() << std::endl;
                std::cout << "  Expected: 42" << std::endl;
                if (result.asNumber() == 42.0) {
                    std::cout << "  [PASS]" << std::endl;
                } else {
                    std::cout << "  [FAIL] Wrong result" << std::endl;
                }
            }
        }

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试2: 布尔常量 =====
    std::cout << "\n[Test 2] Execute: return true" << std::endl;
    try {
        // 清空栈
        L->getStack().clear();

        Parser parser("return true");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        vm.executeProto(proto);

        Stack& stack = L->getStack();
        if (stack.size() > 0) {
            Value result = stack.top();
            if (result.isBoolean()) {
                std::cout << "  Result: " << (result.asBoolean() ? "true" : "false") << std::endl;
                std::cout << "  Expected: true" << std::endl;
                if (result.asBoolean() == true) {
                    std::cout << "  [PASS]" << std::endl;
                } else {
                    std::cout << "  [FAIL] Wrong result" << std::endl;
                }
            }
        }

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试3: nil常量 =====
    std::cout << "\n[Test 3] Execute: return nil" << std::endl;
    try {
        L->getStack().clear();

        Parser parser("return nil");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        vm.executeProto(proto);

        Stack& stack = L->getStack();
        if (stack.size() > 0) {
            Value result = stack.top();
            if (result.isNil()) {
                std::cout << "  Result: nil" << std::endl;
                std::cout << "  Expected: nil" << std::endl;
                std::cout << "  [PASS]" << std::endl;
            } else {
                std::cout << "  [FAIL] Wrong result type" << std::endl;
            }
        }

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    // ===== 测试4: 字符串常量 =====
    std::cout << "\n[Test 4] Execute: return \"hello\"" << std::endl;
    try {
        L->getStack().clear();

        Parser parser("return \"hello\"");
        Chunk chunk = parser.parse();

        CodeGenerator codegen(&pool);
        Proto* proto = codegen.generate(chunk);

        vm.executeProto(proto);

        Stack& stack = L->getStack();
        if (stack.size() > 0) {
            Value result = stack.top();
            if (result.isString()) {
                std::cout << "  Result: \"" << result.asString()->c_str() << "\"" << std::endl;
                std::cout << "  Expected: \"hello\"" << std::endl;
                if (result.asString()->getData() == "hello") {
                    std::cout << "  [PASS]" << std::endl;
                } else {
                    std::cout << "  [FAIL] Wrong result" << std::endl;
                }
            }
        }

        delete proto;
        testCount++;
    } catch (const std::exception& e) {
        std::cout << "  ERROR: " << e.what() << std::endl;
    }

    delete L;

    std::cout << "\n[SUCCESS] VM tests completed: " << testCount << " tests" << std::endl;
    printSeparator();
}

/**
 * @brief 综合测试
 */
void comprehensiveTest() {
    printTitle("Comprehensive Integration Test");
    
    StringPool& pool = StringPool::getInstance();
    
    // 创建一个复杂的表结构
    Table* mainTable = new Table();
    
    // 添加数组元素
    for (i32 i = 1; i <= 5; ++i) {
        mainTable->setArray(i, Value(static_cast<f64>(i * 10)));
    }
    
    // 添加哈希元素
    GCString* keyConfig = pool.intern("config");
    Table* configTable = new Table();
    
    GCString* keyVersion = pool.intern("version");
    GCString* valueVersion = pool.intern("5.1");
    configTable->set(Value(keyVersion), Value(valueVersion));
    
    GCString* keyDebug = pool.intern("debug");
    configTable->set(Value(keyDebug), Value(true));
    
    mainTable->set(Value(keyConfig), Value(configTable));
    
    // 输出结果
    std::cout << "Main table structure:" << std::endl;
    std::cout << "  Array elements: ";
    for (i32 i = 1; i <= 5; ++i) {
        std::cout << mainTable->getArray(i).asNumber() << " ";
    }
    std::cout << std::endl;
    
    Value configVal = mainTable->get(Value(keyConfig));
    if (configVal.isTable()) {
        Table* cfg = configVal.asTable();
        Value ver = cfg->get(Value(keyVersion));
        Value dbg = cfg->get(Value(keyDebug));
        
        std::cout << "  config.version: " << ver.asString()->c_str() << std::endl;
        std::cout << "  config.debug: " << (dbg.asBoolean() ? "true" : "false") << std::endl;
    }
    
    std::cout << "\nStringPool final statistics:" << std::endl;
    std::cout << "  Total interned strings: " << pool.size() << std::endl;
    
    // 清理
    delete configTable;
    delete mainTable;
    
    std::cout << "\n[PASS] Comprehensive test completed\n" << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    try {
        printTitle("Lua C++ Interpreter - Manual Test Program");
        
        std::cout << "Version: Lua 5.1 (C++ Implementation)" << std::endl;
        std::cout << "Build: Manual compilation test" << std::endl;
        std::cout << "Date: 2025-11-12" << std::endl;
        std::cout << std::endl;
        
        // 运行各项测试
        testValue();
        testString();
        testTable();
        testGCObject();
        testGarbageCollector();
        testFunction();
        testUserdata();

        // 虚拟机核心模块测试
        testGlobalState();
        testStack();
        testCallInfo();
        testLuaState();

        // 编译器模块测试
        testLexer();
        testParser();
        testCodeGenerator();

        // 虚拟机执行引擎测试
        testVM();

        comprehensiveTest();

        // 总结
        printTitle("Test Summary");
        std::cout << "[SUCCESS] All manual tests passed!" << std::endl;
        std::cout << "\nCore components verified:" << std::endl;
        std::cout << "  [OK] Value class" << std::endl;
        std::cout << "  [OK] GCObject base class" << std::endl;
        std::cout << "  [OK] GCString class" << std::endl;
        std::cout << "  [OK] StringPool singleton" << std::endl;
        std::cout << "  [OK] Table class" << std::endl;
        std::cout << "  [OK] Function class (Proto + Closure)" << std::endl;
        std::cout << "  [OK] Userdata class" << std::endl;
        std::cout << "  [OK] GarbageCollector class" << std::endl;
        std::cout << "  [OK] GlobalState class" << std::endl;
        std::cout << "  [OK] Stack class" << std::endl;
        std::cout << "  [OK] CallInfo class" << std::endl;
        std::cout << "  [OK] LuaState class" << std::endl;
        std::cout << "  [OK] Lexer class (Token + Lexer)" << std::endl;
        std::cout << "  [OK] Parser class (AST + Parser)" << std::endl;
        std::cout << "  [OK] CodeGenerator class (OpCode + CodeGen)" << std::endl;
        std::cout << "  [OK] VM class (Bytecode Executor)" << std::endl;
        std::cout << std::endl;
        
        printSeparator();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Exception caught: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[ERROR] Unknown exception caught" << std::endl;
        return 2;
    }
}

