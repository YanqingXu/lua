#include "lib_manager.hpp"
#include "base_lib.hpp"
#include "math_lib.hpp"
#include "error_handling.hpp"
#include "type_conversion.hpp"
#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include <iostream>
#include <chrono>
#include <cassert>

namespace Lua {
    /**
     * 测试用的自定义库模块
     * 展示types.hpp类型系统的完整使用
     */
    class TestLib : public LibModule {
    public:
        StrView getName() const noexcept override {
            return "test";
        }
        
        void registerFunctions(FunctionRegistry& registry, const LibraryContext& context) override {
            // 测试基础类型转换
            REGISTER_SAFE_FUNCTION(registry, test_types, testTypes);
            REGISTER_SAFE_FUNCTION(registry, test_containers, testContainers);
            REGISTER_SAFE_FUNCTION(registry, test_error_handling, testErrorHandling);
            REGISTER_SAFE_FUNCTION(registry, test_performance, testPerformance);
            
            // 测试参数提取器
            registry.registerFunction("test_extract", [](State* state, i32 nargs) -> Value {
                try {
                    auto [a, b, c, d, e] = EXTRACT_ARGS(state, nargs, "test_extract", 
                                                       i32, f64, Str, bool, u32);
                    
                    Str result = "Extracted: " + std::to_string(a) + ", " + 
                                std::to_string(b) + ", " + c + ", " + 
                                (d ? "true" : "false") + ", " + std::to_string(e);
                    
                    return Value(result);
                } catch (const LibException& e) {
                    state->error(e.what());
                    return Value(nullptr);
                }
            });
            
            // 测试Lua特定类型
            registry.registerFunction("test_lua_types", [](State* state, i32 nargs) -> Value {
                try {
                    ErrorUtils::checkArgCount(nargs, 3, "test_lua_types");
                    
                    auto luaInt = TypeConverter::toLuaInteger(state->getArg(0), "test_lua_types");
                    auto luaNum = TypeConverter::toLuaNumber(state->getArg(1), "test_lua_types");
                    auto luaBool = TypeConverter::toLuaBoolean(state->getArg(2), "test_lua_types");
                    
                    Str result = "LuaTypes: " + std::to_string(luaInt) + ", " + 
                                std::to_string(luaNum) + ", " + 
                                (luaBool ? "true" : "false");
                    
                    return Value(result);
                } catch (const LibException& e) {
                    state->error(e.what());
                    return Value(nullptr);
                }
            });
        }
        
    private:
        static Value testTypes(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "test_types");
            
            auto value = state->getArg(0);
            Vec<Str> results;
            
            // 测试所有类型转换
            try {
                auto i8Val = TypeConverter::toI8(value, "test_types");
                results.push_back("i8: " + std::to_string(i8Val));
            } catch (const LibException&) {
                results.push_back("i8: conversion failed");
            }
            
            try {
                auto i16Val = TypeConverter::toI16(value, "test_types");
                results.push_back("i16: " + std::to_string(i16Val));
            } catch (const LibException&) {
                results.push_back("i16: conversion failed");
            }
            
            try {
                auto i32Val = TypeConverter::toI32(value, "test_types");
                results.push_back("i32: " + std::to_string(i32Val));
            } catch (const LibException&) {
                results.push_back("i32: conversion failed");
            }
            
            try {
                auto i64Val = TypeConverter::toI64(value, "test_types");
                results.push_back("i64: " + std::to_string(i64Val));
            } catch (const LibException&) {
                results.push_back("i64: conversion failed");
            }
            
            try {
                auto u32Val = TypeConverter::toU32(value, "test_types");
                results.push_back("u32: " + std::to_string(u32Val));
            } catch (const LibException&) {
                results.push_back("u32: conversion failed");
            }
            
            try {
                auto f32Val = TypeConverter::toF32(value, "test_types");
                results.push_back("f32: " + std::to_string(f32Val));
            } catch (const LibException&) {
                results.push_back("f32: conversion failed");
            }
            
            try {
                auto f64Val = TypeConverter::toF64(value, "test_types");
                results.push_back("f64: " + std::to_string(f64Val));
            } catch (const LibException&) {
                results.push_back("f64: conversion failed");
            }
            
            try {
                auto strVal = TypeConverter::toString(value, "test_types");
                results.push_back("string: " + strVal);
            } catch (const LibException&) {
                results.push_back("string: conversion failed");
            }
            
            try {
                auto boolVal = TypeConverter::toBool(value, "test_types");
                results.push_back("bool: " + Str(boolVal ? "true" : "false"));
            } catch (const LibException&) {
                results.push_back("bool: conversion failed");
            }
            
            // 组合结果
            Str result = "Type conversion results:\n";
            for (const auto& res : results) {
                result += res + "\n";
            }
            
            return Value(result);
        }
        
        static Value testContainers(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 0, "test_containers");
            
