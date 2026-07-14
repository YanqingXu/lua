/**
 * @file baselib.cpp
 * @brief Lua基础库实现
 *
 * 使用现代C++流式API进行函数注册（方案二）
 *
 * @author Lua C++ Project
 * @date 2025-11-13
 * @updated 2025-12-18 - 采用流式API改进注册方式
 */

#include "lib/baselib.hpp"
#include "lib/lib_registry.hpp"
#include "lib/lib_manager.hpp"
#include "common/lua_error.hpp"
#include "common/number_conversion.hpp"
#include "core/gc_string.hpp"
#include "core/table.hpp"
#include "core/function.hpp"
#include "core/upvalue.hpp"
#include "core/userdata.hpp"
#include "core/thread.hpp"
#include "gc/garbage_collector.hpp"
#include "vm/state/global_state.hpp"
#include "vm/state/call_info.hpp"
#include "vm/vm.hpp"
#include "runtime/runtime_services.hpp"
#include "compiler/parser/parser.hpp"
#include "compiler/codegen/codegen.hpp"
#include <format>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cmath>
#include <limits>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace Lua {

static Str makeLuaChunkId(StrView chunkName);

static bool shouldReadStdinChunk() {
    std::streamsize available = std::cin.rdbuf()->in_avail();
    if (available > 0) {
        return true;
    }

#ifdef _WIN32
    return _isatty(_fileno(stdin)) == 0;
#else
    return isatty(fileno(stdin)) == 0;
#endif
}

// =====================================================================
// print(...) - 打印任意数量的参数到标准输出
// =====================================================================

i32 luaB_print(LuaState* L) {
    i32 n = L->getTop(); // 参数数量

    for (i32 i = 1; i <= n; i++) {
        const char* s = nullptr;
        Str formatted;

        // 尝试将值转换为字符串
        if (L->isString(i)) {
            s = L->toString(i);
        } else if (L->isNumber(i)) {
            s = L->toString(i);
        } else if (L->isBoolean(i)) {
            s = L->toBoolean(i) ? "true" : "false";
        } else if (L->isNil(i)) {
            s = "nil";
        } else if (L->isTable(i)) {
            // 表类型显示地址
            formatted = std::format("table: {}", static_cast<void*>(L->at(i).asTable()));
            s = formatted.c_str();
        } else if (L->isFunction(i)) {
            // 函数类型显示地址
            formatted = std::format("function: {}", static_cast<void*>(L->at(i).asFunction()));
            s = formatted.c_str();
        } else {
            s = "unknown";
        }

        if (i > 1) {
            std::fputs("\t", stdout);
        }
        std::fputs(s, stdout);
    }
    std::fputs("\n", stdout);
    std::fflush(stdout);

    return 0; // 不返回值
}

// =====================================================================
// type(v) - 返回值的类型字符串
// =====================================================================

i32 luaB_type(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("type: missing argument");
    }

    i32 t = L->type(1);
    const char* typeName = L->typeName(t);

    // 创建字符串并压入栈
    GCString* str = L->getGlobalState().getStringPool().intern(typeName);
    L->pushString(str);

    return 1;
}

// =====================================================================
// tostring(v) - 将值转换为字符串
// =====================================================================

i32 luaB_tostring(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("tostring: missing argument");
    }

    const char* s = nullptr;
    Str formatted;
    Value original = L->at(1);

    if (L->getMetatable(1)) {
        Value mt = L->pop();
        if (mt.isTable()) {
            GCString* tostringKey = L->getGlobalState().getStringPool().intern("__tostring");
            Value tostringMethod = mt.asTable()->get(Value(tostringKey));
            if (tostringMethod.isFunction()) {
                usize savedTop = L->getAbsoluteTop();
                L->setTop(0);
                L->pushValue(tostringMethod);
                L->pushValue(original);

                if (L->pcall(1, 1, 0) == LUA_OK && L->getTop() >= 1) {
                    return 1;
                }

                L->setAbsoluteTop(savedTop);
                L->getStack().setTop(savedTop);
            }
        }
    }

    if (original.isString()) {
        L->pushValue(original);
        return 1;
    }

    // 默认转换逻辑
    if (L->isString(1)) {
        s = L->toString(1);
    } else if (L->isNumber(1)) {
        s = L->toString(1);
    } else if (L->isBoolean(1)) {
        s = L->toBoolean(1) ? "true" : "false";
    } else if (L->isNil(1)) {
        s = "nil";
    } else if (L->isTable(1)) {
        formatted = std::format("table: {}", static_cast<void*>(L->at(1).asTable()));
        s = formatted.c_str();
    } else if (L->isFunction(1)) {
        formatted = std::format("function: {}", static_cast<void*>(L->at(1).asFunction()));
        s = formatted.c_str();
    } else {
        formatted = std::format("{}: {}", L->typeName(L->type(1)), static_cast<void*>(&L->at(1)));
        s = formatted.c_str();
    }

    GCString* str = L->getGlobalState().getStringPool().intern(s);
    L->pushString(str);

    return 1;
}

// =====================================================================
// tonumber(e [, base]) - 将值转换为数字
// =====================================================================

i32 luaB_tonumber(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("tonumber: missing argument");
    }

    bool hasBase = L->getTop() >= 2 && !L->isNil(2);
    i32 base = 10;
    if (hasBase) {
        if (!L->isNumber(2)) {
            L->error("tonumber: base must be a number");
        }
        base = static_cast<i32>(L->toNumber(2));
        if (base < 2 || base > 36) {
            L->error("tonumber: base out of range");
        }
    }

    // 如果已经是数字，直接返回
    if (!hasBase && L->isNumber(1)) {
        L->pushNumber(L->toNumber(1));
        return 1;
    }

    // 尝试从字符串转换
    if (L->isString(1) || (hasBase && L->isNumber(1))) {
        const char* s = L->toString(1);
        if (!s || *s == '\0') {
            L->pushNil();
            return 1;
        }

        // 跳过前导空白
        while (std::isspace(*s))
            s++;

        // 转换数字
        f64 result = 0.0;
        bool hasDigit = false;

        if (base == 10) {
            // 十进制：支持小数点和科学计数法
            char* endptr = nullptr;
            result = std::strtod(s, &endptr);
            if (endptr == s) {
                L->pushNil();
                return 1;
            }
            // 检查是否有非数字字符（跳过尾部空白）
            while (std::isspace(*endptr))
                endptr++;
            if (*endptr != '\0') {
                L->pushNil();
                return 1;
            }
            hasDigit = true;
        } else {
            // 处理符号；Lua 5.1 不允许符号和数字之间出现空白。
            bool negative = false;
            if (*s == '-') {
                negative = true;
                s++;
            } else if (*s == '+') {
                s++;
            }

            // 其他进制：只支持整数
            while (*s != '\0' && !std::isspace(*s)) {
                i32 digit = -1;
                if (*s >= '0' && *s <= '9') {
                    digit = *s - '0';
                } else if (*s >= 'a' && *s <= 'z') {
                    digit = *s - 'a' + 10;
                } else if (*s >= 'A' && *s <= 'Z') {
                    digit = *s - 'A' + 10;
                } else {
                    break;
                }

                if (digit >= base) {
                    break;
                }

                result = result * base + digit;
                hasDigit = true;
                s++;
            }

            // 检查是否有非数字字符
            while (std::isspace(*s))
                s++;
            if (*s != '\0') {
                hasDigit = false;
            }
            if (hasDigit && negative) {
                result = -result;
            }
        }

        if (hasDigit) {
            L->pushNumber(result);
            return 1;
        }
    }

    L->pushNil();
    return 1;
}

