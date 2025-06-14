#ifndef MATH_LIB_TEST_HPP
#define MATH_LIB_TEST_HPP

#include "../../common/types.hpp"
#include <iostream>
#include <string>
#include <cmath>

namespace Lua {
namespace Tests {

    /**
     * @brief Math library test class
     * 
     * Tests all Lua math library functions, including:
     * - Constants: pi, huge, mininteger, maxinteger
     * - Basic functions: abs, ceil, floor, max, min
     * - Power functions: exp, log, log10, pow, sqrt
     * - Trigonometric functions: sin, cos, tan, asin, acos, atan, atan2
     * - Hyperbolic functions: sinh, cosh, tanh
     * - Angle conversion: deg, rad
     * - Random functions: random, randomseed
     * - Utility functions: fmod, modf, frexp, ldexp
     * - Check functions: isfinite, isinf, isnan, isnormal
     * - Interpolation: lerp
     */
    class MathLibTest {
    public:
        /**
         * @brief Run all tests
         * 
         * Execute all test cases in this test class
         */
        static void runAllTests();
        
    private:
        // Constants tests
        static void testConstants();
        
        // Basic math function tests
        static void testAbs();
        static void testCeil();
        static void testFloor();
        static void testMax();
        static void testMin();
        
        // Power function tests
        static void testExp();
        static void testLog();
        static void testLog10();
        static void testPow();
        static void testSqrt();
        
        // Trigonometric function tests
        static void testSin();
        static void testCos();
        static void testTan();
        static void testAsin();
        static void testAcos();
        static void testAtan();
        static void testAtan2();
        
        // Hyperbolic function tests
        static void testSinh();
        static void testCosh();
        static void testTanh();
        
        // Angle conversion tests
        static void testDeg();
        static void testRad();
        
        // Random function tests
        static void testRandom();
        static void testRandomseed();
        
        // Utility function tests
        static void testFmod();
        static void testModf();
        static void testFrexp();
        static void testLdexp();
        
        // Check function tests
        static void testIsfinite();
        static void testIsinf();
        static void testIsnan();
        static void testIsnormal();
        
        // Interpolation tests
        static void testLerp();
        
        // Error handling tests
        static void testErrorHandling();
        
        // Helper methods
        static void printTestResult(const std::string& testName, bool passed);
        static bool isApproximatelyEqual(double a, double b, double epsilon = 1e-9);
    };

} // namespace Tests
} // namespace Lua

#endif // MATH_LIB_TEST_HPP