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

void testPrinterRecursesIntoChildProtosInFullMode(TestSuite& suite) {
    StringPool& pool = StringPool::getInstance();
    Proto root;
    Proto child;
    Proto grandchild;

    root.setSource(pool.intern("root.lua"));
    child.setSource(pool.intern("child.lua"));
    child.setLineDefined(20);
    grandchild.setSource(pool.intern("grandchild.lua"));
    grandchild.setLineDefined(30);

    i32 childConstant = static_cast<i32>(child.addConstant(Value(pool.intern("nested"))));
    child.addProto(&grandchild);
    root.addProto(&child);

    addInstruction(root, 1, CREATE_ABx(OpCode::CLOSURE, 0, 0));
    addInstruction(child, 20, CREATE_ABx(OpCode::LOADK, 0, childConstant));
    addInstruction(child, 21, CREATE_ABx(OpCode::CLOSURE, 1, 0));
    addInstruction(grandchild, 30, CREATE_ABC(OpCode::RETURN, 0, 1, 0));

    std::ostringstream compactOutput;
    printProtoBytecode(&root, compactOutput, false);
    std::string compact = compactOutput.str();

    ASSERT_TRUE(suite, contains(compact, "0000 | line 1 | CLOSURE | A=0 Bx=0 ; proto[0] = child.lua:20"),
                "CLOSURE instruction shows child proto summary");
    ASSERT_TRUE(suite, !contains(compact, "child protos"),
                "compact mode should not print recursive child proto sections");

    std::ostringstream fullOutput;
    printProtoBytecode(&root, fullOutput, true);
    std::string full = fullOutput.str();

    ASSERT_TRUE(suite, contains(full, "child protos (1)"), "full mode prints child proto count");
    ASSERT_TRUE(suite, contains(full, "  proto[0] child.lua:20"), "full mode labels child proto index");
    ASSERT_TRUE(suite, contains(full, "    source: child.lua"), "full mode prints child proto header");
    ASSERT_TRUE(suite, contains(full, "    0000 | line 20 | LOADK | A=0 Bx=0 ; K[0] = string \"nested\""),
                "full mode prints child proto instructions");
    ASSERT_TRUE(suite, contains(full, "    0001 | line 21 | CLOSURE | A=1 Bx=0 ; proto[0] = grandchild.lua:30"),
                "nested CLOSURE instruction shows grandchild proto summary");
    ASSERT_TRUE(suite, contains(full, "      proto[0] grandchild.lua:30"), "full mode labels grandchild proto index");
    ASSERT_TRUE(suite, contains(full, "        source: grandchild.lua"), "full mode recurses into grandchild proto");
    ASSERT_TRUE(suite, contains(full, "        0000 | line 30 | RETURN | A=0 B=1 C=0"),
                "full mode prints grandchild instructions");
}

void testPrinterShowsSideBySideDiff(TestSuite& suite) {
    Proto left;
    Proto right;

    left.setSource(StringPool::getInstance().intern("same.lua"));
    right.setSource(StringPool::getInstance().intern("same.lua"));

    i32 leftConstant = static_cast<i32>(left.addConstant(Value(1.0)));
    i32 rightConstant = static_cast<i32>(right.addConstant(Value(2.0)));

    addInstruction(left, 1, CREATE_ABx(OpCode::LOADK, 0, leftConstant));
    addInstruction(right, 1, CREATE_ABx(OpCode::LOADK, 0, rightConstant));

    std::ostringstream output;
    printProtoBytecodeDiff(&left, &right, output, false, "left.lua", "right.lua");
    std::string text = output.str();

    ASSERT_TRUE(suite, contains(text, "Bytecode diff"), "diff header is printed");
    ASSERT_TRUE(suite, contains(text, "left: left.lua"), "left label is printed");
    ASSERT_TRUE(suite, contains(text, "right: right.lua"), "right label is printed");
    ASSERT_TRUE(suite, contains(text, "mode: compact"), "compact diff mode is printed");
    ASSERT_TRUE(suite, contains(text, "status: different"), "different status is printed");
    ASSERT_TRUE(suite, contains(text, "changed lines: 2"), "changed line count is printed");
    ASSERT_TRUE(suite, contains(text, "line | left | right"), "side-by-side diff header is printed");
    ASSERT_TRUE(suite,
                contains(text,
                         "LOADK | A=0 Bx=0 ; K[0] = number 1 | "
                         "0000 | line 1 | LOADK | A=0 Bx=0 ; K[0] = number 2"),
                "instruction difference is printed side-by-side");
    ASSERT_TRUE(suite, contains(text, "K[0] = number 1 |   K[0] = number 2"),
                "constant table difference is printed side-by-side");
}

void testPrinterShowsIdenticalDiffSummary(TestSuite& suite) {
    Proto left;
    Proto right;

    left.setSource(StringPool::getInstance().intern("left.lua"));
    right.setSource(StringPool::getInstance().intern("right.lua"));

    addInstruction(left, 1, CREATE_ABC(OpCode::RETURN, 0, 1, 0));
    addInstruction(right, 1, CREATE_ABC(OpCode::RETURN, 0, 1, 0));

    std::ostringstream output;
    printProtoBytecodeDiff(&left, &right, output, true, "same-left.lua", "same-right.lua");
    std::string text = output.str();

    ASSERT_TRUE(suite, contains(text, "mode: full"), "full diff mode is printed");
    ASSERT_TRUE(suite, contains(text, "status: identical"), "identical status is printed");
    ASSERT_TRUE(suite, contains(text, "changed lines: 0"), "zero changed line count is printed");
    ASSERT_TRUE(suite, !contains(text, "line | left | right"),
                "identical diff should not print an empty table");
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
    registry.registerTest(kSuiteName, "Recursive Child Protos In Full Mode",
                          testPrinterRecursesIntoChildProtosInFullMode);
    registry.registerTest(kSuiteName, "Side By Side Diff",
                          testPrinterShowsSideBySideDiff);
    registry.registerTest(kSuiteName, "Identical Diff Summary",
                          testPrinterShowsIdenticalDiffSummary);
}
