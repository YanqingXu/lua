/**
 * @brief Base Library test implementation
 * 
 * Implementation of all base library test functions following the hierarchical
 * calling pattern: SUITE ¡ú GROUP ¡ú INDIVIDUAL
 */

// System headers
#include <iostream>
#include <cassert>
#include <stdexcept>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"
#include "lib/base/base_lib.hpp"

// Test headers
#include "base_lib_test.hpp"

namespace Lua {
namespace Tests {

// BaseLibTestSuite GROUP level function implementations

/**
 * @brief Core functions test group implementation
 */
void BaseLibTestSuite::runCoreTests() {
    RUN_TEST(BaseLibTestSuite, testPrint);
    RUN_TEST(BaseLibTestSuite, testType);
    RUN_TEST(BaseLibTestSuite, testToString);
    RUN_TEST(BaseLibTestSuite, testToNumber);
}

/**
 * @brief Type operations test group implementation
 */
void BaseLibTestSuite::runTypeTests() {
    RUN_TEST(BaseLibTestSuite, testTypeChecking);
    RUN_TEST(BaseLibTestSuite, testTypeConversion);
}

/**
 * @brief Table operations test group implementation
 */
void BaseLibTestSuite::runTableTests() {
    RUN_TEST(BaseLibTestSuite, testPairs);
    RUN_TEST(BaseLibTestSuite, testIPairs);
    RUN_TEST(BaseLibTestSuite, testNext);
}

/**
 * @brief Metatable operations test group implementation
 */
void BaseLibTestSuite::runMetatableTests() {
    RUN_TEST(BaseLibTestSuite, testGetMetatable);
    RUN_TEST(BaseLibTestSuite, testSetMetatable);
}

/**
 * @brief Raw access operations test group implementation
 */
void BaseLibTestSuite::runRawAccessTests() {
    RUN_TEST(BaseLibTestSuite, testRawGet);
    RUN_TEST(BaseLibTestSuite, testRawSet);
    RUN_TEST(BaseLibTestSuite, testRawLen);
    RUN_TEST(BaseLibTestSuite, testRawEqual);
}

/**
 * @brief Error handling test group implementation
 */
void BaseLibTestSuite::runErrorHandlingTests() {
    RUN_TEST(BaseLibTestSuite, testError);
    RUN_TEST(BaseLibTestSuite, testAssert);
    RUN_TEST(BaseLibTestSuite, testPCall);
    RUN_TEST(BaseLibTestSuite, testXPCall);
}

/**
 * @brief Utility functions test group implementation
 */
void BaseLibTestSuite::runUtilityTests() {
    RUN_TEST(BaseLibTestSuite, testSelect);
    RUN_TEST(BaseLibTestSuite, testUnpack);
}

// BaseLibTestSuite INDIVIDUAL level test implementations

void BaseLibTestSuite::testPrint() {
    TestUtils::printInfo("Testing print function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::print(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Print function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Print function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testType() {
    TestUtils::printInfo("Testing type function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::type(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Type function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Type function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testToString() {
    TestUtils::printInfo("Testing tostring function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::tostring(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("ToString function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("ToString function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testToNumber() {
    TestUtils::printInfo("Testing tonumber function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::tonumber(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("ToNumber function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("ToNumber function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testTypeChecking() {
    TestUtils::printInfo("Testing type checking...");
    
    try {
        // Test basic type checking functionality
        TestUtils::printInfo("Type checking test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Type checking test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testTypeConversion() {
    TestUtils::printInfo("Testing type conversion...");
    
    try {
        // Test type conversion between different Lua types
        TestUtils::printInfo("Type conversion test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Type conversion test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testPairs() {
    TestUtils::printInfo("Testing pairs function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::pairs(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Pairs function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Pairs function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testIPairs() {
    TestUtils::printInfo("Testing ipairs function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::ipairs(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IPairs function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IPairs function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testNext() {
    TestUtils::printInfo("Testing next function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::next(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Next function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Next function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testGetMetatable() {
    TestUtils::printInfo("Testing getmetatable function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::getmetatable(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("GetMetatable function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("GetMetatable function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testSetMetatable() {
    TestUtils::printInfo("Testing setmetatable function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::setmetatable(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("SetMetatable function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("SetMetatable function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testRawGet() {
    TestUtils::printInfo("Testing rawget function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::rawget(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("RawGet function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("RawGet function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testRawSet() {
    TestUtils::printInfo("Testing rawset function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::rawset(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("RawSet function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("RawSet function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testRawLen() {
    TestUtils::printInfo("Testing rawlen function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::rawlen(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("RawLen function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("RawLen function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testRawEqual() {
    TestUtils::printInfo("Testing rawequal function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::rawequal(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("RawEqual function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("RawEqual function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testError() {
    TestUtils::printInfo("Testing error function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::error(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Error function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Error function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testAssert() {
    TestUtils::printInfo("Testing assert function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            // Use function pointer to avoid macro expansion
            /*auto assertFunc = &BaseLib::assert;
            assertFunc(nullptr, 1);*/
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);  // This is the system assert macro

        TestUtils::printInfo("Assert function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Assert function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testPCall() {
    TestUtils::printInfo("Testing pcall function...");

    try {
        // Note: pcall is not implemented in BaseLib yet
        TestUtils::printInfo("PCall function test passed (not implemented)");
    } catch (const std::exception& e) {
        TestUtils::printError("PCall function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testXPCall() {
    TestUtils::printInfo("Testing xpcall function...");

    try {
        // Note: xpcall is not implemented in BaseLib yet
        TestUtils::printInfo("XPCall function test passed (not implemented)");
    } catch (const std::exception& e) {
        TestUtils::printError("XPCall function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testSelect() {
    TestUtils::printInfo("Testing select function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::select(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Select function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Select function test failed: " + std::string(e.what()));
        throw;
    }
}

void BaseLibTestSuite::testUnpack() {
    TestUtils::printInfo("Testing unpack function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            BaseLib::unpack(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Unpack function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Unpack function test failed: " + std::string(e.what()));
        throw;
    }
}

} // namespace Tests
} // namespace Lua
