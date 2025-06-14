#pragma once

#include "lib_module.hpp"
#include "error_handling.hpp"
#include "type_conversion.hpp"
#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include <memory>
#include <limits>

namespace Lua {
    /**
     * 数学常量
     * 使用简化的类型系统
     */
    namespace MathConstants {
        constexpr f64 PI = 3.14159265358979323846;
        constexpr f64 E = 2.71828182845904523536;
        constexpr f64 SQRT2 = 1.41421356237309504880;
        constexpr f64 SQRT3 = 1.73205080756887729352;
        constexpr f64 LN2 = 0.69314718055994530942;
        constexpr f64 LN10 = 2.30258509299404568402;
        constexpr f64 LOG2E = 1.44269504088896340736;
        constexpr f64 LOG10E = 0.43429448190325182765;
    }
    
    /**
     * 数学工具函数
     */
    namespace MathUtils {
        /**
         * 角度转弧度
         */
        inline f64 degToRad(f64 degrees) {
            return degrees * MathConstants::PI / 180.0;
        }
        
        /**
         * 弧度转角度
         */
        inline f64 radToDeg(f64 radians) {
            return radians * 180.0 / MathConstants::PI;
        }
        
        /**
         * 检查是否为有限数
         */
        inline bool isFinite(f64 value) {
            return std::isfinite(value);
        }
        
        /**
         * 检查是否为NaN
         */
        inline bool isNaN(f64 value) {
            return std::isnan(value);
        }
        
        /**
         * 检查是否为无穷大
         */
        inline bool isInfinite(f64 value) {
            return std::isinf(value);
        }
        
        /**
         * 安全的除法
         */
        inline f64 safeDivide(f64 a, f64 b, f64 defaultValue = 0.0) {
            if (std::abs(b) < std::numeric_limits<f64>::epsilon()) {
                return defaultValue;
            }
            return a / b;
        }
        
        /**
         * 线性插值
         */
        inline f64 lerp(f64 a, f64 b, f64 t) {
            return a + t * (b - a);
        }
        
        /**
         * 平滑步函数
         */
        inline f64 smoothstep(f64 edge0, f64 edge1, f64 x) {
            f64 t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
            return t * t * (3.0 - 2.0 * t);
        }
    }
    
    /**
     * 随机数生成器
     */
    class RandomGenerator {
    public:
        RandomGenerator();
        explicit RandomGenerator(u32 seed);
        
        /**
         * 生成[0, 1)范围的随机数
         */
        f64 random();
        
        /**
         * 生成[min, max)范围的随机数
         */
        f64 random(f64 min, f64 max);
        
        /**
         * 生成[min, max]范围的随机整数
         */
        i32 randomInt(i32 min, i32 max);
        
        /**
         * 设置种子
         */
        void setSeed(u32 seed);
        
    private:
        std::mt19937 gen_;
        std::uniform_real_distribution<f64> dist_;
    };
    
    /**
     * 数学库模块
     * 提供各种数学函数和常量
     */
    class MathLib : public LibModule {
    public:
        MathLib();
        
        StrView getName() const noexcept override;
        void registerFunctions(FunctionRegistry& registry) override;
        
    private:
        UPtr<RandomGenerator> rng_;
        
        void registerConstants(FunctionRegistry& registry);
        
        // 基础数学函数声明
        static Value absFunc(State* state, i32 nargs);
        static Value floorFunc(State* state, i32 nargs);
        static Value ceilFunc(State* state, i32 nargs);
        static Value roundFunc(State* state, i32 nargs);
        static Value truncFunc(State* state, i32 nargs);
        
        // 幂函数声明
        static Value powFunc(State* state, i32 nargs);
        static Value sqrtFunc(State* state, i32 nargs);
        static Value cbrtFunc(State* state, i32 nargs);
        static Value expFunc(State* state, i32 nargs);
        static Value exp2Func(State* state, i32 nargs);
        
        // 对数函数声明
        static Value logFunc(State* state, i32 nargs);
        static Value log2Func(State* state, i32 nargs);
        static Value log10Func(State* state, i32 nargs);
        
        // 三角函数声明
        static Value sinFunc(State* state, i32 nargs);
        static Value cosFunc(State* state, i32 nargs);
        static Value tanFunc(State* state, i32 nargs);
        static Value asinFunc(State* state, i32 nargs);
        static Value acosFunc(State* state, i32 nargs);
        static Value atanFunc(State* state, i32 nargs);
        static Value atan2Func(State* state, i32 nargs);
        
        // 双曲函数声明
        static Value sinhFunc(State* state, i32 nargs);
        static Value coshFunc(State* state, i32 nargs);
        static Value tanhFunc(State* state, i32 nargs);
        
        // 角度转换声明
        static Value degFunc(State* state, i32 nargs);
        static Value radFunc(State* state, i32 nargs);
        
        // 最值函数声明
        static Value minFunc(State* state, i32 nargs);
        static Value maxFunc(State* state, i32 nargs);
        static Value clampFunc(State* state, i32 nargs);
        
        // 随机数函数声明（非静态，需要访问实例成员）
        Value randomFunc(State* state, i32 nargs);
        Value randomseedFunc(State* state, i32 nargs);
        Value randomintFunc(State* state, i32 nargs);
        
        // 工具函数声明
        static Value signFunc(State* state, i32 nargs);
        static Value fmodFunc(State* state, i32 nargs);
        static Value modfFunc(State* state, i32 nargs);
        static Value frexpFunc(State* state, i32 nargs);
        static Value ldexpFunc(State* state, i32 nargs);
        
        // 检查函数声明
        static Value isfiniteFunc(State* state, i32 nargs);
        static Value isnanFunc(State* state, i32 nargs);
        static Value isinfFunc(State* state, i32 nargs);
        
        // 插值函数声明
        static Value lerpFunc(State* state, i32 nargs);
        static Value smoothstepFunc(State* state, i32 nargs);
    };
}