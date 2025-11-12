/**
 * @file gc_string.cpp
 * @brief GCString类的实现
 */

#include "core/gc_string.hpp"

namespace Lua {

/**
 * @brief 构造函数 - 从字符串视图创建GCString
 */
GCString::GCString(StrView str)
    : GCObject(GCObjectType::String)
    , hash_(computeHash(str))
    , length_(str.length())
    , data_(str)
{
}

/**
 * @brief 获取对象占用的内存大小
 */
usize GCString::getSize() const {
    // GCString对象大小 = 基类大小 + 成员变量大小 + 字符串数据大小
    // 注意：Str可能有SSO（小字符串优化），但我们按实际容量计算
    return sizeof(GCString) + data_.capacity();
}

/**
 * @brief 计算字符串的哈希值
 *
 * 实现说明：
 * 使用与Lua 5.1.5相似的哈希算法。对于长字符串，采用采样策略：
 * 不是扫描整个字符串，而是以固定步长采样，以提高性能。
 *
 * 算法特点：
 * - 短字符串：扫描所有字符
 * - 长字符串：采样计算（每隔一定步长取一个字符）
 * - 包含字符串长度，避免不同长度字符串的哈希冲突
 *
 * 参考：lua_c_analysis/src/lstring.c 中的哈希算法
 */
usize GCString::computeHash(StrView str) noexcept {
    usize len = str.length();
    usize hash = len;  // 初始哈希值为字符串长度
    
    // 采样步长：对于长字符串，不扫描所有字符
    // Lua 5.1使用 (len >> 5) + 1 作为步长
    usize step = (len >> 5) + 1;
    
    // 采样计算哈希值
    for (usize i = 0; i < len; i += step) {
        // 使用简单但有效的哈希组合公式
        hash = hash ^ ((hash << 5) + (hash >> 2) + static_cast<unsigned char>(str[i]));
    }
    
    return hash;
}

} // namespace Lua

