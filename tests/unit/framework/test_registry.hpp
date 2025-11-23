/**
 * @file test_registry.hpp
 * @brief 测试注册函数声明 - 供main.cpp使用
 * 
 * 这个头文件声明了所有测试注册函数，使得main.cpp可以调用这些函数
 * 而不需要包含测试实现文件。
 * 
 * @author Lua C++ Project
 * @date 2025-11-14
 */

#ifndef LUA_TEST_REGISTRY_HPP
#define LUA_TEST_REGISTRY_HPP

// 声明所有测试注册函数
// 这些函数在各自的测试文件中实现

/**
 * @brief 注册Value类测试
 */
void registerValueTests();

/**
 * @brief 注册GCString和StringPool测试
 */
void registerGCStringTests();

/**
 * @brief 注册Table类测试
 */
void registerTableTests();

/**
 * @brief 注册VM核心测试（GlobalState、Stack、CallInfo、LuaState）
 */
void registerVMCoreTests();

/**
 * @brief 注册Function和Proto测试
 */
void registerFunctionTests();

/**
 * @brief 注册GC系统测试（GCObject、GarbageCollector、Upvalue）
 */
void registerGCTests();

/**
 * @brief 注册二元和一元表达式代码生成测试
 */
void registerBinaryUnaryExprTests();

/**
 * @brief 注册函数定义和调用代码生成测试
 */
void registerFunctionCodegenTests();

/**
 * @brief 注册基础库函数测试
 */
void registerBaselibTests();

/**
 * @brief 注册Lua文件编译测试
 */
void registerLuaFunctionTests();

/**
 * @brief 注册算术元方法测试
 */
void registerMetamethodArithTests();

/**
 * @brief 注册完整元方法测试（包含更多元方法类型）
 */
void registerMetamethodCompleteTests();

#endif // LUA_TEST_REGISTRY_HPP

