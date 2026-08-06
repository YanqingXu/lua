/**
 * @file test_debug_info.cpp
 * @brief Debug source normalization and Proto metadata contract tests.
 */

#include "../framework/test_framework.hpp"

#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "core/gc_string.hpp"
#include "debugger/debug_info.hpp"
#include "runtime/runtime_services.hpp"

#include <optional>

using namespace Lua;
using namespace Lua::Debugger;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "Debugger Debug Info";

Proto* generateProto(StrView source, StrView sourceName) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    Parser parser{Str(source)};
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }

    CodeGenerator codegen(services);
    return codegen.generate(*parsed, sourceName);
}

void collectProtoTree(const Proto& proto, Vec<const Proto*>& output) {
    output.push_back(&proto);
    for (usize index = 0; index < proto.getSubProtoCount(); ++index) {
        const Proto* child = proto.getSubProto(index);
        if (child != nullptr) {
            collectProtoTree(*child, output);
        }
    }
}

const LocVar* findLocal(const Proto& proto, StrView name) {
    for (usize index = 0; index < proto.getLocVarCount(); ++index) {
        const LocVar& local = proto.getLocVar(index);
        if (local.varname != nullptr && local.varname->view() == name) {
            return &local;
        }
    }
    return nullptr;
}

bool hasUpvalueName(const Proto& proto, StrView name) {
    for (usize index = 0; index < proto.getUpvalueNameCount(); ++index) {
        const GCString* upvalueName = proto.getUpvalueName(index);
        if (upvalueName != nullptr && upvalueName->view() == name) {
            return true;
        }
    }
    return false;
}

} // namespace

void testDebuggerSourceNormalization(TestSuite& suite) {
    const NormalizedSource windows = normalizeSourceName("@C:\\Game\\scripts\\.\\systems\\..\\main.lua");
    ASSERT_TRUE(suite, windows.kind == SourceKind::File, "Windows chunk source is classified as a file");
    ASSERT_EQ(suite, Str("C:/Game/scripts/main.lua"), windows.displayName,
              "Windows file display path uses normalized separators");
    ASSERT_EQ(suite, Str("file:c:/game/scripts/main.lua"), windows.identity,
              "Windows file identity is case insensitive");

    const NormalizedSource equivalentWindows = normalizeSourceName("@c:/game/scripts/main.lua");
    ASSERT_EQ(suite, windows.identity, equivalentWindows.identity,
              "Equivalent Windows spellings share a source identity");

    const NormalizedSource posix = normalizeSourceName("@/srv/game/./scripts/../main.lua");
    ASSERT_EQ(suite, Str("/srv/game/main.lua"), posix.displayName, "POSIX file path is lexically normalized");
    ASSERT_EQ(suite, Str("file:/srv/game/main.lua"), posix.identity, "POSIX source identity preserves case");

    const NormalizedSource named = normalizeSourceName("=stdin");
    ASSERT_TRUE(suite, named.kind == SourceKind::Named, "Equals-prefixed source is a named chunk");
    ASSERT_EQ(suite, Str("stdin"), named.displayName, "Named chunk display omits the Lua marker");

    const NormalizedSource memory = normalizeSourceName("return 42");
    ASSERT_TRUE(suite, memory.kind == SourceKind::Memory, "Plain source name is a memory chunk");
    ASSERT_FALSE(suite, memory.bindableFile(), "Memory chunk is not exposed as a bindable disk file");

    const NormalizedSource unknown = normalizeSourceName("");
    ASSERT_FALSE(suite, unknown.valid(), "Empty source remains unknown");
}

