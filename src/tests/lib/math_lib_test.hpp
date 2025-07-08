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
 * - Trigonometric: sin, cos, tan, asin, acos, atan, atan2
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
        RUN_TEST_GROUP("Basic Math Functions", runBasicMathTests);
        RUN_TEST_GROUP("Power Functions", runPowerTests);
        RUN_TEST_GROUP("Trigonometric Functions", runTrigTests);
        RUN_TEST_GROUP("Angle Conversion", runAngleTests);
        RUN_TEST_GROUP("Random Functions", runRandomTests);
        RUN_TEST_GROUP("Math Utility Functions", runMathUtilityTests);
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
