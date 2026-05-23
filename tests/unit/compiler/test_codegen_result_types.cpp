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

struct ValuePayloadSnapshot {
    ValueResult::Kind payloadKind = ValueResult::Kind::None;
    ValueResult::ImmediateKind payloadImmediate = ValueResult::ImmediateKind::None;
    ValueResult::AccessKind payloadAccess = ValueResult::AccessKind::None;
    i32 payloadReg = -1;
    i32 payloadConstIndex = -1;
    i32 payloadAux = -1;
    i32 payloadInstructionPc = NO_JUMP;
    bool payloadBoolValue = false;
    f64 payloadNumberValue = 0.0;
    bool payloadOwnsRegister = false;
    bool payloadIsMultiResult = false;
    bool payloadIsSingleValue = true;
};

ValuePayloadSnapshot snapshotValuePayload(const ValueResult& value) {
    return value.visit(ValueResultVisitor{
        [](const ValueResult::None&) -> ValuePayloadSnapshot {
            return {};
        },
        [](const ValueResult::Immediate& immediate) -> ValuePayloadSnapshot {
            ValuePayloadSnapshot result;
            result.payloadKind = ValueResult::Kind::Immediate;
            result.payloadImmediate = immediate.kind;
            result.payloadBoolValue = immediate.boolValue;
            result.payloadNumberValue = immediate.numberValue;
            return result;
        },
        [](const ValueResult::ConstantRef& constant) -> ValuePayloadSnapshot {
            ValuePayloadSnapshot result;
            result.payloadKind = ValueResult::Kind::Constant;
            result.payloadConstIndex = constant.constIndex;
            return result;
        },
        [](const ValueResult::RegisterRef& reg) -> ValuePayloadSnapshot {
            ValuePayloadSnapshot result;
            result.payloadKind = ValueResult::Kind::Register;
            result.payloadAccess = reg.access;
            result.payloadReg = reg.reg;
            result.payloadOwnsRegister = reg.ownsRegister;
            return result;
        },
        [](const ValueResult::PendingLoad& pending) -> ValuePayloadSnapshot {
            ValuePayloadSnapshot result;
            result.payloadKind = ValueResult::Kind::PendingLoad;
            result.payloadAccess = pending.access;
            result.payloadReg = pending.reg;
            result.payloadConstIndex = pending.constIndex;
            result.payloadAux = pending.aux;
            return result;
        },
        [](const ValueResult::Relocatable& relocatable) -> ValuePayloadSnapshot {
            ValuePayloadSnapshot result;
            result.payloadKind = ValueResult::Kind::Relocatable;
            result.payloadInstructionPc = relocatable.instructionPc;
            return result;
        },
        [](const ValueResult::MultiRet& multi) -> ValuePayloadSnapshot {
            ValuePayloadSnapshot result;
            result.payloadKind = ValueResult::Kind::MultiRet;
            result.payloadAccess = multi.access;
            result.payloadReg = multi.reg;
            result.payloadInstructionPc = multi.instructionPc;
            result.payloadIsMultiResult = true;
            result.payloadIsSingleValue = false;
            return result;
        },
        [](const ValueResult::PendingJump& pending) -> ValuePayloadSnapshot {
            ValuePayloadSnapshot result;
            result.payloadKind = ValueResult::Kind::PendingJump;
            result.payloadInstructionPc = pending.instructionPc;
            return result;
        },
    });
}

void desyncLegacyNumberFields(ValueResult& value) {
    ValueResult::LegacyFields fields = value.legacyFields();
    fields.kind = ValueResult::Kind::Register;
    fields.reg = 99;
    fields.numberValue = -1.0;
    detail::ValueResultLegacyMirrorProbe::overwriteForCharacterization(value, fields);
}