void testDebuggerCompiledProtoMetadataIsComplete(TestSuite& suite) {
    constexpr StrView source = "-- debugger metadata fixture\n"
                               "local total = 0\n"
                               "local function add(value)\n"
                               "    local doubled = value * 2\n"
                               "    total = total + doubled\n"
                               "end\n"
                               "for i = 1, 2 do\n"
                               "    add(i)\n"
                               "end\n"
                               "return total\n";
    constexpr StrView sourceName = "@C:\\Game\\scripts\\debug_info.lua";
    Proto* root = generateProto(source, sourceName);

    ASSERT_TRUE(suite, root != nullptr, "Debug metadata fixture compiles");
    if (root == nullptr) {
        return;
    }

    Vec<const Proto*> protos;
    collectProtoTree(*root, protos);
    ASSERT_TRUE(suite, protos.size() >= 2, "Nested function creates a child Proto");

    bool allLineTablesComplete = true;
    bool allSourcesInherited = true;
    for (const Proto* proto : protos) {
        allLineTablesComplete = allLineTablesComplete && proto->getInstructionCount() == proto->getLineInfo().size();
        allSourcesInherited =
            allSourcesInherited && proto->getSource() != nullptr && proto->getSource()->view() == sourceName;
    }
    ASSERT_TRUE(suite, allLineTablesComplete, "Every compiled instruction has one line-info entry");
    ASSERT_TRUE(suite, allSourcesInherited, "Nested compiled protos preserve the parent source name");

    DebugInfoIndex index(*root);
    ASSERT_EQ(suite, protos.size(), index.protoStatuses().size(), "Debug index covers the complete Proto tree");

    const std::optional<i32> firstLine = index.resolveExecutableLine("@c:/game/scripts/debug_info.lua", 1);
    ASSERT_TRUE(suite, firstLine.has_value(), "Breakpoint resolver finds the first executable line");
    if (firstLine) {
        ASSERT_TRUE(suite, *firstLine >= 2, "Comment-only line is not reported as executable");
        ASSERT_FALSE(suite, index.locationsForLine(sourceName, *firstLine).empty(),
                     "Resolved executable line has at least one PC");
    }

    const Vec<DebugCodeLocation> nestedLocations = index.locationsForLine(sourceName, 4);
    ASSERT_FALSE(suite, nestedLocations.empty(), "Nested function body line maps to bytecode");
    bool nestedLocationBelongsToChild = false;
    for (const DebugCodeLocation& location : nestedLocations) {
        nestedLocationBelongsToChild = nestedLocationBelongsToChild || location.proto != root;
        const std::optional<DebugCodeLocation> reverse = index.locationForPc(*location.proto, location.pc);
        ASSERT_TRUE(suite, reverse.has_value() && reverse->line == location.line,
                    "Each source-to-PC mapping can be reversed safely");
    }
    ASSERT_TRUE(suite, nestedLocationBelongsToChild, "Nested line is indexed against its child Proto");
}

void testDebuggerLocalAndUpvalueLifetimes(TestSuite& suite) {
    constexpr StrView source = "local total = 10\n"
                               "local function add(value)\n"
                               "    local doubled = value * 2\n"
                               "    total = total + doubled\n"
                               "    return total\n"
                               "end\n"
                               "return add(3)\n";
    Proto* root = generateProto(source, "@debugger/local_lifetime.lua");

    ASSERT_TRUE(suite, root != nullptr && root->getSubProtoCount() == 1,
                "Local lifetime fixture creates one nested function");
    if (root == nullptr || root->getSubProtoCount() != 1) {
        return;
    }

    const Proto* child = root->getSubProto(0);
    const LocVar* value = findLocal(*child, "value");
    const LocVar* doubled = findLocal(*child, "doubled");
    ASSERT_TRUE(suite, value != nullptr, "Function parameter has local debug metadata");
    ASSERT_TRUE(suite, doubled != nullptr, "Block local has debug metadata");

    const auto validLifetime = [child](const LocVar* local) {
        return local != nullptr && local->startpc >= 0 && local->startpc < local->endpc && local->reg >= 0 &&
               static_cast<usize>(local->endpc) <= child->getInstructionCount();
    };
    ASSERT_TRUE(suite, validLifetime(value), "Parameter lifetime is a valid half-open PC range");
    ASSERT_TRUE(suite, validLifetime(doubled), "Block local lifetime is a valid half-open PC range");
    ASSERT_TRUE(suite, hasUpvalueName(*child, "total"), "Captured outer local keeps its upvalue name");
}

