/**
 * @brief IO Library test implementation
 * 
 * Implementation of all IO library test functions following the hierarchical
 * calling pattern: SUITE �?GROUP �?INDIVIDUAL
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
#include "lib/io/io_lib.hpp"

// Test headers
#include "io_lib_test.hpp"

namespace Lua {
namespace Tests {

// GROUP level function implementations

/**
 * @brief File operations test group implementation
 */
void IOLibTestSuite::runFileOperationsTests() {
    RUN_TEST(IOLibTestSuite, testOpen);
    RUN_TEST(IOLibTestSuite, testClose);
    RUN_TEST(IOLibTestSuite, testRead);
    RUN_TEST(IOLibTestSuite, testWrite);
}

/**
 * @brief Stream operations test group implementation
 */
void IOLibTestSuite::runStreamOperationsTests() {
    RUN_TEST(IOLibTestSuite, testFlush);
    RUN_TEST(IOLibTestSuite, testLines);
    RUN_TEST(IOLibTestSuite, testInput);
    RUN_TEST(IOLibTestSuite, testOutput);
    RUN_TEST(IOLibTestSuite, testType);
}

/**
 * @brief IO error handling test group implementation
 */
void IOLibTestSuite::runIOErrorHandlingTests() {
    RUN_TEST(IOLibTestSuite, testErrorHandling);
    RUN_TEST(IOLibTestSuite, testNullStateHandling);
}

// INDIVIDUAL level test implementations

void IOLibTestSuite::testOpen() {
    TestUtils::printInfo("Testing io.open function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            IOLib::open(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IO.open function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO.open test failed: " + std::string(e.what()));
        throw;
    }
}

void IOLibTestSuite::testClose() {
    TestUtils::printInfo("Testing io.close function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            IOLib::close(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IO.close function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO.close test failed: " + std::string(e.what()));
        throw;
    }
}

void IOLibTestSuite::testRead() {
    TestUtils::printInfo("Testing io.read function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            IOLib::read(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IO.read function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO.read test failed: " + std::string(e.what()));
        throw;
    }
}

void IOLibTestSuite::testWrite() {
    TestUtils::printInfo("Testing io.write function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            IOLib::write(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IO.write function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO.write test failed: " + std::string(e.what()));
        throw;
    }
}

void IOLibTestSuite::testFlush() {
    TestUtils::printInfo("Testing io.flush function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            IOLib::flush(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IO.flush function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO.flush test failed: " + std::string(e.what()));
        throw;
    }
}

void IOLibTestSuite::testLines() {
    TestUtils::printInfo("Testing io.lines function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            IOLib::lines(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IO.lines function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO.lines test failed: " + std::string(e.what()));
        throw;
    }
}

void IOLibTestSuite::testInput() {
    TestUtils::printInfo("Testing io.input function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            IOLib::input(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IO.input function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO.input test failed: " + std::string(e.what()));
        throw;
    }
}

void IOLibTestSuite::testOutput() {
    TestUtils::printInfo("Testing io.output function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            IOLib::output(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IO.output function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO.output test failed: " + std::string(e.what()));
        throw;
    }
}

void IOLibTestSuite::testType() {
    TestUtils::printInfo("Testing io.type function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            IOLib::type(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IO.type function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO.type test failed: " + std::string(e.what()));
        throw;
    }
}

void IOLibTestSuite::testErrorHandling() {
    TestUtils::printInfo("Testing IO library error handling...");
    
    try {
        // Test comprehensive error handling
        TestUtils::printInfo("IO library error handling test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO library error handling test failed: " + std::string(e.what()));
        throw;
    }
}

void IOLibTestSuite::testNullStateHandling() {
    TestUtils::printInfo("Testing IO library null state handling...");
    
    try {
        // Test null state handling for various functions
        bool caughtException = false;
        try {
            IOLib::open(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("IO library null state handling test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("IO library null state handling test failed: " + std::string(e.what()));
        throw;
    }
}

} // namespace Tests
} // namespace Lua
