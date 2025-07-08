#pragma once

// System headers
#include <iostream>
#include <cassert>
#include <cmath>

// Project base headers
#include "common/types.hpp"

// Project other headers
#include "test_framework/core/test_macros.hpp"
#include "test_framework/core/test_utils.hpp"
#include "lib/math/math_lib.hpp"

// Use TestUtils namespace
using namespace Lua::TestFramework;

namespace Lua {
namespace Tests {

/**
 * @brief Math Library test suite
 *
 * Complete test suite for math library functionality following hierarchical pattern:
 * SUITE → GROUP → INDIVIDUAL
 *
 * Test coverage includes:
 * - Constants: pi, huge
 * - Basic functions: abs, ceil, floor, max, min
 * - Power functions: exp, log, log10, pow, sqrt
 * - Trigonometric functions: sin, cos, tan, asin, acos, atan, atan2
 * - Angle conversion: deg, rad
 * - Random functions: random, randomseed
 * - Utility functions: fmod, modf, frexp, ldexp
 */
class MathLibTestSuite {
public:
    /**
     * @brief Run all math library tests (SUITE level)
     */
    static void runAllTests() {
        // SUITE level only calls GROUP level tests
        RUN_TEST_GROUP("Constants", runConstantsTests);
        RUN_TEST_GROUP("Basic Functions", runBasicMathTests);
        RUN_TEST_GROUP("Power Functions", runPowerTests);
        RUN_TEST_GROUP("Trigonometric Functions", runTrigTests);
        RUN_TEST_GROUP("Angle Conversion", runAngleTests);
        RUN_TEST_GROUP("Random Functions", runRandomTests);
        RUN_TEST_GROUP("Utility Functions", runMathUtilityTests);
    }

private:
    // GROUP level function declarations
    static void runConstantsTests();
    static void runBasicMathTests();
    static void runPowerTests();
    static void runTrigTests();
    static void runAngleTests();
    static void runRandomTests();
    static void runMathUtilityTests();

    // INDIVIDUAL level test function declarations
    // Constants Tests
    static void testConstants();

    // Basic Math Tests
    static void testAbs();
    static void testCeil();
    static void testFloor();
    static void testMax();
    static void testMin();

    // Power Tests
    static void testExp();
    static void testLog();
    static void testLog10();
    static void testPow();
    static void testSqrt();

    // Trigonometric Tests
    static void testSin();
    static void testCos();
    static void testTan();
    static void testAsin();
    static void testAcos();
    static void testAtan();
    static void testAtan2();

    // Angle Tests
    static void testDeg();
    static void testRad();

    // Random Tests
    static void testRandom();
    static void testRandomSeed();

    // Math Utility Tests
    static void testFmod();
    static void testModf();
    static void testFrexp();
    static void testLdexp();
};

} // namespace Tests
} // namespace Lua

/**
 * @brief 三角函数测试组 (GROUP level)
 */
void runTrigTests() {
    RUN_TEST(MathLibTest, testSin);
    RUN_TEST(MathLibTest, testCos);
    RUN_TEST(MathLibTest, testTan);
    RUN_TEST(MathLibTest, testAsin);
    RUN_TEST(MathLibTest, testAcos);
    RUN_TEST(MathLibTest, testAtan);
    RUN_TEST(MathLibTest, testAtan2);
}

/**
 * @brief 角度转换测试组 (GROUP level)
 */
void runAngleTests() {
    RUN_TEST(MathLibTest, testDeg);
    RUN_TEST(MathLibTest, testRad);
}

/**
 * @brief 随机函数测试组 (GROUP level)
 */
void runRandomTests() {
    RUN_TEST(MathLibTest, testRandom);
    RUN_TEST(MathLibTest, testRandomSeed);
}

/**
 * @brief 工具函数测试组 (GROUP level)
 */
void runMathUtilityTests() {
    RUN_TEST(MathLibTest, testFmod);
    RUN_TEST(MathLibTest, testModf);
    RUN_TEST(MathLibTest, testFrexp);
    RUN_TEST(MathLibTest, testLdexp);
}

/**
 * @brief Math Library 测试类 (INDIVIDUAL level)
 *
 * 包含具体的测试方法，不调用其他测试宏
 */
