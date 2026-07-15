/**
 * @file mathlib.cpp
 * @brief Lua数学库实现
 * 
 * 使用现代C++流式API进行函数注册
 * 遵循Lua 5.1.5标准数学库规范
 * 
 * @author Lua C++ Project
 * @date 2025-12-19
 */

#include "lib/mathlib.hpp"
#include "common/number_conversion.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "vm/state/global_state.hpp"
#include <cmath>
#include <algorithm>
#include <limits>
#include <numbers>
#include <format>
#include <cstdlib>
#include <cerrno>
#include <cctype>
#include <ctime>

namespace Lua {

// =====================================================================
// 辅助函数
// =====================================================================

namespace {

constexpr f64 kPi = std::numbers::pi_v<f64>;

} // namespace

/**
 * @brief 从栈中获取数字参数
 * @param L Lua状态机指针
 * @param idx 参数索引（1-based）
 * @param argName 参数名称（用于错误消息）
 * @return 数字值
 */
static inline f64 getNumberArg(LuaState* L, i32 idx, const char* argName) {
    if (L->isNumber(idx)) {
        return L->toNumber(idx);
    }

    if (L->isString(idx)) {
        const Value& value = L->at(idx);
        if (value.isString()) {
            LuaNumber number = 0.0;
            if (luaStringToNumber(value.asString()->view(), number)) {
                return number;
            }
        }
    }

    {
        L->error(std::format("bad argument #{} to '{}' (number expected)", idx, argName).c_str());
    }
}

/**
 * @brief 检查参数数量
 * @param L Lua状态机指针
 * @param expected 期望的参数数量
 * @param funcName 函数名称
 */
static inline void checkArgCount(LuaState* L, i32 expected, const char* funcName) {
    i32 actual = L->getTop();
    if (actual < expected) {
        L->error(std::format("math.{}: expected {} argument(s), got {}",
                             funcName, expected, actual).c_str());
    }
}

// =====================================================================
// 基础数学函数实现
// =====================================================================

i32 math_abs(LuaState* L) {
    checkArgCount(L, 1, "abs");
    f64 x = getNumberArg(L, 1, "abs");
    L->pushNumber(std::abs(x));
    return 1;
}

i32 math_floor(LuaState* L) {
    checkArgCount(L, 1, "floor");
    f64 x = getNumberArg(L, 1, "floor");
    L->pushNumber(std::floor(x));
    return 1;
}

i32 math_ceil(LuaState* L) {
    checkArgCount(L, 1, "ceil");
    f64 x = getNumberArg(L, 1, "ceil");
    L->pushNumber(std::ceil(x));
    return 1;
}

i32 math_sqrt(LuaState* L) {
    checkArgCount(L, 1, "sqrt");
    f64 x = getNumberArg(L, 1, "sqrt");
    L->pushNumber(std::sqrt(x));
    return 1;
}

i32 math_pow(LuaState* L) {
    checkArgCount(L, 2, "pow");
    f64 x = getNumberArg(L, 1, "pow");
    f64 y = getNumberArg(L, 2, "pow");
    L->pushNumber(std::pow(x, y));
    return 1;
}

i32 math_fmod(LuaState* L) {
    checkArgCount(L, 2, "fmod");
    f64 x = getNumberArg(L, 1, "fmod");
    f64 y = getNumberArg(L, 2, "fmod");
    L->pushNumber(std::fmod(x, y));
    return 1;
}

i32 math_mod(LuaState* L) {
    checkArgCount(L, 2, "mod");
    f64 x = getNumberArg(L, 1, "mod");
    f64 y = getNumberArg(L, 2, "mod");
    L->pushNumber(x - std::floor(x / y) * y);
    return 1;
}

i32 math_modf(LuaState* L) {
    checkArgCount(L, 1, "modf");
    f64 x = getNumberArg(L, 1, "modf");
    f64 intpart;
    f64 fracpart = std::modf(x, &intpart);
    L->pushNumber(intpart);
    L->pushNumber(fracpart);
    return 2;
}

// =====================================================================
// 三角函数实现
// =====================================================================

i32 math_sin(LuaState* L) {
    checkArgCount(L, 1, "sin");
    f64 x = getNumberArg(L, 1, "sin");
    L->pushNumber(std::sin(x));
    return 1;
}

i32 math_cos(LuaState* L) {
    checkArgCount(L, 1, "cos");
    f64 x = getNumberArg(L, 1, "cos");
    L->pushNumber(std::cos(x));
    return 1;
}

i32 math_tan(LuaState* L) {
    checkArgCount(L, 1, "tan");
    f64 x = getNumberArg(L, 1, "tan");
    L->pushNumber(std::tan(x));
    return 1;
}

i32 math_sinh(LuaState* L) {
    checkArgCount(L, 1, "sinh");
    f64 x = getNumberArg(L, 1, "sinh");
    L->pushNumber(std::sinh(x));
    return 1;
}

i32 math_cosh(LuaState* L) {
    checkArgCount(L, 1, "cosh");
    f64 x = getNumberArg(L, 1, "cosh");
    L->pushNumber(std::cosh(x));
    return 1;
}

i32 math_tanh(LuaState* L) {
    checkArgCount(L, 1, "tanh");
    f64 x = getNumberArg(L, 1, "tanh");
    L->pushNumber(std::tanh(x));
    return 1;
}

i32 math_asin(LuaState* L) {
    checkArgCount(L, 1, "asin");
    f64 x = getNumberArg(L, 1, "asin");
    L->pushNumber(std::asin(x));
    return 1;
}

i32 math_acos(LuaState* L) {
    checkArgCount(L, 1, "acos");
    f64 x = getNumberArg(L, 1, "acos");
    L->pushNumber(std::acos(x));
    return 1;
}

i32 math_atan(LuaState* L) {
    checkArgCount(L, 1, "atan");
    f64 x = getNumberArg(L, 1, "atan");
    L->pushNumber(std::atan(x));
    return 1;
}

i32 math_atan2(LuaState* L) {
    checkArgCount(L, 2, "atan2");
    f64 y = getNumberArg(L, 1, "atan2");
    f64 x = getNumberArg(L, 2, "atan2");
    L->pushNumber(std::atan2(y, x));
    return 1;
}

// =====================================================================
// 对数和指数函数实现
// =====================================================================

i32 math_exp(LuaState* L) {
    checkArgCount(L, 1, "exp");
    f64 x = getNumberArg(L, 1, "exp");
    L->pushNumber(std::exp(x));
    return 1;
}

i32 math_log(LuaState* L) {
    checkArgCount(L, 1, "log");
    f64 x = getNumberArg(L, 1, "log");
    L->pushNumber(std::log(x));
    return 1;
}

i32 math_log10(LuaState* L) {
    checkArgCount(L, 1, "log10");
    f64 x = getNumberArg(L, 1, "log10");
    L->pushNumber(std::log10(x));
    return 1;
}

i32 math_ldexp(LuaState* L) {
    checkArgCount(L, 2, "ldexp");
    f64 m = getNumberArg(L, 1, "ldexp");
    f64 eVal = getNumberArg(L, 2, "ldexp");
    i32 e = static_cast<i32>(eVal);
    L->pushNumber(std::ldexp(m, e));
    return 1;
}

i32 math_frexp(LuaState* L) {
    checkArgCount(L, 1, "frexp");
    f64 x = getNumberArg(L, 1, "frexp");
    i32 e;
    f64 m = std::frexp(x, &e);
    L->pushNumber(m);
    L->pushNumber(static_cast<f64>(e));
    return 2;
}

// =====================================================================
// 最值函数实现
// =====================================================================

i32 math_min(LuaState* L) {
    i32 n = L->getTop();
    if (n == 0) {
        L->error("math.min: expected at least 1 argument");
    }

    f64 minVal = getNumberArg(L, 1, "min");
    for (i32 i = 2; i <= n; i++) {
        f64 val = getNumberArg(L, i, "min");
        if (val < minVal) {
            minVal = val;
        }
    }

    L->pushNumber(minVal);
    return 1;
}

i32 math_max(LuaState* L) {
    i32 n = L->getTop();
    if (n == 0) {
        L->error("math.max: expected at least 1 argument");
    }

    f64 maxVal = getNumberArg(L, 1, "max");
    for (i32 i = 2; i <= n; i++) {
        f64 val = getNumberArg(L, i, "max");
        if (val > maxVal) {
            maxVal = val;
        }
    }

    L->pushNumber(maxVal);
    return 1;
}

// =====================================================================
// 随机数函数实现
// =====================================================================

i32 math_random(LuaState* L) {
    // 使用静态变量确保只初始化一次（如果未设置种子）
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }

