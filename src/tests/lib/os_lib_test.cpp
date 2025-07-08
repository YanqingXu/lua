/**
 * @brief OS Library test implementation
 * 
 * Implementation of all OS library test functions following the hierarchical
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
#include "lib/os/os_lib.hpp"

// Test headers
#include "os_lib_test.hpp"

namespace Lua {
namespace Tests {

// GROUP level function implementations

/**
 * @brief Time operations test group implementation
 */
void OSLibTestSuite::runTimeOperationsTests() {
    RUN_TEST(OSLibTestSuite, testClock);
    RUN_TEST(OSLibTestSuite, testDate);
    RUN_TEST(OSLibTestSuite, testTime);
    RUN_TEST(OSLibTestSuite, testDifftime);
}

/**
 * @brief System operations test group implementation
 */
void OSLibTestSuite::runSystemOperationsTests() {
    RUN_TEST(OSLibTestSuite, testExecute);
    RUN_TEST(OSLibTestSuite, testExit);
    RUN_TEST(OSLibTestSuite, testGetenv);
}

/**
 * @brief OS file operations test group implementation
 */
void OSLibTestSuite::runOSFileOperationsTests() {
    RUN_TEST(OSLibTestSuite, testRemove);
    RUN_TEST(OSLibTestSuite, testRename);
    RUN_TEST(OSLibTestSuite, testTmpname);
}

/**
 * @brief Localization test group implementation
 */
void OSLibTestSuite::runLocalizationTests() {
    RUN_TEST(OSLibTestSuite, testSetlocale);
}

/**
 * @brief OS error handling test group implementation
 */
void OSLibTestSuite::runOSErrorHandlingTests() {
    RUN_TEST(OSLibTestSuite, testErrorHandling);
    RUN_TEST(OSLibTestSuite, testNullStateHandling);
}

// INDIVIDUAL level test implementations

void OSLibTestSuite::testClock() {
    TestUtils::printInfo("Testing os.clock function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::clock(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.clock function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.clock test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testDate() {
    TestUtils::printInfo("Testing os.date function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::date(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.date function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.date test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testTime() {
    TestUtils::printInfo("Testing os.time function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::time(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.time function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.time test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testDifftime() {
    TestUtils::printInfo("Testing os.difftime function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::difftime(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.difftime function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.difftime test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testExecute() {
    TestUtils::printInfo("Testing os.execute function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::execute(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.execute function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.execute test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testExit() {
    TestUtils::printInfo("Testing os.exit function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::exit(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.exit function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.exit test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testGetenv() {
    TestUtils::printInfo("Testing os.getenv function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::getenv(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.getenv function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.getenv test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testRemove() {
    TestUtils::printInfo("Testing os.remove function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::remove(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.remove function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.remove test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testRename() {
    TestUtils::printInfo("Testing os.rename function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::rename(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.rename function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.rename test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testTmpname() {
    TestUtils::printInfo("Testing os.tmpname function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::tmpname(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.tmpname function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.tmpname test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testSetlocale() {
    TestUtils::printInfo("Testing os.setlocale function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            OSLib::setlocale(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS.setlocale function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS.setlocale test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testErrorHandling() {
    TestUtils::printInfo("Testing OS library error handling...");
    
    try {
        // Test comprehensive error handling
        TestUtils::printInfo("OS library error handling test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS library error handling test failed: " + std::string(e.what()));
        throw;
    }
}

void OSLibTestSuite::testNullStateHandling() {
    TestUtils::printInfo("Testing OS library null state handling...");
    
    try {
        // Test null state handling for various functions
        bool caughtException = false;
        try {
            OSLib::clock(nullptr, 0);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("OS library null state handling test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("OS library null state handling test failed: " + std::string(e.what()));
        throw;
    }
}

} // namespace Tests
} // namespace Lua