// =====================================================================
// error(message [, level]) - 抛出错误
// =====================================================================

static Str sourceLocationForErrorLevel(LuaState* L, i32 level) {
    if (L == nullptr || level <= 0) {
        return Str();
    }

    usize currentIndex = L->getCurrentCI();
    if (static_cast<usize>(level) > currentIndex) {
        return Str();
    }

    usize targetIndex = currentIndex - static_cast<usize>(level);
    LuaVector<CallInfo>& frames = L->getCallStack();
    if (targetIndex >= frames.size()) {
        return Str();
    }

    const CallInfo& ci = frames[targetIndex];
    Stack& stack = L->getStack();
    if (ci.func >= stack.size()) {
        return Str();
    }

    const Value& funcValue = stack[ci.func];
    if (!funcValue.isFunction()) {
        return Str();
    }

    Function* func = funcValue.asFunction();
    if (func == nullptr || func->isCFunction()) {
        return Str();
    }

    Proto* proto = func->getProto();
    if (proto == nullptr) {
        return Str();
    }

    usize pc = 0;
    const auto code = proto->getInstructionSpan();
    if (!code.empty() && ci.savedpc != nullptr) {
        pc = static_cast<usize>(ci.savedpc - code.data());
        if (pc > 0) {
            --pc;
        }
        if (pc >= code.size()) {
            pc = code.size() - 1;
        }
    }

    i32 line = proto->getLine(pc);
    if (line <= 0) {
        return Str();
    }

    StrView source = proto->getSource() ? proto->getSource()->view() : StrView("=?");
    return makeLuaChunkId(source) + ":" + std::to_string(line) + ": ";
}

i32 luaB_error(LuaState* L) {
    if (L->getTop() < 1) {
        L->pushNil();
    }

    i32 level = 1;
    if (L->getTop() >= 2 && L->isNumber(2)) {
        level = static_cast<i32>(L->toNumber(2));
    }

    if (level > 0) {
        const char* message = L->toString(1);
        if (message != nullptr) {
            Str location = sourceLocationForErrorLevel(L, level);
            if (!location.empty()) {
                Str fullMessage = location + message;
                GCString* str = L->getGlobalState().getStringPool().intern(fullMessage.c_str());
                L->setTop(0);
                L->pushString(str);
                return L->error();
            }
        }
    }

    L->setTop(1); // 只保留错误消息
    return L->error();
}

// =====================================================================
// assert(v [, message]) - 断言
// =====================================================================

i32 luaB_assert(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("assert: missing argument");
    }

    if (!L->toBoolean(1)) {
        // 断言失败
        if (L->getTop() >= 2) {
            // 使用提供的错误消息
            L->setTop(2);
            return L->error();
        } else {
            // 使用默认错误消息
            L->error("assertion failed!");
        }
    }

    // 断言成功，返回所有参数
    return L->getTop();
}

// =====================================================================
// setmetatable(table, metatable) - 设置表的元表
// =====================================================================

i32 luaB_setmetatable(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("setmetatable: missing arguments");
    }

    if (!L->isTable(1)) {
        L->error("setmetatable: first argument must be a table");
    }

    i32 t = L->type(2);
    if (t != 0 && t != 5) { // 不是nil也不是table
        L->error("setmetatable: second argument must be nil or table");
    }

    // 检查 __metatable 字段（保护机制）
    if (L->getMetatable(1)) {
        // 如果表已经有元表，检查是否受保护
        Value oldMt = L->pop();
        if (oldMt.isTable()) {
            GCString* metatableKey = L->getGlobalState().getStringPool().intern("__metatable");
            Value protectedField = oldMt.asTable()->get(Value(metatableKey));
            if (!protectedField.isNil()) {
                // 元表受保护，不能修改
                L->error("cannot change a protected metatable");
            }
        }
    }

    if (!L->setMetatable(1)) {
        L->error("setmetatable: cannot set metatable");
    }

    L->setTop(1); // 只保留表
    return 1;
}

// =====================================================================
// getmetatable(object) - 获取对象的元表
// =====================================================================

i32 luaB_getmetatable(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("getmetatable: missing argument");
    }

    if (!L->getMetatable(1)) {
        L->pushNil();
        return 1;
    }

    // 检查 __metatable 字段
    Value mt = L->top();
    if (mt.isTable()) {
        GCString* metatableKey = L->getGlobalState().getStringPool().intern("__metatable");
        Value protectedField = mt.asTable()->get(Value(metatableKey));
        if (!protectedField.isNil()) {
            // 如果设置了 __metatable 字段，返回该字段的值而不是元表本身
            L->pop();
            L->pushValue(protectedField);
        }
    }

    return 1;
}

// =====================================================================
// newproxy([boolean|proxy]) - Lua 5.1 compatibility userdata factory
// =====================================================================

static Table* currentFunctionEnvironment(LuaState* L) {
    if (L->getCurrentCI() == 0) {
        return L->getGlobalTable();
    }

    const usize functionIndex = L->getCurrentCallInfo().func;
    if (functionIndex >= L->getStack().size() || !L->getStack().at(functionIndex).isFunction()) {
        return L->getGlobalTable();
    }
    Function* closure = L->getStack().at(functionIndex).asFunction();
    return closure != nullptr && closure->getEnv() != nullptr ? closure->getEnv() : L->getGlobalTable();
}

static i32 luaB_newproxy(LuaState* L) {
    auto& gc = L->getGlobalState().getGC();
    Table* metatable = nullptr;

    if (L->getTop() >= 1) {
        const Value& arg = L->at(1);
        if (arg.isBoolean()) {
            if (arg.asBoolean()) {
                metatable = gc.create<Table>();
            }
        } else if (arg.isUserdata()) {
            metatable = arg.asUserdata()->getMetatable();
        } else if (!arg.isNil()) {
            L->error("bad argument #1 to 'newproxy' (boolean or proxy expected)");
        }
    }

    Userdata* userdata = gc.create<Userdata>(1);
    userdata->setEnvironment(currentFunctionEnvironment(L));
    userdata->setMetatable(metatable);
    L->pushUserdata(userdata);
    return 1;
}

// =====================================================================
// 迭代器函数
// =====================================================================

/**
 * @brief next(table [, index])
 *
 * 返回表中的下一个键值对。如果index为nil，返回第一个键值对。
 * 如果index是表中的最后一个键，返回nil。
 */
static i32 luaB_next(LuaState* L) {
    if (L->getTop() < 1) {
        throw std::runtime_error("next: table expected");
    }

    Value tableVal = L->at(1);
    if (!tableVal.isTable()) {
        throw std::runtime_error("next: table expected");
    }

    Table* table = tableVal.asTable();
    Value key = L->getTop() > 1 ? L->at(2) : Value(); // nil if not provided

    // 获取下一个键值对
    Value nextKey, nextValue;
    if (table->next(key, nextKey, nextValue)) {
        // 找到下一个键值对
        L->pushValue(nextKey);
        L->pushValue(nextValue);
        return 2;
    } else {
        // 没有更多元素
        return 0;
    }
}