void testDebuggerSameLineAndLoopLocations(TestSuite& suite) {
    constexpr StrView source = "local value = 0; value = value + 1; value = value + 2\n"
                               "while value < 5 do value = value + 1 end\n"
                               "return value\n";
    constexpr StrView sourceName = "@debugger/same_line.lua";
    Proto* root = generateProto(source, sourceName);
    ASSERT_TRUE(suite, root != nullptr, "Same-line fixture compiles");
    if (root == nullptr) {
        return;
    }

    DebugInfoIndex index(*root);
    const Vec<DebugCodeLocation> firstLine = index.locationsForLine(sourceName, 1);
    const Vec<DebugCodeLocation> loopLine = index.locationsForLine(sourceName, 2);
    ASSERT_TRUE(suite, firstLine.size() > 1, "Multiple statements on one line map to multiple PCs");
    ASSERT_TRUE(suite, loopLine.size() > 1, "Loop source line maps to its test, body, and back-edge PCs");
}

void testDebuggerStrippedAndInheritedMetadataIsSafe(TestSuite& suite) {
    RuntimeServices services = RuntimeServices::fromSingletons();
    Proto* stripped = services.gc.create<Proto>();
    stripped->setSource(services.strings.intern("@debugger/stripped.lua"));
    stripped->addInstruction(0);

    DebugInfoIndex strippedIndex(*stripped);
    ASSERT_EQ(suite, static_cast<usize>(1), strippedIndex.protoStatuses().size(),
              "Stripped Proto still has a status record");
    ASSERT_FALSE(suite, strippedIndex.protoStatuses().front().hasCompleteLineInfo(),
                 "Missing line table is reported as incomplete");
    ASSERT_FALSE(suite, strippedIndex.resolveExecutableLine("@debugger/stripped.lua", 1).has_value(),
                 "Stripped Proto does not fabricate an executable line");
    ASSERT_FALSE(suite, strippedIndex.locationForPc(*stripped, 0).has_value(),
                 "PC lookup on stripped metadata fails without out-of-bounds access");

    Proto* parent = services.gc.create<Proto>();
    parent->setSource(services.strings.intern("@debugger/inherited.lua"));
    parent->addInstruction(0);
    parent->addLineInfo(1);
    Proto* child = services.gc.create<Proto>();
    child->addInstruction(0);
    child->addLineInfo(7);
    parent->addProto(child);

    DebugInfoIndex inheritedIndex(*parent);
    const Vec<DebugCodeLocation> inheritedLocations = inheritedIndex.locationsForLine("@debugger/inherited.lua", 7);
    ASSERT_EQ(suite, static_cast<usize>(1), inheritedLocations.size(),
              "Child with null source inherits parent source for debugger lookup");
    if (!inheritedLocations.empty()) {
        ASSERT_TRUE(suite, inheritedLocations.front().proto == child,
                    "Inherited source location retains the child Proto observer");
    }
}

void registerDebuggerDebugInfoTests() {
    auto& registry = TestRegistry::getInstance();
    registry.registerTest(kSuiteName, "Source Normalization", testDebuggerSourceNormalization);
    registry.registerTest(kSuiteName, "Compiled Proto Metadata Is Complete",
                          testDebuggerCompiledProtoMetadataIsComplete);
    registry.registerTest(kSuiteName, "Local And Upvalue Lifetimes", testDebuggerLocalAndUpvalueLifetimes);
    registry.registerTest(kSuiteName, "Same Line And Loop Locations", testDebuggerSameLineAndLoopLocations);
    registry.registerTest(kSuiteName, "Stripped And Inherited Metadata Is Safe",
                          testDebuggerStrippedAndInheritedMetadataIsSafe);
}
