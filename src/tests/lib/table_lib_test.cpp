#include "table_lib_test.hpp"
#include "../../lib/table_lib.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../gc/core/gc_ref.hpp"
#include <iostream>
#include <cassert>

namespace Lua {
namespace Tests {

void TableLibTest::runAllTests() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "        TABLE LIBRARY TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Running all table library tests..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
        testInsert();
        testRemove();
        testConcat();
        testSort();
        testPack();
        testUnpack();
        testMove();
        testMaxn();
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [OK] ALL TABLE LIBRARY TESTS PASSED" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "    [FAILED] TABLE LIBRARY TESTS FAILED" << std::endl;
        std::cout << "    Error: " << e.what() << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw;
    }
}

void TableLibTest::testInsert() {
    printTestHeader("table.insert");
    
    try {
        // Create a state and initialize table library
        auto state = std::make_unique<State>();
        TableLib tableLib;
        tableLib.registerModule(state.get());
        
        // Create test table
        auto testTable = make_gc_table();
        testTable->set(Value(1), Value("first"));
        testTable->set(Value(2), Value("second"));
        
        // Test 1: Append to end
        state->push(Value(testTable));
        state->push(Value("third"));
        
        Value result = TableLib::insert(state.get(), 2);
        assertTrue(result.isNil(), "table.insert should return nil");
        
        // Clear stack after first insert
        state->pop(); // Remove table from stack
        
        // Verify insertion
        Value val = testTable->get(Value(3));
        assertTrue(val.isString() && val.toString() == "third", "Element should be inserted at end");
        
        // Test 2: Insert at specific position
        state->push(Value(testTable));
        state->push(Value(2));
        state->push(Value("inserted"));
        
        TableLib::insert(state.get(), 3);
        
        // Clear stack after second insert
        state->pop(); // Remove table from stack
        
        // Verify insertion at position
        Value insertedVal = testTable->get(Value(2));
        assertTrue(insertedVal.isString() && insertedVal.toString() == "inserted", 
                  "Element should be inserted at specified position");
        
        Value shiftedVal = testTable->get(Value(3));
        assertTrue(shiftedVal.isString() && shiftedVal.toString() == "second", 
                  "Existing element should be shifted");
        
        printTestResult("table.insert", true);
        
    } catch (const std::exception& e) {
        printTestResult("table.insert", false, e);
        throw;
    }
}

void TableLibTest::testRemove() {
    printTestHeader("table.remove");
    
    try {
        auto state = std::make_unique<State>();
        TableLib tableLib;
        
        // Create test table
        auto testTable = make_gc_table();
        testTable->set(Value(1), Value("first"));
        testTable->set(Value(2), Value("second"));
        testTable->set(Value(3), Value("third"));
        
        // Test 1: Remove from end (default)
        state->push(Value(testTable));
        
        Value removed = TableLib::remove(state.get(), 1);
        assertTrue(removed.isString() && removed.toString() == "third", 
                  "Should remove and return last element");
        
        // Verify removal
        Value val = testTable->get(Value(3));
        assertTrue(val.isNil(), "Last element should be removed");
        
        // Test 2: Remove from specific position
        state->push(Value(testTable));
        state->push(Value(1));
        
        Value removedFirst = TableLib::remove(state.get(), 2);
        assertTrue(removedFirst.isString() && removedFirst.toString() == "first", 
                  "Should remove and return first element");
        
        // Verify shift
        Value newFirst = testTable->get(Value(1));
        assertTrue(newFirst.isString() && newFirst.toString() == "second", 
                  "Second element should shift to first position");
        
        printTestResult("table.remove", true);
        
    } catch (const std::exception& e) {
        printTestResult("table.remove", false, e);
        throw;
    }
}

void TableLibTest::testConcat() {
    printTestHeader("table.concat");
    
    try {
        auto state = std::make_unique<State>();
        
        // Create test table
        auto testTable = make_gc_table();
        testTable->set(Value(1), Value("hello"));
        testTable->set(Value(2), Value("world"));
        testTable->set(Value(3), Value("test"));
        
        // Test 1: Default concatenation (no separator)
        state->push(Value(testTable));
        
        Value result = TableLib::concat(state.get(), 1);
        assertTrue(result.isString() && result.toString() == "helloworldtest", 
                  "Should concatenate without separator");
        
        // Test 2: With separator
        state->push(Value(testTable));
        state->push(Value(" "));
        
        Value resultWithSep = TableLib::concat(state.get(), 2);
        assertTrue(resultWithSep.isString() && resultWithSep.toString() == "hello world test", 
                  "Should concatenate with separator");
        
        // Test 3: With range
        state->push(Value(testTable));
        state->push(Value("-"));
        state->push(Value(1));
        state->push(Value(2));
        
        Value resultWithRange = TableLib::concat(state.get(), 4);
        assertTrue(resultWithRange.isString() && resultWithRange.toString() == "hello-world", 
                  "Should concatenate specified range");
        
        printTestResult("table.concat", true);
        
    } catch (const std::exception& e) {
        printTestResult("table.concat", false, e);
        throw;
    }
}