/**
 * @brief pairs(t)
 *
 * 返回三个值：迭代器函数、表、nil（初始键）
 * 用于泛型for循环遍历表的所有键值对。
 */
static i32 luaB_pairs(LuaState* L) {
    if (L->getTop() < 1) {
        throw std::runtime_error("pairs: table expected");
    }

    Value tableVal = L->at(1);
    if (!tableVal.isTable()) {
        throw std::runtime_error("pairs: table expected");
    }

    // 返回：next函数, 表, nil
    GlobalState& gs = L->getGlobalState();

    // 创建next函数对象
    Function* nextFunc = gs.getGC().create<Function>(luaB_next);

    L->pushFunction(nextFunc);
    L->pushValue(tableVal);
    L->pushNil();

    return 3;
}

/**
 * @brief ipairs迭代器辅助函数
 *
 * 接收(table, index)，返回(index+1, value)
 */
static i32 ipairsIter(LuaState* L) {
    if (L->getTop() < 2) {
        return 0;
    }

    Value tableVal = L->at(1);
    Value indexVal = L->at(2);

    if (!tableVal.isTable() || !indexVal.isNumber()) {
        return 0;
    }

    Table* table = tableVal.asTable();
    i32 index = static_cast<i32>(indexVal.asNumber());
    i32 nextIndex = index + 1;

    // 获取下一个元素
    Value nextValue = table->getArray(nextIndex);
    if (nextValue.isNil()) {
        return 0; // 结束迭代
    }

    L->pushNumber(static_cast<f64>(nextIndex));
    L->pushValue(nextValue);
    return 2;
}

/**
 * @brief ipairs(t)
 *
 * 返回三个值：迭代器函数、表、0（初始索引）
 * 用于泛型for循环遍历表的数组部分。
 */
static i32 luaB_ipairs(LuaState* L) {
    if (L->getTop() < 1) {
        throw std::runtime_error("ipairs: table expected");
    }

    Value tableVal = L->at(1);
    if (!tableVal.isTable()) {
        throw std::runtime_error("ipairs: table expected");
    }

    // 创建ipairs迭代器函数
    GlobalState& gs = L->getGlobalState();

    Function* iterFunc = gs.getGC().create<Function>(ipairsIter);

    L->pushFunction(iterFunc);
    L->pushValue(tableVal);
    L->pushNumber(0.0); // 初始索引0

    return 3;
}

// =====================================================================
// rawget(table, index) - 绕过元方法直接获取表元素
// =====================================================================

/**
 * @brief rawget(table, index)
 *
 * 在不触发任何元方法的情况下获取 table[index] 的值。
 * table 必须是一个表；index 可以是任何值。
 */
i32 luaB_rawget(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("rawget: missing arguments");
    }

    if (!L->isTable(1)) {
        L->error("rawget: table expected");
    }

    Table* table = L->at(1).asTable();
    Value index = L->at(2);
    Value result = table->get(index);

    L->pushValue(result);
    return 1;
}

// =====================================================================
// rawset(table, index, value) - 绕过元方法直接设置表元素
// =====================================================================

/**
 * @brief rawset(table, index, value)
 *
 * 在不触发任何元方法的情况下设置 table[index] = value。
 * table 必须是一个表，index 不能是 nil 或 NaN，value 可以是任何值。
 * 返回 table。
 */
i32 luaB_rawset(LuaState* L) {
    if (L->getTop() < 3) {
        L->error("rawset: missing arguments");
    }

    if (!L->isTable(1)) {
        L->error("rawset: table expected");
    }

    Value index = L->at(2);
    if (index.isNil()) {
        L->error("rawset: table index is nil");
    }

    // 检查 NaN（如果是数字）
    if (index.isNumber()) {
        f64 num = index.asNumber();
        if (num != num) { // NaN check
            L->error("rawset: table index is NaN");
        }
    }

    Table* table = L->at(1).asTable();
    Value value = L->at(3);
    table->set(index, value);

    L->pushValue(L->at(1)); // 返回表本身
    return 1;
}

// =====================================================================
// rawequal(v1, v2) - 绕过元方法直接比较两个值
// =====================================================================

/**
 * @brief rawequal(v1, v2)
 *
 * 在不触发任何元方法的情况下检查 v1 是否等于 v2。
 * 返回布尔值。
 */
i32 luaB_rawequal(LuaState* L) {
    if (L->getTop() < 2) {
        L->error("rawequal: missing arguments");
    }

    Value v1 = L->at(1);
    Value v2 = L->at(2);
    bool result = (v1 == v2);

    L->pushBoolean(result);
    return 1;
}

// =====================================================================
// select(index, ...) - 从可变参数中选择特定范围的参数
// =====================================================================

/**
 * @brief select(index, ...)
 *
 * 如果 index 是数字，返回参数 index 之后的所有参数。
 * 如果 index 是字符串 "#"，返回额外参数的总数。
 * 负数索引从末尾开始计数（-1 是最后一个参数）。
 */
i32 luaB_select(LuaState* L) {
    i32 n = L->getTop();

    if (n < 1) {
        L->error("select: missing index argument");
    }

    // 检查是否是 "#"
    if (L->isString(1)) {
        const char* s = L->toString(1);
        if (s && std::strcmp(s, "#") == 0) {
            L->pushNumber(static_cast<f64>(n - 1));
            return 1;
        }
    }

    // 必须是数字索引
    if (!L->isNumber(1)) {
        L->error("select: index must be a number or '#'");
    }

    i32 index = static_cast<i32>(L->toNumber(1));

    // 处理负数索引
    if (index < 0) {
        index = n + index;
    } else if (index > n) {
        index = n;
    }

    if (index < 1) {
        L->error("select: index out of range");
    }

    // 返回从 index+1 到 n 的所有参数
    i32 count = n - index;
    for (i32 i = index + 1; i <= n; i++) {
        L->pushValue(L->at(i));
    }

    return count;
}

// =====================================================================
// pcall(f, arg1, ...) - 保护模式调用函数
// =====================================================================

i32 luaB_pcall(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("pcall: function expected");
    }

    // 调用 pcall(nargs-1, MULTRET, 0)
    i32 status = L->pcall(nargs - 1, MULTRET, 0);
    // 压入状态（true 表示成功，false 表示失败）
    L->pushBoolean(status == LUA_OK);

    // 将 boolean 插入到栈底（索引 1）
    L->insert(1);

    // 返回所有值（boolean + 所有结果或错误消息）
    return L->getTop();
}

// =====================================================================
// xpcall(f, msgh, arg1, ...) - 带错误处理器的保护调用
// =====================================================================

