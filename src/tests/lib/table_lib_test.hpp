#pragma once

#include "../../common/types.hpp"
#include <iostream>
#include <string>

namespace Lua {
namespace Tests {

    /**
     * @brief Table库测试类
     * 
     * 测试Lua table库的所有功能，包括：
     * - table.insert: 插入元素
     * - table.remove: 移除元素
     * - table.concat: 连接元素
     * - table.sort: 排序
     * - table.pack: 打包参数
     * - table.unpack: 解包表
     * - table.move: 移动元素
     * - table.maxn: 最大索引
     */
    class TableLibTest {
    public:
        static void runAllTests();
        
    private:
        // Individual test methods
        static void testInsert();
        static void testRemove();
        static void testConcat();
        static void testSort();
        static void testPack();
        static void testUnpack();
        static void testMove();
        static void testMaxn();
        
        // Helper methods
        static void printTestHeader(const std::string& testName);
        static void printTestResult(const std::string& testName, bool passed);
        static void printTestResult(const std::string& testName, bool passed, const std::exception& e);
        static void assertTrue(bool condition, const std::string& message);
    };

} // namespace Tests
} // namespace Lua
