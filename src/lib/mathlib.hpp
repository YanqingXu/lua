/**
 * @file mathlib.hpp
 * @brief Lua数学库：标准数学函数集合
 * 
 * 详细说明：
 * 本模块实现了Lua 5.1.5标准数学库的完整功能，提供了常用的数学运算、
 * 三角函数、对数指数函数、随机数生成等功能。
 * 
 * 主要功能：
 * 1. 基础数学运算：abs, floor, ceil, sqrt, pow, fmod
 * 2. 三角函数：sin, cos, tan, asin, acos, atan, atan2
 * 3. 对数和指数：log, log10, exp, ldexp, frexp
 * 4. 最值函数：min, max
 * 5. 随机数：random, randomseed
 * 6. 角度转换：deg, rad
 * 7. 数值分解：modf
 * 
 * 数学常量：
 * - math.pi: 圆周率π
 * - math.huge: 正无穷大（HUGE_VAL）
 * 
 * 参考实现：
 * - lua_c_analysis/src/lmathlib.c 中的C实现
 * - lua_with_cpp/src/lib/math/ 中的C++实现参考
 * 
 * @author Lua C++ Project
 * @date 2025-12-19
 */

#pragma once

#include "common/types.hpp"
#include "lib/lib_module.hpp"
#include "vm/lua_state.hpp"

namespace Lua {

/**
 * @brief 数学库模块
 * 
 * 实现Lua标准数学库的所有功能。所有函数都注册在全局表"math"中。
 */
class MathLibModule : public LibModule {
public:
	const char* getName() const override { return "math"; }

	void registerFunctions(LuaState* L) override;