i32 luaB_xpcall(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 2) {
        L->error("xpcall: function and error handler expected");
    }

    // 手动调整栈：只保留前 2 个参数（func 和 msgh）
    while (L->getTop() > 2) {
        L->pop();
    }

    // 将错误处理器插入到函数下面：[func] [msgh] -> [msgh] [func]
    L->insert(1);

    // 调用 pcall(0, MULTRET, 1)
    // 0 个参数（xpcall 在 Lua 5.1 中不接受额外参数）
    // 1 表示错误处理器在索引 1
    i32 status = L->pcall(0, MULTRET, 1);

    // 压入状态（true 表示成功，false 表示失败）
    L->pushBoolean(status == LUA_OK);

    // 用 boolean 替换索引 1 的值（错误处理器）
    L->replace(1);

    // 返回所有值（boolean + 所有结果或错误消息）
    return L->getTop();
}

// =====================================================================
// Project-local binary chunk loader for string.dump/load round-trips
// =====================================================================

class DumpReader {
public:
    DumpReader(StrView data, StringPool& pool, GarbageCollector& gc) : data_(data), pool_(pool), gc_(gc) {}

    Proto* readChunk() {
        readHeader();
        Proto* proto = readProto();
        if (pos_ != data_.size()) {
            throw std::runtime_error("binary chunk has trailing data");
        }
        return proto;
    }

private:
    StrView data_;
    StringPool& pool_;
    GarbageCollector& gc_;
    usize pos_ = 0;
    bool projectLocal_ = false;
    u8 sizeTSize_ = static_cast<u8>(sizeof(usize));

    void require(usize count) const {
        if (count > data_.size() || pos_ > data_.size() - count) {
            throw std::runtime_error("truncated binary chunk");
        }
    }

    u8 readByte() {
        require(1);
        return static_cast<u8>(data_[pos_++]);
    }

    u32 readU32() {
        require(4);
        u32 value = 0;
        for (i32 i = 0; i < 4; ++i) {
            value |= static_cast<u32>(static_cast<u8>(data_[pos_++])) << (i * 8);
        }
        return value;
    }

    i32 readI32() {
        return static_cast<i32>(readU32());
    }

    u64 readU64() {
        require(8);
        u64 value = 0;
        for (i32 i = 0; i < 8; ++i) {
            value |= static_cast<u64>(static_cast<u8>(data_[pos_++])) << (i * 8);
        }
        return value;
    }

    LuaNumber readNumber() {
        u64 bits = readU64();
        LuaNumber value = 0;
        static_assert(sizeof(bits) == sizeof(value), "LuaNumber undump expects 64-bit double");
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    usize readSize() {
        return static_cast<usize>(readU32());
    }

    usize readSizeT() {
        if (projectLocal_) {
            return static_cast<usize>(readU32());
        }
        if (sizeTSize_ == 4) {
            return static_cast<usize>(readU32());
        }
        if (sizeTSize_ == 8) {
            return static_cast<usize>(readU64());
        }
        throw std::runtime_error("unsupported binary chunk size_t width");
    }

    void readHeader() {
        require(12);
        if (data_.substr(0, 4) != StrView("\x1bLua", 4)) {
            throw std::runtime_error("not a binary chunk");
        }
        pos_ = 4;
        u8 version = readByte();
        u8 format = readByte();
        u8 endian = readByte();
        u8 intSize = readByte();
        sizeTSize_ = readByte();
        u8 instructionSize = readByte();
        u8 numberSize = readByte();
        u8 integralFlag = readByte();
        if (version != 0x51 || format != 0 || endian != 1 || intSize != sizeof(i32) ||
            instructionSize != sizeof(Instruction) || numberSize != sizeof(LuaNumber) || integralFlag != 0) {
            throw std::runtime_error("unsupported binary chunk format");
        }
        if (pos_ + 4 <= data_.size() && data_.substr(pos_, 4) == StrView("LC++", 4)) {
            projectLocal_ = true;
            if (sizeTSize_ != sizeof(usize)) {
                throw std::runtime_error("unsupported project binary chunk size_t width");
            }
            pos_ += 4;
        }
    }

    GCString* readMaybeString() {
        if (projectLocal_) {
            u32 len = readU32();
            if (len == std::numeric_limits<u32>::max()) {
                return {};
            }
            require(len);
            GCString* str = pool_.intern(data_.data() + pos_, static_cast<usize>(len));
            pos_ += static_cast<usize>(len);
            return str;
        }

        usize len = readSizeT();
        if (len == 0) {
            return {};
        }
        require(len);
        usize textLen = len;
        if (textLen > 0 && data_[pos_ + textLen - 1] == '\0') {
            --textLen;
        }
        GCString* str = pool_.intern(data_.data() + pos_, textLen);
        pos_ += len;
        return str;
    }

    Value readConstant() {
        u8 tag = readByte();
        switch (tag) {
        case 0:
            return Value();
        case 1:
            return Value(readByte() != 0);
        case 3:
            return Value(readNumber());
        case 4:
            return Value(readMaybeString());
        default:
            throw std::runtime_error("unsupported constant in binary chunk");
        }
    }

    Proto* readProto() {
        Proto* proto = gc_.create<Proto>();
        proto->setSource(readMaybeString());
        proto->setLineDefined(readI32());
        proto->setLastLineDefined(readI32());
        if (projectLocal_) {
            proto->setNumParams(readByte());
            proto->setVarargFlags(readByte());
            proto->setMaxStackSize(readByte());
            proto->setNumUpvalues(readByte());
        } else {
            proto->setNumUpvalues(readByte());
            proto->setNumParams(readByte());
            proto->setVarargFlags(readByte());
            proto->setMaxStackSize(readByte());
        }

        usize instructionCount = readSize();
        for (usize i = 0; i < instructionCount; ++i) {
            proto->addInstruction(readU32());
        }

        usize constantCount = readSize();
        for (usize i = 0; i < constantCount; ++i) {
            proto->appendConstantSlot(readConstant());
        }

        usize subProtoCount = readSize();
        for (usize i = 0; i < subProtoCount; ++i) {
            proto->addProto(readProto());
        }

        usize lineInfoCount = readSize();
        for (usize i = 0; i < lineInfoCount; ++i) {
            proto->addLineInfo(readI32());
        }

        usize locVarCount = readSize();
        for (usize i = 0; i < locVarCount; ++i) {
            GCString* name = readMaybeString();
            i32 startpc = readI32();
            i32 endpc = readI32();
            i32 reg = projectLocal_ ? readI32() : -1;
            proto->addLocVar(name, startpc, endpc, reg);
        }

        usize upvalueNameCount = readSize();
        for (usize i = 0; i < upvalueNameCount; ++i) {
            proto->addUpvalueName(readMaybeString());
        }

        return proto;
    }
};

static bool isProjectBinaryChunk(StrView source) {
    return source.size() >= 4 && source.substr(0, 4) == StrView("\x1bLua", 4);
}

static StrView skipInitialHashCommentLine(StrView source) {
    if (source.empty() || source.front() != '#') {
        return source;
    }

    usize newline = source.find_first_of("\r\n");
    if (newline == StrView::npos) {
        return StrView();
    }

    usize next = newline + 1;
    while (next < source.size() && (source[next] == '\r' || source[next] == '\n')) {
        next++;
    }
    return source.substr(next);
}

static Function* createLuaFunctionFromProto(LuaState* L, Proto* proto) {
    Function* func = L->getGlobalState().getGC().create<Function>(proto);
    func->setEnv(L->getGlobalTable());

    for (u8 i = 0; i < proto->getNumUpvalues(); ++i) {
        Upvalue* upvalue = L->getGlobalState().getGC().create<Upvalue>(Value());
        func->addUpvalue(upvalue);
    }

    return func;
}

static GCObject* loadGCObjectFromValue(const Value& value) {
    if (value.isString()) {
        return value.asString();
    }
    if (value.isTable()) {
        return value.asTable();
    }
    if (value.isFunction()) {
        return value.asFunction();
    }
    if (value.isUserdata()) {
        return value.asUserdata();
    }
    if (value.isThread()) {
        return value.asThread();
    }
    return {};
}

class ScopedLoadResultRoots {
public:
    explicit ScopedLoadResultRoots(LuaState* L) : gc_(L->getGlobalState().getGC()) {
        for (i32 i = 1; i <= L->getTop(); ++i) {
            GCObject* obj = loadGCObjectFromValue(L->at(i));
            if (obj == nullptr) {
                continue;
            }
            bool wasRoot = gc_.isRoot(obj);
            gc_.addRoot(obj);
            if (!wasRoot) {
                roots_.push_back(obj);
            }
        }
    }

