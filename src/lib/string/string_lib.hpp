#pragma once

#include "../core/lib_module.hpp"
#include "../core/lib_registry.hpp"

namespace Lua {

/**
 * @brief String library implementation
 *
 * Provides Lua string manipulation functions:
 * - len: Get string length
 * - sub: Extract substring
 * - upper: Convert to uppercase
 * - lower: Convert to lowercase
 * - reverse: Reverse string
 * - rep: Repeat string
 *
 * This implementation follows the simplified framework design
 * for better performance and maintainability.
 */
class StringLib : public LibModule {
public:
    /**
     * @brief Get module name
     * @return Module name as string literal
     */
    const char* getName() const override { return "string"; }

    /**
     * @brief Register string library functions to the state
     * @param state Lua state to register functions to
     * @throws std::invalid_argument if state is null
     */
    void registerFunctions(State* state) override;

    /**
     * @brief Initialize the string library
     * @param state Lua state to initialize
     * @throws std::invalid_argument if state is null
     */
    void initialize(State* state) override;

    // String function declarations with proper documentation
    /**
     * @brief Get string length function
     * @param state Lua state containing string argument
     * @param nargs Number of arguments (should be 1)
     * @return Number value representing string length
     * @throws std::invalid_argument if state is null
     */
    static Value len(State* state, i32 nargs);

    /**
     * @brief Extract substring function
     * @param state Lua state containing string and indices
     * @param nargs Number of arguments (2 or 3)
     * @return String value containing substring
     * @throws std::invalid_argument if state is null
     */
    static Value sub(State* state, i32 nargs);

    /**
     * @brief Convert string to uppercase
     * @param state Lua state containing string argument
     * @param nargs Number of arguments (should be 1)
     * @return String value in uppercase
     * @throws std::invalid_argument if state is null
     */
    static Value upper(State* state, i32 nargs);

    /**
     * @brief Convert string to lowercase
     * @param state Lua state containing string argument
     * @param nargs Number of arguments (should be 1)
     * @return String value in lowercase
     * @throws std::invalid_argument if state is null
     */
    static Value lower(State* state, i32 nargs);

    /**
     * @brief Reverse string function
     * @param state Lua state containing string argument
     * @param nargs Number of arguments (should be 1)
     * @return String value reversed
     * @throws std::invalid_argument if state is null
     */
    static Value reverse(State* state, i32 nargs);

    /**
     * @brief Repeat string function
     * @param state Lua state containing string and count
     * @param nargs Number of arguments (should be 2)
     * @return String value repeated specified times
     * @throws std::invalid_argument if state is null
     */
    static Value rep(State* state, i32 nargs);

    // 模式匹配函数（简化版本）
    static Value find(State* state, i32 nargs);
    static Value match(State* state, i32 nargs);
    static Value gsub(State* state, i32 nargs);

    // 格式化函数
    static Value format(State* state, i32 nargs);
};

/**
 * @brief 便捷的StringLib初始化函数
 * @param state Lua状态机
 */
void initializeStringLib(State* state);

} // namespace Lua
