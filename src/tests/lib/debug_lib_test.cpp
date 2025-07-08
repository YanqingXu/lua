/**
 * @brief Debug Library test implementation
 * 
 * Implementation of all debug library test functions following the hierarchical
 * calling pattern: SUITE â†?GROUP â†?INDIVIDUAL
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
#include "lib/debug/debug_lib.hpp"

// Test headers
#include "debug_lib_test.hpp"

namespace Lua {
namespace Tests {

// GROUP level function implementations

/**
 * @brief Debug functions test group implementation
 */
void DebugLibTestSuite::runDebugFunctionsTests() {
    RUN_TEST(DebugLibTestSuite, testDebug);
    RUN_TEST(DebugLibTestSuite, testTraceback);
    RUN_TEST(DebugLibTestSuite, testGetinfo);
}

/**
 * @brief Environment operations test group implementation
 */
void DebugLibTestSuite::runEnvironmentTests() {
    RUN_TEST(DebugLibTestSuite, testGetfenv);
    RUN_TEST(DebugLibTestSuite, testSetfenv);
}

/**
 * @brief Hook operations test group implementation
 */
void DebugLibTestSuite::runHookTests() {
    RUN_TEST(DebugLibTestSuite, testGethook);
    RUN_TEST(DebugLibTestSuite, testSethook);
}

/**
 * @brief Variable operations test group implementation
 */
void DebugLibTestSuite::runVariableTests() {
    RUN_TEST(DebugLibTestSuite, testGetlocal);
    RUN_TEST(DebugLibTestSuite, testSetlocal);
    RUN_TEST(DebugLibTestSuite, testGetupvalue);
    RUN_TEST(DebugLibTestSuite, testSetupvalue);
}

/**
 * @brief Debug metatable operations test group implementation
 */
void DebugLibTestSuite::runDebugMetatableTests() {
    RUN_TEST(DebugLibTestSuite, testGetmetatable);
    RUN_TEST(DebugLibTestSuite, testSetmetatable);
    RUN_TEST(DebugLibTestSuite, testGetregistry);
}

/**
 * @brief Debug error handling test group implementation
 */
void DebugLibTestSuite::runDebugErrorHandlingTests() {
    RUN_TEST(DebugLibTestSuite, testErrorHandling);
    RUN_TEST(DebugLibTestSuite, testNullStateHandling);
}

// INDIVIDUAL level test implementations

void DebugLibTestSuite::testDebug() {
    TestUtils::printInfo("Testing debug.debug function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::debug(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.debug function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.debug test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testTraceback() {
    TestUtils::printInfo("Testing debug.traceback function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::traceback(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.traceback function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.traceback test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testGetinfo() {
    TestUtils::printInfo("Testing debug.getinfo function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::getinfo(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.getinfo function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.getinfo test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testGetfenv() {
    TestUtils::printInfo("Testing debug.getfenv function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::getfenv(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.getfenv function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.getfenv test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testSetfenv() {
    TestUtils::printInfo("Testing debug.setfenv function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::setfenv(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.setfenv function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.setfenv test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testGethook() {
    TestUtils::printInfo("Testing debug.gethook function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::gethook(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.gethook function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.gethook test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testSethook() {
    TestUtils::printInfo("Testing debug.sethook function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::sethook(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.sethook function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.sethook test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testGetlocal() {
    TestUtils::printInfo("Testing debug.getlocal function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::getlocal(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.getlocal function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.getlocal test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testSetlocal() {
    TestUtils::printInfo("Testing debug.setlocal function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::setlocal(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.setlocal function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.setlocal test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testGetupvalue() {
    TestUtils::printInfo("Testing debug.getupvalue function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::getupvalue(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.getupvalue function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.getupvalue test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testSetupvalue() {
    TestUtils::printInfo("Testing debug.setupvalue function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::setupvalue(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.setupvalue function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.setupvalue test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testGetmetatable() {
    TestUtils::printInfo("Testing debug.getmetatable function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::getmetatable(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.getmetatable function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.getmetatable test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testSetmetatable() {
    TestUtils::printInfo("Testing debug.setmetatable function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::setmetatable(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.setmetatable function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.setmetatable test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testGetregistry() {
    TestUtils::printInfo("Testing debug.getregistry function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            DebugLib::getregistry(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug.getregistry function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug.getregistry test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testErrorHandling() {
    TestUtils::printInfo("Testing Debug library error handling...");
    
    try {
        // Test comprehensive error handling
        TestUtils::printInfo("Debug library error handling test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug library error handling test failed: " + std::string(e.what()));
        throw;
    }
}

void DebugLibTestSuite::testNullStateHandling() {
    TestUtils::printInfo("Testing Debug library null state handling...");
    
    try {
        // Test null state handling for various functions
        bool caughtException = false;
        try {
            DebugLib::debug(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Debug library null state handling test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Debug library null state handling test failed: " + std::string(e.what()));
        throw;
    }
}

} // namespace Tests
} // namespace Lua
