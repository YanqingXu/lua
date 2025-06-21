#include "compiler_multi_return_test.hpp"

namespace Lua::Tests {

void CompilerMultiReturnTest::runAllTests() {
    RUN_TEST(CompilerMultiReturnTest, testSingleReturnCompilation);
    RUN_TEST(CompilerMultiReturnTest, testMultipleReturnCompilation);
    RUN_TEST(CompilerMultiReturnTest, testEmptyReturnCompilation);
    RUN_TEST(CompilerMultiReturnTest, testComplexReturnCompilation);
}

void CompilerMultiReturnTest::testSingleReturnCompilation() {
    const Str code = "return 42;";
    testReturnCompilation(code, "Single return value");
}

void CompilerMultiReturnTest::testMultipleReturnCompilation() {
    const Str code1 = "return 1, 2;";
    testReturnCompilation(code1, "Two return values");
    
    const Str code2 = "return 1, 2, 3;";
    testReturnCompilation(code2, "Three return values");
    
    const Str code3 = "return a, b, c, d;";
    testReturnCompilation(code3, "Four variable returns");
}

void CompilerMultiReturnTest::testEmptyReturnCompilation() {
    const Str code = "return;";
    testReturnCompilation(code, "Empty return");
}

void CompilerMultiReturnTest::testComplexReturnCompilation() {
    const Str code1 = "return a + b, c * d;";
    testReturnCompilation(code1, "Arithmetic expressions");
    
    const Str code2 = "return func(), var, 42;";
    testReturnCompilation(code2, "Mixed expressions");
}

void CompilerMultiReturnTest::testReturnCompilation(const Str& code, const Str& description) {
    try {
        TestUtils::printInfo("Testing: " + description + " - Code: " + code);
        
        // Parsing
        Parser parser(code);
        auto ast = parser.parse();
        
        if (ast.empty()) {
            TestUtils::printError("Failed to parse code");
            return;
        }
        
        // Compilation
        Compiler compiler;
        auto function = compiler.compile(ast);
        
        if (!function) {
            TestUtils::printError("Failed to compile code");
            return;
        }
        
        auto& bytecode = function->getCode();
        TestUtils::printInfo("Generated " + std::to_string(bytecode.size()) + " instructions");
        
        // Print generated instructions for debugging
        for (size_t i = 0; i < bytecode.size(); ++i) {
            auto instr = bytecode[i];
            TestUtils::printInfo("  [" + std::to_string(i) + "] OpCode: " + std::to_string(static_cast<int>(instr.getOpCode()))
                      + ", A: " + std::to_string(static_cast<int>(instr.getA()))
                      + ", B: " + std::to_string(static_cast<int>(instr.getB()))
                      + ", C: " + std::to_string(static_cast<int>(instr.getC())));
        }
        
    } catch (const std::exception& e) {
        TestUtils::printError(std::string("Exception: ") + e.what());
    }
}

} // namespace Lua::Tests