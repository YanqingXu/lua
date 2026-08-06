/**
 * @file test_debug_types.cpp
 * @brief Debugger domain-ID, source-registry, and pause-handle tests.
 */

#include "../framework/test_framework.hpp"

#include "debugger/pause_handles.hpp"
#include "debugger/source_registry.hpp"

#include <type_traits>

using namespace Lua;
using namespace Lua::Debugger;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Debugger Domain Types";

static_assert(!std::is_same_v<SourceId, FrameId>);
static_assert(!std::is_convertible_v<u64, SourceId>);
static_assert(!std::is_constructible_v<FrameId, const void*>);

} // namespace

void testDebuggerStrongIds(TestSuite& suite) {
    const SourceId source{7};
    const FrameId frame{7};

    ASSERT_TRUE(suite, source.valid(), "Non-zero source ID is valid");
    ASSERT_EQ(suite, u64{7}, source.value(), "Opaque ID exposes only its integer value");
    ASSERT_FALSE(suite, SourceId{}.valid(), "Zero is reserved as the invalid ID");
    ASSERT_EQ(suite, u64{7}, frame.value(), "Different ID domains can carry the same integer safely");
}

void testDebuggerSourceRegistry(TestSuite& suite) {
    SourceRegistry registry;

    const SourceId first = registry.registerSource("@C:\\Game\\scripts\\.\\main.lua");
    const SourceId equivalent = registry.registerSource("@c:/game/scripts/main.lua");
    const SourceId editorPath = registry.registerFilePath("C:/GAME/scripts/main.lua");
    const SourceId named = registry.registerSource("=console");

    ASSERT_TRUE(suite, first.valid(), "File source receives a non-zero ID");
    ASSERT_EQ(suite, first, equivalent, "Equivalent Windows spellings receive the same ID");
    ASSERT_EQ(suite, first, editorPath, "Editor disk path and Lua @file source converge on one ID");
    ASSERT_FALSE(suite, first == named, "Different source identities receive different IDs");
    ASSERT_EQ(suite, usize{2}, registry.size(), "Registry stores one record per normalized identity");

    const RegisteredSource* stored = registry.lookup(first);
    ASSERT_TRUE(suite, stored != nullptr, "Registered source can be resolved by integer ID");
    ASSERT_EQ(suite, Str("file:c:/game/scripts/main.lua"), stored == nullptr ? Str{} : stored->source.identity,
              "Registry preserves the normalized source identity");
    ASSERT_TRUE(suite, registry.find("@C:/GAME/SCRIPTS/main.lua") == first,
                "Source lookup applies the same normalization as registration");
    ASSERT_FALSE(suite, registry.registerSource("").valid(), "Unknown source does not receive a protocol ID");
}

void testDebuggerPauseHandleGenerations(TestSuite& suite) {
    PauseHandleTable<FrameId, Str> frames;

    const PauseGeneration firstPause = frames.beginPause();
    const DebugResult<FrameId> firstFrame = frames.add("first frame");
    ASSERT_TRUE(suite, firstPause.valid(), "First stop creates a non-zero pause generation");
    ASSERT_TRUE(suite, firstFrame.has_value(), "A paused session can allocate frame handles");
    ASSERT_EQ(suite, Str("first frame"), firstFrame ? frames.lookup(*firstFrame)->get() : Str{},
              "A current-generation handle resolves its value");

    frames.endPause();
    const auto expiredAfterResume = frames.lookup(*firstFrame);
    ASSERT_FALSE(suite, expiredAfterResume.has_value(), "Resume invalidates all frame handles");
    ASSERT_TRUE(suite, !expiredAfterResume && expiredAfterResume.error().code == DebugErrorCode::StaleReference,
                "Expired handles report staleReference rather than dereferencing memory");

    const PauseGeneration secondPause = frames.beginPause();
    const DebugResult<FrameId> secondFrame = frames.add("second frame");
    ASSERT_FALSE(suite, firstPause == secondPause, "Each stop advances the pause generation");
    ASSERT_TRUE(suite, secondFrame && firstFrame && *secondFrame != *firstFrame,
                "Handle integers are never reused across pauses");
    ASSERT_TRUE(
        suite, !frames.lookup(*firstFrame) && frames.lookup(*firstFrame).error().code == DebugErrorCode::StaleReference,
        "Old handle cannot alias a new-generation frame");
    ASSERT_EQ(suite, Str("second frame"), secondFrame ? frames.lookup(*secondFrame)->get() : Str{},
              "New generation handle resolves normally");
}

void testDebuggerPauseHandleErrors(TestSuite& suite) {
    PauseHandleTable<VariableReference, i32> variables;

    const auto addWhileRunning = variables.add(42);
    ASSERT_TRUE(suite, !addWhileRunning && addWhileRunning.error().code == DebugErrorCode::InvalidState,
                "Handles cannot be created while the VM is running");

    [[maybe_unused]] const PauseGeneration generation = variables.beginPause();
    const auto unknown = variables.lookup(VariableReference{999});
    ASSERT_TRUE(suite, !unknown && unknown.error().code == DebugErrorCode::InvalidReference,
                "Never-issued integer is rejected as invalidReference");
    ASSERT_TRUE(suite, !variables.lookup(VariableReference{}), "Zero handle is always invalid");
}

void registerDebuggerDomainTypeTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "Strong Opaque IDs", testDebuggerStrongIds);
    registry.registerTest(kSuiteName, "Stable Source Registry", testDebuggerSourceRegistry);
    registry.registerTest(kSuiteName, "Pause Handle Generations", testDebuggerPauseHandleGenerations);
    registry.registerTest(kSuiteName, "Pause Handle Errors", testDebuggerPauseHandleErrors);
}
