#pragma once

#include "../../common/types.hpp"

namespace Lua::Tests {

/**
 * @brief 新 Base Library 测试套件
 * 
 * 展示统一架构设计的测试覆盖和使用方式
 * 测试内容包括：
 * - 核心函数：print, type, tostring, tonumber, error, assert
 * - 表操作：pairs, ipairs, next
 * - 元表操作：getmetatable, setmetatable
 * - 原始访问：rawget, rawset, rawlen, rawequal
 * - 错误处理：pcall, xpcall
 * - 工具函数：select, unpack
 */
class BaseLibTestSuite {
public:
    /**
     * @brief 运行所有新架构的基础库测试
     * 
     * 执行完整的测试套件，验证统一架构的正确性
     */
    static void runAllTests();
};

} // namespace Lua::Tests