    i32 n = L->getTop();
    
    if (n == 0) {
        // math.random() - 返回 [0, 1) 之间的浮点数
        f64 r = static_cast<f64>(std::rand()) / (static_cast<f64>(RAND_MAX) + 1.0);
        L->pushNumber(r);
    } else if (n == 1) {
        // math.random(m) - 返回 [1, m] 之间的整数
        f64 mVal = getNumberArg(L, 1, "random");
        i32 m = static_cast<i32>(mVal);
        if (m < 1) {
            L->error("math.random: interval is empty");
        }
        i32 r = (std::rand() % m) + 1;
        L->pushNumber(static_cast<f64>(r));
    } else {
        // math.random(m, n) - 返回 [m, n] 之间的整数
        f64 mVal = getNumberArg(L, 1, "random");
        f64 nVal = getNumberArg(L, 2, "random");
        i32 m = static_cast<i32>(mVal);
        i32 n_int = static_cast<i32>(nVal);
        if (m > n_int) {
            L->error("math.random: interval is empty");
        }
        i32 r = (std::rand() % (n_int - m + 1)) + m;
        L->pushNumber(static_cast<f64>(r));
    }

    return 1;
}

i32 math_randomseed(LuaState* L) {
    checkArgCount(L, 1, "randomseed");
    f64 seedVal = getNumberArg(L, 1, "randomseed");
    unsigned int seed = static_cast<unsigned int>(seedVal);
    std::srand(seed);
    return 0;
}

