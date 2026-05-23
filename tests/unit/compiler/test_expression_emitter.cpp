/**
 * @file test_expression_emitter.cpp
 * @brief Tests for the CodeGenerator expression lowering boundary.
 */

#include "../framework/test_framework.hpp"
#include "compiler/ast.hpp"
#include "compiler/codegen/codegen.hpp"
#include "compiler/codegen/codegen_types.hpp"
#include "compiler/codegen/expression_emitter.hpp"
#include "runtime/runtime_services.hpp"

#include <type_traits>
#include <utility>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Expression Emitter";

struct ImmediatePayload {
    ValueResult::ImmediateKind kind = ValueResult::ImmediateKind::None;
    bool boolValue = false;
    f64 numberValue = 0.0;
    bool matched = false;
};

ImmediatePayload readImmediatePayload(const ValueResult& value) {
    return value.visit(ValueResultVisitor{
        [](const ValueResult::Immediate& immediate) -> ImmediatePayload {
            return {immediate.kind, immediate.boolValue, immediate.numberValue, true};
        },
        [](const auto&) -> ImmediatePayload {
            return {};
        },
    });
}

void desyncLegacyNumberFields(ValueResult& value) {
    ValueResult::LegacyFields fields = value.legacyFields();
    fields.kind = ValueResult::Kind::Register;
    fields.reg = 42;
    fields.numberValue = -1.0;
    detail::ValueResultLegacyMirrorProbe::overwriteForCharacterization(value, fields);
}

}  // namespace

void testExpressionEmitterPublicBoundary(TestSuite& suite) {
    using EmitValueResult =
        decltype(std::declval<ExpressionEmitter&>().emitValue(std::declval<const Expr&>()));
    using EmitCondResult =
        decltype(std::declval<ExpressionEmitter&>().emitCondResult(std::declval<const Expr&>()));
    using EmitLValueResult =
        decltype(std::declval<ExpressionEmitter&>().emitLValue(std::declval<const Expr&>()));
    constexpr bool constructibleFromFacade = std::is_constructible_v<ExpressionEmitter, CodeGenerator&>;
    constexpr bool emitValueKeepsContract = std::is_same_v<EmitValueResult, ValueResult>;
    constexpr bool emitCondKeepsContract = std::is_same_v<EmitCondResult, CondResult>;
    constexpr bool emitLValueKeepsContract = std::is_same_v<EmitLValueResult, LValueRef>;

    ASSERT_TRUE(suite, constructibleFromFacade,
                "ExpressionEmitter should be constructible from CodeGenerator facade");
    ASSERT_TRUE(suite, emitValueKeepsContract,
                "emitValue should keep the ValueResult contract");
    ASSERT_TRUE(suite, emitCondKeepsContract,
                "emitCondResult should keep the CondResult contract");
    ASSERT_TRUE(suite, emitLValueKeepsContract,
                "emitLValue should keep the LValueRef contract");
}

void testExpressionEmitterLowersImmediateValues(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    CodeGenerator codegen(services);
    ExpressionEmitter expressions(codegen);

    NumberExpr number{};
    number.value = 42.5;
    Expr numberExpr(number);

    ValueResult numberValue = expressions.emitValue(numberExpr);
    ImmediatePayload numberPayload = readImmediatePayload(numberValue);
    ASSERT_TRUE(suite, numberPayload.matched,
                "number literal should use the Immediate payload");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::ImmediateKind::Number),
              static_cast<int>(numberPayload.kind),
              "number literal immediate kind should be number");
    ASSERT_EQ(suite, 42.5, numberPayload.numberValue, "number literal value should be preserved");

    BoolExpr boolean{};
    boolean.value = true;
    Expr boolExpr(boolean);

    ValueResult boolValue = expressions.emitValue(boolExpr);
    ImmediatePayload boolPayload = readImmediatePayload(boolValue);
    ASSERT_TRUE(suite, boolPayload.matched,
                "boolean literal should use the Immediate payload");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::ImmediateKind::Boolean),
              static_cast<int>(boolPayload.kind),
              "boolean literal immediate kind should be boolean");
    ASSERT_TRUE(suite, boolPayload.boolValue, "boolean literal value should be preserved");
}

void testExpressionEmitterMaterializesPayloadWhenLegacyFieldsDrift(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    CodeGenerator codegen(services);

    Chunk chunk;
    Proto* proto = codegen.generate(chunk, "test_expression_emitter");
    ExpressionEmitter expressions(codegen);

    ValueResult value = ValueResult::makeNumber(21.0);
    // Deliberately desync the compatibility fields; materialization should read the payload.
    desyncLegacyNumberFields(value);

    usize pc = proto->getInstructionCount();
    expressions.materializeValue(value, 0);

    Instruction inst = proto->getInstruction(pc);
    ASSERT_EQ(suite, static_cast<int>(OpCode::LOADK), static_cast<int>(GET_OPCODE(inst)),
              "materializeValue should dispatch from payload, not drifted legacy kind");
    ASSERT_EQ(suite, 0, GETARG_A(inst), "materialized payload number targets the requested register");

    Value constant = proto->getConstant(static_cast<usize>(GETARG_Bx(inst)));
    ASSERT_TRUE(suite, constant.isNumber(), "payload number materializes as a numeric constant");
    ASSERT_EQ(suite, 21.0, constant.asNumber(), "payload number value is preserved");

    delete proto;
}

void registerExpressionEmitterTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Public Boundary", testExpressionEmitterPublicBoundary);
    registry.registerTest(kSuiteName, "Lowers Immediate Values", testExpressionEmitterLowersImmediateValues);
    registry.registerTest(kSuiteName, "Materializes Payload When Legacy Fields Drift",
                          testExpressionEmitterMaterializesPayloadWhenLegacyFieldsDrift);
}
