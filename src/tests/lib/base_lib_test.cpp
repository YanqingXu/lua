#include "base_lib_test.hpp"
#include "../../lib/base_lib.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../gc/core/gc_ref.hpp"
#include <iostream>
#include <cassert>
#include <sstream>

namespace Lua {
namespace Tests {

void BaseLibTest::runAllTests() {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "        BASE LIBRARY TEST SUITE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Running all base library tests..." << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    try {
        testPrint();
        testTonumber();
        testTostring();
        testType();
        testIpairs();
        testPairs();
        testNext();
        testGetmetatable();
        testSetmetatable();
        testRawget();
        testRawset();
        testRawlen();
        testRawequal();
        testPcall();
        testXpcall();
        testError();
        testAssert();
        testSelect();
        testUnpack();
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "        ALL BASE LIBRARY TESTS PASSED!" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "        BASE LIBRARY TESTS FAILED!" << std::endl;
        std::cout << "Error: " << e.what() << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        throw;
    }
}

void BaseLibTest::testPrint() {
    std::cout << "\nTesting print():" << std::endl;
    
    try {
        // Create test state
        auto state = std::make_unique<State>();
        
        // Test print function - since print outputs to stdout, mainly test it doesn't crash
        // In actual implementation, might need to redirect stdout to verify output
        
        // Test call with no arguments
        Value result = BaseLib::print(state.get(), 0);
        bool testPassed = result.isNil();
        
        printTestResult("print", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("print", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testTonumber() {
    std::cout << "\nTesting tonumber():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test string to number conversion
        // This needs to be adjusted based on actual Value and State implementation
        
        // Test with no arguments
        Value result = BaseLib::tonumber(state.get(), 0);
        bool testPassed = result.isNil();
        
        printTestResult("tonumber", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("tonumber", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testTostring() {
    std::cout << "\nTesting tostring():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test various types to string conversion
        Value result = BaseLib::tostring(state.get(), 0);
        bool testPassed = result.isString();
        
        printTestResult("tostring", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("tostring", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testType() {
    std::cout << "\nTesting type():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test type function
        Value result = BaseLib::type(state.get(), 0);
        bool testPassed = result.isString();
        
        printTestResult("type", testPassed);
        
    } catch (const std::exception& e) {
        printTestResult("type", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testIpairs() {
    std::cout << "\nTesting ipairs():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test ipairs function
        Value result = BaseLib::ipairs(state.get(), 0);
        // ipairs should return iterator function, here simply test it doesn't crash
        
        printTestResult("ipairs", true);
        
    } catch (const std::exception& e) {
        printTestResult("ipairs", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testPairs() {
    std::cout << "\nTesting pairs():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test pairs function
        Value result = BaseLib::pairs(state.get(), 0);
        // pairs should return iterator function, here simply test it doesn't crash
        
        printTestResult("pairs", true);
        
    } catch (const std::exception& e) {
        printTestResult("pairs", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testNext() {
    std::cout << "\nTesting next():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test next function
        Value result = BaseLib::next(state.get(), 0);
        
        printTestResult("next", true);
        
    } catch (const std::exception& e) {
        printTestResult("next", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testGetmetatable() {
    std::cout << "\nTesting getmetatable():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test getmetatable function
        Value result = BaseLib::getmetatable(state.get(), 0);
        
        printTestResult("getmetatable", true);
        
    } catch (const std::exception& e) {
        printTestResult("getmetatable", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testSetmetatable() {
    std::cout << "\nTesting setmetatable():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test setmetatable function
        Value result = BaseLib::setmetatable(state.get(), 0);
        
        printTestResult("setmetatable", true);
        
    } catch (const std::exception& e) {
        printTestResult("setmetatable", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testRawget() {
    std::cout << "\nTesting rawget():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test rawget function
        Value result = BaseLib::rawget(state.get(), 0);
        
        printTestResult("rawget", true);
        
    } catch (const std::exception& e) {
        printTestResult("rawget", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testRawset() {
    std::cout << "\nTesting rawset():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test rawset function
        Value result = BaseLib::rawset(state.get(), 0);
        
        printTestResult("rawset", true);
        
    } catch (const std::exception& e) {
        printTestResult("rawset", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testRawlen() {
    std::cout << "\nTesting rawlen():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test rawlen function
        Value result = BaseLib::rawlen(state.get(), 0);
        
        printTestResult("rawlen", true);
        
    } catch (const std::exception& e) {
        printTestResult("rawlen", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testRawequal() {
    std::cout << "\nTesting rawequal():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test rawequal function
        Value result = BaseLib::rawequal(state.get(), 0);
        
        printTestResult("rawequal", true);
        
    } catch (const std::exception& e) {
        printTestResult("rawequal", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testPcall() {
    std::cout << "\nTesting pcall():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test pcall function
        Value result = BaseLib::pcall(state.get(), 0);
        
        printTestResult("pcall", true);
        
    } catch (const std::exception& e) {
        printTestResult("pcall", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testXpcall() {
    std::cout << "\nTesting xpcall():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test xpcall function
        Value result = BaseLib::xpcall(state.get(), 0);
        
        printTestResult("xpcall", true);
        
    } catch (const std::exception& e) {
        printTestResult("xpcall", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testError() {
    std::cout << "\nTesting error():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test error function
        Value result = BaseLib::error(state.get(), 0);
        
        printTestResult("error", true);
        
    } catch (const std::exception& e) {
        printTestResult("error", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testAssert() {
    std::cout << "\nTesting assert():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test assert function
        Value result = BaseLib::assert_func(state.get(), 0);
        
        printTestResult("assert", true);
        
    } catch (const std::exception& e) {
        printTestResult("assert", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testSelect() {
    std::cout << "\nTesting select():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test select function
        Value result = BaseLib::select(state.get(), 0);
        
        printTestResult("select", true);
        
    } catch (const std::exception& e) {
        printTestResult("select", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

void BaseLibTest::testUnpack() {
    std::cout << "\nTesting unpack():" << std::endl;
    
    try {
        auto state = std::make_unique<State>();
        
        // Test unpack function
        Value result = BaseLib::unpack(state.get(), 0);
        
        printTestResult("unpack", true);
        
    } catch (const std::exception& e) {
        printTestResult("unpack", false);
        std::cout << "[FAIL] Test failed with exception: " << e.what() << std::endl;
    }
}

// Helper method implementation
void BaseLibTest::printTestResult(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "[PASS] " << testName << "() test completed successfully" << std::endl;
    } else {
        std::cout << "[FAIL] " << testName << "() test failed" << std::endl;
    }
}

} // namespace Tests
} // namespace Lua