// =====================================================================
// 角度转换函数实现
// =====================================================================

i32 math_deg(LuaState* L) {
    checkArgCount(L, 1, "deg");
    f64 x = getNumberArg(L, 1, "deg");
    L->pushNumber(x * 180.0 / kPi);
    return 1;
}

i32 math_rad(LuaState* L) {
    checkArgCount(L, 1, "rad");
    f64 x = getNumberArg(L, 1, "rad");
    L->pushNumber(x * kPi / 180.0);
    return 1;
}

// =====================================================================
// 数学库注册入口（使用现代C++流式API）
// =====================================================================

void MathLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    // 创建 math 表
    Table* mathTable = FunctionRegistrar::createLibTable(L, "math");
    if (!mathTable) {
        L->error("Failed to create math library table");
        return;
    }

    // 使用流式API注册所有数学函数到 math 表
    FunctionRegistrar(L)
        // 基础数学函数
        .addGlobal("abs", math_abs)
        .addGlobal("floor", math_floor)
        .addGlobal("ceil", math_ceil)
        .addGlobal("sqrt", math_sqrt)
        .addGlobal("pow", math_pow)
        .addGlobal("fmod", math_fmod)
        .addGlobal("mod", math_mod)
        .addGlobal("modf", math_modf)
        // 三角函数
        .addGlobal("sin", math_sin)
        .addGlobal("cos", math_cos)
        .addGlobal("tan", math_tan)
        .addGlobal("sinh", math_sinh)
        .addGlobal("cosh", math_cosh)
        .addGlobal("tanh", math_tanh)
        .addGlobal("asin", math_asin)
        .addGlobal("acos", math_acos)
        .addGlobal("atan", math_atan)
        .addGlobal("atan2", math_atan2)
        // 对数和指数函数
        .addGlobal("exp", math_exp)
        .addGlobal("log", math_log)
        .addGlobal("log10", math_log10)
        .addGlobal("ldexp", math_ldexp)
        .addGlobal("frexp", math_frexp)
        // 最值函数
        .addGlobal("min", math_min)
        .addGlobal("max", math_max)
        // 随机数函数
        .addGlobal("random", math_random)
        .addGlobal("randomseed", math_randomseed)
        // 角度转换函数
        .addGlobal("deg", math_deg)
        .addGlobal("rad", math_rad)
        .commitToTable(mathTable);
}

void MathLibModule::initialize(LuaState* L) {
    if (!L) {
        return;
    }

    // 获取 math 表并设置数学常量
    auto& gs = L->getGlobalState();
    GCString* mathKey = gs.getStringPool().intern("math");
    Value mathTableVal = L->getGlobal(mathKey->getData());
    
    if (mathTableVal.isTable()) {
        Table* mathTable = mathTableVal.asTable();
        
        // 设置 math.pi
        GCString* piKey = gs.getStringPool().intern("pi");
        mathTable->set(Value(piKey), Value(kPi));
        
        // 设置 math.huge
        GCString* hugeKey = gs.getStringPool().intern("huge");
        mathTable->set(Value(hugeKey), Value(std::numeric_limits<f64>::infinity()));
    }
}

void openMathLib(LuaState* L) {
    if (!L) {
        return;
    }

    L->requireStandardLibrary("math");
    MathLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
