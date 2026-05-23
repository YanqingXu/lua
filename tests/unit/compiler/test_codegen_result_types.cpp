/**
 * @file test_codegen_result_types.cpp
 * @brief Minimal PR-1 tests for result-type compatibility helpers.
 */

#include "../framework/test_framework.hpp"
#include "compiler/codegen/codegen_types.hpp"

#include <type_traits>
#include <variant>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Codegen Result Types";

}  // namespace

void testDefaultResultTypeState(TestSuite& suite) {
    PatchList patchList;
    CondResult cond;
    ValueResult value;
    LValueRef lvalue;
    CallResultInfo call;

    ASSERT_TRUE(suite, patchList.empty(), "PatchList defaults to empty");
    ASSERT_EQ(suite, 0, static_cast<int>(patchList.size()), "PatchList default size");
    ASSERT_EQ(suite, NO_JUMP, patchList.front(), "PatchList default front");

    ASSERT_TRUE(suite, cond.trueList.empty(), "CondResult default true list empty");
    ASSERT_TRUE(suite, cond.falseList.empty(), "CondResult default false list empty");
    ASSERT_FALSE(suite, cond.knownConstant, "CondResult default constant flag");

    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::None), static_cast<int>(value.kind), "ValueResult default kind");
    ASSERT_EQ(suite, static_cast<int>(LValueRef::Kind::None), static_cast<int>(lvalue.kind), "LValueRef default kind");
    ASSERT_EQ(suite, static_cast<int>(CallResultInfo::Kind::None), static_cast<int>(call.kind), "CallResultInfo default kind");
    ASSERT_FALSE(suite, lvalue.valid(), "LValueRef default invalid");
    ASSERT_FALSE(suite, call.valid(), "CallResultInfo default invalid");
}

void testPatchListAppendAndMerge(TestSuite& suite) {
    PatchList left;
    left.append(NO_JUMP);
    left.append(3);
    left.append(7);

    PatchList right;
    right.append(11);

    ASSERT_FALSE(suite, left.empty(), "PatchList append stores valid PCs");
    ASSERT_EQ(suite, 2, static_cast<int>(left.size()), "PatchList ignores NO_JUMP");
    ASSERT_EQ(suite, 3, left.front(), "PatchList front preserves first PC");

    PatchList merged = PatchList::merge(left, right);
    ASSERT_EQ(suite, 3, static_cast<int>(merged.size()), "PatchList merge combines entries");
    ASSERT_EQ(suite, 3, merged.front(), "PatchList merge keeps left front");

    left.append(right);
    ASSERT_EQ(suite, 3, static_cast<int>(left.size()), "PatchList append(list) concatenates entries");
}

void testValueResultVariantPrototype(TestSuite& suite) {
    using Variant = ValueResult::Variant;

    constexpr bool usesStdVariant = std::is_same_v<
        Variant,
        std::variant<ValueResult::None, ValueResult::Immediate, ValueResult::ConstantRef,
                     ValueResult::RegisterRef, ValueResult::PendingLoad, ValueResult::Relocatable,
                     ValueResult::MultiRet, ValueResult::PendingJump>>;
    ASSERT_TRUE(suite, usesStdVariant, "ValueResult exposes a std::variant prototype payload");

    ValueResult none;
    ASSERT_TRUE(suite, std::holds_alternative<ValueResult::None>(none.payload()),
                "default ValueResult payload is None");

    ValueResult number = ValueResult::makeNumber(12.5);
    ASSERT_TRUE(suite, std::holds_alternative<ValueResult::Immediate>(number.payload()),
                "number ValueResult uses Immediate payload");
    const auto* immediate = std::get_if<ValueResult::Immediate>(&number.payload());
    ASSERT_TRUE(suite, immediate != nullptr, "number immediate payload is readable");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::ImmediateKind::Number),
              static_cast<int>(immediate->kind), "number payload keeps immediate kind");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::Immediate),
              static_cast<int>(number.kind), "number factory keeps legacy kind field");
    ASSERT_EQ(suite, 12.5, number.numberValue, "number factory keeps legacy number field");

    ValueResult local = ValueResult::makeRegister(3, false, ValueResult::AccessKind::Local);
    ASSERT_TRUE(suite, std::holds_alternative<ValueResult::RegisterRef>(local.payload()),
                "local ValueResult uses RegisterRef payload");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::Register),
              static_cast<int>(local.kind), "register factory keeps legacy kind field");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::AccessKind::Local),
              static_cast<int>(local.access), "register factory keeps legacy access field");
    ASSERT_EQ(suite, 3, local.reg, "register factory keeps legacy register field");

    ValueResult global = ValueResult::makePendingLoad(ValueResult::AccessKind::Global, -1, 7, -1);
    ASSERT_TRUE(suite, std::holds_alternative<ValueResult::PendingLoad>(global.payload()),
                "global read ValueResult uses PendingLoad payload");
    ASSERT_EQ(suite, 7, global.constIndex, "pending-load factory keeps constant index");

    ValueResult call = ValueResult::makeMultiRet(ValueResult::AccessKind::Call, 2, 9);
    ASSERT_TRUE(suite, std::holds_alternative<ValueResult::MultiRet>(call.payload()),
                "call ValueResult uses MultiRet payload");
    ASSERT_TRUE(suite, call.isMultiResult, "multi-ret factory keeps legacy multi flag");
    ASSERT_FALSE(suite, call.isSingleValue, "multi-ret factory clears legacy single flag");
}

void testValueResultPayloadVisitIgnoresLegacyDrift(TestSuite& suite) {
    ValueResult number = ValueResult::makeNumber(12.5);
    number.kind = ValueResult::Kind::Register;
    number.reg = 99;
    number.numberValue = -1.0;

    f64 visitedNumber = number.visit(ValueResultVisitor{
        [](const ValueResult::Immediate& immediate) -> f64 {
            return immediate.kind == ValueResult::ImmediateKind::Number ? immediate.numberValue : -1.0;
        },
        [](const auto&) -> f64 {
            return -1.0;
        },
    });
    ASSERT_EQ(suite, 12.5, visitedNumber, "payload visit reads the variant, not drifted legacy fields");

    ValueResult call = ValueResult::makeMultiRet(ValueResult::AccessKind::Call, 2, 9);
    call.access = ValueResult::AccessKind::Vararg;
    call.reg = 77;
    call.instructionPc = 101;

    i32 visitedBase = call.visit(ValueResultVisitor{
        [](const ValueResult::MultiRet& multi) -> i32 {
            return multi.access == ValueResult::AccessKind::Call ? multi.reg : -1;
        },
        [](const auto&) -> i32 {
            return -1;
        },
    });
    ASSERT_EQ(suite, 2, visitedBase, "payload visit keeps multi-ret base independent from legacy fields");
}

void registerCodegenResultTypeTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Default Result Type State", testDefaultResultTypeState);
    registry.registerTest(kSuiteName, "PatchList Append And Merge", testPatchListAppendAndMerge);
    registry.registerTest(kSuiteName, "ValueResult Variant Prototype", testValueResultVariantPrototype);
    registry.registerTest(kSuiteName, "ValueResult Payload Visit Ignores Legacy Drift",
                          testValueResultPayloadVisitIgnoresLegacyDrift);
}
