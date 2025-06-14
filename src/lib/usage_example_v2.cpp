/**
 * 新LibModule接口使用示例
 * 展示如何使用重新设计的接口
 */

#include "lib_module_v2.hpp"
#include "lib_manager_v2.hpp"
#include "base_lib_v2.hpp"
#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include <iostream>
#include <chrono>

namespace Lua {
    
    /**
     * 示例：创建一个简单的数学库
     */
    class MathLibV2 : public LibModule {
    public:
        StrView getName() const noexcept override {
            return "math";
        }
        
        void registerFunctions(FunctionRegistry& registry) override {
            // 使用lambda注册函数，更加简洁
            registry.registerFunction("abs", [](State* state, i32 nargs) -> Value {
                if (nargs < 1) return Value(0.0);
                auto val = state->get(1);
                if (val.isNumber()) {
                    return Value(std::abs(val.asNumber()));
                }
                return Value(0.0);
            });
            
            registry.registerFunction("max", [](State* state, i32 nargs) -> Value {
                if (nargs < 2) return Value(0.0);
                auto a = state->get(1);
                auto b = state->get(2);
                if (a.isNumber() && b.isNumber()) {
                    return Value(std::max(a.asNumber(), b.asNumber()));
                }
                return Value(0.0);
            });
            
            registry.registerFunction("min", [](State* state, i32 nargs) -> Value {
                if (nargs < 2) return Value(0.0);
                auto a = state->get(1);
                auto b = state->get(2);
                if (a.isNumber() && b.isNumber()) {
                    return Value(std::min(a.asNumber(), b.asNumber()));
                }
                return Value(0.0);
            });
            
            // 使用宏注册静态函数
            REGISTER_FUNCTION(registry, sqrt, sqrt);
            REGISTER_FUNCTION(registry, pow, pow);
        }
        
    private:
        static Value sqrt(State* state, i32 nargs) {
            if (nargs < 1) return Value(0.0);
            auto val = state->get(1);
            if (val.isNumber()) {
                return Value(std::sqrt(val.asNumber()));
            }
            return Value(0.0);
        }
        
        static Value pow(State* state, i32 nargs) {
            if (nargs < 2) return Value(0.0);
            auto base = state->get(1);
            auto exp = state->get(2);
            if (base.isNumber() && exp.isNumber()) {
                return Value(std::pow(base.asNumber(), exp.asNumber()));
            }
            return Value(0.0);
        }
    };
    
    /**
     * 示例：使用命名空间的库
     */
    class UtilsLibV2 : public LibModule {
    public:
        StrView getName() const noexcept override {
            return "utils";
        }
        
        void registerFunctions(FunctionRegistry& registry) override {
            // 使用命名空间注册函数
            REGISTER_NAMESPACED_FUNCTION(registry, "utils", len, getLength);
            REGISTER_NAMESPACED_FUNCTION(registry, "utils", reverse, reverseString);
        }
        
    private:
        static Value getLength(State* state, i32 nargs) {
            if (nargs < 1) return Value(0);
            auto val = state->get(1);
            if (val.isString()) {
                return Value(static_cast<double>(val.asString().length()));
            }
            return Value(0);
        }
        
        static Value reverseString(State* state, i32 nargs) {
            if (nargs < 1) return Value("");
            auto val = state->get(1);
            if (val.isString()) {
                std::string str = val.asString();
                std::reverse(str.begin(), str.end());
                return Value(str);
            }
            return Value("");
        }
    };
}

/**
 * 使用示例函数
 */
void demonstrateNewLibInterface() {
    using namespace Lua;
    
    std::cout << "=== 新LibModule接口使用示例 ===\n\n";
    
    // 1. 创建状态和管理器
    auto state = std::make_unique<State>();
    auto manager = std::make_unique<LibManagerV2>();
    
    std::cout << "1. 创建库管理器和状态\n";
    
    // 2. 注册模块
    std::cout << "2. 注册模块\n";
    manager->registerModule(make_unique<BaseLibV2>());
    manager->registerModule(make_unique<MathLibV2>());
    manager->registerModule(make_unique<UtilsLibV2>());
    
    // 3. 加载模块
    std::cout << "3. 加载模块\n";
    manager->loadModule("base", state.get());
    manager->loadModule("math", state.get());
    manager->loadModule("utils", state.get());
    
    // 4. 显示统计信息
    auto stats = manager->getStats();
    std::cout << "   总模块数: " << stats.totalModules << "\n";
    std::cout << "   已加载模块: " << stats.loadedModules << "\n";
    std::cout << "   注册函数: " << stats.totalFunctions << "\n";
    
    // 5. 测试函数调用
    std::cout << "4. 测试函数调用\n";
    
    // 检查函数是否存在
    if (manager->hasFunction("abs")) {
        std::cout << "   math.abs 函数已注册\n";
    }
    
    if (manager->hasFunction("utils.len")) {
        std::cout << "   utils.len 函数已注册\n";
    }
    
    // 6. 演示工厂模式
    std::cout << "5. 演示工厂模式\n";
    auto factory = make_unique<TypedModuleFactory<MathLibV2>>();
    std::cout << "   工厂模块名: " << factory->getModuleName() << "\n";
    std::cout << "   工厂版本: " << factory->getVersion() << "\n";
    
    // 7. 获取已加载模块列表
    std::cout << "6. 已加载的模块:\n";
    auto loadedModules = manager->getLoadedModules();
    for (const auto& moduleName : loadedModules) {
        std::cout << "   - " << moduleName << "\n";
    }
    
    // 8. 清理
    std::cout << "7. 清理资源\n";
    manager->clear(state.get());
    
    std::cout << "\n=== 示例完成 ===\n";
}

/**
 * 性能对比示例
 */
void performanceComparison() {
    using namespace Lua;
    
    std::cout << "\n=== 性能对比示例 ===\n";
    
    auto state = std::make_unique<State>();
    auto manager = std::make_unique<LibManagerV2>();
    
    // 注册大量函数进行性能测试
    auto mathLib = make_unique<MathLibV2>();
    manager->registerModule(std::move(mathLib));
    manager->loadModule("math", state.get());
    
    // 测试函数查找性能
    const i32 iterations = 100000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (i32 i = 0; i < iterations; ++i) {
        manager->hasFunction("abs");
        manager->hasFunction("max");
        manager->hasFunction("min");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "函数查找性能测试 (" << iterations << " 次迭代):\n";
    std::cout << "总时间: " << duration.count() << " 微秒\n";
    std::cout << "平均时间: " << static_cast<f64>(duration.count()) / (iterations * 3.0) << " 微秒/查找\n";
    
    std::cout << "\n=== 性能测试完成 ===\n";
}

int main() {
    demonstrateNewLibInterface();
    performanceComparison();
    return 0;
}