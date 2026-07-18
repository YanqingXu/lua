/**
 * @file test_vm_internal_boundaries.cpp
 * @brief Compile-time checks for VM implementation-slice boundaries.
 */

#include "../framework/test_framework.hpp"
#include "compiler/opcode.hpp"
#include "core/function.hpp"
#include "core/value.hpp"
#include "runtime/bytecode_verifier.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/vm_internal.hpp"

#include <array>
#include <span>
#include <string>
#include <type_traits>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "VM Internal Boundaries";

void testOperationHelpersExposeStableSignatures(TestSuite& suite) {
    static_assert(std::is_same_v<decltype(&VM::detail::gettable),
                                 void (*)(LuaState*, Value, const Value&, Value&)>);
    static_assert(std::is_same_v<decltype(&VM::detail::settable),
                                 void (*)(LuaState*, Value, const Value&, const Value&)>);
    static_assert(std::is_same_v<decltype(&VM::detail::arith),
                                 void (*)(LuaState*, Value&, const Value&, const Value&, OpCode)>);
    static_assert(std::is_same_v<decltype(&VM::detail::execArithmetic),
                                 void (*)(LuaState*, Proto*, Value*&, i32, i32, i32, OpCode)>);
    static_assert(std::is_same_v<decltype(&VM::detail::equal),
                                 bool (*)(LuaState*, const Value&, const Value&)>);
    static_assert(std::is_same_v<decltype(&VM::detail::lessThan),
                                 bool (*)(LuaState*, const Value&, const Value&)>);
    static_assert(std::is_same_v<decltype(&VM::detail::lessEqual),
                                 bool (*)(LuaState*, const Value&, const Value&)>);
    static_assert(std::is_same_v<decltype(&VM::detail::unaryMinus),
                                 void (*)(LuaState*, Value&, const Value&)>);
    static_assert(std::is_same_v<decltype(&VM::detail::length),
                                 void (*)(LuaState*, Value&, const Value&)>);
    static_assert(std::is_same_v<decltype(&VM::detail::concat),
                                 void (*)(RuntimeServices&, LuaState*, Value*, i32, i32, i32)>);

    ASSERT_TRUE(suite, true, "operation helper signatures are stable");
}

void testCallHelpersExposeStableSignatures(TestSuite& suite) {
    static_assert(std::is_same_v<decltype(&VM::detail::precall),
                                 bool (*)(LuaState*, i32, i32, i32)>);
    static_assert(std::is_same_v<decltype(&VM::detail::postcall),
                                 void (*)(LuaState*, i32, i32, usize)>);
    static_assert(std::is_same_v<decltype(&VM::detail::reuseCurrentFrameForTailCall),
                                 void (*)(LuaState*, usize, usize, i32)>);
    static_assert(std::is_same_v<decltype(&VM::detail::dispatchCallHook),
                                 void (*)(LuaState*)>);
    static_assert(std::is_same_v<decltype(&VM::detail::dispatchReturnHook),
                                 void (*)(LuaState*)>);
    static_assert(std::is_same_v<decltype(&VM::detail::shouldDumpBytecode),
                                 bool (*)(LuaState*)>);

    ASSERT_TRUE(suite, true, "call helper signatures are stable");
}

void testTraceAndDebugHelpersExposeStableSignatures(TestSuite& suite) {
    static_assert(std::is_same_v<decltype(&VM::detail::dispatchCountHook),
                                 void (*)(LuaState*)>);
    static_assert(std::is_same_v<decltype(&VM::detail::dispatchLineHook),
                                 void (*)(LuaState*, Proto*, usize)>);
    static_assert(std::is_same_v<decltype(&VM::detail::emitInstructionTrace),
                                 void (*)(LuaState*, Proto*, Value*, usize, Instruction, i32)>);
    static_assert(std::is_same_v<decltype(&VM::detail::captureTraceRegisters),
                                 Vec<Value> (*)(LuaState*, usize, i32)>);
    static_assert(std::is_same_v<decltype(&VM::detail::emitInstructionTraceDiff),
                                 void (*)(Proto*, LuaState*, usize, usize, Instruction, i32,
                                          const Vec<Value>&)>);
    static_assert(std::is_same_v<decltype(&VM::detail::emitCallTrace),
                                 void (*)(LuaState*, Proto*, Value*, usize, i32, i32)>);
    static_assert(std::is_same_v<decltype(&VM::detail::emitReturnTrace),
                                 void (*)(LuaState*, Proto*, usize, i32)>);

    ASSERT_TRUE(suite, true, "trace and debug helper signatures are stable");
}