	void initialize(LuaState* L) override;
};

/**
 * @brief 注册数学库到全局环境
 * @param L Lua状态机指针
 * 
 * 创建全局"math"表并注册所有数学函数和常量。
 */
void openMathLib(LuaState* L);

// =====================================================================
// 基础数学函数声明
// =====================================================================

/**
 * @brief math.abs(x) - 返回x的绝对值
 * @param L Lua状态机指针
 * @return 返回值数量（1个：绝对值）
 */
i32 math_abs(LuaState* L);

/**
 * @brief math.floor(x) - 返回不大于x的最大整数
 * @param L Lua状态机指针
 * @return 返回值数量（1个：floor值）
 */
i32 math_floor(LuaState* L);

/**
 * @brief math.ceil(x) - 返回不小于x的最小整数
 * @param L Lua状态机指针
 * @return 返回值数量（1个：ceil值）
 */
i32 math_ceil(LuaState* L);

/**
 * @brief math.sqrt(x) - 返回x的平方根
 * @param L Lua状态机指针
 * @return 返回值数量（1个：平方根）
 */
i32 math_sqrt(LuaState* L);

/**
 * @brief math.pow(x, y) - 返回x的y次方
 * @param L Lua状态机指针
 * @return 返回值数量（1个：幂运算结果）
 */
i32 math_pow(LuaState* L);

/**
 * @brief math.fmod(x, y) - 返回x除以y的余数
 * @param L Lua状态机指针
 * @return 返回值数量（1个：余数）
 */
i32 math_fmod(LuaState* L);

/**
 * @brief math.modf(x) - 返回x的整数和小数部分
 * @param L Lua状态机指针
 * @return 返回值数量（2个：整数部分，小数部分）
 */
i32 math_modf(LuaState* L);

// =====================================================================
// 三角函数声明
// =====================================================================

/**
 * @brief math.sin(x) - 返回x的正弦值（x为弧度）
 * @param L Lua状态机指针
 * @return 返回值数量（1个：正弦值）
 */
i32 math_sin(LuaState* L);

/**
 * @brief math.cos(x) - 返回x的余弦值（x为弧度）
 * @param L Lua状态机指针
 * @return 返回值数量（1个：余弦值）
 */
i32 math_cos(LuaState* L);

/**
 * @brief math.tan(x) - 返回x的正切值（x为弧度）
 * @param L Lua状态机指针
 * @return 返回值数量（1个：正切值）
 */
i32 math_tan(LuaState* L);

i32 math_sinh(LuaState* L);

i32 math_cosh(LuaState* L);

i32 math_tanh(LuaState* L);

/**
 * @brief math.asin(x) - 返回x的反正弦值（结果为弧度）
 * @param L Lua状态机指针
 * @return 返回值数量（1个：反正弦值）
 */
i32 math_asin(LuaState* L);

/**
 * @brief math.acos(x) - 返回x的反余弦值（结果为弧度）
 * @param L Lua状态机指针
 * @return 返回值数量（1个：反余弦值）
 */
i32 math_acos(LuaState* L);

/**
 * @brief math.atan(x) - 返回x的反正切值（结果为弧度）
 * @param L Lua状态机指针
 * @return 返回值数量（1个：反正切值）
 */
i32 math_atan(LuaState* L);

/**
 * @brief math.atan2(y, x) - 返回y/x的反正切值（结果为弧度）
 * @param L Lua状态机指针
 * @return 返回值数量（1个：反正切值）
 */
i32 math_atan2(LuaState* L);

// =====================================================================
// 对数和指数函数声明
// =====================================================================

/**
 * @brief math.exp(x) - 返回e的x次方
 * @param L Lua状态机指针
 * @return 返回值数量（1个：指数值）
 */
i32 math_exp(LuaState* L);

/**
 * @brief math.log(x) - 返回x的自然对数
 * @param L Lua状态机指针
 * @return 返回值数量（1个：对数值）
 */
i32 math_log(LuaState* L);

/**
 * @brief math.log10(x) - 返回x的以10为底的对数
 * @param L Lua状态机指针
 * @return 返回值数量（1个：对数值）
 */
i32 math_log10(LuaState* L);

/**
 * @brief math.ldexp(m, e) - 返回m * 2^e
 * @param L Lua状态机指针
 * @return 返回值数量（1个：结果值）
 */
i32 math_ldexp(LuaState* L);

/**
 * @brief math.frexp(x) - 将x分解为尾数m和指数e，使得x = m * 2^e
 * @param L Lua状态机指针
 * @return 返回值数量（2个：尾数，指数）
 */
i32 math_frexp(LuaState* L);

// =====================================================================
// 最值函数声明
// =====================================================================

/**
 * @brief math.min(...) - 返回所有参数中的最小值
 * @param L Lua状态机指针
 * @return 返回值数量（1个：最小值）
 */
i32 math_min(LuaState* L);

/**
 * @brief math.max(...) - 返回所有参数中的最大值
 * @param L Lua状态机指针
 * @return 返回值数量（1个：最大值）
 */
i32 math_max(LuaState* L);

// =====================================================================
// 随机数函数声明
// =====================================================================

/**
 * @brief math.random([m [, n]]) - 生成伪随机数
 * 
 * 三种调用方式：
 * - math.random(): 返回[0,1)之间的随机浮点数
 * - math.random(m): 返回[1,m]之间的随机整数
 * - math.random(m, n): 返回[m,n]之间的随机整数
 * 
 * @param L Lua状态机指针
 * @return 返回值数量（1个：随机数）
 */
i32 math_random(LuaState* L);

/**
 * @brief math.randomseed(x) - 设置伪随机数生成器的种子
 * @param L Lua状态机指针
 * @return 返回值数量（0个）
 */
i32 math_randomseed(LuaState* L);

// =====================================================================
// 角度转换函数声明
// =====================================================================

/**
 * @brief math.deg(x) - 将弧度转换为角度
 * @param L Lua状态机指针
 * @return 返回值数量（1个：角度值）
 */
i32 math_deg(LuaState* L);

/**
 * @brief math.rad(x) - 将角度转换为弧度
 * @param L Lua状态机指针
 * @return 返回值数量（1个：弧度值）
 */
i32 math_rad(LuaState* L);

} // namespace Lua
