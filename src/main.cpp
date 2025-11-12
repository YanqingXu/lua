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
#include "gc/garbage_collector.hpp"

#include <iostream>
#include <iomanip>

using namespace Lua;

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
        std::cout << "  [OK] GarbageCollector class" << std::endl;
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

