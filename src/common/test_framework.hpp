#pragma once

#include <iostream>
#include <string>
#include <stdexcept>

namespace Lua {

    /**
     * @brief 简单的测试框架
     * 
     * 提供基本的断言和测试工具函数
     */
    
    /**
     * @brief 测试断言宏
     * @param condition 要检查的条件
     * @param message 失败时的错误消息
     */
    #define TEST_ASSERT(condition, message) \
        do { \
            if (!(condition)) { \
                throw std::runtime_error(std::string("断言失败: ") + (message) + \
                    " 在文件 " + __FILE__ + " 第 " + std::to_string(__LINE__) + " 行"); \
            } \
        } while(0)
    
    /**
     * @brief 测试相等断言
     * @param expected 期望值
     * @param actual 实际值
     * @param message 失败时的错误消息
     */
    #define TEST_ASSERT_EQ(expected, actual, message) \
        do { \
            if ((expected) != (actual)) { \
                throw std::runtime_error(std::string("断言失败: ") + (message) + \
                    " 期望: " + std::to_string(expected) + \
                    " 实际: " + std::to_string(actual) + \
                    " 在文件 " + __FILE__ + " 第 " + std::to_string(__LINE__) + " 行"); \
            } \
        } while(0)
    
    /**
     * @brief 测试不相等断言
     * @param expected 不期望的值
     * @param actual 实际值
     * @param message 失败时的错误消息
     */
    #define TEST_ASSERT_NE(expected, actual, message) \
        do { \
            if ((expected) == (actual)) { \
                throw std::runtime_error(std::string("断言失败: ") + (message) + \
                    " 不应该等于: " + std::to_string(expected) + \
                    " 在文件 " + __FILE__ + " 第 " + std::to_string(__LINE__) + " 行"); \
            } \
        } while(0)
    
    /**
     * @brief 测试空指针断言
     * @param ptr 要检查的指针
     * @param message 失败时的错误消息
     */
    #define TEST_ASSERT_NULL(ptr, message) \
        do { \
            if ((ptr) != nullptr) { \
                throw std::runtime_error(std::string("断言失败: ") + (message) + \
                    " 指针应该为null 在文件 " + __FILE__ + " 第 " + std::to_string(__LINE__) + " 行"); \
            } \
        } while(0)
    
    /**
     * @brief 测试非空指针断言
     * @param ptr 要检查的指针
     * @param message 失败时的错误消息
     */
    #define TEST_ASSERT_NOT_NULL(ptr, message) \
        do { \
            if ((ptr) == nullptr) { \
                throw std::runtime_error(std::string("断言失败: ") + (message) + \
                    " 指针不应该为null 在文件 " + __FILE__ + " 第 " + std::to_string(__LINE__) + " 行"); \
            } \
        } while(0)
    
    /**
     * @brief 测试异常断言
     * @param expression 要执行的表达式
     * @param exception_type 期望的异常类型
     * @param message 失败时的错误消息
     */
    #define TEST_ASSERT_THROWS(expression, exception_type, message) \
        do { \
            bool caught = false; \
            try { \
                (expression); \
            } catch (const exception_type&) { \
                caught = true; \
            } catch (...) { \
                throw std::runtime_error(std::string("断言失败: ") + (message) + \
                    " 抛出了错误的异常类型 在文件 " + __FILE__ + " 第 " + std::to_string(__LINE__) + " 行"); \
            } \
            if (!caught) { \
                throw std::runtime_error(std::string("断言失败: ") + (message) + \
                    " 没有抛出期望的异常 在文件 " + __FILE__ + " 第 " + std::to_string(__LINE__) + " 行"); \
            } \
        } while(0)
    
    /**
     * @brief 测试不抛出异常断言
     * @param expression 要执行的表达式
     * @param message 失败时的错误消息
     */
    #define TEST_ASSERT_NO_THROW(expression, message) \
        do { \
            try { \
                (expression); \
            } catch (...) { \
                throw std::runtime_error(std::string("断言失败: ") + (message) + \
                    " 不应该抛出异常 在文件 " + __FILE__ + " 第 " + std::to_string(__LINE__) + " 行"); \
            } \
        } while(0)
    
    /**
     * @brief 测试工具类
     */
    class TestUtils {
    public:
        /**
         * @brief 打印测试开始信息
         * @param testName 测试名称
         */
        static void printTestStart(const std::string& testName) {
            std::cout << "[开始] " << testName << std::endl;
        }
        
        /**
         * @brief 打印测试成功信息
         * @param testName 测试名称
         */
        static void printTestSuccess(const std::string& testName) {
            std::cout << "[成功] " << testName << std::endl;
        }
        
        /**
         * @brief 打印测试失败信息
         * @param testName 测试名称
         * @param error 错误信息
         */
        static void printTestFailure(const std::string& testName, const std::string& error) {
            std::cout << "[失败] " << testName << ": " << error << std::endl;
        }
        
        /**
         * @brief 运行单个测试
         * @param testName 测试名称
         * @param testFunc 测试函数
         * @return true 如果测试成功
         */
        template<typename TestFunc>
        static bool runTest(const std::string& testName, TestFunc testFunc) {
            printTestStart(testName);
            try {
                testFunc();
                printTestSuccess(testName);
                return true;
            } catch (const std::exception& e) {
                printTestFailure(testName, e.what());
                return false;
            }
        }
    };

} // namespace Lua