void testRemainingHelperSignatures(TestSuite& suite) {
    static_assert(std::is_same_v<decltype(&VM::detail::setList),
                                 void (*)(LuaState*, Value*, i32, i32, i32)>);
    static_assert(std::is_same_v<decltype(&VM::detail::closure),
                                 void (*)(LuaState*, Value*, Proto*, Function*, usize&, i32, i32)>);
    static_assert(std::is_same_v<decltype(&VM::detail::vararg),
                                 void (*)(LuaState*, Value*&, Proto*, i32, i32)>);
    static_assert(std::is_same_v<decltype(&VM::detail::tforLoop),
                                 void (*)(LuaState*, Value*&, Proto*, usize&, i32, i32)>);

    ASSERT_TRUE(suite, true, "remaining VM helper signatures are stable");
}

void testProtoInstructionSpanBoundary(TestSuite& suite) {
    static_assert(std::is_same_v<decltype(std::declval<const Proto&>().getInstructionSpan()),
                                 std::span<const Instruction>>);

    ASSERT_TRUE(suite, true, "proto instruction stream exposes a read-only span");
}

void appendOpcodeFixture(Proto& proto, OpCode opcode) {
    switch (opcode) {
    case OpCode::MOVE:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, 0));
        break;
    case OpCode::LOADK:
    case OpCode::GETGLOBAL:
    case OpCode::SETGLOBAL:
    case OpCode::CLOSURE:
        proto.addInstruction(CREATE_ABx(opcode, 0, 0));
        break;
    case OpCode::LOADBOOL:
        proto.addInstruction(CREATE_ABC(opcode, 0, 0, 0));
        break;
    case OpCode::LOADNIL:
        proto.addInstruction(CREATE_ABC(opcode, 0, 2, 0));
        break;
    case OpCode::GETUPVAL:
    case OpCode::SETUPVAL:
        proto.addInstruction(CREATE_ABC(opcode, 0, 0, 0));
        break;
    case OpCode::GETTABLE:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, RKASK(0)));
        break;
    case OpCode::SETTABLE:
        proto.addInstruction(CREATE_ABC(opcode, 0, RKASK(0), RKASK(0)));
        break;
    case OpCode::NEWTABLE:
        proto.addInstruction(CREATE_ABC(opcode, 0, 0, 0));
        break;
    case OpCode::SELF:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, RKASK(0)));
        break;
    case OpCode::ADD:
    case OpCode::SUB:
    case OpCode::MUL:
    case OpCode::DIV:
    case OpCode::MOD:
    case OpCode::POW:
        proto.addInstruction(CREATE_ABC(opcode, 0, RKASK(0), 1));
        break;
    case OpCode::UNM:
    case OpCode::NOT:
    case OpCode::LEN:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, 0));
        break;
    case OpCode::CONCAT:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, 2));
        break;
    case OpCode::JMP:
    case OpCode::FORLOOP:
    case OpCode::FORPREP:
        proto.addInstruction(CREATE_AsBx(opcode, 0, 0));
        break;
    case OpCode::EQ:
    case OpCode::LT:
    case OpCode::LE:
        proto.addInstruction(CREATE_ABC(opcode, 0, RKASK(0), 1));
        proto.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));
        break;
    case OpCode::TEST:
        proto.addInstruction(CREATE_ABC(opcode, 0, 0, 0));
        proto.addInstruction(CREATE_AsBx(OpCode::JMP, 0, 0));
        break;
    case OpCode::TESTSET:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, 0));
        proto.addInstruction(CREATE_AsBx(OpCode::JMP, 0, 0));
        break;
    case OpCode::CALL:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, 1));
        break;
    case OpCode::TAILCALL:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, 0));
        break;
    case OpCode::RETURN:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, 0));
        return;
    case OpCode::TFORLOOP:
        proto.addInstruction(CREATE_ABC(opcode, 0, 0, 1));
        proto.addInstruction(CREATE_AsBx(OpCode::JMP, 0, 0));
        break;
    case OpCode::SETLIST:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, 1));
        break;
    case OpCode::CLOSE:
        proto.addInstruction(CREATE_ABC(opcode, 0, 0, 0));
        break;
    case OpCode::VARARG:
        proto.addInstruction(CREATE_ABC(opcode, 0, 1, 0));
        break;
    }
    proto.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));
}

void testBytecodeVerifierCoversEveryOpcode(TestSuite& suite) {
    for (i32 rawOpcode = 0; rawOpcode < NUM_OPCODES; ++rawOpcode) {
        Proto child;
        child.setMaxStackSize(2);
        child.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));

        Proto proto;
        proto.setMaxStackSize(20);
        proto.setNumUpvalues(2);
        proto.setVararg(true);
        proto.addConstant(Value(42.0));
        proto.addProto(&child);

        const OpCode opcode = static_cast<OpCode>(rawOpcode);
        appendOpcodeFixture(proto, opcode);
        const auto result = BytecodeVerifier::verify(proto);
        ASSERT_TRUE(suite, result.has_value(), std::string("verifier accepts valid ") + getOpName(opcode));
    }
}

