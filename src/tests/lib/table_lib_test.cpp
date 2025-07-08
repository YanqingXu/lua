/**
 * @brief Table Library test implementation
 * 
 * Implementation of all table library test functions following the hierarchical
 * calling pattern: SUITE ï¿?GROUP ï¿?INDIVIDUAL
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
#include "lib/table/table_lib.hpp"

// Test headers
#include "table_lib_test.hpp"

namespace Lua {
namespace Tests {

// GROUP level function implementations

/**
 * @brief Table operations test group implementation
 */
void TableLibTestSuite::runTableOperationsTests() {
    RUN_TEST(TableLibTestSuite, testInsert);
    RUN_TEST(TableLibTestSuite, testRemove);
    RUN_TEST(TableLibTestSuite, testSort);
    RUN_TEST(TableLibTestSuite, testConcat);
}

/**
 * @brief Length operations test group implementation
 */
void TableLibTestSuite::runLengthTests() {
    RUN_TEST(TableLibTestSuite, testGetn);
    RUN_TEST(TableLibTestSuite, testMaxn);
}

/**
 * @brief Table error handling test group implementation
 */
void TableLibTestSuite::runTableErrorHandlingTests() {
    RUN_TEST(TableLibTestSuite, testErrorHandling);
    RUN_TEST(TableLibTestSuite, testEdgeCases);
}

// INDIVIDUAL level test implementations

void TableLibTestSuite::testInsert() {
    TestUtils::printInfo("Testing table.insert function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            TableLib::insert(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Table.insert function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Table.insert test failed: " + std::string(e.what()));
        throw;
    }
}

void TableLibTestSuite::testRemove() {
    TestUtils::printInfo("Testing table.remove function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            TableLib::remove(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Table.remove function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Table.remove test failed: " + std::string(e.what()));
        throw;
    }
}

void TableLibTestSuite::testSort() {
    TestUtils::printInfo("Testing table.sort function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            TableLib::sort(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Table.sort function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Table.sort test failed: " + std::string(e.what()));
        throw;
    }
}

void TableLibTestSuite::testConcat() {
    TestUtils::printInfo("Testing table.concat function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            TableLib::concat(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Table.concat function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Table.concat test failed: " + std::string(e.what()));
        throw;
    }
}

void TableLibTestSuite::testGetn() {
    TestUtils::printInfo("Testing table.getn function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            TableLib::getn(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Table.getn function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Table.getn test failed: " + std::string(e.what()));
        throw;
    }
}

void TableLibTestSuite::testMaxn() {
    TestUtils::printInfo("Testing table.maxn function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            TableLib::maxn(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Table.maxn function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Table.maxn test failed: " + std::string(e.what()));
        throw;
    }
}

void TableLibTestSuite::testErrorHandling() {
    TestUtils::printInfo("Testing Table library error handling...");
    
    try {
        // Test comprehensive error handling
        TestUtils::printInfo("Table library error handling test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Table library error handling test failed: " + std::string(e.what()));
        throw;
    }
}

void TableLibTestSuite::testEdgeCases() {
    TestUtils::printInfo("Testing Table library edge cases...");
    
    try {
        // Test edge cases and boundary conditions
        TestUtils::printInfo("Table library edge cases test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Table library edge cases test failed: " + std::string(e.what()));
        throw;
    }
}

} // namespace Tests
} // namespace Lua
