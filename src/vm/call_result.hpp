/**
 * @file call_result.hpp
 * @brief 多返回值结构体定义
 * 
 * 用于处理 Lua 函数的多返回值特性。
 * 
 * @author Lua C++ Project
 * @date 2025-01-23
 */

#pragma once

#include "common/types.hpp"
#include "core/value.hpp"

namespace Lua {

/**
 * @brief 函数调用结果结构体
 * 
 * 封装函数调用的多个返回值。
 */
struct CallResult {
    Vec<Value> values;  ///< 返回值列表
    usize count;        ///< 返回值数量
    
    /**
     * @brief 默认构造函数 - 无返回值
     */
    CallResult() : count(0) {}
    
    /**
     * @brief 单返回值构造函数
     * @param singleValue 单个返回值
     */
    explicit CallResult(const Value& singleValue) : count(1) {
        values.push_back(singleValue);
    }
    
    /**
     * @brief 多返回值构造函数
     * @param multipleValues 多个返回值
     */
    CallResult(const Vec<Value>& multipleValues) 
        : values(multipleValues), count(multipleValues.size()) {}
    
    /**
     * @brief 获取第一个返回值（向后兼容）
     * @return 第一个返回值，如果没有则返回 nil
     */
    Value getFirst() const {
        return count > 0 ? values[0] : Value();
    }
    
    /**
     * @brief 获取指定索引的返回值
     * @param index 索引（从0开始）
     * @return 指定索引的返回值
     */
    Value getValue(usize index) const {
        return index < count ? values[index] : Value();
    }
    
    /**
     * @brief 检查是否有返回值
     * @return 如果有返回值返回 true，否则返回 false
     */
    bool hasValues() const {
        return count > 0;
    }
};

} // namespace Lua