    ~ScopedLoadResultRoots() {
        for (GCObject* obj : roots_) {
            gc_.removeRoot(obj);
        }
    }

    ScopedLoadResultRoots(const ScopedLoadResultRoots&) = delete;
    ScopedLoadResultRoots& operator=(const ScopedLoadResultRoots&) = delete;

private:
    GarbageCollector& gc_;
    Vec<GCObject*> roots_;
};

static void settleLoadGC(LuaState* L) {
    ScopedLoadResultRoots roots(L);
    (void)L->getGlobalState().getGC().collectAutomatic(L);
}

static Function* loadBinaryChunk(LuaState* L, StrView source) {
    DumpReader reader(source, L->getGlobalState().getStringPool(), L->getGlobalState().getGC());
    Proto* proto = reader.readChunk();
    return createLuaFunctionFromProto(L, proto);
}

static Str makeStringChunkSnippet(StrView source) {
    constexpr usize kChunkSnippetLimit = 60;
    Str snippet;
    bool truncated = false;

    for (char ch : source) {
        if (ch == '\n' || ch == '\r') {
            truncated = true;
            break;
        }
        if (snippet.size() >= kChunkSnippetLimit) {
            truncated = true;
            break;
        }

        if (ch == '"' || ch == '\\') {
            snippet.push_back('\\');
        }
        snippet.push_back(ch);
    }

    if (truncated) {
        snippet += "...";
    }
    return snippet;
}

static Str makeLuaChunkId(StrView chunkName) {
    if (!chunkName.empty()) {
        if (chunkName.front() == '=') {
            return Str(chunkName.substr(1));
        }
        if (chunkName.front() == '@') {
            return Str(chunkName.substr(1));
        }
    }

    return Str("[string \"") + makeStringChunkSnippet(chunkName) + "\"]";
}

static Str formatLoadSyntaxError(StrView chunkName, const ParseError& error) {
    return makeLuaChunkId(chunkName) + ":" + std::to_string(error.getLine()) + ": " + error.what();
}

static bool stopOnDefinitiveReaderSyntaxError(LuaState* L, StringPool& pool, RuntimeServices& services,
                                              const Str& source) {
    if (source.size() < 2 || static_cast<u8>(source[0]) == 0x1b) {
        return false;
    }

    usize first = 0;
    while (first < source.size() && std::isspace(static_cast<unsigned char>(source[first])) != 0) {
        first++;
    }
    if (first >= source.size() || source[first] != '*') {
        return false;
    }

    Parser parser(source, services);
    auto parsed = parser.parse();
    if (parsed) {
        return false;
    }

    Str message = parsed.error().what();
    L->setTop(0);
    L->pushNil();
    L->pushString(pool.intern((Str("load: ") + message).c_str()));
    return true;
}

// =====================================================================
// loadstring(string [, chunkname]) - 编译字符串为函数
// =====================================================================

i32 luaB_loadstring(LuaState* L) {
    auto& pool = L->getGlobalState().getStringPool();
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->setTop(0);
        L->pushNil();
        L->pushString(pool.intern("loadstring: string expected"));
        return 2;
    }

    Value codeVal = L->at(1);
    if (!codeVal.isString()) {
        L->setTop(0);
        L->pushNil();
        L->pushString(pool.intern("loadstring: string expected"));
        return 2;
    }

    Str code = codeVal.asString()->getData();
    Str chunkname = (nargs >= 2 && L->at(2).isString()) ? L->at(2).asString()->getData() : code;

    try {
        if (isProjectBinaryChunk(StrView(code.data(), code.size()))) {
            Function* func = loadBinaryChunk(L, StrView(code.data(), code.size()));
            L->setTop(0);
            L->pushValue(Value(func));
            settleLoadGC(L);
            return 1;
        }

        RuntimeServices services(L->getGlobalState());

        // 解析代码
        Parser parser(code, services);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);

        // 生成字节码
        CodeGenerator codegen(services);
        Proto* proto = codegen.generate(chunk, chunkname);

        if (!proto) {
            L->setTop(0);
            L->pushNil();
            L->pushString(pool.intern("loadstring: compilation failed"));
            settleLoadGC(L);
            return 2;
        }

        // 创建函数对象
        Function* func = createLuaFunctionFromProto(L, proto);

        // 返回成功
        L->setTop(0);
        L->pushValue(Value(func));
        settleLoadGC(L);
        return 1;

    } catch (const MemoryError&) {
        throw;
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const ParseError& e) {
        L->setTop(0);
        L->pushNil();
        Str errorMsg = formatLoadSyntaxError(StrView(chunkname.data(), chunkname.size()), e);
        L->pushString(pool.intern(errorMsg.c_str()));
        settleLoadGC(L);
        return 2;
    } catch (const std::exception& e) {
        L->setTop(0);
        L->pushNil();
        Str errorMsg = Str("loadstring: ") + e.what();
        L->pushString(pool.intern(errorMsg.c_str()));
        settleLoadGC(L);
        return 2;
    }
}

// =====================================================================
// loadfile([filename]) - 编译文件为函数
// =====================================================================

