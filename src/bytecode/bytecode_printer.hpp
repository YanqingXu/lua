#pragma once

/**
 * @file bytecode_printer.hpp
 * @brief Lua 函数原型字节码的打印与差异比较接口
 */

#include <iosfwd>
#include <string_view>

namespace Lua {

class Proto;

/**
 * @brief 打印函数原型字节码
 * @param f 要打印的函数原型
 * @param out 输出流
 * @param full 是否递归打印子函数原型
 * @note 输出格式尽量与 Lua 5.1 的 luac -l 对齐。
 */
void printProtoBytecode(const Proto* f, std::ostream& out, bool full);

/**
 * @brief 比较并打印两个函数原型的字节码差异
 * @param left 左侧函数原型
 * @param right 右侧函数原型
 * @param out 输出流
 * @param full 是否递归比较子函数原型
 * @param leftLabel 左侧标签
 * @param rightLabel 右侧标签
 */
void printProtoBytecodeDiff(const Proto* left,
                            const Proto* right,
                            std::ostream& out,
                            bool full = false,
                            std::string_view leftLabel = "left",
                            std::string_view rightLabel = "right");

/**
 * @brief 打印函数原型的 Mermaid 控制流图
 * @param f 要打印的函数原型
 * @param out 输出流
 * @param full 是否递归打印子函数原型的控制流图
 */
void printProtoBytecodeCfg(const Proto* f, std::ostream& out, bool full = false);

} // namespace Lua
