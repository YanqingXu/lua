/**
 * @file test_lua_functions.cpp
 * @brief 测试Lua函数文件的编译
 */

#include "../framework/test_framework.hpp"
#include "compiler/lexer/lexer.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/opcode.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include <fstream>
#include <sstream>

using namespace Lua;
using namespace LuaTest;

std::string readFile(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error(std::string("Cannot open file: ") + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void testLuaFunctionFile(TestSuite& suite) {
    std::string code = readFile("tests/lua/functions/test_functions.lua");
    ASSERT_TRUE(suite, !code.empty(), "File loaded");

    RuntimeServices services = RuntimeServices::fromSingletons();
    Parser parser(code.c_str());
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    Chunk chunk = std::move(*parsed);

    CodeGenerator codegen(services);
    Proto* proto = codegen.generate(chunk);

    ASSERT_TRUE(suite, proto != nullptr, "Proto generated");
    ASSERT_TRUE(suite, proto->getInstructionCount() > 0, "Has instructions");
    ASSERT_TRUE(suite, proto->getConstantCount() > 0, "Fixture produces constants");
    ASSERT_EQ(suite, static_cast<usize>(5), proto->getSubProtoCount(), "Fixture produces five top-level functions");
}

void registerLuaFunctionTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("Lua File Compilation", "test_functions.lua", testLuaFunctionFile);
}