            // 测试Vec
            Vec<i32> intVec = {1, 2, 3, 4, 5};
            Vec<Str> strVec = {"hello", "world", "test"};
            
            // 测试HashMap
            HashMap<Str, i32> intMap = {
                {"one", 1},
                {"two", 2},
                {"three", 3}
            };
            
            HashMap<Str, f64> floatMap = {
                {"pi", 3.14159},
                {"e", 2.71828},
                {"sqrt2", 1.41421}
            };
            
            // 测试HashSet
            HashSet<Str> strSet = {"apple", "banana", "cherry"};
            
            Str result = "Container tests:\n";
            
            // Vec测试
            result += "Vec<i32> size: " + std::to_string(intVec.size()) + "\n";
            result += "Vec<Str> size: " + std::to_string(strVec.size()) + "\n";
            
            // HashMap测试
            result += "HashMap<Str, i32> size: " + std::to_string(intMap.size()) + "\n";
            result += "HashMap<Str, f64> size: " + std::to_string(floatMap.size()) + "\n";
            
            // HashSet测试
            result += "HashSet<Str> size: " + std::to_string(strSet.size()) + "\n";
            
            // 测试容器操作
            if (intMap.find("two") != intMap.end()) {
                result += "Found 'two' in intMap: " + std::to_string(intMap["two"]) + "\n";
            }
            
            if (strSet.find("banana") != strSet.end()) {
                result += "Found 'banana' in strSet\n";
            }
            