i32 luaB_loadfile(LuaState* L) {
    auto& pool = L->getGlobalState().getStringPool();
    i32 nargs = L->getTop();
    Str displayName = (nargs < 1 || L->isNil(1)) ? Str("stdin") : Str();

    try {
        Str source;
        Str sourceName;

        if (nargs < 1 || L->isNil(1)) {
            if (!shouldReadStdinChunk()) {
                L->setTop(0);
                L->pushNil();
                L->pushString(pool.intern("loadfile: stdin is interactive"));
                return 2;
            }
            std::ostringstream input;
            input << std::cin.rdbuf();
            source = input.str();
            sourceName = "=stdin";
            displayName = "stdin";
        } else {
            Value filenameVal = L->at(1);
            if (!filenameVal.isString()) {
                L->setTop(0);
                L->pushNil();
                L->pushString(pool.intern("loadfile: filename must be a string"));
                return 2;
            }

            Str filename = filenameVal.asString()->c_str();
            displayName = filename;
            sourceName = Str("@") + filename;

            std::ifstream file(filename, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                L->setTop(0);
                L->pushNil();
                std::error_code pathError;
                const bool pathExists = std::filesystem::exists(std::filesystem::path(filename), pathError);
                Str errorMsg = pathExists && !pathError
                                   ? Str("loadfile: cannot read ") + filename
                                   : Str("loadfile: cannot open ") + filename + ": No such file or directory";
                L->pushString(pool.intern(errorMsg.c_str()));
                return 2;
            }

            std::streamsize size = file.tellg();
            if (size < 0) {
                L->setTop(0);
                L->pushNil();
                Str errorMsg = Str("loadfile: cannot read ") + filename;
                L->pushString(pool.intern(errorMsg.c_str()));
                return 2;
            }
            file.seekg(0, std::ios::beg);
            if (!file) {
                L->setTop(0);
                L->pushNil();
                Str errorMsg = Str("loadfile: cannot read ") + filename;
                L->pushString(pool.intern(errorMsg.c_str()));
                return 2;
            }

            source.resize(static_cast<usize>(size));
            if (size > 0 && !file.read(&source[0], size)) {
                L->setTop(0);
                L->pushNil();
                Str errorMsg = Str("loadfile: cannot read ") + filename;
                L->pushString(pool.intern(errorMsg.c_str()));
                return 2;
            }
        }

        StrView loadSource(source.data(), source.size());
        StrView binarySource = skipInitialHashCommentLine(loadSource);
        if (isProjectBinaryChunk(binarySource)) {
            Function* func = loadBinaryChunk(L, binarySource);
            L->setTop(0);
            L->pushValue(Value(func));
            return 1;
        }

        RuntimeServices services(L->getGlobalState());

        // 解析代码
        Parser parser(source, services);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);

        // 生成字节码
        CodeGenerator codegen(services);
        Proto* proto = codegen.generate(chunk, sourceName);

        if (!proto) {
            L->setTop(0);
            L->pushNil();
            Str errorMsg = Str("loadfile: compilation failed for ") + displayName;
            L->pushString(pool.intern(errorMsg.c_str()));
            return 2;
        }

        // 创建函数对象
        Function* func = createLuaFunctionFromProto(L, proto);

        // 返回成功
        L->setTop(0);
        L->pushValue(Value(func));
        return 1;

    } catch (const MemoryError&) {
        throw;
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const ParseError& e) {
        L->setTop(0);
        L->pushNil();
        Str errorMsg = Str("loadfile: ") + (displayName.empty() ? Str() : displayName + ": ") + e.what();
        L->pushString(pool.intern(errorMsg.c_str()));
        return 2;
    } catch (const std::exception& e) {
        L->setTop(0);
        L->pushNil();
        Str errorMsg = Str("loadfile: ") + (displayName.empty() ? Str() : displayName + ": ") + e.what();
        L->pushString(pool.intern(errorMsg.c_str()));
        return 2;
    }
}

// =====================================================================
// dofile([filename]) - 加载并执行文件
// =====================================================================

i32 luaB_dofile(LuaState* L) {
    auto& pool = L->getGlobalState().getStringPool();
    // 使用 loadfile 加载文件
    i32 loadResult = luaB_loadfile(L);

    if (loadResult != 1) {
        // Lua 5.1 dofile raises the loader error; it does not return the
        // loadfile-style (nil, message) pair.
        Value errorValue = L->getTop() > 0 ? L->top() : Value(pool.intern("dofile: load failed"));
        L->setTop(0);
        L->pushValue(errorValue);
        return L->error();
    }

    // 获取加载的函数
    Value func = L->at(1);
    if (!func.isFunction()) {
        L->error("dofile: loadfile did not return a function");
    }

    if (func.asFunction() == nullptr) {
        L->error("dofile: invalid function");
    }

    // Execute the function already returned at the top of this C frame.
    // VM::call replaces it with the chunk results and preserves the frame's
    // logical stack. Using Stack::size()/push() here mixed reserved physical
    // register capacity with Lua-visible values and could return the loaded
    // function itself instead of the chunk's result.
    RuntimeServices services(L->getGlobalState());
    VM::call(services, L, 0, MULTRET);
    return L->getTop();
}

// =====================================================================
// gcinfo() - 获取GC内存使用量（已废弃，兼容性函数）
// =====================================================================

/**
 * @brief gcinfo()
 *
 * 返回GC使用的内存量（KB）。这是一个已废弃的兼容性函数，
 * 在Lua 5.1中保留用于向后兼容。
 * @note 已废弃，建议使用collectgarbage("count")代替
 */
i32 luaB_gcinfo(LuaState* L) {
    // 获取GC内存使用量（字节）
    auto& gc = L->getGlobalState().getGC();
    usize totalBytes = gc.getTotalMemory();

    // 转换为KB（返回整数部分）
    i32 memoryKB = static_cast<i32>(totalBytes / 1024);

    // 压入数值
    L->pushNumber(static_cast<LuaNumber>(memoryKB));
    return 1;
}

// =====================================================================
// getfenv(f) - 获取函数环境表
// =====================================================================

static Function* functionAtStackLevel(LuaState* L, i32 level) {
    if (level < 0) {
        return nullptr;
    }

    LuaVector<CallInfo>& frames = L->getCallStack();
    usize targetIndex = L->getCurrentCI();
    if (targetIndex >= frames.size()) {
        return nullptr;
    }

    while (level > 0 && targetIndex > 0) {
        const CallInfo& ci = frames[targetIndex];
        --level;
        level -= ci.tailcalls;
        --targetIndex;
    }

    if (level != 0) {
        return nullptr;
    }

    const CallInfo& ci = frames[targetIndex];
    if (ci.func >= L->getStack().size()) {
        return nullptr;
    }

    Value& funcVal = L->getStack().at(ci.func);
    if (!funcVal.isFunction()) {
        return nullptr;
    }

    return funcVal.asFunction();
}

/**
 * @brief getfenv(f)
 *
 * 获取指定函数的环境表。C函数和Lua函数都可拥有独立环境；
 * 未设置时回退到全局环境表。
 * @param L Lua状态机指针
 * @return 返回值数量（1个：环境表）
 *
 * 参数说明：
 * - 参数1：函数对象或调用栈级别（可选，默认为1）
 *
 * @note C函数和Lua函数都可以有独立的环境
 */
i32 luaB_getfenv(LuaState* L) {
    // 获取参数（函数对象或栈级别）
    i32 nargs = L->getTop();

    Function* func = nullptr;

    if (nargs >= 1 && L->isUserdata(1)) {
        Table* environment = L->at(1).asUserdata()->getEnvironment();
        L->pushTable(environment != nullptr ? environment : L->getGlobalTable());
        return 1;
    }
    if (nargs >= 1 && L->isFunction(1)) {
        // 参数是函数对象
        Value v = L->at(1);
        func = v.asFunction();
    } else if (nargs >= 1 && L->isNumber(1)) {
        i32 level = static_cast<i32>(L->toNumber(1));
        if (level == 0) {
            L->pushTable(L->getGlobalTable());
            return 1;
        }
        func = functionAtStackLevel(L, level);
        if (func == nullptr) {
            L->error("getfenv: invalid level");
        }
    } else {
        func = functionAtStackLevel(L, 1);
        if (func == nullptr) {
            L->pushTable(L->getGlobalTable());
            return 1;
        }
    }

    if (!func) {
        L->pushTable(L->getGlobalTable());
        return 1;
    }

    Table* env = func->getEnv();
    if (env) {
        L->pushTable(env);
    } else {
        L->pushTable(L->getGlobalTable());
    }

    return 1;
}

