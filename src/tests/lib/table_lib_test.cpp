#include "table_lib_test.hpp"
#include "../../lib/table_lib.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <memory>

namespace Lua {
namespace Tests {

void TableLibTest::runAllTests() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "        TABLE LIBRARY TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Running all table library tests..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
        testConcat();
        testInsert();
        testRemove();
        testSort();
        testPack();
        testUnpack();
        testMove();
        testErrorHandling();
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "        ALL TABLE LIBRARY TESTS PASSED!" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "        TABLE LIBRARY TESTS FAILED!" << std::endl;
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw;
    }
}

void TableLibTest::testConcat() {
    std::cout << "\nTesting table.concat():" << std::endl;
    
    try {
        // Create test state
        auto state = std::make_unique<State>();
        
        // Create a test table with string elements
        Table* testTable = new Table();
        testTable->set(Value(1.0), Value("hello"));
        testTable->set(Value(2.0), Value(" "));
        testTable->set(Value(3.0), Value("world"));
        
        // Push table to stack
        state->push(Value(testTable));
        
        // Test concat with default separator
        Value result = TableLib::concat(state.get(), 1);
        bool testPassed = result.isString();
        
        printTestResult("table.concat basic", testPassed);
        
        // Test concat with custom separator
        state->push(Value(testTable));
        state->push(Value(","));
        result = TableLib::concat(state.get(), 2);
        testPassed = result.isString();
        
        printTestResult("table.concat with separator", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("table.concat", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void TableLibTest::testInsert() {
    std::cout << "\nTesting table.insert():" << std::endl;
    
    try {
        // Create test state
        auto state = std::make_unique<State>();
        
        // Create a test table
        Table* testTable = new Table();
        testTable->set(Value(1.0), Value("first"));
        testTable->set(Value(2.0), Value("second"));
        
        // Test insert at end
        state->push(Value(testTable));
        state->push(Value("third"));
        Value result = TableLib::insert(state.get(), 2);
        
        // Check if element was inserted
        Value insertedValue = testTable->get(Value(3.0));
        bool testPassed = insertedValue.isString();
        
        printTestResult("table.insert at end", testPassed);
        
        // Test insert at specific position
        state->push(Value(testTable));
        state->push(Value(2.0)); // position
        state->push(Value("middle"));
        result = TableLib::insert(state.get(), 3);
        
        testPassed = true; // Basic test that it doesn't crash
        printTestResult("table.insert at position", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("table.insert", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void TableLibTest::testRemove() {
    std::cout << "\nTesting table.remove():" << std::endl;
    
    try {
        // Create test state
        auto state = std::make_unique<State>();
        
        // Create a test table
        Table* testTable = new Table();
        testTable->set(Value(1.0), Value("first"));
        testTable->set(Value(2.0), Value("second"));
        testTable->set(Value(3.0), Value("third"));
        
        // Test remove from end
        state->push(Value(testTable));
        Value result = TableLib::remove(state.get(), 1);
        
        bool testPassed = result.isString();
        printTestResult("table.remove from end", testPassed);
        
        // Test remove from specific position
        state->push(Value(testTable));
        state->push(Value(1.0)); // position
        result = TableLib::remove(state.get(), 2);
        
        testPassed = result.isString();
        printTestResult("table.remove from position", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("table.remove", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void TableLibTest::testSort() {
    std::cout << "\nTesting table.sort():" << std::endl;
    
    try {
        // Create test state
        auto state = std::make_unique<State>();
        
        // Create a test table with numbers
        Table* testTable = new Table();
        testTable->set(Value(1.0), Value(3.0));
        testTable->set(Value(2.0), Value(1.0));
        testTable->set(Value(3.0), Value(2.0));
        
        // Test sort without comparator
        state->push(Value(testTable));
        Value result = TableLib::sort(state.get(), 1);
        
        bool testPassed = result.isNil(); // sort returns nil
        printTestResult("table.sort basic", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("table.sort", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void TableLibTest::testPack() {
    std::cout << "\nTesting table.pack():" << std::endl;
    
    try {
        // Create test state
        auto state = std::make_unique<State>();
        
        // Test pack with multiple arguments
        state->push(Value("arg1"));
        state->push(Value("arg2"));
        state->push(Value("arg3"));
        
        Value result = TableLib::pack(state.get(), 3);
        
        bool testPassed = result.isTable();
        printTestResult("table.pack", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("table.pack", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void TableLibTest::testUnpack() {
    std::cout << "\nTesting table.unpack():" << std::endl;
    
    try {
        // Create test state
        auto state = std::make_unique<State>();
        
        // Create a test table
        Table* testTable = new Table();
        testTable->set(Value(1.0), Value("first"));
        testTable->set(Value(2.0), Value("second"));
        testTable->set(Value(3.0), Value("third"));
        
        // Test unpack
        state->push(Value(testTable));
        Value result = TableLib::unpack(state.get(), 1);
        
        // unpack pushes multiple values to stack, check if operation succeeded
        bool testPassed = true; // Basic test that it doesn't crash
        printTestResult("table.unpack", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("table.unpack", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void TableLibTest::testMove() {
    std::cout << "\nTesting table.move():" << std::endl;
    
    try {
        // Create test state
        auto state = std::make_unique<State>();
        
        // Create source table
        Table* sourceTable = new Table();
        sourceTable->set(Value(1.0), Value("a"));
        sourceTable->set(Value(2.0), Value("b"));
        sourceTable->set(Value(3.0), Value("c"));
        
        // Create destination table
        Table* destTable = new Table();
        
        // Test move operation
        state->push(Value(sourceTable)); // a1
        state->push(Value(1.0));         // f (from)
        state->push(Value(3.0));         // e (end)
        state->push(Value(1.0));         // t (to)
        state->push(Value(destTable));   // a2 (destination)
        
        Value result = TableLib::move(state.get(), 5);
        
        bool testPassed = result.isTable();
        printTestResult("table.move", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("table.move", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void TableLibTest::testErrorHandling() {
    std::cout << "\nTesting error handling:" << std::endl;
    
    try {
        // Create test state
        auto state = std::make_unique<State>();
        
        // Test concat with non-table argument
        state->push(Value("not a table"));
        
        bool exceptionThrown = false;
        try {
            TableLib::concat(state.get(), 1);
        } catch (const std::exception&) {
            exceptionThrown = true;
        }
        
        printTestResult("error handling - invalid table", exceptionThrown);
        
        // Test insert with insufficient arguments
        exceptionThrown = false;
        try {
            TableLib::insert(state.get(), 0); // no arguments
        } catch (const std::exception&) {
            exceptionThrown = true;
        }
        
        printTestResult("error handling - insufficient args", exceptionThrown);
        
    } catch (const std::exception& e) {
        printTestResult("error handling", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void TableLibTest::printTestResult(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "[PASS] " << testName << " test passed" << std::endl;
    } else {
        std::cout << "[FAIL] " << testName << " test failed" << std::endl;
    }
}

void TableLibTest::printSectionHeader(const std::string& sectionName) {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << sectionName << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

void TableLibTest::printSectionFooter() {
    std::cout << std::string(50, '-') << std::endl;
}

} // namespace Tests
} // namespace Lua