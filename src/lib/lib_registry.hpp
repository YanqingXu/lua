#pragma once

/**
 * @file lib_registry.hpp
 * @brief Lua 标准库函数与模块注册辅助接口
 */

#include "lib/lib_module.hpp"
#include "common/types.hpp"

#include <expected>

namespace Lua {

class Table;
class Function;

/** @brief 库函数注册失败的分类。 */
enum class LibRegistrationErrorCode : u8 {
    NullState,
    NullTable,
    NullName,
    NullFunction,
};

/** @brief 库函数注册失败的错误信息。 */
struct LibRegistrationError {
    LibRegistrationErrorCode code;
    StrView operation;
};

/**
 * @brief Lua函数注册工具（现代C++风格）
 *
 * 提供统一的、现代化的API用于注册Lua C函数到全局环境或表中。
 * 支持两种使用模式：静态方法（立即注册）和流式接口（批量注册）。
 *
 * 核心特性：
 * - 链式调用：每个addGlobal返回*this，支持.method().method()模式
 * - 零开销抽象：使用基于范围的循环，无需哨兵元素
 * - RAII风格：commit时统一注册，异常安全
 * - 类型安全：编译器检查函数签名
 * - const 正确：commit() 为 const 成员函数
 * - 静态方法：提供便捷的单函数注册接口
 *
 * 使用示例：
 *
 * // 方式1：批量注册全局函数（流式风格）
 * FunctionRegistrar(L)
 *     .addGlobal("print", luaB_print)
 *     .addGlobal("type", luaB_type)
 *     .commit();
 *
 * // 方式2：注册到表（流式风格）
 * Table* mathTable = FunctionRegistrar::createLibTable(L, "math");
 * FunctionRegistrar(L)
 *     .addGlobal("abs", math_abs)
 *     .addGlobal("sin", math_sin)
 *     .commitToTable(mathTable);
 *
 * // 方式3：单个函数注册（静态方法）
 * FunctionRegistrar::registerGlobal(L, "myFunc", myFunc);
 *
 * // 方式4：条件注册
 * FunctionRegistrar reg(L);
 * reg.addGlobal("core_func", core_func);
 * #ifdef DEBUG
 *     reg.addGlobal("debug_func", debug_func);
 * #endif
 * reg.commit();
 */
class FunctionRegistrar {
public:
    // =====================================================================
    // 静态方法：立即注册（便捷接口）
    // =====================================================================

    /**
     * @brief 注册单个全局函数（静态方法）
     * @param L Lua状态指针
     * @param name 函数名
     * @param func C函数指针
     */
    static void registerGlobal(LuaState* L, const char* name, LibCFunction func);

    /**
     * @brief 注册单个表函数（静态方法）
     * @param L Lua状态指针
     * @param table 目标表
     * @param name 函数名
     * @param func C函数指针
     */
    static void registerToTable(LuaState* L, Table* table, const char* name, LibCFunction func);

    /**
     * @brief 创建库表并注册为全局变量（静态方法）
     * @param L Lua状态指针
     * @param libName 库名称
     * @return 创建的表指针
     */
    static Table* createLibTable(LuaState* L, const char* libName);

    /**
     * @brief 以 expected 表达创建闭包时的参数错误
     */
    [[nodiscard]] static std::expected<Function*, LibRegistrationError>
    tryCreateClosure(LuaState* L, LibCFunction func);

    /**
     * @brief 以 expected 表达库表创建/注册时的参数错误
     */
    [[nodiscard]] static std::expected<Table*, LibRegistrationError>
    tryCreateLibTable(LuaState* L, StrView libName);

    // =====================================================================
    // 实例方法：流式接口（批量注册）
    // =====================================================================

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
     * 使用基于范围的循环遍历所有函数条目并注册。
     * 如果entries_为空，则不执行任何操作。
     */
    void commit() const {
        if (entries_.empty()) return;
        
        for (const auto& entry : entries_) {
            registerGlobal(state_, entry.name, entry.func);
        }
    }

    /**
     * @brief 提交所有函数到指定表
     * @param table 目标表对象
     *
     * 使用基于范围的循环遍历所有函数条目并注册到表中。
     * 如果entries_为空，则不执行任何操作。
     */
    void commitToTable(Table* table) const {
        if (entries_.empty()) return;
        
        for (const auto& entry : entries_) {
            registerToTable(state_, table, entry.name, entry.func);
        }
    }

private:
    // =====================================================================
    // 私有辅助方法
    // =====================================================================

    /**
     * @brief 创建函数闭包
     * @param L Lua状态指针
     * @param func C函数指针
     * @return 函数对象指针
     */
    static Function* createClosure(LuaState* L, LibCFunction func);

    // =====================================================================
    // 成员变量
    // =====================================================================

    /** @brief Lua状态指针 */
    LuaState* state_;
    /** @brief 函数入口列表 */
    Vec<LibFunctionEntry> entries_;
};

} // namespace Lua