// =====================================================================
// setfenv(f, table) - 设置函数环境表
// =====================================================================

/**
 * @brief setfenv(f, table)
 *
 * 为指定函数设置新的环境表。Lua函数和C函数都支持环境表。
 * @param L Lua状态机指针
 * @return 返回值数量（1个：被修改的函数对象）
 *
 * 参数说明：
 * - 参数1：函数对象或调用栈级别
 * - 参数2：新的环境表（必须是table类型）
 *
 * @note 简化实现：暂不支持线程环境设置
 */
i32 luaB_setfenv(LuaState* L) {
    i32 nargs = L->getTop();

    // 检查参数数量
    if (nargs < 2) {
        L->error("setfenv: expected 2 arguments");
    }

    // 检查第二个参数必须是table
    if (!L->isTable(2)) {
        L->error("setfenv: 'table' expected");
    }

    Function* func = nullptr;
    if (L->isUserdata(1)) {
        L->at(1).asUserdata()->setEnvironment(L->at(2).asTable());
        L->pushValue(1);
        return 1;
    }
    if (L->isFunction(1)) {
        Value funcVal = L->at(1);
        func = funcVal.asFunction();
    } else if (L->isNumber(1)) {
        i32 level = static_cast<i32>(L->toNumber(1));
        if (level == 0) {
            Table* newEnv = L->at(2).asTable();
            if (!newEnv) {
                L->error("setfenv: invalid table");
            }
            L->setGlobalTable(newEnv);
            L->pushNumber(0.0);
            return 1;
        }
        func = functionAtStackLevel(L, level);
    } else {
        L->error("setfenv: 'function' or stack level expected");
    }

    if (!func) {
        L->error("setfenv: invalid function");
    }

    // 获取新的环境表
    Value tableVal = L->at(2);
    Table* newEnv = tableVal.asTable();

    if (!newEnv) {
        L->error("setfenv: invalid table");
    }

    // 设置Lua函数的环境表
    func->setEnv(newEnv);

    // 返回函数对象
    L->pushFunction(func);
    return 1;
}

// =====================================================================
// collectgarbage(opt [, arg]) - 垃圾回收控制
// =====================================================================

static i32 collectGarbageControlArgument(LuaState* L) {
    if (L->getTop() < 2 || L->at(2).isNil()) {
        return 0;
    }

    const Value value = L->at(2);
    LuaNumber number = 0.0;
    if (value.isNumber()) {
        number = value.asNumber();
    } else if (!value.isString() || !luaStringToNumber(value.asString()->view(), number)) {
        L->error("bad argument #2 to 'collectgarbage' (number expected)");
    }

    if (!std::isfinite(number)) {
        L->error("bad argument #2 to 'collectgarbage' (finite number expected)");
    }

    const LuaNumber truncated = std::trunc(number);
    if (truncated < static_cast<LuaNumber>(std::numeric_limits<i32>::min()) ||
        truncated > static_cast<LuaNumber>(std::numeric_limits<i32>::max())) {
        L->error("bad argument #2 to 'collectgarbage' (number out of range)");
    }
    return static_cast<i32>(truncated);
}

/**
 * @brief collectgarbage(opt [, arg])
 *
 * 控制垃圾回收器的行为，支持多种操作模式。
 * @param L Lua状态机指针
 * @return 返回值数量（1个：操作结果）
 *
 * 参数说明：
 * - 参数1：操作类型字符串（可选，默认为"collect"）
 * - 参数2：可选的额外参数（默认为0）
 *
 * 支持的操作：
 * - "collect"：执行完整的垃圾回收（完整实现）
 * - "count"：返回内存使用量（KB，包含小数）（完整实现）
 * - "stop"：停止自动垃圾回收
 * - "restart"：重启自动垃圾回收
 * - "step"：推进一段有界 GC 工作，完成一轮收集时返回 true
 * - "strategy"：查询或切换教学用GC策略（mark-sweep / incremental placeholder）
 * - "setpause"：设置自动 GC 暂停参数，返回旧值
 * - "setstepmul"：设置 step 工作量倍数，返回旧值
 */
i32 luaB_collectgarbage(LuaState* L) {
    // 获取GC引用
    auto& gc = L->getGlobalState().getGC();

    // 获取操作类型（默认为"collect"）
    const char* opt = "collect";
    if (L->getTop() >= 1) {
        Value v = L->at(1);
        if (v.isString()) {
            opt = v.asString()->c_str();
        }
    }

    // 根据操作类型执行相应操作
    // 使用字符串的第一个字符来快速判断（优化）
    char firstChar = opt[0];

    if (firstChar == 'c') {
        if (strcmp(opt, "collect") == 0) {
            (void)gc.collect(L);
            L->pushNumber(0);
            return 1;
        } else if (strcmp(opt, "count") == 0) {
            // 返回内存使用量（KB，包含小数）
            usize totalBytes = gc.getTotalMemory();
            LuaNumber memoryKB = static_cast<LuaNumber>(totalBytes) / 1024.0;
            L->pushNumber(memoryKB);
            return 1;
        }
    } else if (firstChar == 's') {
        if (strcmp(opt, "stop") == 0) {
            gc.stopAutomatic();
            L->pushNumber(0);
            return 1;
        } else if (strcmp(opt, "step") == 0) {
            L->pushBoolean(gc.step(L, collectGarbageControlArgument(L)));
            return 1;
        } else if (strcmp(opt, "strategy") == 0) {
            if (L->getTop() >= 2) {
                Value strategy = L->at(2);
                if (!strategy.isString() || !gc.useStrategy(strategy.asString()->c_str())) {
                    L->error("collectgarbage: invalid strategy");
                }
            }

            GCString* name = L->getGlobalState().getStringPool().intern(gc.getStrategyName());
            L->pushString(name);
            return 1;
        } else if (strcmp(opt, "setpause") == 0) {
            L->pushNumber(static_cast<LuaNumber>(gc.setPause(collectGarbageControlArgument(L))));
            return 1;
        } else if (strcmp(opt, "setstepmul") == 0) {
            L->pushNumber(static_cast<LuaNumber>(gc.setStepMultiplier(collectGarbageControlArgument(L))));
            return 1;
        }
    } else if (firstChar == 'r') {
        if (strcmp(opt, "restart") == 0) {
            gc.restartAutomatic();
            L->pushNumber(0);
            return 1;
        }
    }

    // 无效的操作类型
    L->error("collectgarbage: invalid option");
}

// =====================================================================
// unpack(list [, i [, j]]) - 返回表中指定范围的元素
// =====================================================================

