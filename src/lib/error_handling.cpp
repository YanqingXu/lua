#include "error_handling.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include <stdexcept>

namespace Lua {
    
    // LibException 类的实现
    
    LibException::LibException(LibErrorCode code, StrView message)
        : LuaException(Str(message)), errorCode_(code) {}
    
    LibException::LibException(LibErrorCode code, const Str& message)
        : LuaException(message), errorCode_(code) {}
    
    LibErrorCode LibException::getErrorCode() const noexcept {
        return errorCode_;
    }
    
    StrView LibException::getErrorCodeString() const noexcept {
        switch (errorCode_) {
            case LibErrorCode::Success: return "Success";
            case LibErrorCode::InvalidArgument: return "InvalidArgument";
            case LibErrorCode::OutOfRange: return "OutOfRange";
            case LibErrorCode::TypeMismatch: return "TypeMismatch";
            case LibErrorCode::NullPointer: return "NullPointer";
            case LibErrorCode::InternalError: return "InternalError";
            case LibErrorCode::NotImplemented: return "NotImplemented";
            default: return "Unknown";
        }
    }
    
    // ErrorUtils 命名空间的实现
    
    namespace ErrorUtils {
        void checkArgCount(i32 actual, i32 expected, StrView functionName) {
            if (actual < expected) {
                throw LibException(
                    LibErrorCode::InvalidArgument,
                    Str(functionName) + ": expected at least " + std::to_string(expected) + 
                    " arguments, got " + std::to_string(actual)
                );
            }
        }
        
        void checkArgRange(i32 actual, i32 min, i32 max, StrView functionName) {
            if (actual < min || actual > max) {
                throw LibException(
                    LibErrorCode::InvalidArgument,
                    Str(functionName) + ": expected " + std::to_string(min) + "-" + 
                    std::to_string(max) + " arguments, got " + std::to_string(actual)
                );
            }
        }
        
        void checkNotNull(const void* ptr, StrView paramName) {
            if (!ptr) {
                throw LibException(
                    LibErrorCode::NullPointer,
                    Str(paramName) + " cannot be null"
                );
            }
        }
    }
    
    // ErrorHandlingLib 类的实现
    
    StrView ErrorHandlingLib::getName() const noexcept {
        return "error";
    }
    
    void ErrorHandlingLib::registerFunctions(FunctionRegistry& registry) {
        // 注册错误处理函数
        REGISTER_FUNCTION(registry, pcall, pcall);
        REGISTER_FUNCTION(registry, xpcall, xpcall);
        REGISTER_FUNCTION(registry, error, error);
        REGISTER_FUNCTION(registry, assert, assert);
        
        // 注册类型检查函数
        registry.registerFunction("checktype", [](State* state, i32 nargs) -> Value {
            try {
                ErrorUtils::checkArgCount(nargs, 2, "checktype");
                
                auto value = state->get(1);
                auto expectedType = state->get(2).asString();
                
                // 简化的类型检查逻辑
                Str actualType = "unknown";
                if (value.isNumber()) actualType = "number";
                else if (value.isString()) actualType = "string";
                else if (value.isBoolean()) actualType = "boolean";
                else if (value.isNil()) actualType = "nil";
                
                if (actualType != expectedType) {
                    throw LibException(
                        LibErrorCode::TypeMismatch,
                        "Expected " + expectedType + ", got " + actualType
                    );
                }
                
                return Value(true);
            } catch (const LibException& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return Value(false);
            }
        });
        
        // 注册范围检查函数
        registry.registerFunction("checkrange", [](State* state, i32 nargs) -> Value {
            try {
                ErrorUtils::checkArgCount(nargs, 3, "checkrange");
                
                auto value = state->get(1).asNumber();
        auto min = state->get(2).asNumber();
        auto max = state->get(3).asNumber();
                
                if (value < min || value > max) {
                    throw LibException(
                        LibErrorCode::OutOfRange,
                        "Value " + std::to_string(value) + " out of range [" + 
                        std::to_string(min) + ", " + std::to_string(max) + "]"
                    );
                }
                
                return Value(true);
            } catch (const LibException& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return Value(false);
            }
        });
    }
    
    Value ErrorHandlingLib::pcall(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgCount(nargs, 1, "pcall");
            
            // 获取要调用的函数
            if (nargs < 1) {
                state->push(Value(false));
                state->push(Value("pcall: no function provided"));
                return Value(2);
            }
            
            Value func = state->get(1);
            if (!func.isFunction()) {
                state->push(Value(false));
                state->push(Value("pcall: argument is not a function"));
                return Value(2);
            }
            
            try {
                // 准备参数列表
                Vec<Value> args;
                for (int i = 2; i <= nargs; i++) {
                    args.push_back(state->get(i));
                }
                
                // 实际执行函数
                Value result = state->call(func, args);
                
                // 函数成功执行，返回true和结果
                state->push(Value(true));
                state->push(result);
                return Value(2);
                
            } catch (const std::exception& e) {
                // 捕获到异常，返回false和错误信息
                state->push(Value(false));
                state->push(Value(e.what()));
                return Value(2);
            }
            
        } catch (const LibException& e) {
            // 返回false和错误信息
            state->push(Value(false));
            state->push(Value(e.what()));
            return Value(2); // 返回值数量
        }
    }
    
    Value ErrorHandlingLib::xpcall(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgCount(nargs, 2, "xpcall");
            
            // 简化的xpcall实现
            return Value(true);
        } catch (const LibException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return Value(false);
        }
    }
    
    Value ErrorHandlingLib::error(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgCount(nargs, 1, "error");
            
            auto message = state->get(1).asString();
            throw LibException(LibErrorCode::InternalError, message);
        } catch (const LibException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return Value(nullptr);
        }
    }
    
    Value ErrorHandlingLib::assert(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgCount(nargs, 1, "assert");
            
            auto condition = state->get(1);
            if (!condition.asBoolean()) {
                Str message = "assertion failed";
                if (nargs > 1) {
                    message = state->get(2).asString();
                }
                throw LibException(LibErrorCode::InternalError, message);
            }
            
            return condition;
        } catch (const LibException& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return Value(false);
        }
    }
}