#pragma once

#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"

namespace Lua {
    // Forward declarations
    class State;
    class Value;
    class FunctionRegistry;
    
    // Modern function signature using simplified types
    using LibFn = std::function<Value(State*, i32)>;
    
    /**
     * 新的简化LibModule接口
     * 移除了复杂的继承层次，专注于核心功能
     */
    class LibModule {
    public:
        virtual ~LibModule() = default;
        
        /**
         * 获取模块名称
         * 使用StrView避免不必要的字符串拷贝
         */
        virtual StrView getName() const noexcept = 0;
        
        /**
         * 注册函数到函数注册表
         * 使用注册表模式替代直接操作State
         */
        virtual void registerFunctions(FunctionRegistry& registry) = 0;
        
        /**
         * 可选的初始化钩子
         * 用于需要特殊初始化逻辑的模块
         */
        virtual void initialize(State* state) {}
        
        /**
         * 可选的清理钩子
         * 用于资源清理
         */
        virtual void cleanup(State* state) {}
    };
    
    /**
     * 函数注册表 - 核心改进
     * 使用哈希表优化函数查找性能
     */
    class FunctionRegistry {
    public:
        /**
         * 注册函数 - 支持完美转发
         */
        template<typename F>
        void registerFunction(StrView name, F&& func) {
            static_assert(std::is_invocable_r_v<Value, F, State*, i32>, 
                         "Function must have signature Value(State*, i32)");
            
            functions_.emplace(Str(name), std::forward<F>(func));
        }
        
        /**
         * 调用函数
         */
        Value callFunction(StrView name, State* state, i32 nargs) const;
        
        /**
         * 检查函数是否存在
         */
        bool hasFunction(StrView name) const noexcept;
        
        /**
         * 获取所有注册的函数名
         */
        Vec<Str> getFunctionNames() const;
        
        /**
         * 清空注册表
         */
        void clear();
        
        /**
         * 获取函数数量
         */
        usize size() const noexcept;
        
    private:
        // 使用HashMap优化查找性能
        HashMap<Str, LibFn> functions_;
    };
    
    /**
     * 简化的函数注册宏
     * 减少样板代码
     */
    #define REGISTER_FUNCTION(registry, name, func) \
        (registry).registerFunction(#name, [](State* s, i32 n) -> Value { return func(s, n); })
    
    /**
     * 带命名空间的函数注册宏
     * 用于避免函数名冲突
     */
    #define REGISTER_NAMESPACED_FUNCTION(registry, namespace_name, func_name, func) \
        (registry).registerFunction(namespace_name "." #func_name, [](State* s, i32 n) -> Value { return func(s, n); })
    
    /**
     * 模块工厂接口
     * 用于动态创建模块实例
     */
    class ModuleFactory {
    public:
        virtual ~ModuleFactory() = default;
        virtual UPtr<LibModule> createModule() = 0;
        virtual StrView getModuleName() const noexcept = 0;
        virtual StrView getVersion() const noexcept = 0;
    };
    
    /**
     * 模板化的模块工厂
     * 简化模块注册
     */
    template<typename ModuleType>
    class TypedModuleFactory : public ModuleFactory {
    public:
        UPtr<LibModule> createModule() override;
        StrView getModuleName() const noexcept override;
        StrView getVersion() const noexcept override;
    };
    
    /**
     * 模块注册宏
     * 简化模块注册过程
     */
    #define REGISTER_MODULE(ModuleClass) \
        make_unique<TypedModuleFactory<ModuleClass>>()
}