#include "state.hpp"
#include "table.hpp"
#include "function.hpp"
#include "../common/defines.hpp"
#include "../parser/parser.hpp"
#include "../compiler/compiler.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>

namespace Lua {
    State::State() : top(0) {
        // 初始化栈空间
        stack.resize(LUAI_MAXSTACK);
    }
    
    State::~State() {
        // 清理资源
    }
    
    void State::push(const Value& value) {
        if (top >= LUAI_MAXSTACK) {
            throw LuaException("stack overflow");
        }
        stack[top++] = value;
    }
    
    Value State::pop() {
        if (top <= 0) {
            throw LuaException("stack underflow");
        }
        return stack[--top];
    }
    
    Value& State::get(int idx) {
        static Value nil;  // 静态nil值用于无效索引的返回
        
        // 处理绝对索引和相对索引
        int abs_idx;
        if (idx > 0) {
            abs_idx = idx - 1;  // 将1-based转为0-based
        } else if (idx < 0) {
            abs_idx = top + idx;  // 相对于栈顶的索引
        } else {
            // 索引为0是无效的
            return nil;
        }
        
        // 检查索引是否在范围内
        if (abs_idx < 0 || abs_idx >= top) {
            return nil;
        }
        
        return stack[abs_idx];
    }
    
    void State::set(int idx, const Value& value) {
        // 处理绝对索引和相对索引
        int abs_idx;
        if (idx > 0) {
            abs_idx = idx - 1;  // 将1-based转为0-based
        } else if (idx < 0) {
            abs_idx = top + idx;  // 相对于栈顶的索引
        } else {
            // 索引为0是无效的
            return;
        }
        
        // 检查索引是否在范围内
        if (abs_idx < 0 || abs_idx >= top) {
            return;
        }
        
        stack[abs_idx] = value;
    }
    
    // 类型检查函数
    bool State::isNil(int idx) const {
        if (idx <= 0 || idx > top) return true;
        return stack[idx - 1].isNil();
    }
    
    bool State::isBoolean(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isBoolean();
    }
    
    bool State::isNumber(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isNumber();
    }
    
    bool State::isString(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isString();
    }
    
    bool State::isTable(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isTable();
    }
    
    bool State::isFunction(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].isFunction();
    }
    
    // 类型转换函数
    LuaBoolean State::toBoolean(int idx) const {
        if (idx <= 0 || idx > top) return false;
        return stack[idx - 1].asBoolean();
    }
    
    LuaNumber State::toNumber(int idx) const {
        if (idx <= 0 || idx > top) return 0.0;
        return stack[idx - 1].asNumber();
    }
    
    Str State::toString(int idx) const {
        if (idx <= 0 || idx > top) return "";
        return stack[idx - 1].toString();
    }
    
    Ptr<Table> State::toTable(int idx) {
        if (idx <= 0 || idx > top) return nullptr;
        return stack[idx - 1].asTable();
    }
    
    Ptr<Function> State::toFunction(int idx) {
        if (idx <= 0 || idx > top) return nullptr;
        return stack[idx - 1].asFunction();
    }
    
    // 全局变量操作
    void State::setGlobal(const Str& name, const Value& value) {
        globals[name] = value;
    }
    
    Value State::getGlobal(const Str& name) {
        auto it = globals.find(name);
        if (it != globals.end()) {
            return it->second;
        }
        return Value(nullptr);  // nil
    }
    
    // 函数调用
    Value State::call(const Value& func, const Vec<Value>& args) {
        if (!func.isFunction()) {
            throw LuaException("attempt to call a non-function value");
        }
        
        auto function = func.asFunction();
        
        // 原生函数调用
        if (function->getType() == Function::Type::Native) {
            auto nativeFn = function->getNative();
            if (!nativeFn) {
                throw LuaException("attempt to call a nil value");
            }
            
            // 保存当前栈顶
            int oldTop = top;
            
            // 将参数压入栈
            for (const auto& arg : args) {
                push(arg);
            }
            
            // 调用函数
            Value result = nativeFn(this, args.size());
            
            // 恢复栈顶
            top = oldTop;
            
            return result;
        }
        
        // 对于Lua函数的调用，在实际的VM中实现
        // 简化版本中，我们暂时不实现完整的Lua函数调用
        throw LuaException("Lua function call not implemented");
    }
    
    // 执行字符串中的Lua代码
    bool State::doString(const Str& code) {
        try {
            // 1. 使用我们的语法分析器解析代码
            Parser parser(code);
            auto statements = parser.parse();
            
            // 检查语法分析阶段是否有错误
            if (parser.hasError()) {
                return false;
            }
            
            // 2. 使用编译器生成字节码
            Compiler compiler;
            Ptr<Function> function = compiler.compile(statements);
            
            if (!function) {
                return false;
            }
            
            // 3. 调用函数
            // 注意：完整的实现需要调用VM执行生成的字节码
            // 这里我们使用简化的实现
            push(function);
            call(function, {});
            
            return true;
        } catch (const LuaException& e) {
            // 可以在这里处理或记录错误
            return false;
        }
    }
    
    // 从文件加载并执行Lua代码
    bool State::doFile(const Str& filename) {
        try {
            // 1. 打开指定文件
            std::ifstream file(filename);
            if (!file.is_open()) {
                return false;
            }
            
            // 2. 读取文件内容到字符串
            std::stringstream buffer;
            buffer << file.rdbuf();
            
            // 3. 关闭文件
            file.close();
            
            // 4. 调用doString执行该字符串
            return doString(buffer.str());
        } catch (const std::exception& e) {
            // 文件操作可能会抛出各种异常
            return false;
        }
    }
}