void desyncLegacyMultiRetFields(ValueResult& value) {
    ValueResult::LegacyFields fields = value.legacyFields();
    fields.access = ValueResult::AccessKind::Vararg;
    fields.reg = 77;
    fields.instructionPc = 101;
    detail::ValueResultLegacyMirrorProbe::overwriteForCharacterization(value, fields);
}

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

    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::None),
              static_cast<int>(snapshotValuePayload(value).payloadKind), "ValueResult default payload kind");
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
    ASSERT_TRUE(suite, snapshotValuePayload(none).payloadKind == ValueResult::Kind::None,
                "default ValueResult payload is None");

    ValueResult number = ValueResult::makeNumber(12.5);
    ValuePayloadSnapshot numberPayload = snapshotValuePayload(number);
    ASSERT_TRUE(suite, numberPayload.payloadKind == ValueResult::Kind::Immediate,
                "number ValueResult uses Immediate payload");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::ImmediateKind::Number),
              static_cast<int>(numberPayload.payloadImmediate), "number payload keeps immediate kind");
    ASSERT_EQ(suite, 12.5, numberPayload.payloadNumberValue, "number payload keeps numeric value");

    ValueResult local = ValueResult::makeRegister(3, false, ValueResult::AccessKind::Local);
    ValuePayloadSnapshot localPayload = snapshotValuePayload(local);
    ASSERT_TRUE(suite, localPayload.payloadKind == ValueResult::Kind::Register,
                "local ValueResult uses RegisterRef payload");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::AccessKind::Local),
              static_cast<int>(localPayload.payloadAccess), "register payload keeps access");
    ASSERT_EQ(suite, 3, localPayload.payloadReg, "register payload keeps register index");

    ValueResult global = ValueResult::makePendingLoad(ValueResult::AccessKind::Global, -1, 7, -1);
    ValuePayloadSnapshot globalPayload = snapshotValuePayload(global);
    ASSERT_TRUE(suite, globalPayload.payloadKind == ValueResult::Kind::PendingLoad,
                "global read ValueResult uses PendingLoad payload");
    ASSERT_EQ(suite, 7, globalPayload.payloadConstIndex, "pending-load payload keeps constant index");

    ValueResult call = ValueResult::makeMultiRet(ValueResult::AccessKind::Call, 2, 9);
    ValuePayloadSnapshot callPayload = snapshotValuePayload(call);
    ASSERT_TRUE(suite, callPayload.payloadKind == ValueResult::Kind::MultiRet,
                "call ValueResult uses MultiRet payload");
    ASSERT_TRUE(suite, callPayload.payloadIsMultiResult, "multi-ret payload is multi result");
    ASSERT_FALSE(suite, callPayload.payloadIsSingleValue, "multi-ret payload is not a single value");
}

void testValueResultLegacySnapshotStaysSynced(TestSuite& suite) {
    ValueResult number = ValueResult::makeNumber(12.5);
    ValueResult::LegacyFields numberLegacy = number.legacyFields();
    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::Immediate),
              static_cast<int>(numberLegacy.kind), "number factory keeps legacy kind snapshot");
    ASSERT_EQ(suite, 12.5, numberLegacy.numberValue, "number factory keeps legacy number snapshot");

    ValueResult local = ValueResult::makeRegister(3, false, ValueResult::AccessKind::Local);
    ValueResult::LegacyFields localLegacy = local.legacyFields();
    ASSERT_EQ(suite, static_cast<int>(ValueResult::Kind::Register),
              static_cast<int>(localLegacy.kind), "register factory keeps legacy kind snapshot");
    ASSERT_EQ(suite, static_cast<int>(ValueResult::AccessKind::Local),
              static_cast<int>(localLegacy.access), "register factory keeps legacy access snapshot");
    ASSERT_EQ(suite, 3, localLegacy.reg, "register factory keeps legacy register snapshot");

    ValueResult global = ValueResult::makePendingLoad(ValueResult::AccessKind::Global, -1, 7, -1);
    ValueResult::LegacyFields globalLegacy = global.legacyFields();
    ASSERT_EQ(suite, 7, globalLegacy.constIndex, "pending-load factory keeps legacy constant snapshot");

    ValueResult call = ValueResult::makeMultiRet(ValueResult::AccessKind::Call, 2, 9);
    ValueResult::LegacyFields callLegacy = call.legacyFields();
    ASSERT_TRUE(suite, callLegacy.isMultiResult, "multi-ret factory keeps legacy multi snapshot");
    ASSERT_FALSE(suite, callLegacy.isSingleValue, "multi-ret factory clears legacy single snapshot");
}

void testValueResultPayloadVisitIgnoresLegacyDrift(TestSuite& suite) {
    ValueResult number = ValueResult::makeNumber(12.5);
    desyncLegacyNumberFields(number);

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
    desyncLegacyMultiRetFields(call);

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
    registry.registerTest(kSuiteName, "ValueResult Legacy Snapshot Stays Synced",
                          testValueResultLegacySnapshotStaysSynced);
    registry.registerTest(kSuiteName, "ValueResult Payload Visit Ignores Legacy Drift",
                          testValueResultPayloadVisitIgnoresLegacyDrift);
}
