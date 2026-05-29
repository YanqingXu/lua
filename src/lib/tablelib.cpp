/**
 * @file tablelib.cpp
 * @brief Lua Table Library Implementation - 表操作库实现
 * 
 * @author Lua C++ Project
 * @date 2026-01-23
 */

#include "lib/tablelib.hpp"
#include "vm/state/lua_state.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "lib/lib_registry.hpp"
#include "vm/vm.hpp"
#include "vm/vm_constants.hpp"
#include "vm/vm_internal.hpp"

#include <string>
#include <sstream>
#include <algorithm>
#include <vector>
#include <cstdio>
#include <cstring>

namespace Lua {

// =====================================================================
// 辅助函数
// =====================================================================

/**
 * @brief 获取表参数
 */
static Table* getTableArg(LuaState* L, i32 idx, const char* funcName) {
    if (!L->isTable(idx)) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "bad argument #%d to 'table.%s' (table expected)", idx, funcName);
        L->error(buffer);
    }
    return L->at(idx).asTable();
}

/**
 * @brief 获取数字参数
 */
static f64 getNumberArg(LuaState* L, i32 idx, const char* funcName) {
    if (!L->isNumber(idx)) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "bad argument #%d to 'table.%s' (number expected)", idx, funcName);
        L->error(buffer);
    }
    return L->toNumber(idx);
}

static Function* getFunctionArg(LuaState* L, i32 idx, const char* funcName) {
    if (!L->isFunction(idx)) {
        char buffer[128];
        std::snprintf(buffer, sizeof(buffer), "bad argument #%d to 'table.%s' (function expected)", idx, funcName);
        L->error(buffer);
    }
    return L->at(idx).asFunction();
}

static Str tableNumberToLuaString(f64 value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.14g", value);
    return Str(buffer);
}

/**
 * @brief 获取表的数组长度（使用 Lua 的 # 运算符语义）
 */
static i32 getTableLength(Table* table) {
    return static_cast<i32>(table->length());
}

static bool defaultSortLess(LuaState* L, const Value& left, const Value& right) {
    return VM::detail::lessThan(L, left, right);
}

static bool callSortComparator(LuaState* L, Function* comparator, const Value& left, const Value& right) {
    i32 originalTop = L->getTop();
    usize savedCI = L->getCurrentCI();
    usize savedTop = L->getAbsoluteTop();
    Vec<Value> savedStack;
    savedStack.reserve(savedTop);
    for (usize i = 0; i < savedTop; ++i) {
        savedStack.push_back(L->getStack().at(i));
    }

    try {
        L->pushFunction(comparator);
        L->pushValue(left);
        L->pushValue(right);
        VM::call(L, 2, 1);
    } catch (const std::exception& e) {
        while (L->getCurrentCI() > savedCI) {
            L->popCallInfo();
        }
        L->getStack().setTop(0);
        L->setAbsoluteTop(0);
        for (const Value& value : savedStack) {
            L->pushValue(value);
        }
        L->error(e.what());
    }

    bool result = L->toBoolean(-1);
    L->setTop(originalTop);
    return result;
}

static Value callForeachCallback(LuaState* L, Function* callback, const Value& key, const Value& value) {
    usize savedCI = L->getCurrentCI();
    usize savedTop = L->getAbsoluteTop();
    Vec<Value> savedStack;
    savedStack.reserve(savedTop);
    for (usize i = 0; i < savedTop; ++i) {
        savedStack.push_back(L->getStack().at(i));
    }

    try {
        L->pushFunction(callback);
        L->pushValue(key);
        L->pushValue(value);
        VM::call(L, 2, 1);
        Value result = L->top();

        L->getStack().setTop(0);
        L->setAbsoluteTop(0);
        for (const Value& stackValue : savedStack) {
            L->pushValue(stackValue);
        }
        return result;
    } catch (const std::exception& e) {
        while (L->getCurrentCI() > savedCI) {
            L->popCallInfo();
        }
        L->getStack().setTop(0);
        L->setAbsoluteTop(0);
        for (const Value& stackValue : savedStack) {
            L->pushValue(stackValue);
        }
        L->error(e.what());
    }
}

// =====================================================================
// table.insert 实现
// =====================================================================

i32 table_insert(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 2) {
        L->error("table.insert: missing arguments");
    }

    Table* table = getTableArg(L, 1, "insert");
    
    if (nargs == 2) {
        // table.insert(table, value) - 在末尾插入
        i32 len = getTableLength(table);
        Value value = L->at(2);
        table->set(Value(static_cast<f64>(len + 1)), value);
    } else {
        // table.insert(table, pos, value) - 在指定位置插入
        i32 pos = static_cast<i32>(getNumberArg(L, 2, "insert"));
        Value value = L->at(3);
        i32 len = getTableLength(table);
        
        // 将 pos 到 len 的元素后移
        for (i32 i = len; i >= pos; i--) {
            Value v = table->get(Value(static_cast<f64>(i)));
            table->set(Value(static_cast<f64>(i + 1)), v);
        }
        
        // 插入新值
        table->set(Value(static_cast<f64>(pos)), value);
    }
    
    return 0;
}

