// compatibility_tests.cpp
#include <gtest/gtest.h>
#include "vm/state.hpp"
#include "lib/base_lib.hpp"
#include <fstream>
#include <string>

using namespace Lua;

// 标准Lua测试套件的测试用例
class CompatibilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        state = std::make_unique<State>();
        registerBaseLib(state.get());
        // 注册所有标准库
        // registerStringLib(state.get());
        // registerTableLib(state.get());
        // registerMathLib(state.get());
        // ...
    }
    
    // 从文件加载Lua代码
    std::string loadFile(const std::string& path) {
        std::ifstream file(path);
        EXPECT_TRUE(file) << "Failed to open file: " << path;
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    std::unique_ptr<State> state;
};

// 测试兼容性 - 运行官方测试套件的部分测试
TEST_F(CompatibilityTest, OfficialTestSuite) {
    // 这里应该使用官方的测试套件
    // 例如，可以从Lua官方仓库复制一些测试用例
    const char* officialTests[] = {
        // "attrib.lua",
        // "calls.lua",
        // "closure.lua",
        // ...
    };
    
    for (const auto& testName : officialTests) {
        std::string path = "tests/lua-tests/" + std::string(testName);
        std::string code = loadFile(path);
        EXPECT_NO_THROW(state->doString(code)) << "Test failed: " << testName;
    }
}

// 测试特定的Lua行为
TEST_F(CompatibilityTest, SpecificBehaviors) {
    // 测试Lua特有的一些行为
    
    // 多返回值
    state->doString(R"(
        function returns_multiple()
            return 1, "two", true
        end
        
        local a, b, c = returns_multiple()
        result_a, result_b, result_c = a, b, c
    )");
    
    EXPECT_DOUBLE_EQ(1, state->getGlobal("result_a").asNumber());
    EXPECT_EQ("two", state->getGlobal("result_b").asString());
    EXPECT_TRUE(state->getGlobal("result_c").asBoolean());
    
    // 可变参数
    state->doString(R"(
        function sum(...)
            local args = {...}
            local total = 0
            for i=1,#args do
                total = total + args[i]
            end
            return total
        end
        
        result = sum(1, 2, 3, 4, 5)
    )");
    
    EXPECT_DOUBLE_EQ(15, state->getGlobal("result").asNumber());
}