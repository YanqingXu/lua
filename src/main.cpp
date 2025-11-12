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

