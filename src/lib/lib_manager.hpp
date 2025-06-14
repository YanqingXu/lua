#pragma once

#include "lib_module.hpp"
#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"

namespace Lua {
    
    /**
     * 新的库管理器设计
     * 移除单例模式，支持依赖注入
     */
    class LibManager {
    public:
        /**
         * 构造函数
         * 可以传入自定义的函数注册表
         */
        explicit LibManager(UPtr<FunctionRegistry> registry = nullptr);
        
        /**
         * 注册模块
         */
        void registerModule(UPtr<LibModule> module);
        
        /**
         * 通过工厂注册模块
         */
        void registerModuleFactory(UPtr<ModuleFactory> factory);
        
        /**
         * 加载单个模块
         */
        bool loadModule(StrView name, State* state);
        
        /**
         * 加载所有已注册的模块
         */
        void loadAllModules(State* state);
        
        /**
         * 卸载模块
         */
        bool unloadModule(StrView name, State* state = nullptr);
        
        /**
         * 检查模块是否已加载
         */
        bool isModuleLoaded(StrView name) const noexcept;
        
        /**
         * 获取已加载的模块列表
         */
        Vec<Str> getLoadedModules() const;
        
        /**
         * 调用函数
         */
        Value callFunction(StrView name, State* state, i32 nargs) const;
        
        /**
         * 检查函数是否存在
         */
        bool hasFunction(StrView name) const noexcept;
        
        /**
         * 获取函数注册表（用于高级操作）
         */
        FunctionRegistry& getRegistry();
        const FunctionRegistry& getRegistry() const;
        
        /**
         * 清空所有模块
         */
        void clear(State* state = nullptr);
        
        /**
         * 库管理器统计信息
         */
        struct LibManagerStats {
            usize totalModules = 0;
            usize loadedModules = 0;
            usize totalFunctions = 0;
            Vec<Str> moduleNames;
        };
        
        LibManagerStats getStats() const;
        
    private:
        // 已注册的模块
        HashMap<Str, UPtr<LibModule>> modules_;
        
        // 模块工厂
        HashMap<Str, UPtr<ModuleFactory>> factories_;
        
        // 已加载的模块名称
        HashSet<Str> loadedModules_;
        
        // 函数注册表
        UPtr<FunctionRegistry> registry_;
    };
    
    /**
     * 创建标准库管理器的便利函数
     */
    UPtr<LibManager> createStandardLibManager();
    
    /**
     * 快速初始化标准库的便利函数
     */
    void initStandardLibraries(State* state, LibManager& manager);
}