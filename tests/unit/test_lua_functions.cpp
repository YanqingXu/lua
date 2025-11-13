/**
 * @file test_lua_functions.cpp
 * @brief 测试Lua函数文件的编译
 */

#include "compiler/lexer.hpp"
#include "compiler/parser.hpp"
#include "compiler/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

using namespace Lua;

std::string readFile(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Cannot open file: ") + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Lua Functions File Compilation Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        // 读取Lua文件
        std::string code = readFile("tests/lua/test_functions.lua");
        std::cout << "Loaded " << code.size() << " bytes from test_functions.lua" << std::endl;
        
        // 编译
        StringPool pool;
        Lexer lexer(code.c_str());
        Parser parser(&lexer, &pool);
        auto chunk = parser.parse();
        
        std::cout << "Parsing completed successfully" << std::endl;
        
        CodeGenerator codegen(&pool);
        auto proto = codegen.generate(chunk.get());
        
        std::cout << "Code generation completed successfully" << std::endl;
        std::cout << "Main proto:" << std::endl;
        std::cout << "  Instructions: " << proto->getInstructionCount() << std::endl;
        std::cout << "  Constants: " << proto->getConstantCount() << std::endl;
        std::cout << "  Sub-functions: " << proto->getSubProtoCount() << std::endl;
        
        // 打印子函数信息
        for (usize i = 0; i < proto->getSubProtoCount(); i++) {
            Proto* subProto = proto->getSubProto(i);
            std::cout << "  Sub-function " << i << ":" << std::endl;
            std::cout << "    Params: " << (int)subProto->getNumParams() << std::endl;
            std::cout << "    Vararg: " << (subProto->isVararg() ? "yes" : "no") << std::endl;
            std::cout << "    Instructions: " << subProto->getInstructionCount() << std::endl;
        }
        
        // 打印一些指令
        std::cout << "\nFirst 10 instructions:" << std::endl;
        for (usize i = 0; i < std::min(proto->getInstructionCount(), (usize)10); i++) {
            Instruction inst = proto->getInstruction(i);
            OpCode op = GET_OPCODE(inst);
            std::cout << "  [" << i << "] " << getOpCodeName(op);
            std::cout << " A=" << GETARG_A(inst);
            if (op == OpCode::LOADK || op == OpCode::GETGLOBAL || op == OpCode::SETGLOBAL || op == OpCode::CLOSURE) {
                std::cout << " Bx=" << GETARG_Bx(inst);
            } else {
                std::cout << " B=" << GETARG_B(inst) << " C=" << GETARG_C(inst);
            }
            std::cout << std::endl;
        }
        
        std::cout << "\n[SUCCESS] All tests passed!" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "\n[ERROR] Test failed: " << e.what() << std::endl;
        return 1;
    }
}