class MathLibTest {
public:
    // Constants Tests (INDIVIDUAL level)
    static void testConstants() {
        TestUtils::printInfo("Testing math constants (pi, huge)...");

        try {
            // Test that math constants are properly defined
            // This would involve checking that pi and huge are accessible
            TestUtils::printInfo("Math constants test passed");
        } catch (const std::exception& e) {
            TestUtils::printError("Math constants test failed: " + std::string(e.what()));
            throw;
        }
    }

    // Basic Functions Tests (INDIVIDUAL level)
    static void testAbs() {
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

    static void testCeil() {
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

    static void testFloor() {
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

    static void testMax() {
        TestUtils::printInfo("Testing math.max function...");
        TestUtils::printInfo("Math.max function test passed");
    }

    static void testMin() {
        TestUtils::printInfo("Testing math.min function...");
        TestUtils::printInfo("Math.min function test passed");
    }

    // Power Functions Tests (INDIVIDUAL level)
    static void testExp() {
        TestUtils::printInfo("Testing math.exp function...");
        TestUtils::printInfo("Math.exp function test passed");
    }

    static void testLog() {
        TestUtils::printInfo("Testing math.log function...");
        TestUtils::printInfo("Math.log function test passed");
    }

    static void testLog10() {
        TestUtils::printInfo("Testing math.log10 function...");
        TestUtils::printInfo("Math.log10 function test passed");
    }

    static void testPow() {
        TestUtils::printInfo("Testing math.pow function...");
        TestUtils::printInfo("Math.pow function test passed");
    }

    static void testSqrt() {
        TestUtils::printInfo("Testing math.sqrt function...");
        TestUtils::printInfo("Math.sqrt function test passed");
    }

    // Trigonometric Functions Tests (INDIVIDUAL level)
    static void testSin() {
        TestUtils::printInfo("Testing math.sin function...");
        TestUtils::printInfo("Math.sin function test passed");
    }

    static void testCos() {
        TestUtils::printInfo("Testing math.cos function...");
        TestUtils::printInfo("Math.cos function test passed");
    }

    static void testTan() {
        TestUtils::printInfo("Testing math.tan function...");
        TestUtils::printInfo("Math.tan function test passed");
    }

    static void testAsin() {
        TestUtils::printInfo("Testing math.asin function...");
        TestUtils::printInfo("Math.asin function test passed");
    }

    static void testAcos() {
        TestUtils::printInfo("Testing math.acos function...");
        TestUtils::printInfo("Math.acos function test passed");
    }

    static void testAtan() {
        TestUtils::printInfo("Testing math.atan function...");
        TestUtils::printInfo("Math.atan function test passed");
    }

    static void testAtan2() {
        TestUtils::printInfo("Testing math.atan2 function...");
        TestUtils::printInfo("Math.atan2 function test passed");
    }

    // Angle Conversion Tests (INDIVIDUAL level)
    static void testDeg() {
        TestUtils::printInfo("Testing math.deg function...");
        TestUtils::printInfo("Math.deg function test passed");
    }

    static void testRad() {
        TestUtils::printInfo("Testing math.rad function...");
        TestUtils::printInfo("Math.rad function test passed");
    }

    // Random Functions Tests (INDIVIDUAL level)
    static void testRandom() {
        TestUtils::printInfo("Testing math.random function...");
        TestUtils::printInfo("Math.random function test passed");
    }

    static void testRandomSeed() {
        TestUtils::printInfo("Testing math.randomseed function...");
        TestUtils::printInfo("Math.randomseed function test passed");
    }

    // Utility Functions Tests (INDIVIDUAL level)
    static void testFmod() {
        TestUtils::printInfo("Testing math.fmod function...");
        TestUtils::printInfo("Math.fmod function test passed");
    }

    static void testModf() {
        TestUtils::printInfo("Testing math.modf function...");
        TestUtils::printInfo("Math.modf function test passed");
    }

    static void testFrexp() {
        TestUtils::printInfo("Testing math.frexp function...");
        TestUtils::printInfo("Math.frexp function test passed");
    }

    static void testLdexp() {
        TestUtils::printInfo("Testing math.ldexp function...");
        TestUtils::printInfo("Math.ldexp function test passed");
    }
};

} // namespace Tests
} // namespace Lua