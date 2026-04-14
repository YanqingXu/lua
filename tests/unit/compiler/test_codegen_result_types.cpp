/**
 * @file test_codegen_result_types.cpp
 * @brief Minimal PR-1 tests for result-type compatibility helpers.
 */

#include "../framework/test_framework.hpp"
#include "compiler/codegen_types.hpp"

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

void testLegacyExprDescAdapters(TestSuite& suite) {
    ExprDesc local;
    local.kind = ExprKind::Local;
    local.u.s.info = 4;

    ValueResult localValue = adaptLegacyExprDescValue(local);
    LValueRef localRef = adaptLegacyExprDescLValue(local);

    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::Register), static_cast<int>(localValue.kind), "Local adapts to register value");
    ASSERT_EQ(suite, 4, localValue.reg, "Local register preserved");
    ASSERT_FALSE(suite, localValue.ownsRegister, "Local register is not treated as temp");
    ASSERT_EQ(suite, static_cast<int>(LValueRef::Kind::Local), static_cast<int>(localRef.kind), "Local adapts to local lvalue");
    ASSERT_EQ(suite, 4, localRef.slot, "Local slot preserved");

    ExprDesc number;
    number.kind = ExprKind::Number;
    number.u.nval = 3.5;

    ValueResult numberValue = adaptLegacyExprDescValue(number);
    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::Immediate), static_cast<int>(numberValue.kind), "Number adapts to immediate");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::ImmediateKind::Number), static_cast<int>(numberValue.immediate), "Number immediate kind preserved");
    ASSERT_TRUE(suite, numberValue.numberValue == 3.5, "Number payload preserved");

    ExprDesc indexed;
    indexed.kind = ExprKind::Indexed;
    indexed.u.s.info = 2;
    indexed.u.s.aux = 9;

    LValueRef indexedRef = adaptLegacyExprDescLValue(indexed);
    ASSERT_EQ(suite, static_cast<int>(LValueRef::Kind::Indexed), static_cast<int>(indexedRef.kind), "Indexed adapts to indexed lvalue");
    ASSERT_EQ(suite, 2, indexedRef.tableReg, "Indexed table register preserved");
    ASSERT_EQ(suite, 9, indexedRef.key, "Indexed key preserved");

    ExprDesc callDesc;
    callDesc.kind = ExprKind::Call;
    callDesc.u.s.info = 6;
    callDesc.u.s.aux = 18;

    CallResultInfo callInfo = adaptLegacyExprDescCall(callDesc);
    ValueResult callValue = adaptLegacyExprDescValue(callDesc);

    ASSERT_TRUE(suite, callInfo.valid(), "CallResultInfo adapts from call expr");
    ASSERT_EQ(suite, static_cast<int>(CallResultInfo::Kind::Call), static_cast<int>(callInfo.kind), "CallResultInfo kind");
    ASSERT_EQ(suite, 6, callInfo.baseReg, "Call base register preserved");
    ASSERT_EQ(suite, 18, callInfo.instructionPc, "Call instruction PC preserved");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::MultiRet), static_cast<int>(callValue.kind), "Call value adapts to multret");
    ASSERT_TRUE(suite, callValue.isMultiResult, "Call value marks multi-result");
}

void registerCodegenResultTypeTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Default Result Type State", testDefaultResultTypeState);
    registry.registerTest(kSuiteName, "PatchList Append And Merge", testPatchListAppendAndMerge);
    registry.registerTest(kSuiteName, "Legacy ExprDesc Adapters", testLegacyExprDescAdapters);
}
