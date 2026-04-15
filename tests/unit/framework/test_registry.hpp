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
 * @brief 注册条件/短路护栏测试
 */
void registerCodegenConditionTests();

/**
 * @brief 注册多返回值护栏测试
 */
void registerCodegenMultiRetTests();

/**
 * @brief 注册 PR-1 结果类型兼容测试
 */
void registerCodegenResultTypeTests();

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

/*
* @brief 注册语法糖测试
*/
void registerSyntaxSugarTests();

/**
 * @brief 注册所有表索引访问测试
 */
void registerIndexedAccessTests();

/**
 * @brief 注册所有方法调用测试
 */
void registerMethodCallTests();

/**
 * @brief 注册所有变量存储测试
 */
void registerStorevarTests();

/**
 * @brief 注册 LValue Pipeline 测试（PR-3）
 */
void registerLValuePipelineTests();

/**
 * @brief 注册 ValueResult Pipeline 测试（PR-4）
 */
void registerValuePipelineTests();

/**
 * @brief 注册词法分析器测试
 */
void registerLexerNumberTests();

/**
 * @brief 注册词法分析器Token预读测试
 */
void registerLexerLookaheadTests();

/**
 * @brief 注册解析器测试
 */
void registerParserRecursionTests();

/*
* @brief 注册解析器错误恢复测试
*/
void registerParserErrorRecoveryTests();

/**
 * @brief 注册解析器内存池测试
 */
void registerParserMemoryPoolTests();

/**
 * @brief 注册DynamicBuffer类测试
 */
void registerDynamicBufferTests();

/**
 * @brief 注册InputStream类测试
 */
void registerInputStreamStringTests();

/**
 * @brief 注册InputStream流模式测试
 */
void registerInputStreamStreamTests();

/**
 * @brief 注册InputStream文件模式测试
 */
void registerInputStreamFileTests();

/**
 * @brief 注册LuaState初始化测试
 */
void registerLuaStateInitTests();

/**
 * @brief 注册数学库测试
 */
void registerMathLibTests();

/**
 * @brief 注册字符串库测试
 */
void registerStringLibTests();

void registerCoroutineLibTests();

void registerDebugLibTests();

void registerPackageLibTests();

/**
 * @brief 注册 Call Pipeline 测试（PR-5）
 */
void registerCallPipelineTests();

#endif // LUA_TEST_REGISTRY_HPP

