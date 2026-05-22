/**
 * @file test_codegen_state.cpp
 * @brief Tests for the CodeGenerator state boundary.
 */

#include "../framework/test_framework.hpp"
#include "common/lua_error.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/codegen/codegen_state.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "runtime/runtime_services.hpp"

#include <expected>
#include <string>
#include <type_traits>
#include <utility>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Codegen State";

void testResetForProtoClearsTransientState(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    CodegenState state(services);

    state.pc = 12;
    state.currentLine = 34;
    state.localScope.localVars_.emplace_back("old", 0, 0);
    state.localScope.activeVarCount_ = 1;
    state.upvalueContext.add("captured", true, 0);
    state.blockManager.enterBlock(true, 1);
    state.blockManager.jpc_ = 9;
    state.registers.setFreeReg(5);

    Proto proto;
    state.resetForProto(proto, true, "state_test.lua");

    ASSERT_TRUE(suite, state.proto == &proto, "state should bind current proto");
    ASSERT_EQ(suite, 0, state.pc, "pc should reset");
    ASSERT_EQ(suite, 0, state.currentLine, "current line should reset");
    ASSERT_EQ(suite, 0, state.registers.current(), "register allocator should reset");
    ASSERT_TRUE(suite, state.localScope.localVars_.empty(), "locals should clear");
    ASSERT_EQ(suite, 0, state.localScope.activeVarCount_, "active locals should reset");
    ASSERT_EQ(suite, -1, state.upvalueContext.find("captured"), "upvalues should clear");
    ASSERT_TRUE(suite, state.blockManager.currentBlock_ == nullptr, "block stack should clear");
    ASSERT_EQ(suite, NO_JUMP, state.blockManager.jpc_, "pending jump list should reset");
    ASSERT_TRUE(suite, proto.isVararg(), "proto vararg flag should be set");
    ASSERT_EQ(suite, 2, static_cast<int>(proto.getMaxStackSize()), "proto minimum stack should be set");
    ASSERT_TRUE(suite, proto.getSource() != nullptr, "proto source should be set");
    ASSERT_EQ(suite, Str("state_test.lua"), proto.getSource()->getData(), "proto source should match");
}

void testTryGenerateReturnsExpectedType(TestSuite& suite) {
    using GenerateResult = decltype(std::declval<CodeGenerator&>().tryGenerate(std::declval<const Chunk&>()));
    bool hasExpectedSignature = std::is_same_v<GenerateResult, std::expected<Proto*, CodegenError>>;
    ASSERT_TRUE(suite, hasExpectedSignature, "tryGenerate returns expected proto or codegen error");
}

void testTryGenerateReturnsProtoOnSuccess(TestSuite& suite) {
    Parser parser("return 42");
    auto parsed = parser.parse();
    ASSERT_TRUE(suite, parsed.has_value(), "valid source should parse");
    if (!parsed) {
        return;
    }

    RuntimeServices services = RuntimeServices::fromSingletons();
    CodeGenerator codegen(services);
    auto generated = codegen.tryGenerate(*parsed, "=(codegen expected success)");

    ASSERT_TRUE(suite, generated.has_value(), "valid chunk should generate a proto");
    if (!generated) {
        return;
    }

    Proto* proto = *generated;
    ASSERT_TRUE(suite, proto != nullptr, "generated proto should not be null");
    ASSERT_TRUE(suite, proto->getSource() != nullptr, "generated proto should keep source name");
    ASSERT_EQ(suite, Str("=(codegen expected success)"), proto->getSource()->getData(),
              "generated proto source should match");
}

void testTryGenerateReturnsCodegenErrorOnFailure(TestSuite& suite) {
    Parser parser("break");
    auto parsed = parser.parse();
    ASSERT_TRUE(suite, parsed.has_value(), "semantic codegen failure source should still parse");
    if (!parsed) {
        return;
    }

    RuntimeServices services = RuntimeServices::fromSingletons();
    CodeGenerator codegen(services);
    auto generated = codegen.tryGenerate(*parsed, "=(codegen expected failure)");

    ASSERT_TRUE(suite, !generated.has_value(), "invalid codegen input should return CodegenError");
    if (generated) {
        return;
    }

    const CodegenError& error = generated.error();
    std::string message = error.what();
    ASSERT_TRUE(suite, message.find("no loop to break") != std::string::npos,
                "CodegenError should preserve the original message");
    ASSERT_TRUE(suite, (std::is_base_of<LuaError, CodegenError>::value),
                "CodegenError derives from LuaError");
}

void testGenerateKeepsThrowingForCompatibility(TestSuite& suite) {
    Parser parser("break");
    auto parsed = parser.parse();
    ASSERT_TRUE(suite, parsed.has_value(), "semantic codegen failure source should still parse");
    if (!parsed) {
        return;
    }

    RuntimeServices services = RuntimeServices::fromSingletons();
    CodeGenerator codegen(services);

    bool threwCodegenError = false;
    try {
        (void)codegen.generate(*parsed, "=(codegen compatibility failure)");
    } catch (const CodegenError& error) {
        threwCodegenError = std::string(error.what()).find("no loop to break") != std::string::npos;
    }

    ASSERT_TRUE(suite, threwCodegenError, "legacy generate should still throw on codegen failure");
}

}  // namespace

void registerCodegenStateTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Reset For Proto Clears Transient State", testResetForProtoClearsTransientState);
    registry.registerTest(kSuiteName, "tryGenerate returns expected type", testTryGenerateReturnsExpectedType);
    registry.registerTest(kSuiteName, "tryGenerate returns proto on success", testTryGenerateReturnsProtoOnSuccess);
    registry.registerTest(kSuiteName, "tryGenerate returns CodegenError on failure",
                          testTryGenerateReturnsCodegenErrorOnFailure);
    registry.registerTest(kSuiteName, "generate keeps throwing for compatibility",
                          testGenerateKeepsThrowingForCompatibility);
}
