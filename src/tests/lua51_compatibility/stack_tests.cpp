#include "test_suite.hpp"
#include "../../vm/lua_state.hpp"
#include <cmath>

namespace Lua {
namespace Testing {

    // StackOperationTests Implementation
    
    TestResult StackOperationTests::testPushPop() {
        LuaState* L = createTestState();
        if (!L) {
            return TestResult("Stack Push/Pop", false, "Failed to create test state");
        }
        
        try {
            // Test basic push/pop operations
            i32 initialTop = L->getTop();
            
            // Push various types
            L->pushNumber(42.5);
            L->pushString("test string");
            L->pushBoolean(true);
            L->pushNil();
            
            // Check stack size
            if (L->getTop() != initialTop + 4) {
                cleanupTestState(L);
                return TestResult("Stack Push/Pop", false, "Stack size incorrect after pushes");
            }
            
            // Test type checking
            if (!L->isNil(-1) || !L->isBoolean(-2) || !L->isString(-3) || !L->isNumber(-4)) {
                cleanupTestState(L);
                return TestResult("Stack Push/Pop", false, "Type checking failed");
            }
            
            // Test value retrieval
            if (L->toBoolean(-2) != true) {
                cleanupTestState(L);
                return TestResult("Stack Push/Pop", false, "Boolean value incorrect");
            }
            
            if (std::abs(L->toNumber(-4) - 42.5) > 1e-9) {
                cleanupTestState(L);
                return TestResult("Stack Push/Pop", false, "Number value incorrect");
            }
            
            std::string str = L->toString(-3);
            if (str != "test string") {
                cleanupTestState(L);
                return TestResult("Stack Push/Pop", false, "String value incorrect");
            }
            
            // Test pop operations
            L->pop(2); // Remove nil and boolean
            if (L->getTop() != initialTop + 2) {
                cleanupTestState(L);
                return TestResult("Stack Push/Pop", false, "Stack size incorrect after pop");
            }
            
            // Verify remaining values
            if (!L->isString(-1) || !L->isNumber(-2)) {
                cleanupTestState(L);
                return TestResult("Stack Push/Pop", false, "Remaining values incorrect after pop");
            }
            
            cleanupTestState(L);
            return TestResult("Stack Push/Pop", true);
            
        } catch (const std::exception& e) {
            cleanupTestState(L);
            return TestResult("Stack Push/Pop", false, std::string("Exception: ") + e.what());
        }
    }
    
    TestResult StackOperationTests::testStackManipulation() {
        LuaState* L = createTestState();
        if (!L) {
            return TestResult("Stack Manipulation", false, "Failed to create test state");
        }
        
        try {
            // Test stack manipulation functions
            L->pushNumber(1.0);
            L->pushNumber(2.0);
            L->pushNumber(3.0);
            
            // Test copy
            L->pushValue(-2); // Copy second element (2.0)
            if (std::abs(L->toNumber(-1) - 2.0) > 1e-9) {
                cleanupTestState(L);
                return TestResult("Stack Manipulation", false, "pushValue failed");
            }
            
            // Test remove
            L->remove(-2); // Remove the copy
            if (L->getTop() != 3) {
                cleanupTestState(L);
                return TestResult("Stack Manipulation", false, "remove failed");
            }
            
            // Test insert
            L->pushNumber(4.0);
            L->insert(-2); // Insert 4.0 before 3.0
            
            // Stack should now be: 1.0, 2.0, 4.0, 3.0
            if (std::abs(L->toNumber(-1) - 3.0) > 1e-9 || 
                std::abs(L->toNumber(-2) - 4.0) > 1e-9) {
                cleanupTestState(L);
                return TestResult("Stack Manipulation", false, "insert failed");
            }
            
            // Test replace
            L->pushNumber(5.0);
            L->replace(-3); // Replace 4.0 with 5.0
            
            // Stack should now be: 1.0, 2.0, 5.0, 3.0
            if (std::abs(L->toNumber(-2) - 5.0) > 1e-9) {
                cleanupTestState(L);
                return TestResult("Stack Manipulation", false, "replace failed");
            }
            
            cleanupTestState(L);
            return TestResult("Stack Manipulation", true);
            
        } catch (const std::exception& e) {
            cleanupTestState(L);
            return TestResult("Stack Manipulation", false, std::string("Exception: ") + e.what());
        }
    }
    
