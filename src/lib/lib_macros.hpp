/**
 * @file lib_macros.hpp
 * @brief Lua库函数注册 - 现代C++流式API
 * 
 * 提供流式（Fluent）API，简化Lua库函数的注册过程。
 * 
 * 设计目标：
 * - 减少样板代码和重复
 * - 提高类型安全性
 * - 增强可读性和可维护性
 * - 支持链式调用
 * - RAII风格资源管理
 * - 使用 std::span 实现零开销抽象
 * 
 * 使用示例：
 * FunctionRegistrar(L)
 *     .addGlobal("print", luaB_print)
 *     .addGlobal("type", luaB_type)
 *     .commit();
 * 
 * @author Lua C++ Project
 * @date 2025-12-18
 */

#pragma once

#include "lib/lib_module.hpp"
#include "common/types.hpp"

namespace Lua {

    /**
     * @brief 流式函数注册类（现代C++风格）
     *
     * 提供现代C++风格的链式API，用于注册Lua C函数。
     *
     * 核心特性：
     * - 链式调用：每个addGlobal返回*this，支持.method().method()模式
     * - 零开销抽象：使用 std::span，无需哨兵元素
     * - RAII风格：commit时统一注册，异常安全
     * - 类型安全：编译器检查函数签名
     * - const 正确：commit() 为 const 成员函数
     *
     * 使用示例：
     *
     * // 注册全局函数
     * FunctionRegistrar(L)
     *     .addGlobal("print", luaB_print)
     *     .addGlobal("type", luaB_type)
     *     .commit();
     *
     * // 注册到表
     * Table* mathTable = LibRegistry::createLibTable(L, "math");
     * FunctionRegistrar(L)
     *     .addGlobal("abs", math_abs)
     *     .addGlobal("sin", math_sin)
     *     .commitToTable(mathTable);
     *
     * // 条件注册
     * FunctionRegistrar reg(L);
     * reg.addGlobal("core_func", core_func);
     * #ifdef DEBUG
     *     reg.addGlobal("debug_func", debug_func);
     * #endif
     * reg.commit();
     */
    class FunctionRegistrar {
    public:
        /**
         * @brief 构造函数
         * @param L Lua状态指针
         */
        explicit FunctionRegistrar(LuaState* L) : state_(L) {}

        /**
         * @brief 添加全局函数
         * @param name 函数在Lua中的名称
         * @param func C函数指针
         * @return *this 支持链式调用
         */
        FunctionRegistrar& addGlobal(const char* name, LibCFunction func) {
            entries_.push_back({ name, func });
            return *this;
        }

        /**
         * @brief 提交所有函数到全局环境
         *
         * 使用 std::span 直接传递数组视图，无需修改内部状态。
         * 如果entries_为空，则不执行任何操作。
         */
        void commit() const {
            if (entries_.empty()) return;
            LibRegistry::registerGlobalFunctions(state_, entries_);
        }

        /**
         * @brief 提交所有函数到指定表
         * @param table 目标表对象
         *
         * 使用 std::span 直接传递数组视图，无需修改内部状态。
         * 如果entries_为空，则不执行任何操作。
         */
        void commitToTable(Table* table) const {
            if (entries_.empty()) return;
            LibRegistry::registerTableFunctions(state_, table, entries_);
        }

    private:
        LuaState* state_;                       // <Lua状态指针
        Vec<LibFunctionEntry> entries_;         // <函数入口列表
    };
} // namespace Lua
