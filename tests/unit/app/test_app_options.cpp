#include "../framework/test_framework.hpp"
#include "app/app_options.hpp"

#include <cstring>

using namespace Lua;
using namespace LuaTest;

static void testParseArgsModes(TestSuite& suite) {
    {
        char* argv[] = {const_cast<char*>("lua")};
        const AppOptions opt = parseArgs(1, argv);

        ASSERT_EQ(suite, RunMode::DefaultBehavior, opt.mode, "No args should use default behavior");
        ASSERT_TRUE(suite, !opt.scriptFile.has_value(), "Default mode has no script");
        ASSERT_TRUE(suite, !opt.traceFile.has_value(), "Default mode has no trace file");
        ASSERT_TRUE(suite, !opt.traceDiff, "Default mode has trace diff disabled");
        ASSERT_EQ(suite, static_cast<i32>(-1), opt.scriptIndex, "Default mode has no script index");
    }

    {
        char* argv[] = {const_cast<char*>("lua"), const_cast<char*>("-v")};
        const AppOptions opt = parseArgs(2, argv);

        ASSERT_EQ(suite, RunMode::ShowVersion, opt.mode, "-v should select version mode");
    }

    {
        char* argv[] = {const_cast<char*>("lua"), const_cast<char*>("-h")};
        const AppOptions opt = parseArgs(2, argv);

        ASSERT_EQ(suite, RunMode::ShowHelp, opt.mode, "-h should select help mode");
    }

    {
        char* argv[] = {const_cast<char*>("lua"), const_cast<char*>("-i")};
        const AppOptions opt = parseArgs(2, argv);

        ASSERT_EQ(suite, RunMode::Repl, opt.mode, "-i should select REPL mode");
    }

    {
        char* argv[] = {const_cast<char*>("lua"), const_cast<char*>("script.lua"), const_cast<char*>("-i")};
        const AppOptions opt = parseArgs(3, argv);

        ASSERT_EQ(suite, RunMode::Script, opt.mode, "First non-option should select script mode");
        ASSERT_TRUE(suite, opt.scriptFile.has_value(), "Script path should be recorded");
        ASSERT_TRUE(suite, opt.scriptFile.value() == "script.lua", "Script path should match");
        ASSERT_EQ(suite, static_cast<i32>(1), opt.scriptIndex, "Script index should be recorded");
    }

    {
        char* argv[] = {const_cast<char*>("lua"), const_cast<char*>("--trace"), const_cast<char*>("trace.jsonl"),
                        const_cast<char*>("script.lua")};
        const AppOptions opt = parseArgs(4, argv);

        ASSERT_EQ(suite, RunMode::Script, opt.mode, "--trace before script should still run script mode");
        ASSERT_TRUE(suite, opt.traceFile.has_value(), "Trace path should be recorded");
        ASSERT_TRUE(suite, opt.traceFile.value() == "trace.jsonl", "Trace path should match");
        ASSERT_TRUE(suite, !opt.traceDiff, "Plain trace should leave trace diff disabled");
        ASSERT_TRUE(suite, opt.scriptFile.has_value(), "Script path after trace should be recorded");
        ASSERT_TRUE(suite, opt.scriptFile.value() == "script.lua", "Script path after trace should match");
        ASSERT_EQ(suite, static_cast<i32>(3), opt.scriptIndex, "Script index after trace should be recorded");
    }

    {
        char* argv[] = {const_cast<char*>("lua"), const_cast<char*>("--trace-diff"),
                        const_cast<char*>("trace-diff.jsonl"), const_cast<char*>("script.lua")};
        const AppOptions opt = parseArgs(4, argv);

        ASSERT_EQ(suite, RunMode::Script, opt.mode, "--trace-diff before script should still run script mode");
        ASSERT_TRUE(suite, opt.traceFile.has_value(), "Trace diff path should be recorded");
        ASSERT_TRUE(suite, opt.traceFile.value() == "trace-diff.jsonl", "Trace diff path should match");
        ASSERT_TRUE(suite, opt.traceDiff, "--trace-diff should enable trace diff mode");
        ASSERT_TRUE(suite, opt.scriptFile.has_value(), "Script path after trace diff should be recorded");
        ASSERT_TRUE(suite, opt.scriptFile.value() == "script.lua", "Script path after trace diff should match");
        ASSERT_EQ(suite, static_cast<i32>(3), opt.scriptIndex, "Script index after trace diff should be recorded");
    }

    {
        char* argv[] = {const_cast<char*>("lua"), const_cast<char*>("-v"), const_cast<char*>("-h")};
        const AppOptions opt = parseArgs(3, argv);

        ASSERT_EQ(suite, RunMode::ShowVersion, opt.mode, "-v should keep existing precedence over -h");
    }
}

void registerAppOptionsTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("AppOptions", "Parse CLI modes", testParseArgsModes);
}