void TableLibTest::testSort() {
    printTestHeader("table.sort");
    
    try {
        auto state = std::make_unique<State>();
        
        // Create test table with numbers
        auto testTable = make_gc_table();
        testTable->set(Value(1), Value(3.0));
        testTable->set(Value(2), Value(1.0));
        testTable->set(Value(3), Value(4.0));
        testTable->set(Value(4), Value(2.0));
        
        // Test default sorting (ascending)
        state->push(Value(testTable));
        
        Value result = TableLib::sort(state.get(), 1);
        assertTrue(result.isNil(), "table.sort should return nil");
        
        // Verify sorting
        assertTrue(testTable->get(Value(1)).asNumber() == 1.0, "First element should be 1");
        assertTrue(testTable->get(Value(2)).asNumber() == 2.0, "Second element should be 2");
        assertTrue(testTable->get(Value(3)).asNumber() == 3.0, "Third element should be 3");
        assertTrue(testTable->get(Value(4)).asNumber() == 4.0, "Fourth element should be 4");
        
        printTestResult("table.sort", true);
        
    } catch (const std::exception& e) {
        printTestResult("table.sort", false, e);
        throw;
    }
}

void TableLibTest::testPack() {
    printTestHeader("table.pack");
    
    try {
        auto state = std::make_unique<State>();
        
        // Test packing arguments
        state->push(Value("first"));
        state->push(Value("second"));
        state->push(Value(42.0));
        
        Value result = TableLib::pack(state.get(), 3);
        assertTrue(result.isTable(), "table.pack should return a table");
        
        auto packed = result.asTable();
        
        // Verify packed values
        assertTrue(packed->get(Value(1)).toString() == "first", "First element should be 'first'");
        assertTrue(packed->get(Value(2)).toString() == "second", "Second element should be 'second'");
        assertTrue(packed->get(Value(3)).asNumber() == 42.0, "Third element should be 42");
        assertTrue(packed->get(Value("n")).asNumber() == 3.0, "n field should be 3");
        
        printTestResult("table.pack", true);
        
    } catch (const std::exception& e) {
        printTestResult("table.pack", false, e);
        throw;
    }
}

void TableLibTest::testUnpack() {
    printTestHeader("table.unpack");
    
    try {
        auto state = std::make_unique<State>();
        
        // Create test table
        auto testTable = make_gc_table();
        testTable->set(Value(1), Value("first"));
        testTable->set(Value(2), Value("second"));
        testTable->set(Value(3), Value("third"));
        testTable->set(Value("n"), Value(3.0));
        
        // Test unpacking (simplified - returns first element)
        state->push(Value(testTable));
        
        Value result = TableLib::unpack(state.get(), 1);
        assertTrue(result.isString() && result.toString() == "first", 
                  "table.unpack should return first element");
        
        printTestResult("table.unpack", true);
        
    } catch (const std::exception& e) {
        printTestResult("table.unpack", false, e);
        throw;
    }
}

void TableLibTest::testMove() {
    printTestHeader("table.move");
    
    try {
        auto state = std::make_unique<State>();
        
        // Create source table
        auto sourceTable = make_gc_table();
        sourceTable->set(Value(1), Value("a"));
        sourceTable->set(Value(2), Value("b"));
        sourceTable->set(Value(3), Value("c"));
        sourceTable->set(Value(4), Value("d"));
        
        // Test moving within same table
        state->push(Value(sourceTable));
        state->push(Value(2)); // from
        state->push(Value(3)); // to  
        state->push(Value(5)); // dest position
        
        Value result = TableLib::move(state.get(), 4);
        assertTrue(result.isTable(), "table.move should return source table");
        
        // Verify move
        assertTrue(sourceTable->get(Value(5)).toString() == "b", "Element should be moved to position 5");
        assertTrue(sourceTable->get(Value(6)).toString() == "c", "Element should be moved to position 6");
        
        printTestResult("table.move", true);
        
    } catch (const std::exception& e) {
        printTestResult("table.move", false, e);
        throw;
    }
}

void TableLibTest::testMaxn() {
    printTestHeader("table.maxn");
    
    try {
        auto state = std::make_unique<State>();
        
        // Create test table
        auto testTable = make_gc_table();
        testTable->set(Value(1), Value("a"));
        testTable->set(Value(5), Value("b"));
        testTable->set(Value(10), Value("c"));
        testTable->set(Value("key"), Value("value")); // Non-numeric key
        
        // Test finding maximum numeric index
        state->push(Value(testTable));
        
        Value result = TableLib::maxn(state.get(), 1);
        assertTrue(result.isNumber() && result.asNumber() == 10.0, 
                  "table.maxn should return maximum numeric index");
        
        printTestResult("table.maxn", true);
        
    } catch (const std::exception& e) {
        printTestResult("table.maxn", false, e);
        throw;
    }
}

// Helper method implementations

void TableLibTest::printTestHeader(const std::string& testName) {
    std::cout << "\n" << std::string(50, '-') << std::endl;
    std::cout << "  Testing: " << testName << std::endl;
    std::cout << std::string(50, '-') << std::endl;
}

void TableLibTest::printTestResult(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "  [OK] " << testName << " passed" << std::endl;
    } else {
        std::cout << "  [FAILED] " << testName << " failed" << std::endl;
    }
    std::cout << std::string(50, '-') << std::endl;
}

void TableLibTest::printTestResult(const std::string& testName, bool passed, const std::exception& e) {
    if (passed) {
        std::cout << "  [OK] " << testName << " passed" << std::endl;
    } else {
        std::cout << "  [FAILED] " << testName << " failed" << std::endl;
        std::cout << "  Error: " << e.what() << std::endl;
    }
    std::cout << std::string(50, '-') << std::endl;
}

void TableLibTest::assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error("Assertion failed: " + message);
    }
}

} // namespace Tests
} // namespace Lua
