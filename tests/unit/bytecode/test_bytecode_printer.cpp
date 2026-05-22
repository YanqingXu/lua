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
    addInstruction(proto, 13, CREATE_ABx(OpCode::SETGLOBAL, 2, stringIndex));
    addInstruction(proto, 14, CREATE_AsBx(OpCode::JMP, 0, 2));

    std::ostringstream output;
    printProtoBytecode(&proto, output, false);
    std::string text = output.str();

    ASSERT_TRUE(suite, contains(text, "source: sample.lua"), "source metadata is printed");
    ASSERT_TRUE(suite, contains(text, "numparams: 2"), "numparams metadata is printed");
    ASSERT_TRUE(suite, contains(text, "is_vararg: true"), "vararg metadata is printed");
    ASSERT_TRUE(suite, contains(text, "flags=2"), "raw vararg flags are printed");
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
                contains(text, "0003 | line 13 | SETGLOBAL | A=2 Bx=0 ; K[0] = string \"answer\""),
                "SETGLOBAL instruction shows decoded constant");
    ASSERT_TRUE(suite,
                contains(text, "0002 | line 12 | ADD | A=2 B=257 C=257 ; B=K[1] = number 42; C=K[1] = number 42"),
                "RK operands show decoded constants");
    ASSERT_TRUE(suite, contains(text, "0004 | line 14 | JMP | A=0 sBx=2 ; target=7"),
                "JMP instruction shows absolute target");

    (void)boolIndex;
    (void)nilIndex;
}

void testPrinterShowsAsBxTargetsForLoopOpcodes(TestSuite& suite) {
    Proto proto;

    addInstruction(proto, 30, CREATE_AsBx(OpCode::FORPREP, 0, 4));
    addInstruction(proto, 31, CREATE_AsBx(OpCode::FORLOOP, 0, -2));

    std::ostringstream output;
    printProtoBytecode(&proto, output, false);
    std::string text = output.str();

    ASSERT_TRUE(suite, contains(text, "0000 | line 30 | FORPREP | A=0 sBx=4 ; target=5"),
                "FORPREP instruction shows absolute target");
    ASSERT_TRUE(suite, contains(text, "0001 | line 31 | FORLOOP | A=0 sBx=-2 ; target=0"),
                "FORLOOP instruction shows absolute target");
}

void testPrinterKeepsEscapedStringsAndOutOfRangeConstants(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    Proto proto;

    proto.setSource(pool.intern("escape.lua"));
    proto.addConstant(Value(pool.intern("quote\" slash\\ line\n tab\t")));

    addInstruction(proto, 20, CREATE_ABx(OpCode::LOADK, 0, 3));

    std::ostringstream output;
    printProtoBytecode(&proto, output, false);
    std::string text = output.str();

    ASSERT_TRUE(suite, contains(text, "K[0] = string \"quote\\\" slash\\\\ line\\n tab\\t\""),
                "string constants keep escaped printable form");
    ASSERT_TRUE(suite, contains(text, "0000 | line 20 | LOADK | A=0 Bx=3 ; K[3] = <out of range>"),
                "out of range constants keep decoded instruction comment");
}

} // namespace

void registerBytecodePrinterTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "Proto Metadata And Constants",
                          testPrinterShowsProtoMetadataAndConstants);
    registry.registerTest(kSuiteName, "Loop Opcode Targets",
                          testPrinterShowsAsBxTargetsForLoopOpcodes);
    registry.registerTest(kSuiteName, "Escaped Strings And Out Of Range Constants",
                          testPrinterKeepsEscapedStringsAndOutOfRangeConstants);
}
