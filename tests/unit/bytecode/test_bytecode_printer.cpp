/**
 * @file test_bytecode_printer.cpp
 * @brief Output contract tests for the lua_bytecode printer.
 */

#include "../framework/test_framework.hpp"
#include "bytecode/bytecode_printer.hpp"
#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/string_pool.hpp"

#include <sstream>
#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Bytecode Printer";

bool contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

void addInstruction(Proto& proto, i32 line, Instruction instruction) {
    proto.addInstruction(instruction);
    proto.addLineInfo(line);
}

void testPrinterShowsProtoMetadataAndConstants(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    Proto proto;

    proto.setSource(pool.intern("sample.lua"));
    proto.setNumParams(2);
    proto.setVararg(true);
    proto.setMaxStackSize(7);
    proto.addUpvalueName(pool.intern("outer"));

    i32 stringIndex = static_cast<i32>(proto.addConstant(Value(pool.intern("answer"))));
    i32 numberIndex = static_cast<i32>(proto.addConstant(Value(42.0)));
    i32 boolIndex = static_cast<i32>(proto.addConstant(Value(true)));
    i32 nilIndex = static_cast<i32>(proto.addConstant(Value()));

    addInstruction(proto, 10, CREATE_ABx(OpCode::LOADK, 0, numberIndex));
    addInstruction(proto, 11, CREATE_ABx(OpCode::GETGLOBAL, 1, stringIndex));
    addInstruction(proto, 12, CREATE_ABC(OpCode::ADD, 2, RKASK(numberIndex), RKASK(numberIndex)));
    addInstruction(proto, 13, CREATE_AsBx(OpCode::JMP, 0, 2));

    std::ostringstream output;
    printProtoBytecode(&proto, output, false);
    std::string text = output.str();

    ASSERT_TRUE(suite, contains(text, "source: sample.lua"), "source metadata is printed");
    ASSERT_TRUE(suite, contains(text, "numparams: 2"), "numparams metadata is printed");
    ASSERT_TRUE(suite, contains(text, "is_vararg: true"), "vararg metadata is printed");
    ASSERT_TRUE(suite, contains(text, "maxStackSize: 7"), "max stack metadata is printed");
    ASSERT_TRUE(suite, contains(text, "upvalues (1): outer"), "upvalue names are printed");
    ASSERT_TRUE(suite, contains(text, "constants (4)"), "constant table header is printed");
    ASSERT_TRUE(suite, contains(text, "K[0] = string \"answer\""), "string constant is printed");
    ASSERT_TRUE(suite, contains(text, "K[1] = number 42"), "number constant is printed");
    ASSERT_TRUE(suite, contains(text, "K[2] = boolean true"), "boolean constant is printed");
    ASSERT_TRUE(suite, contains(text, "K[3] = nil"), "nil constant is printed");
    ASSERT_TRUE(suite,
                contains(text, "0000 | line 10 | LOADK | A=0 Bx=1 ; K[1] = number 42"),
                "LOADK instruction shows decoded constant");
    ASSERT_TRUE(suite,
                contains(text, "0001 | line 11 | GETGLOBAL | A=1 Bx=0 ; K[0] = string \"answer\""),
                "GETGLOBAL instruction shows decoded constant");
    ASSERT_TRUE(suite,
                contains(text, "0002 | line 12 | ADD | A=2 B=257 C=257 ; B=K[1] = number 42; C=K[1] = number 42"),
                "RK operands show decoded constants");
    ASSERT_TRUE(suite, contains(text, "0003 | line 13 | JMP | A=0 sBx=2 ; target=6"),
                "JMP instruction shows absolute target");

    (void)boolIndex;
    (void)nilIndex;
}

} // namespace

void registerBytecodePrinterTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "Proto Metadata And Constants",
                          testPrinterShowsProtoMetadataAndConstants);
}
