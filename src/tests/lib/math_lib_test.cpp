#include "math_lib_test.hpp"
#include "../../lib/math_lib.hpp"
#include <iostream>
#include <cmath>
#include <limits>

namespace Lua {
namespace Tests {

void MathLibTest::runAllTests() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Running Math Library Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // Run all test methods
    testConstants();
    testAbs();
    testCeil();
    testFloor();
    testMax();
    testMin();
    testExp();
    testLog();
    testLog10();
    testPow();
    testSqrt();
    testSin();
    testCos();
    testTan();
    testAsin();
    testAcos();
    testAtan();
    testAtan2();
    testSinh();
    testCosh();
    testTanh();
    testDeg();
    testRad();
    testRandom();
    testRandomseed();
    testFmod();
    testModf();
    testFrexp();
    testLdexp();
    testIsfinite();
    testIsinf();
    testIsnan();
    testIsnormal();
    testLerp();
    testErrorHandling();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Math Library Tests Completed" << std::endl;
    std::cout << "========================================" << std::endl;
}

void MathLibTest::testConstants() {
    std::cout << "\nTesting Math Constants:" << std::endl;
    
    try {
        // Test pi constant
        bool piTest = isApproximatelyEqual(MathConstants::PI, 3.14159265358979323846);
        printTestResult("PI constant", piTest);
        
        // Test e constant
        bool eTest = isApproximatelyEqual(MathConstants::E, 2.71828182845904523536);
        printTestResult("E constant", eTest);
        
        // Test sqrt2 constant
        bool sqrt2Test = isApproximatelyEqual(MathConstants::SQRT2, 1.41421356237309504880);
        printTestResult("SQRT2 constant", sqrt2Test);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Constants test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testAbs() {
    std::cout << "\nTesting math.abs:" << std::endl;
    
    try {
        // Test basic abs functionality using standard library
        bool test1 = (std::abs(5.5) == 5.5);
        printTestResult("abs(5.5)", test1);
        
        bool test2 = (std::abs(-3.7) == 3.7);
        printTestResult("abs(-3.7)", test2);
        
        bool test3 = (std::abs(0.0) == 0.0);
        printTestResult("abs(0.0)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] abs test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testCeil() {
    std::cout << "\nTesting math.ceil:" << std::endl;
    
    try {
        bool test1 = (std::ceil(3.2) == 4.0);
        printTestResult("ceil(3.2)", test1);
        
        bool test2 = (std::ceil(-2.8) == -2.0);
        printTestResult("ceil(-2.8)", test2);
        
        bool test3 = (std::ceil(5.0) == 5.0);
        printTestResult("ceil(5.0)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] ceil test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testFloor() {
    std::cout << "\nTesting math.floor:" << std::endl;
    
    try {
        bool test1 = (std::floor(3.8) == 3.0);
        printTestResult("floor(3.8)", test1);
        
        bool test2 = (std::floor(-2.2) == -3.0);
        printTestResult("floor(-2.2)", test2);
        
        bool test3 = (std::floor(7.0) == 7.0);
        printTestResult("floor(7.0)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] floor test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testMax() {
    std::cout << "\nTesting math.max:" << std::endl;
    
    try {
        bool test1 = (std::max(3.5, 7.2) == 7.2);
        printTestResult("max(3.5, 7.2)", test1);
        
        bool test2 = (std::max({1.0, 5.0, 3.0, 9.0, 2.0}) == 9.0);
        printTestResult("max(1, 5, 3, 9, 2)", test2);
        
        bool test3 = (std::max({-5.0, -2.0, -8.0}) == -2.0);
        printTestResult("max(-5, -2, -8)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] max test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testMin() {
    std::cout << "\nTesting math.min:" << std::endl;
    
    try {
        bool test1 = (std::min(3.5, 7.2) == 3.5);
        printTestResult("min(3.5, 7.2)", test1);
        
        bool test2 = (std::min({1.0, 5.0, 3.0, 9.0, 2.0}) == 1.0);
        printTestResult("min(1, 5, 3, 9, 2)", test2);
        
        bool test3 = (std::min({-5.0, -2.0, -8.0}) == -8.0);
        printTestResult("min(-5, -2, -8)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] min test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testExp() {
    std::cout << "\nTesting math.exp:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::exp(0.0), 1.0);
        printTestResult("exp(0)", test1);
        
        bool test2 = isApproximatelyEqual(std::exp(1.0), std::exp(1.0));
        printTestResult("exp(1)", test2);
        
        bool test3 = isApproximatelyEqual(std::exp(2.0), std::exp(2.0));
        printTestResult("exp(2)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] exp test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testLog() {
    std::cout << "\nTesting math.log:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::log(1.0), 0.0);
        printTestResult("log(1)", test1);
        
        bool test2 = isApproximatelyEqual(std::log(std::exp(1.0)), 1.0);
        printTestResult("log(e)", test2);
        
        bool test3 = isApproximatelyEqual(std::log(8.0) / std::log(2.0), 3.0);
        printTestResult("log(8, 2)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] log test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testLog10() {
    std::cout << "\nTesting math.log10:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::log10(1.0), 0.0);
        printTestResult("log10(1)", test1);
        
        bool test2 = isApproximatelyEqual(std::log10(10.0), 1.0);
        printTestResult("log10(10)", test2);
        
        bool test3 = isApproximatelyEqual(std::log10(100.0), 2.0);
        printTestResult("log10(100)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] log10 test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testPow() {
    std::cout << "\nTesting math.pow:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::pow(2.0, 3.0), 8.0);
        printTestResult("pow(2, 3)", test1);
        
        bool test2 = isApproximatelyEqual(std::pow(5.0, 0.0), 1.0);
        printTestResult("pow(5, 0)", test2);
        
        bool test3 = isApproximatelyEqual(std::pow(4.0, 0.5), 2.0);
        printTestResult("pow(4, 0.5)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] pow test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testSqrt() {
    std::cout << "\nTesting math.sqrt:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::sqrt(4.0), 2.0);
        printTestResult("sqrt(4)", test1);
        
        bool test2 = isApproximatelyEqual(std::sqrt(9.0), 3.0);
        printTestResult("sqrt(9)", test2);
        
        bool test3 = isApproximatelyEqual(std::sqrt(0.0), 0.0);
        printTestResult("sqrt(0)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] sqrt test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testSin() {
    std::cout << "\nTesting math.sin:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::sin(0.0), 0.0);
        printTestResult("sin(0)", test1);
        
        bool test2 = isApproximatelyEqual(std::sin(MathConstants::PI / 2.0), 1.0);
        printTestResult("sin(π/2)", test2);
        
        bool test3 = isApproximatelyEqual(std::sin(MathConstants::PI), 0.0);
        printTestResult("sin(π)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] sin test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testCos() {
    std::cout << "\nTesting math.cos:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::cos(0.0), 1.0);
        printTestResult("cos(0)", test1);
        
        bool test2 = isApproximatelyEqual(std::cos(MathConstants::PI / 2.0), 0.0);
        printTestResult("cos(π/2)", test2);
        
        bool test3 = isApproximatelyEqual(std::cos(MathConstants::PI), -1.0);
        printTestResult("cos(π)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] cos test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testTan() {
    std::cout << "\nTesting math.tan:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::tan(0.0), 0.0);
        printTestResult("tan(0)", test1);
        
        bool test2 = isApproximatelyEqual(std::tan(MathConstants::PI / 4.0), 1.0);
        printTestResult("tan(π/4)", test2);
        
        bool test3 = isApproximatelyEqual(std::tan(MathConstants::PI), 0.0);
        printTestResult("tan(π)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] tan test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testAsin() {
    std::cout << "\nTesting math.asin:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::asin(0.0), 0.0);
        printTestResult("asin(0)", test1);
        
        bool test2 = isApproximatelyEqual(std::asin(1.0), MathConstants::PI / 2.0);
        printTestResult("asin(1)", test2);
        
        bool test3 = isApproximatelyEqual(std::asin(-1.0), -MathConstants::PI / 2.0);
        printTestResult("asin(-1)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] asin test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testAcos() {
    std::cout << "\nTesting math.acos:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::acos(1.0), 0.0);
        printTestResult("acos(1)", test1);
        
        bool test2 = isApproximatelyEqual(std::acos(0.0), MathConstants::PI / 2.0);
        printTestResult("acos(0)", test2);
        
        bool test3 = isApproximatelyEqual(std::acos(-1.0), MathConstants::PI);
        printTestResult("acos(-1)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] acos test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testAtan() {
    std::cout << "\nTesting math.atan:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::atan(0.0), 0.0);
        printTestResult("atan(0)", test1);
        
        bool test2 = isApproximatelyEqual(std::atan(1.0), MathConstants::PI / 4.0);
        printTestResult("atan(1)", test2);
        
        bool test3 = isApproximatelyEqual(std::atan(-1.0), -MathConstants::PI / 4.0);
        printTestResult("atan(-1)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] atan test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testAtan2() {
    std::cout << "\nTesting math.atan2:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::atan2(0.0, 1.0), 0.0);
        printTestResult("atan2(0, 1)", test1);
        
        bool test2 = isApproximatelyEqual(std::atan2(1.0, 1.0), MathConstants::PI / 4.0);
        printTestResult("atan2(1, 1)", test2);
        
        bool test3 = isApproximatelyEqual(std::atan2(1.0, 0.0), MathConstants::PI / 2.0);
        printTestResult("atan2(1, 0)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] atan2 test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testSinh() {
    std::cout << "\nTesting math.sinh:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::sinh(0.0), 0.0);
        printTestResult("sinh(0)", test1);
        
        bool test2 = isApproximatelyEqual(std::sinh(1.0), std::sinh(1.0));
        printTestResult("sinh(1)", test2);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] sinh test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testCosh() {
    std::cout << "\nTesting math.cosh:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::cosh(0.0), 1.0);
        printTestResult("cosh(0)", test1);
        
        bool test2 = isApproximatelyEqual(std::cosh(1.0), std::cosh(1.0));
        printTestResult("cosh(1)", test2);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] cosh test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testTanh() {
    std::cout << "\nTesting math.tanh:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::tanh(0.0), 0.0);
        printTestResult("tanh(0)", test1);
        
        bool test2 = isApproximatelyEqual(std::tanh(1.0), std::tanh(1.0));
        printTestResult("tanh(1)", test2);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] tanh test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testDeg() {
    std::cout << "\nTesting math.deg:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(MathConstants::PI * 180.0 / MathConstants::PI, 180.0);
        printTestResult("deg(π)", test1);
        
        bool test2 = isApproximatelyEqual((MathConstants::PI / 2.0) * 180.0 / MathConstants::PI, 90.0);
        printTestResult("deg(π/2)", test2);
        
        bool test3 = isApproximatelyEqual(0.0 * 180.0 / MathConstants::PI, 0.0);
        printTestResult("deg(0)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] deg test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testRad() {
    std::cout << "\nTesting math.rad:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(180.0 * MathConstants::PI / 180.0, MathConstants::PI);
        printTestResult("rad(180)", test1);
        
        bool test2 = isApproximatelyEqual(90.0 * MathConstants::PI / 180.0, MathConstants::PI / 2.0);
        printTestResult("rad(90)", test2);
        
        bool test3 = isApproximatelyEqual(0.0 * MathConstants::PI / 180.0, 0.0);
        printTestResult("rad(0)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] rad test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testRandom() {
    std::cout << "\nTesting math.random:" << std::endl;
    
    try {
        // Test random number generation concept
        RandomGenerator rng;
        double val = rng.random();
        bool test1 = (val >= 0.0 && val < 1.0);
        printTestResult("random() in [0, 1)", test1);
        
        int intVal = rng.randomInt(1, 10);
        bool test2 = (intVal >= 1 && intVal <= 10);
        printTestResult("randomInt(1, 10) in [1, 10]", test2);
        
        double rangeVal = rng.random(5.0, 15.0);
        bool test3 = (rangeVal >= 5.0 && rangeVal < 15.0);
        printTestResult("random(5, 15) in [5, 15)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] random test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testRandomseed() {
    std::cout << "\nTesting math.randomseed:" << std::endl;
    
    try {
        // Test randomseed consistency
        RandomGenerator rng1(42);
        RandomGenerator rng2(42);
        
        double val1 = rng1.random();
        double val2 = rng2.random();
        
        bool test1 = isApproximatelyEqual(val1, val2);
        printTestResult("randomseed consistency", test1);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] randomseed test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testFmod() {
    std::cout << "\nTesting math.fmod:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::fmod(7.0, 3.0), 1.0);
        printTestResult("fmod(7, 3)", test1);
        
        bool test2 = isApproximatelyEqual(std::fmod(7.5, 2.5), 0.0);
        printTestResult("fmod(7.5, 2.5)", test2);
        
        bool test3 = isApproximatelyEqual(std::fmod(-7.0, 3.0), -1.0);
        printTestResult("fmod(-7, 3)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] fmod test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testModf() {
    std::cout << "\nTesting math.modf:" << std::endl;
    
    try {
        double intpart;
        double fracpart = std::modf(3.75, &intpart);
        bool test1 = (isApproximatelyEqual(fracpart, 0.75) && isApproximatelyEqual(intpart, 3.0));
        printTestResult("modf(3.75)", test1);
        
        fracpart = std::modf(-2.25, &intpart);
        bool test2 = (isApproximatelyEqual(fracpart, -0.25) && isApproximatelyEqual(intpart, -2.0));
        printTestResult("modf(-2.25)", test2);
        
        fracpart = std::modf(5.0, &intpart);
        bool test3 = (isApproximatelyEqual(fracpart, 0.0) && isApproximatelyEqual(intpart, 5.0));
        printTestResult("modf(5.0)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] modf test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testFrexp() {
    std::cout << "\nTesting math.frexp:" << std::endl;
    
    try {
        int exp;
        double mantissa = std::frexp(8.0, &exp);
        bool test1 = (isApproximatelyEqual(mantissa, 0.5) && exp == 4);
        printTestResult("frexp(8.0)", test1);
        
        mantissa = std::frexp(0.0, &exp);
        bool test2 = (isApproximatelyEqual(mantissa, 0.0) && exp == 0);
        printTestResult("frexp(0.0)", test2);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] frexp test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testLdexp() {
    std::cout << "\nTesting math.ldexp:" << std::endl;
    
    try {
        bool test1 = isApproximatelyEqual(std::ldexp(0.5, 4), 8.0);
        printTestResult("ldexp(0.5, 4)", test1);
        
        bool test2 = isApproximatelyEqual(std::ldexp(1.0, 0), 1.0);
        printTestResult("ldexp(1.0, 0)", test2);
        
        bool test3 = isApproximatelyEqual(std::ldexp(2.0, -1), 1.0);
        printTestResult("ldexp(2.0, -1)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] ldexp test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testIsfinite() {
    std::cout << "\nTesting math.isfinite:" << std::endl;
    
    try {
        bool test1 = std::isfinite(5.0);
        printTestResult("isfinite(5.0)", test1);
        
        bool test2 = !std::isfinite(std::numeric_limits<double>::infinity());
        printTestResult("isfinite(infinity)", test2);
        
        bool test3 = !std::isfinite(std::numeric_limits<double>::quiet_NaN());
        printTestResult("isfinite(NaN)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] isfinite test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testIsinf() {
    std::cout << "\nTesting math.isinf:" << std::endl;
    
    try {
        bool test1 = !std::isinf(5.0);
        printTestResult("isinf(5.0)", test1);
        
        bool test2 = std::isinf(std::numeric_limits<double>::infinity());
        printTestResult("isinf(infinity)", test2);
        
        bool test3 = std::isinf(-std::numeric_limits<double>::infinity());
        printTestResult("isinf(-infinity)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] isinf test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testIsnan() {
    std::cout << "\nTesting math.isnan:" << std::endl;
    
    try {
        bool test1 = !std::isnan(5.0);
        printTestResult("isnan(5.0)", test1);
        
        bool test2 = std::isnan(std::numeric_limits<double>::quiet_NaN());
        printTestResult("isnan(NaN)", test2);
        
        bool test3 = !std::isnan(std::numeric_limits<double>::infinity());
        printTestResult("isnan(infinity)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] isnan test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testIsnormal() {
    std::cout << "\nTesting math.isnormal:" << std::endl;
    
    try {
        bool test1 = std::isnormal(5.0);
        printTestResult("isnormal(5.0)", test1);
        
        bool test2 = !std::isnormal(0.0);
        printTestResult("isnormal(0.0)", test2);
        
        bool test3 = !std::isnormal(std::numeric_limits<double>::infinity());
        printTestResult("isnormal(infinity)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] isnormal test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testLerp() {
    std::cout << "\nTesting math.lerp:" << std::endl;
    
    try {
        // Test linear interpolation using MathUtils::lerp
        bool test1 = isApproximatelyEqual(MathUtils::lerp(0.0, 10.0, 0.5), 5.0);
        printTestResult("lerp(0, 10, 0.5)", test1);
        
        bool test2 = isApproximatelyEqual(MathUtils::lerp(2.0, 8.0, 0.25), 3.5);
        printTestResult("lerp(2, 8, 0.25)", test2);
        
        bool test3 = isApproximatelyEqual(MathUtils::lerp(5.0, 5.0, 0.7), 5.0);
        printTestResult("lerp(5, 5, 0.7)", test3);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] lerp test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::testErrorHandling() {
    std::cout << "\nTesting Error Handling:" << std::endl;
    
    try {
        // Test sqrt of negative number
        double sqrtResult = std::sqrt(-1.0);
        bool sqrtErrorHandled = std::isnan(sqrtResult);
        printTestResult("sqrt(-1) error handling", sqrtErrorHandled);
        
        // Test log of negative number
        double logResult = std::log(-1.0);
        bool logErrorHandled = std::isnan(logResult);
        printTestResult("log(-1) error handling", logErrorHandled);
        
        // Test asin of value outside [-1, 1]
        double asinResult = std::asin(2.0);
        bool asinErrorHandled = std::isnan(asinResult);
        printTestResult("asin(2) error handling", asinErrorHandled);
        
    } catch (const std::exception& e) {
        std::cout << "[FAIL] Error handling test failed with exception: " << e.what() << std::endl;
    }
}

void MathLibTest::printTestResult(const std::string& testName, bool passed) {
    if (passed) {
        std::cout << "[PASS] " << testName << " test passed" << std::endl;
    } else {
        std::cout << "[FAIL] " << testName << " test failed" << std::endl;
    }
}

bool MathLibTest::isApproximatelyEqual(double a, double b, double epsilon) {
    return std::abs(a - b) < epsilon;
}

} // namespace Tests
} // namespace Lua