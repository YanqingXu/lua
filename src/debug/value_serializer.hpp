/**
 * @file value_serializer.hpp
 * @brief Value → JSON 字符串序列化工具
 *
 * 提供将 Lua Value 和寄存器快照序列化为 JSON 片段的功能。
 * 遵循浅序列化原则：Table/Function 等 GC 对象只输出类型和指针 ID。
 */

#pragma once

#include "common/types.hpp"
#include "core/value.hpp"

namespace Lua {

class Proto;

namespace Trace {

/**
 * @brief 将单个 Value 序列化为 JSON 值片段
 *
 * 序列化规则：
 * - Nil           → "null"
 * - Boolean       → "true" / "false"
 * - Number        → 数字字面量
 * - String        → JSON 转义字符串（带双引号）
 * - Table         → "\"table:0xABCD\""
 * - Function      → "\"function:0xABCD\""
 * - Userdata      → "\"userdata:0xABCD\""
 * - Thread        → "\"thread:0xABCD\""
 * - LightUserdata → "\"lightuserdata:0xABCD\""
 *
 * @param v 要序列化的值
 * @return JSON 值片段字符串
 */
Str serializeValue(const Value& v);

/**
 * @brief 将寄存器快照序列化为 JSON 数组
 *
 * 输出格式：
 * [{"slot":0,"name":"x","value":42,"type":"number"}, ...]
 *
 * @param base      寄存器基地址
 * @param maxStack  栈帧大小
 * @param proto     函数原型（用于获取局部变量名），可为 nullptr
 * @param pc        当前 PC（用于查询活跃变量）
 * @return JSON 数组字符串
 */
Str serializeRegisters(const Value* base, i32 maxStack, Proto* proto, i32 pc);

/**
 * @brief 获取 ValueType 的可读名称
 * @param type 值类型
 * @return 类型名字符串（如 "nil", "number", "string" 等）
 */
const char* getValueTypeName(ValueType type);

/**
 * @brief 对字符串内容进行 JSON 转义
 * @param s 原始字符串
 * @return 转义后的字符串（不含外层双引号）
 */
Str jsonEscape(StrView s);

} // namespace Trace
} // namespace Lua
