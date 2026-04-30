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

void registerCodegenResultTypeTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Default Result Type State", testDefaultResultTypeState);
    registry.registerTest(kSuiteName, "PatchList Append And Merge", testPatchListAppendAndMerge);
}