// =====================================================================
// table.foreach / table.foreachi 实现
// =====================================================================

i32 table_foreach(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 2) {
        L->error("table.foreach: missing arguments");
    }

    Table* table = getTableArg(L, 1, "foreach");
    Function* callback = getFunctionArg(L, 2, "foreach");

    Value key;
    Value nextKey;
    Value nextValue;
    while (table->next(key, nextKey, nextValue)) {
        Value result = callForeachCallback(L, callback, nextKey, nextValue);
        if (!result.isNil()) {
            L->pushValue(result);
            return 1;
        }
        key = nextKey;
    }

    return 0;
}

i32 table_foreachi(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 2) {
        L->error("table.foreachi: missing arguments");
    }

    Table* table = getTableArg(L, 1, "foreachi");
    Function* callback = getFunctionArg(L, 2, "foreachi");
    i32 len = getTableLength(table);

    for (i32 i = 1; i <= len; ++i) {
        Value key(static_cast<f64>(i));
        Value value = table->get(key);
        Value result = callForeachCallback(L, callback, key, value);
        if (!result.isNil()) {
            L->pushValue(result);
            return 1;
        }
    }

    return 0;
}

// =====================================================================
// table.remove 实现
// =====================================================================

i32 table_remove(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("table.remove: missing table argument");
    }

    Table* table = getTableArg(L, 1, "remove");
    i32 len = getTableLength(table);
    
    i32 pos = (nargs >= 2) ? static_cast<i32>(getNumberArg(L, 2, "remove")) : len;
    
    if (pos < 1 || pos > len) {
        L->pushNil();
        return 1;
    }
    
    // 获取要移除的值
    Value removed = table->get(Value(static_cast<f64>(pos)));
    
    // 将 pos+1 到 len 的元素前移
    for (i32 i = pos; i < len; i++) {
        Value v = table->get(Value(static_cast<f64>(i + 1)));
        table->set(Value(static_cast<f64>(i)), v);
    }
    
    // 删除最后一个元素
    table->remove(Value(static_cast<f64>(len)));
    
    // 返回被移除的值
    L->pushValue(removed);
    return 1;
}

// =====================================================================
// table.concat 实现
// =====================================================================

i32 table_concat(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("table.concat: missing table argument");
    }

    Table* table = getTableArg(L, 1, "concat");
    
    // 获取分隔符（默认为空字符串）
    Str sep;
    if (nargs >= 2 && !L->isNil(2)) {
        const char* sepChars = L->toString(2);
        if (sepChars == nullptr) {
            L->error("table.concat: separator must be a string or number");
        }
        const Value& sepVal = L->at(2);
        usize sepLen = sepVal.isString() ? sepVal.asString()->getLength() : std::strlen(sepChars);
        sep.assign(sepChars, sepLen);
    }

    // 获取起始和结束索引
    i32 i = (nargs >= 3) ? static_cast<i32>(getNumberArg(L, 3, "concat")) : 1;
    i32 j = (nargs >= 4) ? static_cast<i32>(getNumberArg(L, 4, "concat")) : getTableLength(table);

    // 连接字符串
    Str result;
    for (i32 idx = i; idx <= j; idx++) {
        if (idx > i) {
            result.append(sep);
        }

        Value v = table->get(Value(static_cast<f64>(idx)));
        if (v.isString()) {
            result.append(v.asString()->getData());
        } else if (v.isNumber()) {
            result.append(tableNumberToLuaString(v.asNumber()));
        } else {
            L->error("table.concat: invalid value (must be string or number)");
        }
    }

    // 返回连接后的字符串
    L->pushString(L->getGlobalState().getStringPool().intern(result));
    return 1;
}

// =====================================================================
// table.sort 实现
// =====================================================================

i32 table_sort(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("table.sort: missing table argument");
    }

    Table* table = getTableArg(L, 1, "sort");
    i32 len = getTableLength(table);
    Function* comparator = nullptr;

    if (nargs >= 2) {
        if (!L->at(2).isFunction()) {
            L->error("bad argument #2 to 'table.sort' (function expected)");
        }
        comparator = L->at(2).asFunction();
    }

    // 提取数组部分到 vector
    std::vector<Value> arr;
    arr.reserve(len);
    for (i32 i = 1; i <= len; i++) {
        arr.push_back(table->get(Value(static_cast<f64>(i))));
    }

    std::sort(arr.begin(), arr.end(), [&](const Value& left, const Value& right) {
        return comparator != nullptr
            ? callSortComparator(L, comparator, left, right)
            : defaultSortLess(L, left, right);
    });

    // 将排序后的值写回表
    for (i32 i = 0; i < len; i++) {
        table->set(Value(static_cast<f64>(i + 1)), arr[i]);
    }

    return 0;
}

