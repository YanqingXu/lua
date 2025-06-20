#include "base_lib_test.hpp"
#include "../../lib/base_lib.hpp"
#include "../../vm/state.hpp"
#include "../../vm/value.hpp"
#include "../../vm/table.hpp"
#include "../../gc/core/gc_ref.hpp"
#include <iostream>
#include <cassert>
#include <sstream>

namespace Lua::Tests {

void BaseLibTestSuite::runAllTests() {
    //RUN_TEST_GROUP("Type Conversion Tests", runTypeConversionTests);
    //RUN_TEST_GROUP("Table Access Tests", runTableAccessTests);
    //RUN_TEST_GROUP("Iterator Tests", runIteratorTests);
    //RUN_TEST_GROUP("Metatable Tests", runMetatableTests);
    RUN_TEST_GROUP("Utility Tests", runUtilityTests);
}

void BaseLibTestSuite::runTypeConversionTests() {
    RUN_TEST(BaseLibTestSuite, testTonumber);
    RUN_TEST(BaseLibTestSuite, testTostring);
    RUN_TEST(BaseLibTestSuite, testType);
}

void BaseLibTestSuite::runTableAccessTests() {
    RUN_TEST(BaseLibTestSuite, testRawget);
    RUN_TEST(BaseLibTestSuite, testRawset);
    RUN_TEST(BaseLibTestSuite, testRawlen);
    RUN_TEST(BaseLibTestSuite, testRawequal);
}

void BaseLibTestSuite::runIteratorTests() {
    RUN_TEST(BaseLibTestSuite, testPairs);
    RUN_TEST(BaseLibTestSuite, testIpairs);
    RUN_TEST(BaseLibTestSuite, testNext);
}

void BaseLibTestSuite::runMetatableTests() {
    RUN_TEST(BaseLibTestSuite, testGetmetatable);
    RUN_TEST(BaseLibTestSuite, testSetmetatable);
}

void BaseLibTestSuite::runUtilityTests() {
    RUN_TEST(BaseLibTestSuite, testPrint);
    //RUN_TEST(BaseLibTestSuite, testSelect);
    //RUN_TEST(BaseLibTestSuite, testUnpack);
}

void BaseLibTestSuite::testPrint() {
    // Create test state
    auto state = std::make_unique<State>();
    
    // Test print function - since print outputs to stdout, mainly test it doesn't crash
    // In actual implementation, might need to redirect stdout to verify output
    
    // Test call with no arguments
    Value result = BaseLib::print(state.get(), 0);
    
    // Print should return nil and not throw exceptions
    assert(result.isNil());
}

void BaseLibTestSuite::testTonumber() {
    auto state = std::make_unique<State>();
    
    // Test string to number conversion
    // This needs to be adjusted based on actual Value and State implementation
    
    // Test with no arguments - should return nil
    Value result = BaseLib::tonumber(state.get(), 0);
    assert(result.isNil());
}

void BaseLibTestSuite::testTostring() {
    auto state = std::make_unique<State>();
    
    // Test various types to string conversion
    Value result = BaseLib::tostring(state.get(), 0);
    
    // tostring should return a string representation
    assert(result.isString());
}

void BaseLibTestSuite::testType() {
    auto state = std::make_unique<State>();
    
    // Test type function
    Value result = BaseLib::type(state.get(), 0);
    
    // type should return a string indicating the type
    assert(result.isString());
}

void BaseLibTestSuite::testIpairs() {
    auto state = std::make_unique<State>();
    
    // Test ipairs function
    Value result = BaseLib::ipairs(state.get(), 0);
    
    // ipairs should return iterator function, here simply test it doesn't crash
    // In a more complete implementation, we would test the actual iterator behavior
}

void BaseLibTestSuite::testPairs() {
    auto state = std::make_unique<State>();
    
    // Test pairs function
    Value result = BaseLib::pairs(state.get(), 0);
    
    // pairs should return iterator function, here simply test it doesn't crash
    // In a more complete implementation, we would test the actual iterator behavior
}

void BaseLibTestSuite::testNext() {
    auto state = std::make_unique<State>();
    
    // Test next function
    Value result = BaseLib::next(state.get(), 0);
    
    // next function test - behavior depends on arguments
    // Basic test to ensure it doesn't crash
}

void BaseLibTestSuite::testGetmetatable() {
    auto state = std::make_unique<State>();
    
    // Test getmetatable function
    Value result = BaseLib::getmetatable(state.get(), 0);
    
    // getmetatable test - should handle nil argument gracefully
}

void BaseLibTestSuite::testSetmetatable() {
    auto state = std::make_unique<State>();
    
    // Test setmetatable function
    Value result = BaseLib::setmetatable(state.get(), 0);
    
    // setmetatable test - should handle insufficient arguments
}

void BaseLibTestSuite::testRawget() {
    auto state = std::make_unique<State>();
    
    // Test rawget function
    Value result = BaseLib::rawget(state.get(), 0);
    
    // rawget test - should handle insufficient arguments
}

void BaseLibTestSuite::testRawset() {
    auto state = std::make_unique<State>();
    
    // Test rawset function
    Value result = BaseLib::rawset(state.get(), 0);
    
    // rawset test - should handle insufficient arguments
}

void BaseLibTestSuite::testRawlen() {
    auto state = std::make_unique<State>();
    
    // Test rawlen function
    Value result = BaseLib::rawlen(state.get(), 0);
    
    // rawlen test - should handle nil argument
}

void BaseLibTestSuite::testRawequal() {
    auto state = std::make_unique<State>();
    
    // Test rawequal function
    Value result = BaseLib::rawequal(state.get(), 0);
    
    // rawequal test - should handle insufficient arguments
}

void BaseLibTestSuite::testSelect() {
    auto state = std::make_unique<State>();
    
    // Test select function
    Value result = BaseLib::select(state.get(), 0);
    
    // select test - should handle no arguments case
}

void BaseLibTestSuite::testUnpack() {
    auto state = std::make_unique<State>();
    
    // Test unpack function
    Value result = BaseLib::unpack(state.get(), 0);
    
    // unpack test - should handle no arguments case
}

} // namespace Lua::Tests