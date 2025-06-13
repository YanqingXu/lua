#pragma once

#include "../../common/types.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

    /**
     * @brief 标准库测试统一入口
     * 
     * 协调运行所有标准库相关的测试，包括：
     * - BaseLib: 基础库测试
     * - StringLib: 字符串库测试  
     * - TableLib: 表库测试
     * - MathLib: 数学库测试 (未来)
     * - IOLib: IO库测试 (未来)
     */
    class LibTestSuite {
    public:
        static void runAllTests();
        
    private:
        static void printSectionHeader(const std::string& sectionName);
        static void printSectionFooter();
    };

} // namespace Tests
} // namespace Lua
