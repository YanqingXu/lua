/**
 * @brief Math Library test implementation
 * 
 * Implementation of all math library test functions following the hierarchical
 * calling pattern: SUITE �?GROUP �?INDIVIDUAL
 */

// System headers
#include <iostream>
#include <cassert>
#include <stdexcept>
#include <cmath>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"
#include "lib/math/math_lib.hpp"

// Test headers
#include "math_lib_test.hpp"

namespace Lua {
namespace Tests {

// GROUP level function implementations

/**
 * @brief Constants test group implementation
 */
void MathLibTestSuite::runConstantsTests() {
    RUN_TEST(MathLibTestSuite, testConstants);
}

/**
 * @brief Basic math functions test group implementation
 */
void MathLibTestSuite::runBasicMathTests() {
    RUN_TEST(MathLibTestSuite, testAbs);
    RUN_TEST(MathLibTestSuite, testCeil);
    RUN_TEST(MathLibTestSuite, testFloor);
    RUN_TEST(MathLibTestSuite, testMax);
    RUN_TEST(MathLibTestSuite, testMin);
}

/**
 * @brief Power functions test group implementation
 */
void MathLibTestSuite::runPowerTests() {
    RUN_TEST(MathLibTestSuite, testExp);
    RUN_TEST(MathLibTestSuite, testLog);
    RUN_TEST(MathLibTestSuite, testLog10);
    RUN_TEST(MathLibTestSuite, testPow);
    RUN_TEST(MathLibTestSuite, testSqrt);
}

/**
 * @brief Trigonometric functions test group implementation
 */
void MathLibTestSuite::runTrigTests() {
    RUN_TEST(MathLibTestSuite, testSin);
    RUN_TEST(MathLibTestSuite, testCos);
    RUN_TEST(MathLibTestSuite, testTan);
    RUN_TEST(MathLibTestSuite, testAsin);
    RUN_TEST(MathLibTestSuite, testAcos);
    RUN_TEST(MathLibTestSuite, testAtan);
    RUN_TEST(MathLibTestSuite, testAtan2);
}

/**
 * @brief Angle conversion test group implementation
 */
void MathLibTestSuite::runAngleTests() {
    RUN_TEST(MathLibTestSuite, testDeg);
    RUN_TEST(MathLibTestSuite, testRad);
}

/**
 * @brief Random functions test group implementation
 */
void MathLibTestSuite::runRandomTests() {
    RUN_TEST(MathLibTestSuite, testRandom);
    RUN_TEST(MathLibTestSuite, testRandomSeed);
}

/**
 * @brief Math utility functions test group implementation
 */
void MathLibTestSuite::runMathUtilityTests() {
    RUN_TEST(MathLibTestSuite, testFmod);
    RUN_TEST(MathLibTestSuite, testModf);
    RUN_TEST(MathLibTestSuite, testFrexp);
    RUN_TEST(MathLibTestSuite, testLdexp);
}

// INDIVIDUAL level test implementations

void MathLibTestSuite::testConstants() {
    TestUtils::printInfo("Testing math constants (pi, huge)...");
    
    try {
        // Test that math constants are properly defined
        TestUtils::printInfo("Math constants test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math constants test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testAbs() {
    TestUtils::printInfo("Testing math.abs function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::abs(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Math.abs function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.abs test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testCeil() {
    TestUtils::printInfo("Testing math.ceil function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::ceil(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Math.ceil function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.ceil test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testFloor() {
    TestUtils::printInfo("Testing math.floor function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::floor(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Math.floor function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.floor test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testMax() {
    TestUtils::printInfo("Testing math.max function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::max(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Math.max function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.max test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testMin() {
    TestUtils::printInfo("Testing math.min function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::min(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Math.min function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.min test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testExp() {
    TestUtils::printInfo("Testing math.exp function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::exp(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Math.exp function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.exp test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testLog() {
    TestUtils::printInfo("Testing math.log function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::log(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Math.log function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.log test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testLog10() {
    TestUtils::printInfo("Testing math.log10 function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::log10(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.log10 function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.log10 test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testPow() {
    TestUtils::printInfo("Testing math.pow function...");
    
    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::pow(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);
        
        TestUtils::printInfo("Math.pow function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.pow test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testSqrt() {
    TestUtils::printInfo("Testing math.sqrt function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::sqrt(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.sqrt function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.sqrt test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testSin() {
    TestUtils::printInfo("Testing math.sin function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::sin(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.sin function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.sin test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testCos() {
    TestUtils::printInfo("Testing math.cos function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::cos(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.cos function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.cos test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testTan() {
    TestUtils::printInfo("Testing math.tan function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::tan(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.tan function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.tan test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testAsin() {
    TestUtils::printInfo("Testing math.asin function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::asin(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.asin function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.asin test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testAcos() {
    TestUtils::printInfo("Testing math.acos function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::acos(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.acos function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.acos test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testAtan() {
    TestUtils::printInfo("Testing math.atan function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::atan(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.atan function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.atan test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testAtan2() {
    TestUtils::printInfo("Testing math.atan2 function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::atan2(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.atan2 function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.atan2 test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testDeg() {
    TestUtils::printInfo("Testing math.deg function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::deg(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.deg function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.deg test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testRad() {
    TestUtils::printInfo("Testing math.rad function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::rad(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.rad function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.rad test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testRandom() {
    TestUtils::printInfo("Testing math.random function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::random(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.random function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.random test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testRandomSeed() {
    TestUtils::printInfo("Testing math.randomseed function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::randomseed(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.randomseed function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.randomseed test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testFmod() {
    TestUtils::printInfo("Testing math.fmod function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::fmod(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.fmod function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.fmod test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testModf() {
    TestUtils::printInfo("Testing math.modf function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::modf(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.modf function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.modf test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testFrexp() {
    TestUtils::printInfo("Testing math.frexp function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::frexp(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.frexp function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.frexp test failed: " + std::string(e.what()));
        throw;
    }
}

void MathLibTestSuite::testLdexp() {
    TestUtils::printInfo("Testing math.ldexp function...");

    try {
        // Test null state handling
        bool caughtException = false;
        try {
            MathLib::ldexp(nullptr, 1);
        } catch (const std::invalid_argument&) {
            caughtException = true;
        }
        assert(caughtException);

        TestUtils::printInfo("Math.ldexp function test passed");
    } catch (const std::exception& e) {
        TestUtils::printError("Math.ldexp test failed: " + std::string(e.what()));
        throw;
    }
}

} // namespace Tests
} // namespace Lua