static i32 luaB_unpack(LuaState* L) {
    if (L->getTop() < 1 || !L->isTable(1)) {
        L->error("bad argument #1 to 'unpack' (table expected)");
    }

    Table* table = L->at(1).asTable();
    i32 i = (L->getTop() >= 2 && !L->at(2).isNil()) ? static_cast<i32>(L->toNumber(2)) : 1;
    i32 j =
        (L->getTop() >= 3 && !L->at(3).isNil()) ? static_cast<i32>(L->toNumber(3)) : static_cast<i32>(table->length());

    if (i > j)
        return 0; // empty range

    i32 n = j - i + 1;
    for (i32 k = i; k <= j; k++) {
        Value v = table->getArray(k);
        L->pushValue(v);
    }
    return n;
}

// =====================================================================
// load(func [, chunkname]) - 从函数加载器编译代码块
// =====================================================================

class ScopedAutomaticGCStop {
public:
    explicit ScopedAutomaticGCStop(GarbageCollector& gc) : gc_(gc), wasStopped_(gc.isAutomaticStopped()) {
        gc_.stopAutomatic();
    }

    ~ScopedAutomaticGCStop() {
        if (!wasStopped_) {
            gc_.restartAutomatic();
        }
    }

    ScopedAutomaticGCStop(const ScopedAutomaticGCStop&) = delete;
    ScopedAutomaticGCStop& operator=(const ScopedAutomaticGCStop&) = delete;

private:
    GarbageCollector& gc_;
    bool wasStopped_;
};

static i32 luaB_load(LuaState* L) {
    auto& pool = L->getGlobalState().getStringPool();
    ScopedAutomaticGCStop gcStop(L->getGlobalState().getGC());

    if (L->getTop() < 1 || !L->isFunction(1)) {
        L->setTop(0);
        L->pushNil();
        L->pushString(pool.intern("bad argument #1 to 'load' (function expected)"));
        return 2;
    }

    Value loaderFunc = L->at(1);
    Str chunkname = (L->getTop() >= 2 && L->at(2).isString()) ? L->at(2).asString()->c_str() : "=(load)";

    // Collect source pieces by calling the loader function repeatedly
    // Use VM::call instead of L->pcall to preserve the stack (keeps upvalues valid)
    RuntimeServices services(L->getGlobalState());
    Str source;
    for (;;) {
        usize savedCI = L->getCurrentCI();
        L->pushValue(loaderFunc);
        i32 status = L->pcall(0, 1, 0);
        while (L->getCurrentCI() > savedCI) {
            L->popCallInfo();
        }
        if (status != LUA_OK) {
            Value errorValue = L->top();
            L->setTop(0);
            L->pushNil();
            if (errorValue.isString()) {
                L->pushString(errorValue.asString());
            } else {
                L->pushString(pool.intern("load: reader error"));
            }
            settleLoadGC(L);
            return 2;
        }

        Value result = L->at(L->getTop());

        if (result.isNil()) {
            L->pop();
            break;
        }

        if (!result.isString()) {
            L->setTop(0);
            L->pushNil();
            L->pushString(pool.intern("load: loader must return a string or nil"));
            settleLoadGC(L);
            return 2;
        }

        GCString* piece = result.asString();
        if (piece->getLength() == 0) {
            L->pop();
            break;
        }

        source.append(piece->c_str(), piece->getLength());
        L->pop();

        if (stopOnDefinitiveReaderSyntaxError(L, pool, services, source)) {
            settleLoadGC(L);
            return 2;
        }
    }

    try {
        if (isProjectBinaryChunk(StrView(source.data(), source.size()))) {
            Function* func = loadBinaryChunk(L, StrView(source.data(), source.size()));
            L->setTop(0);
            L->pushValue(Value(func));
            settleLoadGC(L);
            return 1;
        }

        // Compile the collected source
        Parser parser(source, services);
        auto parsed = parser.parse();
        if (!parsed) {
            throw parsed.error();
        }
        Chunk chunk = std::move(*parsed);

        CodeGenerator codegen(services);
        Proto* proto = codegen.generate(chunk, chunkname);

        if (!proto) {
            L->setTop(0);
            L->pushNil();
            L->pushString(pool.intern("load: compilation failed"));
            settleLoadGC(L);
            return 2;
        }

        Function* func = createLuaFunctionFromProto(L, proto);

        L->setTop(0);
        L->pushValue(Value(func));
        settleLoadGC(L);
        return 1;

    } catch (const MemoryError&) {
        throw;
    } catch (const std::bad_alloc&) {
        throw;
    } catch (const ParseError& e) {
        L->setTop(0);
        L->pushNil();
        Str errorMsg = Str("load: ") + e.what();
        L->pushString(pool.intern(errorMsg.c_str()));
        settleLoadGC(L);
        return 2;
    } catch (const std::exception& e) {
        L->setTop(0);
        L->pushNil();
        Str errorMsg = Str("load: ") + e.what();
        L->pushString(pool.intern(errorMsg.c_str()));
        settleLoadGC(L);
        return 2;
    }
}

// =====================================================================
// 基础库注册入口（使用现代C++流式API）
// =====================================================================

void BaseLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    // 使用流式API注册所有全局函数
    FunctionRegistrar(L)
        .addGlobal("print", luaB_print)
        .addGlobal("type", luaB_type)
        .addGlobal("tostring", luaB_tostring)
        .addGlobal("tonumber", luaB_tonumber)
        .addGlobal("error", luaB_error)
        .addGlobal("assert", luaB_assert)
        .addGlobal("setmetatable", luaB_setmetatable)
        .addGlobal("getmetatable", luaB_getmetatable)
        .addGlobal("newproxy", luaB_newproxy)
        .addGlobal("next", luaB_next)
        .addGlobal("pairs", luaB_pairs)
        .addGlobal("ipairs", luaB_ipairs)
        .addGlobal("rawget", luaB_rawget)
        .addGlobal("rawset", luaB_rawset)
        .addGlobal("rawequal", luaB_rawequal)
        .addGlobal("select", luaB_select)
        .addGlobal("pcall", luaB_pcall)
        .addGlobal("xpcall", luaB_xpcall)
        .addGlobal("loadstring", luaB_loadstring)
        .addGlobal("loadfile", luaB_loadfile)
        .addGlobal("dofile", luaB_dofile)
        .addGlobal("gcinfo", luaB_gcinfo)
        .addGlobal("getfenv", luaB_getfenv)
        .addGlobal("setfenv", luaB_setfenv)
        .addGlobal("collectgarbage", luaB_collectgarbage)
        .addGlobal("unpack", luaB_unpack)
        .addGlobal("load", luaB_load)
        .commit();
}

void BaseLibModule::initialize(LuaState* L) {
    if (!L) {
        return;
    }

    L->setGlobal("_G", Value(L->getGlobalTable()));

    auto& gs = L->getGlobalState();
    GCString* versionValue = gs.getStringPool().intern("Lua 5.1 (C core prototype)");
    L->setGlobal("_VERSION", Value(versionValue));
}

void openBaseLib(LuaState* L) {
    if (!L) {
        return;
    }

    BaseLibModule module;
    StandardLibrary::openModule(L, module);
}

} // namespace Lua
