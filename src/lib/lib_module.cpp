#include "lib_module.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"

namespace Lua {
    // FunctionRegistry 类的实现
    
    Value FunctionRegistry::callFunction(StrView name, State* state, i32 nargs) const {
        auto it = functions_.find(Str(name));
        if (it != functions_.end()) {
            return it->second(state, nargs);
        }
        // 返回错误或抛出异常
        return Value(nullptr);
    }
    
    bool FunctionRegistry::hasFunction(StrView name) const noexcept {
        return functions_.find(Str(name)) != functions_.end();
    }
    
    Vec<Str> FunctionRegistry::getFunctionNames() const {
        Vec<Str> names;
        names.reserve(functions_.size());
        for (const auto& [name, _] : functions_) {
            names.push_back(name);
        }
        return names;
    }
    
    void FunctionRegistry::clear() {
        functions_.clear();
    }
    
    usize FunctionRegistry::size() const noexcept {
        return functions_.size();
    }
    
    // TypedModuleFactory 模板类的实现
    template<typename ModuleType>
    UPtr<LibModule> TypedModuleFactory<ModuleType>::createModule() {
        return make_unique<ModuleType>();
    }
    
    template<typename ModuleType>
    StrView TypedModuleFactory<ModuleType>::getModuleName() const noexcept {
        static ModuleType instance;
        return instance.getName();
    }
    
    template<typename ModuleType>
    StrView TypedModuleFactory<ModuleType>::getVersion() const noexcept {
        return "1.0.0"; // 默认版本，可以被重写
    }
}