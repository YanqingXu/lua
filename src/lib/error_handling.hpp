#pragma once

#include "lib_module.hpp"
#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include <stdexcept>
#include <string>
#include <iostream>

namespace Lua {
    /**
     * 错误代码枚举
     * 使用简化的类型系统
     */
    enum class LibErrorCode : u32 {
        Success = 0,
        InvalidArgument = 1,
        OutOfRange = 2,
        TypeMismatch = 3,
        NullPointer = 4,
        InternalError = 5,
        NotImplemented = 6
    };
    
    /**
     * 扩展的Lua异常类
     * 集成错误代码和详细信息
     */
    class LibException : public LuaException {
    public:
        LibException(LibErrorCode code, StrView message);
        LibException(LibErrorCode code, const Str& message);
        
        LibErrorCode getErrorCode() const noexcept;
        StrView getErrorCodeString() const noexcept;
        
    private:
        LibErrorCode errorCode_;
    };
    
    /**
     * 错误处理工具函数
     */
    namespace ErrorUtils {
        /**
         * 检查参数数量
         */
        void checkArgCount(i32 actual, i32 expected, StrView functionName);
        
        /**
         * 检查参数范围
         */
        void checkArgRange(i32 actual, i32 min, i32 max, StrView functionName);
        
        /**
         * 检查空指针
         */
        void checkNotNull(const void* ptr, StrView paramName);
        
        /**
         * 检查数组边界
         */
        template<typename T>
        static void checkBounds(T value, T min, T max, StrView paramName) {
            if (value < min || value > max) {
                throw LibException(
                    LibErrorCode::OutOfRange,
                    Str(paramName) + " out of bounds [" + 
                    std::to_string(min) + ", " + std::to_string(max) + "]"
                );
            }
        }
    }
    
    /**
     * 错误处理库模块
     * 提供错误处理相关的函数
     */
    class ErrorHandlingLib : public LibModule {
    public:
        StrView getName() const noexcept override;
        void registerFunctions(FunctionRegistry& registry) override;
        
    private:
        static Value pcall(State* state, i32 nargs);
        static Value xpcall(State* state, i32 nargs);
        static Value error(State* state, i32 nargs);
        static Value assert(State* state, i32 nargs);
    };
    
    /**
     * 安全函数调用包装器
     * 使用RAII和异常安全的方式调用函数
     */
    template<typename F>
    class SafeFunctionCall {
    public:
        SafeFunctionCall(State* state, StrView functionName, F&& func)
            : state_(state), functionName_(functionName), func_(std::forward<F>(func)) {
            ErrorUtils::checkNotNull(state_, "state");
        }
        
        Value operator()(i32 nargs) noexcept {
            try {
                return func_(state_, nargs);
            } catch (const LibException& e) {
                std::cerr << "[" << functionName_ << "] " << e.what() << std::endl;
                return Value(nullptr);
            } catch (const std::exception& e) {
                std::cerr << "[" << functionName_ << "] Unexpected error: " << e.what() << std::endl;
                return Value(nullptr);
            } catch (...) {
                std::cerr << "[" << functionName_ << "] Unknown error occurred" << std::endl;
                return Value(nullptr);
            }
        }
        
    private:
        State* state_;
        StrView functionName_;
        F func_;
    };
    
    /**
     * 创建安全函数调用的便利函数
     */
    template<typename F>
    auto makeSafeCall(State* state, StrView functionName, F&& func) {
        return SafeFunctionCall<F>(state, functionName, std::forward<F>(func));
    }
    
    /**
     * 安全函数注册宏
     * 自动包装异常处理
     */
    #define REGISTER_SAFE_FUNCTION(registry, name, func) \
        (registry).registerFunction(#name, [](State* s, i32 n) -> Value { \
            auto safeCall = makeSafeCall(s, #name, func); \
            return safeCall(n); \
        })
}