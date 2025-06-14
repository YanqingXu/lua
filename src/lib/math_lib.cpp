#include "math_lib.hpp"

namespace Lua {
    // RandomGenerator 实现
    RandomGenerator::RandomGenerator() : gen_(std::random_device{}()), dist_(0.0, 1.0) {}
    
    RandomGenerator::RandomGenerator(u32 seed) : gen_(seed), dist_(0.0, 1.0) {}
    
    f64 RandomGenerator::random() {
        return dist_(gen_);
    }
    
    f64 RandomGenerator::random(f64 min, f64 max) {
        return min + (max - min) * random();
    }
    
    i32 RandomGenerator::randomInt(i32 min, i32 max) {
        std::uniform_int_distribution<i32> intDist(min, max);
        return intDist(gen_);
    }
    
    void RandomGenerator::setSeed(u32 seed) {
        gen_.seed(seed);
    }
    
    // MathLib 实现
    MathLib::MathLib() : rng_(std::make_unique<RandomGenerator>()) {}
    
    StrView MathLib::getName() const noexcept {
        return "math";
    }
    
    void MathLib::registerFunctions(FunctionRegistry& registry) {
        // 基础数学函数
        REGISTER_FUNCTION(registry, abs, absFunc);
        REGISTER_FUNCTION(registry, floor, floorFunc);
        REGISTER_FUNCTION(registry, ceil, ceilFunc);
        REGISTER_FUNCTION(registry, round, roundFunc);
        REGISTER_FUNCTION(registry, trunc, truncFunc);
        
        // 幂函数
        REGISTER_FUNCTION(registry, pow, powFunc);
        REGISTER_FUNCTION(registry, sqrt, sqrtFunc);
        REGISTER_FUNCTION(registry, cbrt, cbrtFunc);
        REGISTER_FUNCTION(registry, exp, expFunc);
        REGISTER_FUNCTION(registry, exp2, exp2Func);
        
        // 对数函数
        REGISTER_FUNCTION(registry, log, logFunc);
        REGISTER_FUNCTION(registry, log2, log2Func);
        REGISTER_FUNCTION(registry, log10, log10Func);
        
        // 三角函数
        REGISTER_FUNCTION(registry, sin, sinFunc);
        REGISTER_FUNCTION(registry, cos, cosFunc);
        REGISTER_FUNCTION(registry, tan, tanFunc);
        REGISTER_FUNCTION(registry, asin, asinFunc);
        REGISTER_FUNCTION(registry, acos, acosFunc);
        REGISTER_FUNCTION(registry, atan, atanFunc);
        REGISTER_FUNCTION(registry, atan2, atan2Func);
        
        // 双曲函数
        REGISTER_FUNCTION(registry, sinh, sinhFunc);
        REGISTER_FUNCTION(registry, cosh, coshFunc);
        REGISTER_FUNCTION(registry, tanh, tanhFunc);
        
        // 角度转换
        REGISTER_FUNCTION(registry, deg, degFunc);
        REGISTER_FUNCTION(registry, rad, radFunc);
        
        // 最值函数
        REGISTER_FUNCTION(registry, min, minFunc);
        REGISTER_FUNCTION(registry, max, maxFunc);
        REGISTER_FUNCTION(registry, clamp, clampFunc);
        
        // 随机数函数（非静态函数，需要特殊处理）
        registry.registerFunction("random", [this](State* s, i32 n) -> Value { return randomFunc(s, n); });
        registry.registerFunction("randomseed", [this](State* s, i32 n) -> Value { return randomseedFunc(s, n); });
        registry.registerFunction("randomint", [this](State* s, i32 n) -> Value { return randomintFunc(s, n); });
        
        // 工具函数
        REGISTER_FUNCTION(registry, sign, signFunc);
        REGISTER_FUNCTION(registry, fmod, fmodFunc);
        REGISTER_FUNCTION(registry, modf, modfFunc);
        REGISTER_FUNCTION(registry, frexp, frexpFunc);
        REGISTER_FUNCTION(registry, ldexp, ldexpFunc);
        
        // 检查函数
        REGISTER_FUNCTION(registry, isfinite, isfiniteFunc);
        REGISTER_FUNCTION(registry, isnan, isnanFunc);
        REGISTER_FUNCTION(registry, isinf, isinfFunc);
        
        // 插值函数
        REGISTER_FUNCTION(registry, lerp, lerpFunc);
        REGISTER_FUNCTION(registry, smoothstep, smoothstepFunc);
        
        // 注册常量
        registerConstants(registry);
    }
    