// =====================================================================
// table.maxn 实现
// =====================================================================

i32 table_maxn(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("table.maxn: missing table argument");
    }

    Table* table = getTableArg(L, 1, "maxn");
    LuaNumber maxIndex = 0.0;

    Value key;
    Value nextKey;
    Value nextValue;
    while (table->next(key, nextKey, nextValue)) {
        if (nextKey.isNumber()) {
            LuaNumber n = nextKey.asNumber();
            if (n > maxIndex) {
                maxIndex = n;
            }
        }
        key = nextKey;
    }

    L->pushNumber(maxIndex);
    return 1;
}

// =====================================================================
// table.getn 实现（Lua 5.1 兼容函数）
// =====================================================================

i32 table_getn(LuaState* L) {
    if (L->getTop() < 1) {
        L->error("table.getn: missing table argument");
    }

    Table* table = getTableArg(L, 1, "getn");
    L->pushNumber(static_cast<LuaNumber>(getTableLength(table)));
    return 1;
}

// =====================================================================
// table.pack 实现
// =====================================================================

i32 table_pack(LuaState* L) {
    i32 nargs = L->getTop();

    // 创建新表
    Table* result = new Table();
    L->getGlobalState().getGC().registerObject(result);

    // 将所有参数打包到表中
    for (i32 i = 1; i <= nargs; i++) {
        result->set(Value(static_cast<f64>(i)), L->at(i));
    }

    // 设置 "n" 字段为参数数量
    GCString* nKey = L->getGlobalState().getStringPool().intern("n");
    result->set(Value(nKey), Value(static_cast<f64>(nargs)));

    L->pushValue(Value(result));
    return 1;
}

// =====================================================================
// table.unpack 实现
// =====================================================================

i32 table_unpack(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 1) {
        L->error("table.unpack: missing table argument");
    }

    Table* table = getTableArg(L, 1, "unpack");

    // 获取起始和结束索引
    i32 i = (nargs >= 2 && !L->at(2).isNil()) ? static_cast<i32>(getNumberArg(L, 2, "unpack")) : 1;
    i32 j = (nargs >= 3 && !L->at(3).isNil()) ? static_cast<i32>(getNumberArg(L, 3, "unpack")) : getTableLength(table);

    // 将表元素压入栈
    i32 count = 0;
    for (i32 idx = i; idx <= j; idx++) {
        Value v = table->get(Value(static_cast<f64>(idx)));
        L->pushValue(v);
        count++;
    }

    return count;
}

// =====================================================================
// table.move 实现
// =====================================================================

i32 table_move(LuaState* L) {
    i32 nargs = L->getTop();
    if (nargs < 4) {
        L->error("table.move: missing arguments");
    }

    Table* a1 = getTableArg(L, 1, "move");
    i32 f = static_cast<i32>(getNumberArg(L, 2, "move"));
    i32 e = static_cast<i32>(getNumberArg(L, 3, "move"));
    i32 t = static_cast<i32>(getNumberArg(L, 4, "move"));

    // 获取目标表（默认为源表）
    Table* a2 = (nargs >= 5 && L->isTable(5)) ? L->at(5).asTable() : a1;

    // 移动元素
    if (f <= e) {
        // 正向移动
        if (t > f) {
            // 从后向前复制，避免覆盖
            for (i32 i = e - f; i >= 0; i--) {
                Value v = a1->get(Value(static_cast<f64>(f + i)));
                a2->set(Value(static_cast<f64>(t + i)), v);
            }
        } else {
            // 从前向后复制
            for (i32 i = 0; i <= e - f; i++) {
                Value v = a1->get(Value(static_cast<f64>(f + i)));
                a2->set(Value(static_cast<f64>(t + i)), v);
            }
        }
    }

    // 返回目标表
    L->pushValue(Value(a2));
    return 1;
}

// =====================================================================
// 库注册
// =====================================================================

void TableLibModule::registerFunctions(LuaState* L) {
    if (!L) {
        return;
    }

    // 创建 table 表
    Table* tableTable = FunctionRegistrar::createLibTable(L, "table");

    // 注册函数
    FunctionRegistrar(L)
        .addGlobal("insert", table_insert)
        .addGlobal("remove", table_remove)
        .addGlobal("concat", table_concat)
        .addGlobal("sort", table_sort)
        .addGlobal("foreach", table_foreach)
        .addGlobal("foreachi", table_foreachi)
        .addGlobal("maxn", table_maxn)
        .addGlobal("getn", table_getn)
        .addGlobal("pack", table_pack)
        .addGlobal("unpack", table_unpack)
        .addGlobal("move", table_move)
        .commitToTable(tableTable);
}

void TableLibModule::initialize(LuaState* L) {
    // 无需额外初始化
    (void)L;
}

void openTableLib(LuaState* L) {
    if (!L) {
        return;
    }

    TableLibModule module;
    module.registerFunctions(L);
    module.initialize(L);
}

} // namespace Lua


