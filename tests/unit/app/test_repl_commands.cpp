/**
 * @file test_repl_commands.cpp
 * @brief REPL meta command and history tests.
 */

#include "../framework/test_framework.hpp"
#include "core/string_pool.hpp"
#include "lib/lib_manager.hpp"
#include "repl.hpp"
#include "repl/repl_exe.hpp"
#include "repl/repl_prompt.hpp"
#include "vm/state/global_state.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

using namespace Lua;
using namespace LuaTest;

namespace {

constexpr const char* kSuiteName = "REPL Commands";

bool contains(const std::string& text, const char* needle) {
    return text.find(needle) != std::string::npos;
}

bool hasCandidate(const Vec<Str>& candidates, const Str& value) {
    for (const Str& candidate : candidates) {
        if (candidate == value) {
            return true;
        }
    }
    return false;
}

void testParseMetaCommands(TestSuite& suite) {
    const REPL::MetaCommand help = REPL::parseMetaCommand(".help");
    ASSERT_EQ(suite, REPL::MetaCommandKind::Help, help.kind, ".help should parse as help command");
    ASSERT_TRUE(suite, help.argument.empty(), ".help should not keep an argument");

    const REPL::MetaCommand bytecode = REPL::parseMetaCommand("  .bytecode  1 + 2  ");
    ASSERT_EQ(suite, REPL::MetaCommandKind::Bytecode, bytecode.kind,
              ".bytecode should parse as bytecode command");
    ASSERT_EQ(suite, Str("1 + 2"), bytecode.argument, ".bytecode should trim its argument");

    const REPL::MetaCommand ast = REPL::parseMetaCommand("  .ast  local x = 1  ");
    ASSERT_EQ(suite, REPL::MetaCommandKind::Ast, ast.kind, ".ast should parse as ast command");
    ASSERT_EQ(suite, Str("local x = 1"), ast.argument, ".ast should trim its argument");

    const REPL::MetaCommand gc = REPL::parseMetaCommand("  .gc  collect  ");
    ASSERT_EQ(suite, REPL::MetaCommandKind::Gc, gc.kind, ".gc should parse as gc command");
    ASSERT_EQ(suite, Str("collect"), gc.argument, ".gc should trim its argument");

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
    ASSERT_TRUE(suite, contains(text, ".ast <expr|chunk>"), "help should list .ast");
    ASSERT_TRUE(suite, contains(text, ".gc [stats|collect|strategy|help]"), "help should list .gc");
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

void testLinePromptDefaultsAndCustomPrompts(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    REPL::initialize(L);

    ASSERT_EQ(suite, Str("lua:1> "), REPL::detail::getPrompt(L, true, 1),
              "default first-line prompt should include the REPL line number");
    ASSERT_EQ(suite, Str("lua:2>> "), REPL::detail::getPrompt(L, false, 2),
              "default continuation prompt should include the REPL line number");

    StringPool& pool = L->getGlobalState().getStringPool();
    L->setGlobal("_PROMPT", Value(pool.intern("custom> ")));
    L->setGlobal("_PROMPT2", Value(pool.intern("custom>> ")));

    ASSERT_EQ(suite, Str("custom> "), REPL::detail::getPrompt(L, true, 10),
              "custom first-line prompt should remain unchanged");
    ASSERT_EQ(suite, Str("custom>> "), REPL::detail::getPrompt(L, false, 11),
              "custom continuation prompt should remain unchanged");

    delete L;
}

void testIncrementalParsingRecognizesRecoverableEofSources(TestSuite& suite) {
    struct Sample {
        const char* source;
        const char* completed;
        const char* label;
    };

    const Sample samples[] = {
        {"if true then\n",
         "if true then\nprint(1)\nend",
         "if block should wait for end"},
        {"while true do\n",
         "while true do\nbreak\nend",
         "while block should wait for end"},
        {"do\nlocal x = 1\n",
         "do\nlocal x = 1\nend",
         "do block should wait for end"},
        {"for i = 1, 3 do\n",
         "for i = 1, 3 do\nprint(i)\nend",
         "numeric for should wait for end"},
        {"for k, v in pairs(t) do\n",
         "for k, v in pairs(t) do\nprint(k, v)\nend",
         "generic for should wait for end"},
        {"function f(a)\n  return a\n",
         "function f(a)\n  return a\nend",
         "function statement should wait for end"},
        {"local function f(a)\n  return a\n",
         "local function f(a)\n  return a\nend",
         "local function should wait for end"},
        {"repeat\n  local x = 1\n",
         "repeat\n  local x = 1\nuntil true",
         "repeat block should wait for until"},
        {"local t = { name = \n",
         "local t = { name = 1 }",
         "table constructor should wait for a value"},
        {"return (\n",
         "return (1)",
         "parenthesized expression should wait for close"},
    };

    LuaState* L = LuaState::newState();

    for (const Sample& sample : samples) {
        auto incomplete = REPL::detail::prepareInputForExecution(L, sample.source, false);
        ASSERT_FALSE(suite, incomplete.has_value(), sample.label);
        if (!incomplete) {
            ASSERT_TRUE(suite, REPL::detail::isIncompleteInput(incomplete.error().what()),
                        sample.label);
        }

        auto completed = REPL::detail::prepareInputForExecution(L, sample.completed, false);
        ASSERT_TRUE(suite, completed.has_value(), sample.label);
    }

    delete L;
}

void testIncrementalParsingRejectsDefiniteSyntaxErrors(TestSuite& suite) {
    struct Sample {
        const char* source;
        const char* label;
    };

    const Sample samples[] = {
        {"return +\n", "operator without rhs is a definite syntax error"},
        {"local t = { name = }\n", "missing table value before brace is definite"},
        {"if true then )\n", "unexpected close paren is definite"},
        {"function f(a) until true\n", "wrong block closer is definite"},
    };

    LuaState* L = LuaState::newState();

    for (const Sample& sample : samples) {
        auto prepared = REPL::detail::prepareInputForExecution(L, sample.source, false);
        ASSERT_FALSE(suite, prepared.has_value(), sample.label);
        if (!prepared) {
            ASSERT_FALSE(suite, REPL::detail::isIncompleteInput(prepared.error().what()),
                         sample.label);
        }
    }

    delete L;
}

void testIncrementalParsingKeepsQuickExpressionMode(TestSuite& suite) {
    bool wasExplicitReturn = false;
    Str source = REPL::detail::tryAsExpression("=function(a)", wasExplicitReturn);

    ASSERT_TRUE(suite, wasExplicitReturn, "=function should enter expression mode");
    ASSERT_EQ(suite, Str("return function(a)"), source,
              "=function should be compiled as a return expression");

    LuaState* L = LuaState::newState();

    auto incomplete = REPL::detail::prepareInputForExecution(L, source, wasExplicitReturn);
    ASSERT_FALSE(suite, incomplete.has_value(),
                 "unfinished function expression should stay in incremental mode");
    if (!incomplete) {
        ASSERT_TRUE(suite, REPL::detail::isIncompleteInput(incomplete.error().what()),
                    "unfinished function expression should be recoverable");
    }

    source += "\n  return a\nend";
    auto completed = REPL::detail::prepareInputForExecution(L, source, wasExplicitReturn);
    ASSERT_TRUE(suite, completed.has_value(),
                "completed function expression should parse after continuation lines");
    if (completed) {
        ASSERT_TRUE(suite, completed->isExpression,
                    "completed quick expression should preserve expression printing mode");
    }

    delete L;
}

void testCompleteMetaCommand(TestSuite& suite) {
    const REPL::CompletionResult result = REPL::completeInput(nullptr, ".g");

    ASSERT_EQ(suite, Str(".gc"), result.completedLine, "meta completion should complete .gc");
    ASSERT_EQ(suite, static_cast<usize>(1), result.candidates.size(),
              "meta completion should return one candidate");
    ASSERT_EQ(suite, Str(".gc"), result.candidates[0], "meta completion candidate should be .gc");
}

void testCompleteGcOptionUsesCommonPrefix(TestSuite& suite) {
    const REPL::CompletionResult result = REPL::completeInput(nullptr, ".gc st");

    ASSERT_EQ(suite, Str(".gc st"), result.completedLine,
              "ambiguous .gc option should keep shared prefix");
    ASSERT_TRUE(suite, hasCandidate(result.candidates, "stats"),
                "gc option completion should include stats");
    ASSERT_TRUE(suite, hasCandidate(result.candidates, "status"),
                "gc option completion should include status alias");
    ASSERT_TRUE(suite, hasCandidate(result.candidates, "strategy"),
                "gc option completion should include strategy");
}

void testCompleteGlobalName(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);
    REPL::initialize(L);

    const REPL::CompletionResult result = REPL::completeInput(L, "pri");

    ASSERT_EQ(suite, Str("print"), result.completedLine,
              "global completion should complete print");
    ASSERT_EQ(suite, static_cast<usize>(1), result.candidates.size(),
              "global completion should return one candidate");
    ASSERT_EQ(suite, Str("print"), result.candidates[0],
              "global completion candidate should be print");

    delete L;
}

void testCompleteLibraryFieldName(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    StandardLibrary::openAll(L);
    REPL::initialize(L);

    const REPL::CompletionResult result = REPL::completeInput(L, "string.su");

    ASSERT_EQ(suite, Str("string.sub"), result.completedLine,
              "field completion should complete string.sub");
    ASSERT_EQ(suite, static_cast<usize>(1), result.candidates.size(),
              "field completion should return one candidate");
    ASSERT_EQ(suite, Str("string.sub"), result.candidates[0],
              "field completion candidate should be string.sub");

    delete L;
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

void testAstCommandPrintsExpressionTree(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    std::ostringstream out;
    std::ostringstream err;

    REPL::MetaCommand command;
    command.kind = REPL::MetaCommandKind::Ast;
    command.argument = "1 + 2 * 3";

    const int status = REPL::runMetaCommand(L, command, out, err);
    const std::string text = out.str();

    ASSERT_EQ(suite, 0, status, ".ast expression should succeed through dispatcher");
    ASSERT_TRUE(suite, contains(text, "AST"), "ast should print header");
    ASSERT_TRUE(suite, contains(text, "mode: expression"), "ast expression should mark expression mode");
    ASSERT_TRUE(suite, contains(text, "Chunk"), "ast should print chunk root");
    ASSERT_TRUE(suite, contains(text, "ReturnStmt"), "expression fallback should be a return statement");
    ASSERT_TRUE(suite, contains(text, "BinaryExpr op=Add"), "ast should print outer binary add");
    ASSERT_TRUE(suite, contains(text, "BinaryExpr op=Mul"), "ast should preserve precedence tree");
    ASSERT_TRUE(suite, contains(text, "NumberExpr value=1"), "ast should print number literal");
    ASSERT_TRUE(suite, err.str().empty(), ".ast expression should not emit errors");

    delete L;
}

void testAstCommandPrintsChunkTree(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    std::ostringstream out;
    std::ostringstream err;

    const int status = REPL::printAst(L, "local x = {name = \"lua\", 7}\nreturn x.name", out, err);
    const std::string text = out.str();

    ASSERT_EQ(suite, 0, status, ".ast chunk should succeed");
    ASSERT_TRUE(suite, contains(text, "mode: chunk"), "ast chunk should mark chunk mode");
    ASSERT_TRUE(suite, contains(text, "LocalStmt names=[x]"), "ast should print local names");
    ASSERT_TRUE(suite, contains(text, "TableExpr fields=2"), "ast should print table field count");
    ASSERT_TRUE(suite, contains(text, "StringExpr value=\"lua\""), "ast should print string literal");
    ASSERT_TRUE(suite, contains(text, "MemberExpr member=name"), "ast should print member access");
    ASSERT_TRUE(suite, err.str().empty(), ".ast chunk should not emit errors");

    delete L;
}

void testAstCommandRejectsMissingArgument(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    std::ostringstream out;
    std::ostringstream err;

    const int status = REPL::printAst(L, "   ", out, err);

    ASSERT_EQ(suite, 1, status, ".ast without an argument should fail");
    ASSERT_TRUE(suite, out.str().empty(), "empty .ast should not print AST");
    ASSERT_TRUE(suite, contains(err.str(), "usage: .ast <expr|chunk>"),
                "empty .ast should print usage");

    delete L;
}

void testGcCommandPrintsStats(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    std::ostringstream out;
    std::ostringstream err;

    const int status = REPL::printGc(L, "", out, err);
    const std::string text = out.str();

    ASSERT_EQ(suite, 0, status, ".gc stats should succeed");
    ASSERT_TRUE(suite, contains(text, "GC"), "gc should print header");
    ASSERT_TRUE(suite, contains(text, "command: stats"), "gc should default to stats");
    ASSERT_TRUE(suite, contains(text, "strategy: mark-sweep"), "gc should name active strategy");
    ASSERT_TRUE(suite, contains(text, "boundary: RuntimeServices.gc"), "gc should expose boundary");
    ASSERT_TRUE(suite, contains(text, "objects:"), "gc should print object count");
    ASSERT_TRUE(suite, contains(text, "memory bytes:"), "gc should print memory bytes");
    ASSERT_TRUE(suite, err.str().empty(), ".gc stats should not emit errors");

    delete L;
}

void testGcCommandPrintsStrategyBoundary(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    std::ostringstream out;
    std::ostringstream err;

    const int status = REPL::printGc(L, "strategy", out, err);
    const std::string text = out.str();

    ASSERT_EQ(suite, 0, status, ".gc strategy should succeed");
    ASSERT_TRUE(suite, contains(text, "active: mark-sweep"), "gc should print active strategy");
    ASSERT_TRUE(suite, contains(text, "available: mark-sweep"), "gc should print available strategy");
    ASSERT_TRUE(suite, contains(text, "planned: incremental"), "gc should print planned strategy");
    ASSERT_TRUE(suite, contains(text, "RuntimeServices.gc"), "gc should document service boundary");
    ASSERT_TRUE(suite, err.str().empty(), ".gc strategy should not emit errors");

    delete L;
}

void testGcCommandRejectsUnknownOption(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    std::ostringstream out;
    std::ostringstream err;

    const int status = REPL::printGc(L, "warp", out, err);

    ASSERT_EQ(suite, 1, status, ".gc unknown option should fail");
    ASSERT_TRUE(suite, out.str().empty(), "invalid .gc should not print stdout");
    ASSERT_TRUE(suite, contains(err.str(), ".gc: unknown option 'warp'"),
                "invalid .gc should name bad option");
    ASSERT_TRUE(suite, contains(err.str(), "usage: .gc [stats|collect|strategy|help]"),
                "invalid .gc should print usage");

    delete L;
}

void testReportErrorKeepsErrorFormat(TestSuite& suite) {
    std::ostringstream err;
    std::streambuf* oldBuffer = std::cerr.rdbuf(err.rdbuf());

    REPL::setProgName("C:\\tools\\lua_test.exe");
    REPL::reportError("chunk.lua", 17, "syntax boom", true);
    REPL::reportError("stdin", 3, "repl boom", false);

    std::cerr.rdbuf(oldBuffer);
    REPL::setProgName(nullptr);

    const std::string text = err.str();
    ASSERT_TRUE(
        suite,
        contains(text, "lua_test.exe: chunk.lua:17: syntax boom"),
        "script error should include program source line and message"
    );
    ASSERT_TRUE(
        suite,
        contains(text, "stdin:3: repl boom"),
        "repl error should omit program prefix"
    );
    ASSERT_TRUE(suite, !contains(text, "\x1b[31m"),
                "redirected error output should stay plain by default");
}

void testReportErrorColorModeCanBeForced(TestSuite& suite) {
    const REPL::ErrorColorMode oldMode = REPL::getErrorColorMode();
    std::ostringstream err;
    std::streambuf* oldBuffer = std::cerr.rdbuf(err.rdbuf());

    REPL::setErrorColorMode(REPL::ErrorColorMode::Always);
    REPL::reportError("stdin", 3, "color boom", false);

    std::cerr.rdbuf(oldBuffer);
    REPL::setErrorColorMode(oldMode);

    ASSERT_EQ(
        suite,
        std::string("\x1b[31mstdin:3: color boom\x1b[0m\n"),
        err.str(),
        "forced color mode should wrap reportError line in red"
    );
}

void testMetaCommandErrorColorModeCanBeForced(TestSuite& suite) {
    const REPL::ErrorColorMode oldMode = REPL::getErrorColorMode();
    std::ostringstream out;
    std::ostringstream err;

    REPL::MetaCommand command;
    command.kind = REPL::MetaCommandKind::Unknown;
    command.argument = "wat";

    REPL::setErrorColorMode(REPL::ErrorColorMode::Always);
    const int status = REPL::runMetaCommand(nullptr, command, out, err);
    REPL::setErrorColorMode(oldMode);

    ASSERT_EQ(suite, 1, status, "unknown REPL command should still fail");
    ASSERT_TRUE(suite, out.str().empty(), "colored unknown command should not write stdout");
    ASSERT_EQ(
        suite,
        std::string("\x1b[31munknown REPL command: .wat\x1b[0m\n"),
        err.str(),
        "forced color mode should wrap meta command errors in red"
    );
}

void testUnknownMetaCommandErrorFormat(TestSuite& suite) {
    LuaState* L = LuaState::newState();
    std::ostringstream out;
    std::ostringstream err;

    REPL::MetaCommand command;
    command.kind = REPL::MetaCommandKind::Unknown;
    command.argument = "wat";

    const int status = REPL::runMetaCommand(L, command, out, err);

    ASSERT_EQ(suite, 1, status, "unknown REPL command should fail");
    ASSERT_TRUE(suite, out.str().empty(), "unknown REPL command should not write stdout");
    ASSERT_EQ(
        suite,
        std::string("unknown REPL command: .wat\n"),
        err.str(),
        "unknown REPL command error format is stable"
    );

    delete L;
}

} // namespace

void registerReplCommandTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest(kSuiteName, "Parse Meta Commands", testParseMetaCommands);
    registry.registerTest(kSuiteName, "Print Help Shows Supported Commands", testPrintHelpShowsSupportedCommands);
    registry.registerTest(kSuiteName, "History Round Trip", testHistoryRoundTrip);
    registry.registerTest(kSuiteName, "Line Prompt Defaults And Custom Prompts",
                          testLinePromptDefaultsAndCustomPrompts);
    registry.registerTest(kSuiteName, "Incremental Parsing Recognizes Recoverable EOF Sources",
                          testIncrementalParsingRecognizesRecoverableEofSources);
    registry.registerTest(kSuiteName, "Incremental Parsing Rejects Definite Syntax Errors",
                          testIncrementalParsingRejectsDefiniteSyntaxErrors);
    registry.registerTest(kSuiteName, "Incremental Parsing Keeps Quick Expression Mode",
                          testIncrementalParsingKeepsQuickExpressionMode);
    registry.registerTest(kSuiteName, "Complete Meta Command", testCompleteMetaCommand);
    registry.registerTest(kSuiteName, "Complete GC Option Uses Common Prefix",
                          testCompleteGcOptionUsesCommonPrefix);
    registry.registerTest(kSuiteName, "Complete Global Name", testCompleteGlobalName);
    registry.registerTest(kSuiteName, "Complete Library Field Name", testCompleteLibraryFieldName);
    registry.registerTest(kSuiteName, "Bytecode Command Prints Compiled Expression",
                          testBytecodeCommandPrintsCompiledExpression);
    registry.registerTest(kSuiteName, "Bytecode Command Rejects Missing Argument",
                          testBytecodeCommandRejectsMissingArgument);
    registry.registerTest(kSuiteName, "AST Command Prints Expression Tree",
                          testAstCommandPrintsExpressionTree);
    registry.registerTest(kSuiteName, "AST Command Prints Chunk Tree",
                          testAstCommandPrintsChunkTree);
    registry.registerTest(kSuiteName, "AST Command Rejects Missing Argument",
                          testAstCommandRejectsMissingArgument);
    registry.registerTest(kSuiteName, "GC Command Prints Stats",
                          testGcCommandPrintsStats);
    registry.registerTest(kSuiteName, "GC Command Prints Strategy Boundary",
                          testGcCommandPrintsStrategyBoundary);
    registry.registerTest(kSuiteName, "GC Command Rejects Unknown Option",
                          testGcCommandRejectsUnknownOption);
    registry.registerTest(kSuiteName, "Report Error Format", testReportErrorKeepsErrorFormat);
    registry.registerTest(kSuiteName, "Report Error Color Mode Can Be Forced",
                          testReportErrorColorModeCanBeForced);
    registry.registerTest(kSuiteName, "Meta Command Error Color Mode Can Be Forced",
                          testMetaCommandErrorColorModeCanBeForced);
    registry.registerTest(kSuiteName, "Unknown Meta Command Error Format", testUnknownMetaCommandErrorFormat);
}