    void MathLib::registerConstants(FunctionRegistry& registry) {
        // 使用lambda注册常量
        registry.registerFunction("pi", [](State*, i32) -> Value {
            return Value(MathConstants::PI);
        });
        
        registry.registerFunction("e", [](State*, i32) -> Value {
            return Value(MathConstants::E);
        });
        
        registry.registerFunction("sqrt2", [](State*, i32) -> Value {
            return Value(MathConstants::SQRT2);
        });
        
        registry.registerFunction("sqrt3", [](State*, i32) -> Value {
            return Value(MathConstants::SQRT3);
        });
        
        registry.registerFunction("ln2", [](State*, i32) -> Value {
            return Value(MathConstants::LN2);
        });
        
        registry.registerFunction("ln10", [](State*, i32) -> Value {
            return Value(MathConstants::LN10);
        });
        
        registry.registerFunction("huge", [](State*, i32) -> Value {
            return Value(std::numeric_limits<f64>::infinity());
        });
        
        registry.registerFunction("mininteger", [](State*, i32) -> Value {
            return Value(static_cast<f64>(std::numeric_limits<LuaInteger>::min()));
        });
        
        registry.registerFunction("maxinteger", [](State*, i32) -> Value {
            return Value(static_cast<f64>(std::numeric_limits<LuaInteger>::max()));
        });
    }
    