    TestResult StackOperationTests::testTypeChecking() {
        LuaState* L = createTestState();
        if (!L) {
            return TestResult("Type Checking", false, "Failed to create test state");
        }
        
        try {
            // Push different types
            L->pushNil();
            L->pushBoolean(false);
            L->pushNumber(123.456);
            L->pushString("hello");
            
            // Test type checking functions
            if (!L->isNil(-4)) {
                cleanupTestState(L);
                return TestResult("Type Checking", false, "isNil failed");
            }
            
            if (!L->isBoolean(-3)) {
                cleanupTestState(L);
                return TestResult("Type Checking", false, "isBoolean failed");
            }
            
            if (!L->isNumber(-2)) {
                cleanupTestState(L);
                return TestResult("Type Checking", false, "isNumber failed");
            }
            
            if (!L->isString(-1)) {
                cleanupTestState(L);
                return TestResult("Type Checking", false, "isString failed");
            }
            
            // Test type() function
            if (L->type(-4) != LUA_TNIL) {
                cleanupTestState(L);
                return TestResult("Type Checking", false, "type() for nil failed");
            }
            
            if (L->type(-3) != LUA_TBOOLEAN) {
                cleanupTestState(L);
                return TestResult("Type Checking", false, "type() for boolean failed");
            }
            
            if (L->type(-2) != LUA_TNUMBER) {
                cleanupTestState(L);
                return TestResult("Type Checking", false, "type() for number failed");
            }
            
            if (L->type(-1) != LUA_TSTRING) {
                cleanupTestState(L);
                return TestResult("Type Checking", false, "type() for string failed");
            }
            
            // Test typename
            std::string nilTypeName = L->typeName(L->type(-4));
            if (nilTypeName != "nil") {
                cleanupTestState(L);
                return TestResult("Type Checking", false, "typeName for nil failed");
            }
            
            cleanupTestState(L);
            return TestResult("Type Checking", true);
            
        } catch (const std::exception& e) {
            cleanupTestState(L);
            return TestResult("Type Checking", false, std::string("Exception: ") + e.what());
        }
    }
    
    TestResult StackOperationTests::testStackOverflow() {
        LuaState* L = createTestState();
        if (!L) {
            return TestResult("Stack Overflow", false, "Failed to create test state");
        }
        
        try {
            // Test stack growth and overflow protection
            i32 initialTop = L->getTop();
            
            // Push many values to test stack growth
            for (i32 i = 0; i < 1000; ++i) {
                L->pushNumber(static_cast<double>(i));
            }
            
            if (L->getTop() != initialTop + 1000) {
                cleanupTestState(L);
                return TestResult("Stack Overflow", false, "Stack growth failed");
            }
            
            // Verify values are correct
            for (i32 i = 0; i < 100; ++i) { // Check first 100 values
                double expected = static_cast<double>(i);
                double actual = L->toNumber(-(1000 - i));
                if (std::abs(actual - expected) > 1e-9) {
                    cleanupTestState(L);
                    return TestResult("Stack Overflow", false, "Stack values corrupted during growth");
                }
            }
            
            cleanupTestState(L);
            return TestResult("Stack Overflow", true);
            
        } catch (const std::exception& e) {
            cleanupTestState(L);
            return TestResult("Stack Overflow", false, std::string("Exception: ") + e.what());
        }
    }
    
    TestResult StackOperationTests::testStackUnderflow() {
        LuaState* L = createTestState();
        if (!L) {
            return TestResult("Stack Underflow", false, "Failed to create test state");
        }
        
        try {
            // Test stack underflow protection
            i32 initialTop = L->getTop();
            
            // Try to access invalid stack positions
            if (L->isValid(-1)) {
                cleanupTestState(L);
                return TestResult("Stack Underflow", false, "Invalid stack position reported as valid");
            }
            
            // Push some values
            L->pushNumber(1.0);
            L->pushNumber(2.0);
            
            // Valid accesses
            if (!L->isValid(-1) || !L->isValid(-2)) {
                cleanupTestState(L);
                return TestResult("Stack Underflow", false, "Valid stack positions reported as invalid");
            }
            
            // Invalid access beyond stack
            if (L->isValid(-3)) {
                cleanupTestState(L);
                return TestResult("Stack Underflow", false, "Invalid stack position beyond stack reported as valid");
            }
            
            cleanupTestState(L);
            return TestResult("Stack Underflow", true);
            
        } catch (const std::exception& e) {
            cleanupTestState(L);
            return TestResult("Stack Underflow", false, std::string("Exception: ") + e.what());
        }
    }
    
    TestResult StackOperationTests::testStackResize() {
        LuaState* L = createTestState();
        if (!L) {
            return TestResult("Stack Resize", false, "Failed to create test state");
        }
        
        try {
            // Test stack resizing behavior
            i32 initialTop = L->getTop();
            
            // Fill stack with test data
            for (i32 i = 0; i < 500; ++i) {
                L->pushNumber(static_cast<double>(i));
            }
            
            // Clear most of the stack
            L->setTop(initialTop + 10);
            
            if (L->getTop() != initialTop + 10) {
                cleanupTestState(L);
                return TestResult("Stack Resize", false, "setTop failed");
            }
            
            // Verify remaining values
            for (i32 i = 0; i < 10; ++i) {
                double expected = static_cast<double>(i);
                double actual = L->toNumber(initialTop + i + 1);
                if (std::abs(actual - expected) > 1e-9) {
                    cleanupTestState(L);
                    return TestResult("Stack Resize", false, "Values corrupted after resize");
                }
            }
            
            cleanupTestState(L);
            return TestResult("Stack Resize", true);
            
        } catch (const std::exception& e) {
            cleanupTestState(L);
            return TestResult("Stack Resize", false, std::string("Exception: ") + e.what());
        }
    }

} // namespace Testing
} // namespace Lua