void testBytecodeVerifierPseudoInstructions(TestSuite& suite) {
    Proto child;
    child.setMaxStackSize(2);
    child.setNumUpvalues(2);
    child.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));

    Proto proto;
    proto.setMaxStackSize(4);
    proto.setNumUpvalues(1);
    proto.addProto(&child);
    proto.addInstruction(CREATE_ABx(OpCode::CLOSURE, 0, 0));
    proto.addInstruction(CREATE_ABC(OpCode::MOVE, 0, 1, 0));
    proto.addInstruction(CREATE_ABC(OpCode::GETUPVAL, 0, 0, 0));
    proto.addInstruction(CREATE_ABC(OpCode::SETLIST, 0, 1, 0));
    proto.addInstruction(1);
    proto.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));

    ASSERT_TRUE(suite, BytecodeVerifier::verify(proto).has_value(),
                "verifier accepts CLOSURE and SETLIST pseudo instructions");
}

void testBytecodeVerifierRejectsMalformedCode(TestSuite& suite) {
    const auto initializeProto = [](Proto& proto) {
        proto.setMaxStackSize(2);
        proto.addConstant(Value(1.0));
        proto.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));
    };

    {
        Proto proto;
        initializeProto(proto);
        proto.setInstruction(0, CREATE_ABC(OpCode::MOVE, 0, 200, 0));
        ASSERT_FALSE(suite, BytecodeVerifier::verify(proto).has_value(), "reject register overflow");
    }
    {
        Proto proto;
        initializeProto(proto);
        proto.setInstruction(0, static_cast<Instruction>(63));
        ASSERT_FALSE(suite, BytecodeVerifier::verify(proto).has_value(), "reject unknown opcode");
    }
    {
        Proto proto;
        initializeProto(proto);
        proto.setInstruction(0, CREATE_ABC(OpCode::SETLIST, 0, 1, 0));
        ASSERT_FALSE(suite, BytecodeVerifier::verify(proto).has_value(), "reject truncated SETLIST operand");
    }
    {
        Proto child;
        child.setMaxStackSize(1);
        child.setNumUpvalues(1);
        child.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));
        Proto proto;
        proto.setMaxStackSize(2);
        proto.addProto(&child);
        proto.addInstruction(CREATE_ABx(OpCode::CLOSURE, 0, 0));
        proto.addInstruction(CREATE_ABC(OpCode::ADD, 0, 0, 0));
        proto.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));
        ASSERT_FALSE(suite, BytecodeVerifier::verify(proto).has_value(), "reject invalid CLOSURE pseudo opcode");
    }
    {
        Proto child;
        child.setMaxStackSize(1);
        child.setNumUpvalues(1);
        child.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));
        Proto proto;
        proto.setMaxStackSize(2);
        proto.addProto(&child);
        proto.addInstruction(CREATE_ABx(OpCode::CLOSURE, 0, 0));
        proto.addInstruction(CREATE_ABC(OpCode::MOVE, 0, 0, 0));
        proto.addInstruction(CREATE_AsBx(OpCode::JMP, 0, -2));
        proto.addInstruction(CREATE_ABC(OpCode::RETURN, 0, 1, 0));
        ASSERT_FALSE(suite, BytecodeVerifier::verify(proto).has_value(), "reject jump into pseudo instruction");
    }
    {
        Proto proto;
        initializeProto(proto);
        BytecodeVerifierLimits limits;
        limits.maxInstructionCount = 0;
        ASSERT_FALSE(suite, BytecodeVerifier::verify(proto, limits).has_value(), "reject instruction budget overflow");
    }
}

}  // namespace

void registerVMInternalBoundaryTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Operation Helper Signatures", testOperationHelpersExposeStableSignatures);
    registry.registerTest(kSuiteName, "Call Helper Signatures", testCallHelpersExposeStableSignatures);
    registry.registerTest(kSuiteName, "Trace And Debug Helper Signatures",
                          testTraceAndDebugHelpersExposeStableSignatures);
    registry.registerTest(kSuiteName, "Remaining Helper Signatures", testRemainingHelperSignatures);
    registry.registerTest(kSuiteName, "Proto Instruction Span Boundary", testProtoInstructionSpanBoundary);
    registry.registerTest(kSuiteName, "Bytecode Verifier Opcode Coverage",
                          testBytecodeVerifierCoversEveryOpcode);
    registry.registerTest(kSuiteName, "Bytecode Verifier Pseudo Instructions",
                          testBytecodeVerifierPseudoInstructions);
    registry.registerTest(kSuiteName, "Bytecode Verifier Malformed Code",
                          testBytecodeVerifierRejectsMalformedCode);
}
