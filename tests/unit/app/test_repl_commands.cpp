/**
 * @file test_repl_commands.cpp
 * @brief REPL meta command and history tests.
 */

#include "../framework/test_framework.hpp"
#include "repl.hpp"

#include <filesystem>
#include <sstream>
#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "REPL Commands";

bool contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

void testParseMetaCommands(TestSuite& suite) {
    const REPL::MetaCommand help = REPL::parseMetaCommand(".help");
    ASSERT_EQ(suite, REPL::MetaCommandKind::Help, help.kind, ".help should parse as help command");
    ASSERT_TRUE(suite, help.argument.empty(), ".help should not keep an argument");

    const REPL::MetaCommand bytecode = REPL::parseMetaCommand("  .bytecode  1 + 2  ");
    ASSERT_EQ(suite, REPL::MetaCommandKind::Bytecode, bytecode.kind,
              ".bytecode should parse as bytecode command");
    ASSERT_EQ(suite, Str("1 + 2"), bytecode.argument, ".bytecode should trim its argument");

    const REPL::MetaCommand normalInput = REPL::parseMetaCommand("print(1)");
    ASSERT_EQ(suite, REPL::MetaCommandKind::None, normalInput.kind,
              "regular Lua source should not parse as a meta command");

    const REPL::MetaCommand unknown = REPL::parseMetaCommand(".wat");
    ASSERT_EQ(suite, REPL::MetaCommandKind::Unknown, unknown.kind,
              "unknown dot command should be classified");
    ASSERT_EQ(suite, Str("wat"), unknown.argument, "unknown command should keep the command name");
}

void testPrintHelpShowsSupportedCommands(TestSuite& suite) {
    std::ostringstream out;

    REPL::printHelp(out);
    const std::string text = out.str();

    ASSERT_TRUE(suite, contains(text, ".help"), "help should list .help");
    ASSERT_TRUE(suite, contains(text, ".bytecode <expr|chunk>"), "help should list .bytecode");
    ASSERT_TRUE(suite, contains(text, "=expr"), "help should mention quick expression evaluation");
    ASSERT_TRUE(suite, contains(text, "exit"), "help should mention exit");
}

void testHistoryRoundTrip(TestSuite& suite) {
    const std::filesystem::path historyPath =
        std::filesystem::temp_directory_path() / "lua_cpp_pr07_history_test.lua_history";
    std::filesystem::remove(historyPath);

    Vec<Str> history;
    REPL::recordHistory(history, "local x = 1");
    REPL::recordHistory(history, "");
    REPL::recordHistory(history, ".help");

    ASSERT_EQ(suite, static_cast<usize>(2), history.size(), "empty history lines should be ignored");
    ASSERT_TRUE(suite, REPL::saveHistory(historyPath.string(), history), "history should save to disk");

    Vec<Str> loaded;
    ASSERT_TRUE(suite, REPL::loadHistory(historyPath.string(), loaded), "history should load from disk");
    ASSERT_EQ(suite, history.size(), loaded.size(), "loaded history should preserve entry count");
    ASSERT_EQ(suite, history[0], loaded[0], "first history entry should round-trip");
    ASSERT_EQ(suite, history[1], loaded[1], "second history entry should round-trip");

    std::filesystem::remove(historyPath);
}

void testBytecodeCommandPrintsCompiledExpression(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    std::ostringstream out;
    std::ostringstream err;

    const int status = REPL::printBytecode(L, "1 + 2", out, err);
    const std::string text = out.str();

    ASSERT_EQ(suite, 0, status, ".bytecode expression should succeed");
    ASSERT_TRUE(suite, contains(text, "source: =(repl bytecode)"), "bytecode should use REPL source label");
    ASSERT_TRUE(suite, contains(text, "instructions"), "bytecode should print instruction section");
    ASSERT_TRUE(suite, contains(text, "constants"), "bytecode should print constant section");
    ASSERT_TRUE(suite, err.str().empty(), ".bytecode expression should not emit errors");

    delete L;
}

void testBytecodeCommandRejectsMissingArgument(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    std::ostringstream out;
    std::ostringstream err;

    const int status = REPL::printBytecode(L, "   ", out, err);

    ASSERT_EQ(suite, 1, status, ".bytecode without an argument should fail");
    ASSERT_TRUE(suite, out.str().empty(), "empty .bytecode should not print bytecode");
    ASSERT_TRUE(suite, contains(err.str(), "usage: .bytecode <expr|chunk>"),
                "empty .bytecode should print usage");

    delete L;
}

} // namespace

void registerReplCommandTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Parse Meta Commands", testParseMetaCommands);
    registry.registerTest(kSuiteName, "Print Help Shows Supported Commands", testPrintHelpShowsSupportedCommands);
    registry.registerTest(kSuiteName, "History Round Trip", testHistoryRoundTrip);
    registry.registerTest(kSuiteName, "Bytecode Command Prints Compiled Expression",
                          testBytecodeCommandPrintsCompiledExpression);
    registry.registerTest(kSuiteName, "Bytecode Command Rejects Missing Argument",
                          testBytecodeCommandRejectsMissingArgument);
}
