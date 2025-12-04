#pragma once

#include <iosfwd>

namespace Lua {

class Proto;

// 打印 Lua C++ 实现的 Proto 字节码，格式尽量对齐 Lua 5.1 的 luac -l 输出。
// full 参数当前主要为占位，后续可扩展打印常量/局部变量/upvalue 等详细信息。
void printProtoBytecode(const Proto* f, std::ostream& out, bool full);

} // namespace Lua