    // 基础数学函数实现
    Value MathLib::absFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "abs", f64);
        return Value(std::abs(x));
    }
    
    Value MathLib::floorFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "floor", f64);
        return Value(std::floor(x));
    }
    
    Value MathLib::ceilFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "ceil", f64);
        return Value(std::ceil(x));
    }
    
    Value MathLib::roundFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "round", f64);
        return Value(std::round(x));
    }
    
    Value MathLib::truncFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "trunc", f64);
        return Value(std::trunc(x));
    }
    
    // 幂函数实现
    Value MathLib::powFunc(State* state, i32 nargs) {
        auto [x, y] = EXTRACT_ARGS(state, nargs, "pow", f64, f64);
        auto result = std::pow(x, y);
        if (!MathUtils::isFinite(result)) {
            throw LibException(LibErrorCode::OutOfRange, StrView("pow: result is not finite"));
        }
        return Value(result);
    }
    
    Value MathLib::sqrtFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "sqrt", f64);
        if (x < 0) {
            throw LibException(LibErrorCode::InvalidArgument, StrView("sqrt: negative argument"));
        }
        return Value(std::sqrt(x));
    }
    
    Value MathLib::cbrtFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "cbrt", f64);
        return Value(std::cbrt(x));
    }
    
    Value MathLib::expFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "exp", f64);
        auto result = std::exp(x);
        if (!MathUtils::isFinite(result)) {
            throw LibException(LibErrorCode::OutOfRange, StrView("exp: result overflow"));
        }
        return Value(result);
    }
    
    Value MathLib::exp2Func(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "exp2", f64);
        auto result = std::exp2(x);
        if (!MathUtils::isFinite(result)) {
            throw LibException(LibErrorCode::OutOfRange, StrView("exp2: result overflow"));
        }
        return Value(result);
    }
    
    // 对数函数实现
    Value MathLib::logFunc(State* state, i32 nargs) {
        if (nargs == 1) {
            auto [x] = EXTRACT_ARGS(state, nargs, "log", f64);
            if (x <= 0) {
                throw LibException(LibErrorCode::InvalidArgument, StrView("log: non-positive argument"));
            }
            return Value(std::log(x));
        } else {
            auto [x, base] = EXTRACT_ARGS(state, nargs, "log", f64, f64);
            if (x <= 0 || base <= 0 || base == 1) {
                throw LibException(LibErrorCode::InvalidArgument, StrView("log: invalid arguments"));
            }
            return Value(std::log(x) / std::log(base));
        }
    }
    
    Value MathLib::log2Func(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "log2", f64);
        if (x <= 0) {
            throw LibException(LibErrorCode::InvalidArgument, StrView("log2: non-positive argument"));
        }
        return Value(std::log2(x));
    }
    
    Value MathLib::log10Func(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "log10", f64);
        if (x <= 0) {
            throw LibException(LibErrorCode::InvalidArgument, StrView("log10: non-positive argument"));
        }
        return Value(std::log10(x));
    }
    
    // 三角函数实现
    Value MathLib::sinFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "sin", f64);
        return Value(std::sin(x));
    }
    
    Value MathLib::cosFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "cos", f64);
        return Value(std::cos(x));
    }
    
    Value MathLib::tanFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "tan", f64);
        return Value(std::tan(x));
    }
    
    Value MathLib::asinFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "asin", f64);
        if (x < -1 || x > 1) {
            throw LibException(LibErrorCode::InvalidArgument, StrView("asin: argument out of range [-1, 1]"));
        }
        return Value(std::asin(x));
    }
    
    Value MathLib::acosFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "acos", f64);
        if (x < -1 || x > 1) {
            throw LibException(LibErrorCode::InvalidArgument, StrView("acos: argument out of range [-1, 1]"));
        }
        return Value(std::acos(x));
    }
    
    Value MathLib::atanFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "atan", f64);
        return Value(std::atan(x));
    }
    
    Value MathLib::atan2Func(State* state, i32 nargs) {
        auto [y, x] = EXTRACT_ARGS(state, nargs, "atan2", f64, f64);
        return Value(std::atan2(y, x));
    }
    
    // 双曲函数实现
    Value MathLib::sinhFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "sinh", f64);
        return Value(std::sinh(x));
    }
    
    Value MathLib::coshFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "cosh", f64);
        return Value(std::cosh(x));
    }
    
    Value MathLib::tanhFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "tanh", f64);
        return Value(std::tanh(x));
    }
    
    // 角度转换实现
    Value MathLib::degFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "deg", f64);
        return Value(MathUtils::radToDeg(x));
    }
    
    Value MathLib::radFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "rad", f64);
        return Value(MathUtils::degToRad(x));
    }
    
    // 最值函数实现
    Value MathLib::minFunc(State* state, i32 nargs) {
        ErrorUtils::checkArgCount(nargs, 1, "min");
        
        f64 result = TypeConverter::toF64(state->get(1), "min");
        for (i32 i = 2; i <= nargs; ++i) {
            f64 value = TypeConverter::toF64(state->get(i), "min");
            result = std::min(result, value);
        }
        return Value(result);
    }
    
    Value MathLib::maxFunc(State* state, i32 nargs) {
        ErrorUtils::checkArgCount(nargs, 1, "max");
        
        f64 result = TypeConverter::toF64(state->get(1), "max");
        for (i32 i = 2; i <= nargs; ++i) {
            f64 value = TypeConverter::toF64(state->get(i), "max");
            result = std::max(result, value);
        }
        return Value(result);
    }
    
    Value MathLib::clampFunc(State* state, i32 nargs) {
        auto [x, min, max] = EXTRACT_ARGS(state, nargs, "clamp", f64, f64, f64);
        return Value(std::clamp(x, min, max));
    }
    
    // 随机数函数实现（需要访问实例成员）
    Value MathLib::randomFunc(State* state, i32 nargs) {
        if (nargs == 0) {
            return Value(rng_->random());
        } else if (nargs == 1) {
            auto [max] = EXTRACT_ARGS(state, nargs, "random", f64);
            return Value(rng_->random(0.0, max));
        } else {
            auto [min, max] = EXTRACT_ARGS(state, nargs, "random", f64, f64);
            return Value(rng_->random(min, max));
        }
    }
    
    Value MathLib::randomseedFunc(State* state, i32 nargs) {
        auto [seed] = EXTRACT_ARGS(state, nargs, "randomseed", u32);
        rng_->setSeed(seed);
        return Value(nullptr);
    }
    
    Value MathLib::randomintFunc(State* state, i32 nargs) {
        auto [min, max] = EXTRACT_ARGS(state, nargs, "randomint", i32, i32);
        return Value(static_cast<f64>(rng_->randomInt(min, max)));
    }
    
    // 工具函数实现
    Value MathLib::signFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "sign", f64);
        if (x > 0) return Value(1.0);
        if (x < 0) return Value(-1.0);
        return Value(0.0);
    }
    
    Value MathLib::fmodFunc(State* state, i32 nargs) {
        auto [x, y] = EXTRACT_ARGS(state, nargs, "fmod", f64, f64);
        if (y == 0) {
            throw LibException(LibErrorCode::InvalidArgument, StrView("fmod: division by zero"));
        }
        return Value(std::fmod(x, y));
    }
    
    Value MathLib::modfFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "modf", f64);
        f64 intpart;
        f64 fracpart = std::modf(x, &intpart);
        
        // 返回两个值：整数部分和小数部分
        state->push(Value(intpart));
        state->push(Value(fracpart));
        return Value(2); // 返回值数量
    }
    
    Value MathLib::frexpFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "frexp", f64);
        i32 exp;
        f64 mantissa = std::frexp(x, &exp);
        
        state->push(Value(mantissa));
        state->push(Value(static_cast<f64>(exp)));
        return Value(2);
    }
    
    Value MathLib::ldexpFunc(State* state, i32 nargs) {
        auto [x, exp] = EXTRACT_ARGS(state, nargs, "ldexp", f64, i32);
        return Value(std::ldexp(x, exp));
    }
    
    // 检查函数实现
    Value MathLib::isfiniteFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "isfinite", f64);
        return Value(MathUtils::isFinite(x));
    }
    
    Value MathLib::isnanFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "isnan", f64);
        return Value(MathUtils::isNaN(x));
    }
    
    Value MathLib::isinfFunc(State* state, i32 nargs) {
        auto [x] = EXTRACT_ARGS(state, nargs, "isinf", f64);
        return Value(MathUtils::isInfinite(x));
    }
    
    // 插值函数实现
    Value MathLib::lerpFunc(State* state, i32 nargs) {
        auto [a, b, t] = EXTRACT_ARGS(state, nargs, "lerp", f64, f64, f64);
        return Value(MathUtils::lerp(a, b, t));
    }
    
    Value MathLib::smoothstepFunc(State* state, i32 nargs) {
        auto [edge0, edge1, x] = EXTRACT_ARGS(state, nargs, "smoothstep", f64, f64, f64);
        return Value(MathUtils::smoothstep(edge0, edge1, x));
    }
}