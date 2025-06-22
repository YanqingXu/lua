#ifndef LUA_TESTS_MAIN_NEW_HPP
#define LUA_TESTS_MAIN_NEW_HPP

// 新测试框架 - 统一入口
#include "../test_framework/test_framework.hpp"

// 包含所有测试模块头文件
#include "lexer/test_lexer.hpp"
#include "vm/test_vm.hpp"
#include "parser/test_parser.hpp"
#include "compiler/test_compiler.hpp"
#include "gc/test_gc.hpp"
#include "lib/test_lib.hpp"
#include "localization/localization_test.hpp"
#include "plugin/plugin_integration_test.hpp"

namespace Lua {
namespace Tests {

/**
 * @brief 主测试类 - 使用新的测试框架
 * 
 * 这是使用重构后测试框架的主入口点。
 * 提供了更清晰的模块化测试执行和更好的错误处理。
 */
class MainTestSuite {
public:
    /**
     * @brief 运行所有测试模块
     * 
     * 这是整个测试套件的主入口点。
     * 使用新的测试框架运行所有主要模块的测试。
     * 
     * 测试层次结构:
     * MAIN (runAllTests) -> MODULE (各模块测试套件) -> SUITE -> GROUP -> INDIVIDUAL
     */
    static void runAllTests() {
        using namespace Lua::TestFramework;
        
        // 配置测试框架
        TestUtils::setColorEnabled(true);
        TestUtils::setMemoryCheckEnabled(true);
        
        try {
            // 运行所有模块测试（包括各自的错误处理测试）
            RUN_TEST_MODULE("Lexer Module", LexerTestSuite);
            RUN_TEST_MODULE("Parser Module", ParserTestSuite);
            RUN_TEST_MODULE("Compiler Module", CompilerTestSuite);
            RUN_TEST_MODULE("VM Module", VMTestSuite);
            RUN_TEST_MODULE("GC Module", GCTestSuite);
            RUN_TEST_MODULE("Library Module", LibTestSuite);
            //RUN_TEST_MODULE("Localization Module", LocalizationTestSuite);
            //RUN_TEST_MODULE("Plugin Module", PluginTestSuite);
            
            // 打印最终统计报告
            TestUtils::printStatisticsReport();
            
        } catch (const std::exception& e) {
            TestUtils::printError("Fatal error during test execution: " + std::string(e.what()));
            throw;
        } catch (...) {
            TestUtils::printError("Unknown fatal error during test execution");
            throw;
        }
    }
    
    /**
     * @brief 运行特定模块的测试
     * @param moduleName 模块名称
     */
    static void runModuleTests(const std::string& moduleName) {
        using namespace Lua::TestFramework;
        
        TestUtils::setColorEnabled(true);
        TestUtils::setMemoryCheckEnabled(true);
        
        if (moduleName == "lexer") {
            RUN_TEST_MODULE("Lexer Module", LexerTestSuite);
        } else if (moduleName == "parser") {
            RUN_TEST_MODULE("Parser Module", ParserTestSuite);
        } else if (moduleName == "compiler") {
            RUN_TEST_MODULE("Compiler Module", CompilerTestSuite);
        } else if (moduleName == "vm") {
            RUN_TEST_MODULE("VM Module", VMTestSuite);
        } else if (moduleName == "gc") {
            RUN_TEST_MODULE("GC Module", GCTestSuite);
        } else if (moduleName == "lib") {
            RUN_TEST_MODULE("Library Module", LibTestSuite);
        //} else if (moduleName == "localization") {
        //    RUN_TEST_MODULE("Localization Module", LocalizationTestSuite);
        //} else if (moduleName == "plugin") {
        //    RUN_TEST_MODULE("Plugin Module", PluginTestSuite);
        } else {
            TestUtils::printError("Unknown module: " + moduleName);
            TestUtils::printInfo("Available modules: lexer, parser, compiler, vm, gc, lib, localization, plugin");
        }
    }
    
    /**
     * @brief 运行快速测试（仅核心功能）
     */
    static void runQuickTests() {
        using namespace Lua::TestFramework;
        
        TestUtils::setColorEnabled(true);
        TestUtils::setMemoryCheckEnabled(false); // 快速测试跳过内存检查
        
        TestUtils::printInfo("Running quick tests (core functionality only)...");
        
        RUN_TEST_MODULE("Lexer Module", LexerTestSuite);
        RUN_TEST_MODULE("Parser Module", ParserTestSuite);
        RUN_TEST_MODULE("VM Module", VMTestSuite);
        
        TestUtils::printStatisticsReport();
    }
};

/**
 * @brief 便捷函数：运行所有测试
 */
inline void runAllTests() {
    RUN_MAIN_TEST("Lua Interpreter Test Suite", MainTestSuite::runAllTests);
}

/**
 * @brief 便捷函数：运行特定模块测试
 * @param moduleName 模块名称
 */
inline void runModuleTests(const std::string& moduleName) {
    MainTestSuite::runModuleTests(moduleName);
}

/**
 * @brief 便捷函数：运行快速测试
 */
inline void runQuickTests() {
    RUN_MAIN_TEST("Lua Interpreter Quick Test Suite", MainTestSuite::runQuickTests);
}

} // namespace Tests
} // namespace Lua

#endif // LUA_TESTS_MAIN_NEW_HPP