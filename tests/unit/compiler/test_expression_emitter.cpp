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
    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::Immediate),
              static_cast<int>(numberValue.kind),
              "number literal should lower to an immediate ValueResult");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::ImmediateKind::Number),
              static_cast<int>(numberValue.immediate),
              "number literal immediate kind should be number");
    ASSERT_EQ(suite, 42.5, numberValue.numberValue, "number literal value should be preserved");

    BoolExpr boolean{};
    boolean.value = true;
    Expr boolExpr(boolean);

    ValueResult boolValue = expressions.emitValue(boolExpr);
    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::Immediate),
              static_cast<int>(boolValue.kind),
              "boolean literal should lower to an immediate ValueResult");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::ImmediateKind::Boolean),
              static_cast<int>(boolValue.immediate),
              "boolean literal immediate kind should be boolean");
    ASSERT_TRUE(suite, boolValue.boolValue, "boolean literal value should be preserved");
}

void registerExpressionEmitterTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Public Boundary", testExpressionEmitterPublicBoundary);
    registry.registerTest(kSuiteName, "Lowers Immediate Values", testExpressionEmitterLowersImmediateValues);
}
