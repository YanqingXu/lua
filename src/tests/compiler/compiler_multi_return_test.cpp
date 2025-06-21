#include "../test_utils.hpp"
#include "../../compiler/compiler.hpp"
#include "../../parser/parser.hpp"
#include "../../lexer/lexer.hpp"
#include "../../vm/vm.hpp"
#include "../../vm/state_factory.hpp"
#include <iostream>
#include <cassert>

namespace Lua {
    class CompilerMultiReturnTest {
    public:
        static void runAllTests() {
            std::cout << "\n=== Multi Return Value Compilation Tests ===\n";
            testSingleReturnCompilation();
            testMultipleReturnCompilation();
            testEmptyReturnCompilation();
            testComplexReturnCompilation();
            std::cout << "Multi Return Value Compilation Tests completed!\n";
        }

    private:
        static void testSingleReturnCompilation() {
            std::cout << "Testing single return value compilation...\n";
            
            const Str code = "return 42;";
            testReturnCompilation(code, "Single return value");
        }

        static void testMultipleReturnCompilation() {
            std::cout << "Testing multiple return value compilation...\n";
            
            const Str code1 = "return 1, 2;";
            testReturnCompilation(code1, "Two return values");
            
            const Str code2 = "return 1, 2, 3;";
            testReturnCompilation(code2, "Three return values");
            
            const Str code3 = "return a, b, c, d;";
            testReturnCompilation(code3, "Four variable returns");
        }

        static void testEmptyReturnCompilation() {
            std::cout << "Testing empty return compilation...\n";
            
            const Str code = "return;";
            testReturnCompilation(code, "Empty return");
        }

        static void testComplexReturnCompilation() {
            std::cout << "Testing complex return expressions compilation...\n";
            
            const Str code1 = "return a + b, c * d;";
            testReturnCompilation(code1, "Arithmetic expressions");
            
            const Str code2 = "return func(), var, 42;";
            testReturnCompilation(code2, "Mixed expressions");
        }

        static void testReturnCompilation(const Str& code, const Str& description) {
            try {
                std::cout << "  Testing: " << description << " - Code: " << code << std::endl;
                
                // Lexical analysis
                Lexer lexer(code);
                auto tokens = lexer.tokenize();
                
                // Parsing
                Parser parser(tokens);
                auto ast = parser.parse();
                
                if (!ast) {
                    std::cerr << "    ERROR: Failed to parse code" << std::endl;
                    return;
                }
                
                // Compilation
                Compiler compiler;
                auto bytecode = compiler.compile(ast.get());
                
                if (bytecode.empty()) {
                    std::cerr << "    ERROR: Failed to compile code" << std::endl;
                    return;
                }
                
                std::cout << "    SUCCESS: Generated " << bytecode.size() << " instructions" << std::endl;
                
                // Print generated instructions for debugging
                for (size_t i = 0; i < bytecode.size(); ++i) {
                    auto instr = bytecode[i];
                    std::cout << "      [" << i << "] OpCode: " << static_cast<int>(instr.getOpCode())
                              << ", A: " << static_cast<int>(instr.getA())
                              << ", B: " << static_cast<int>(instr.getB())
                              << ", C: " << static_cast<int>(instr.getC()) << std::endl;
                }
                
            } catch (const std::exception& e) {
                std::cerr << "    ERROR: " << e.what() << std::endl;
            }
        }
    };
}