            return Value(result);
        }
        
        static Value testErrorHandling(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "test_error_handling");
            
            auto testType = TypeConverter::toString(state->getArg(0), "test_error_handling");
            
            try {
                if (testType == "arg_count") {
                    ErrorUtils::checkArgCount(0, 5, "test_function");
                } else if (testType == "null_ptr") {
                    i32* nullPtr = nullptr;
                    ErrorUtils::checkNotNull(nullPtr, "test_pointer");
                } else if (testType == "bounds") {
                    Vec<i32> vec = {1, 2, 3};
                    ErrorUtils::checkBounds(10, vec, "test_vector");
                } else if (testType == "type_mismatch") {
                    throw LibException(LibErrorCode::TypeMismatch, "Test type mismatch error");
                } else if (testType == "out_of_range") {
                    throw LibException(LibErrorCode::OutOfRange, "Test out of range error");
                } else {
                    return Value("No error triggered for type: " + testType);
                }
            } catch (const LibException& e) {
                Str result = "Caught LibException: ";
                result += "Code: " + Str(e.getErrorCodeString()) + ", ";
                result += "Message: " + Str(e.what());
                return Value(result);
            }
            
            return Value("No exception thrown");
        }
        
        static Value testPerformance(State* state, i32 nargs) {
            ErrorUtils::checkArgCount(nargs, 1, "test_performance");
            
            auto iterations = TypeConverter::toI32(state->getArg(0), "test_performance");
            
            // 测试类型转换性能
            auto start = std::chrono::high_resolution_clock::now();
            
            for (i32 i = 0; i < iterations; ++i) {
                Value testValue(static_cast<f64>(i));
                
                // 执行多种类型转换
                auto i32Val = TypeConverter::toI32(testValue, "perf_test");
                auto f64Val = TypeConverter::toF64(testValue, "perf_test");
                auto strVal = TypeConverter::toString(testValue, "perf_test");
                auto boolVal = TypeConverter::toBool(testValue, "perf_test");
                
                // 防止编译器优化
                volatile auto dummy = i32Val + static_cast<i32>(f64Val) + 
                                     static_cast<i32>(strVal.length()) + 
                                     (boolVal ? 1 : 0);
                (void)dummy;
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            Str result = "Performance test completed:\n";
            result += "Iterations: " + std::to_string(iterations) + "\n";
            result += "Time: " + std::to_string(duration.count()) + " microseconds\n";
            result += "Average: " + std::to_string(static_cast<f64>(duration.count()) / iterations) + " μs/iteration";
            
            return Value(result);
        }
    };
    
    /**
     * 集成测试函数
     */
    void runIntegrationTests() {
        std::cout << "=== LibModule V2 Integration Tests ===\n\n";
        
        try {
            // 创建库管理器
            auto manager = createStandardLibManager();
            
            // 注册测试库
            auto testLib = make_unique<TestLib>();
            manager->registerModule(std::move(testLib));
            
            // 注册数学库
            auto mathLib = make_unique<MathLib>();
            manager->registerModule(std::move(mathLib));
            
            // 注册错误处理库
            auto errorLib = make_unique<ErrorHandlingLib>();
            manager->registerModule(std::move(errorLib));
            
            // 注册类型转换库
            auto typeLib = make_unique<TypeConversionLib>();
            manager->registerModule(std::move(typeLib));
            
            std::cout << "1. 库注册测试\n";
            auto modules = manager->getLoadedModules();
            std::cout << "   已加载模块数量: " << modules.size() << "\n";
            for (const auto& module : modules) {
                std::cout << "   - " << module << "\n";
            }
            std::cout << "\n";
            
            // 加载所有模块
            manager->loadAllModules();
            
            std::cout << "2. 函数查找测试\n";
            Vec<Str> testFunctions = {
                "base.print", "math.sqrt", "math.pi", "test.test_types",
                "typeconv.toint32", "error.checktype"
            };
            
            for (const auto& funcName : testFunctions) {
                bool exists = manager->hasFunction(funcName);
                std::cout << "   " << funcName << ": " << (exists ? "存在" : "不存在") << "\n";
            }
            std::cout << "\n";
            
            std::cout << "3. 类型系统测试\n";
            
            // 测试基础类型
            std::cout << "   基础类型大小:\n";
            std::cout << "   - i8: " << sizeof(i8) << " bytes\n";
            std::cout << "   - i16: " << sizeof(i16) << " bytes\n";
            std::cout << "   - i32: " << sizeof(i32) << " bytes\n";
            std::cout << "   - i64: " << sizeof(i64) << " bytes\n";
            std::cout << "   - f32: " << sizeof(f32) << " bytes\n";
            std::cout << "   - f64: " << sizeof(f64) << " bytes\n";
            std::cout << "   - usize: " << sizeof(usize) << " bytes\n";
            std::cout << "\n";
            
            // 测试Lua类型
            std::cout << "   Lua类型大小:\n";
            std::cout << "   - LuaInteger: " << sizeof(LuaInteger) << " bytes\n";
            std::cout << "   - LuaNumber: " << sizeof(LuaNumber) << " bytes\n";
            std::cout << "   - LuaBoolean: " << sizeof(LuaBoolean) << " bytes\n";
            std::cout << "\n";
            
            std::cout << "4. 容器测试\n";
            
            // 测试Vec
            Vec<i32> testVec = {1, 2, 3, 4, 5};
            std::cout << "   Vec<i32> 大小: " << testVec.size() << "\n";
            
            // 测试HashMap
            HashMap<Str, i32> testMap = {{"a", 1}, {"b", 2}, {"c", 3}};
            std::cout << "   HashMap<Str, i32> 大小: " << testMap.size() << "\n";
            
            // 测试HashSet
            HashSet<Str> testSet = {"x", "y", "z"};
            std::cout << "   HashSet<Str> 大小: " << testSet.size() << "\n";
            std::cout << "\n";
            
            std::cout << "5. 智能指针测试\n";
            
            // 测试UPtr
            auto uptr = make_unique<i32>(42);
            std::cout << "   UPtr<i32> 值: " << *uptr << "\n";
            
            // 测试SPtr
            auto sptr = std::make_shared<Str>("Hello, World!");
            std::cout << "   SPtr<Str> 值: " << *sptr << "\n";
            std::cout << "\n";
            
            std::cout << "6. 错误处理测试\n";
            
            // 测试LibException
            try {
                throw LibException(LibErrorCode::TypeMismatch, "测试异常");
            } catch (const LibException& e) {
                std::cout << "   捕获异常: " << e.getErrorCodeString() << " - " << e.what() << "\n";
            }
            
            // 测试错误工具函数
            try {
                Vec<i32> vec = {1, 2, 3};
                ErrorUtils::checkBounds(5, vec, "test_vector");
            } catch (const LibException& e) {
                std::cout << "   边界检查异常: " << e.what() << "\n";
            }
            std::cout << "\n";
            
            std::cout << "7. 性能测试\n";
            
            // 函数查找性能测试
            const i32 lookupIterations = 100000;
            auto start = std::chrono::high_resolution_clock::now();
            
            for (i32 i = 0; i < lookupIterations; ++i) {
                volatile bool exists = manager->hasFunction("math.sqrt");
                (void)exists;
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            
            std::cout << "   函数查找性能:\n";
            std::cout << "   - 迭代次数: " << lookupIterations << "\n";
            std::cout << "   - 总时间: " << duration.count() << " ns\n";
            std::cout << "   - 平均时间: " << static_cast<f64>(duration.count()) / lookupIterations << " ns/lookup\n";
            std::cout << "\n";
            
            // 获取统计信息
            auto stats = manager->getStatistics();
            std::cout << "8. 统计信息\n";
            std::cout << "   - 已注册模块: " << stats.registeredModules << "\n";
            std::cout << "   - 已加载模块: " << stats.loadedModules << "\n";
            std::cout << "   - 已注册函数: " << stats.registeredFunctions << "\n";
            std::cout << "\n";
            
            std::cout << "=== 所有测试完成 ===\n";
            
        } catch (const std::exception& e) {
            std::cerr << "测试失败: " << e.what() << "\n";
        }
    }
}

/**
 * 主函数
 */
int main() {
    try {
        Lua::runIntegrationTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "程序异常: " << e.what() << "\n";
        return 1;
    }
}