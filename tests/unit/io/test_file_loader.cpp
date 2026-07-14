#include "../framework/test_framework.hpp"
#include "common/types.hpp"
#include "io/file_loader.hpp"
#include "test_file_fixture.hpp"

#include <cstdio>

using namespace Lua;
using namespace LuaTest;

static void removeTestFile(const Str& path) {
    std::remove(path.c_str());
}

static void testReadTextFile(TestSuite& suite) {
    const Str path = "build/test_file_loader_text.lua";
    const Str content = "print('hello')\nreturn 42\n";

    TemporaryTestFile testFile(path, content);
    const Str result = readWholeFile(path);
    ASSERT_EQ(suite, content, result, "Text file content should match");
}

static void testReadEmptyFile(TestSuite& suite) {
    const Str path = "build/test_file_loader_empty.lua";

    TemporaryTestFile testFile(path, "");
    const Str result = readWholeFile(path);
    ASSERT_TRUE(suite, result.empty(), "Empty file should produce empty string");
}

static void testReadBinaryFile(TestSuite& suite) {
    const Str path = "build/test_file_loader_binary.bin";

    Str content;
    content.push_back('A');
    content.push_back('\0');
    content.push_back('B');
    content.push_back('\0');
    content.push_back('C');

    TemporaryTestFile testFile(path, content);
    const Str result = readWholeFile(path);
    ASSERT_EQ(suite, content.size(), result.size(), "Binary file size should match");
    ASSERT_EQ(suite, content, result, "Binary file content should preserve null bytes");
}

static void testMissingFileThrows(TestSuite& suite) {
    const Str path = "build/test_file_loader_missing.lua";
    removeTestFile(path);

    bool exceptionThrown = false;
    Str exceptionMessage;

    try {
        (void)readWholeFile(path);
    } catch (const std::runtime_error& e) {
        exceptionThrown = true;
        exceptionMessage = e.what();
    }

    ASSERT_TRUE(suite, exceptionThrown, "Missing file should throw");
    ASSERT_TRUE(suite, exceptionMessage.find(path) != Str::npos, "Exception message should contain file path");
}

void registerFileLoaderTests() {
    auto& registry = TestRegistry::getInstance();

    registry.registerTest("FileLoader", "Read text file", testReadTextFile);
    registry.registerTest("FileLoader", "Read empty file", testReadEmptyFile);
    registry.registerTest("FileLoader", "Read binary file", testReadBinaryFile);
    registry.registerTest("FileLoader", "Missing file throws", testMissingFileThrows);
}
