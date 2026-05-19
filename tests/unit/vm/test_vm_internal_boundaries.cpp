/**
 * @file test_vm_internal_boundaries.cpp
 * @brief Compile-time checks for VM implementation-slice boundaries.
 */

#include "../framework/test_framework.hpp"
#include "compiler/opcode.hpp"
#include "core/value.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/vm_internal.hpp"

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
                                 bool (*)()>);

    ASSERT_TRUE(suite, true, "call helper signatures are stable");
}

void testTraceAndDebugHelpersExposeStableSignatures(TestSuite& suite) {
    static_assert(std::is_same_v<decltype(&VM::detail::dispatchCountHook),
                                 void (*)(LuaState*)>);
    static_assert(std::is_same_v<decltype(&VM::detail::dispatchLineHook),
                                 void (*)(LuaState*, Proto*, usize)>);
    static_assert(std::is_same_v<decltype(&VM::detail::emitInstructionTrace),
                                 void (*)(Proto*, Value*, usize, Instruction, i32)>);
    static_assert(std::is_same_v<decltype(&VM::detail::emitCallTrace),
                                 void (*)(Proto*, Value*, usize, i32, i32)>);
    static_assert(std::is_same_v<decltype(&VM::detail::emitReturnTrace),
                                 void (*)(i32)>);

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

}  // namespace

void registerVMInternalBoundaryTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Operation Helper Signatures", testOperationHelpersExposeStableSignatures);
    registry.registerTest(kSuiteName, "Call Helper Signatures", testCallHelpersExposeStableSignatures);
    registry.registerTest(kSuiteName, "Trace And Debug Helper Signatures",
                          testTraceAndDebugHelpersExposeStableSignatures);
    registry.registerTest(kSuiteName, "Remaining Helper Signatures", testRemainingHelperSignatures);
}
