/**
 * @file tablelib.cpp
 * @brief Lua 表操作库实现
 *
 * @author Lua C++ 项目
 * @date 2026-01-23
 */

#include "lib/tablelib.hpp"
#include "vm/state/lua_state.hpp"
#include "core/table.hpp"
#include "core/value.hpp"
#include "core/string_pool.hpp"
#include "core/function.hpp"
#include "lib/lib_registry.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/vm.hpp"
#include "vm/vm_constants.hpp"
#include "vm/vm_internal.hpp"
#include "common/number_conversion.hpp"

#include <format>
#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <sstream>
#include <string>

namespace Lua {

// =====================================================================
// 辅助函数
// =====================================================================

/**
 * @brief 获取表参数
 */
static Table* getTableArg(LuaState* L, i32 idx, const char* funcName) {
    if (!L->isTable(idx)) {
        L->error(std::format("bad argument #{} to 'table.{}' (table expected)", idx, funcName).c_str());
    }
    return L->at(idx).asTable();
}

/**
 * @brief 获取数字参数
 */
static f64 getNumberArg(LuaState* L, i32 idx, const char* funcName) {
    if (!L->isNumber(idx)) {
        L->error(std::format("bad argument #{} to 'table.{}' (number expected)", idx, funcName).c_str());
    }
    return L->toNumber(idx);
}

static Function* getFunctionArg(LuaState* L, i32 idx, const char* funcName) {
    if (!L->isFunction(idx)) {
        L->error(std::format("bad argument #{} to 'table.{}' (function expected)", idx, funcName).c_str());
    }
    return L->at(idx).asFunction();
}

struct TableConcatText {
    std::array<char, 64> numberBuffer{};
    StrView view;
};

static bool tableConcatText(const Value& value, TableConcatText& text) {
    if (value.isString()) {
        text.view = value.asString()->view();
        return true;
    }
    if (!value.isNumber()) {
        return false;
    }

    text.view = luaNumberToView(value.asNumber(), text.numberBuffer);
    return true;
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
    L->consumeNativeWork(savedTop == 0 ? 1 : static_cast<u64>(savedTop));
    LuaVector<Value> savedStack(LuaStdAllocator<Value>(L->getGlobalState().getAllocator()));
    savedStack.reserve(savedTop);
    for (usize i = 0; i < savedTop; ++i) {
        savedStack.push_back(L->getStack().at(i));
    }

    const auto restoreStack = [&]() {
        while (L->getCurrentCI() > savedCI) {
            L->popCallInfo();
        }
        L->getStack().setTop(0);
        L->setAbsoluteTop(0);
        for (const Value& value : savedStack) {
            L->pushValue(value);
        }
    };

    try {
        L->pushFunction(comparator);
        L->pushValue(left);
        L->pushValue(right);
        RuntimeServices services(L->getGlobalState());
        VM::call(services, L, 2, 1);
    } catch (const MemoryError&) {
        restoreStack();
        throw;
    } catch (const std::bad_alloc&) {
        restoreStack();
        throw;
    } catch (const std::exception& e) {
        restoreStack();
        L->error(e.what());
    }

    bool result = L->toBoolean(-1);
    L->setTop(originalTop);
    return result;
}

static i32 getIntegerArg(LuaState* L, i32 idx, const char* funcName,
                         IntegerConversion mode = IntegerConversion::Truncate) {
    const auto converted = checkedLuaInteger(getNumberArg(L, idx, funcName), mode);
    if (!converted) {
        const char* detail = converted.error() == IntegerConversionError::NotFinite
                                 ? "finite number expected"
                             : converted.error() == IntegerConversionError::NotIntegral
                                 ? "integer expected"
                                 : "number out of range";
        L->error(std::format("bad argument #{} to 'table.{}' ({})", idx, funcName, detail).c_str());
    }
    return *converted;
}

static bool checkedSortLess(LuaState* L, Function* comparator, const Value& left, const Value& right,
                            usize& comparisons) {
    const usize comparisonLimit = L->getGlobalState().getResourcePolicy().maxSortComparisons;
    if (comparisons >= comparisonLimit) {
        L->error("table.sort: comparison limit exceeded");
    }
    ++comparisons;
    L->consumeNativeWork();
    const bool result = comparator != nullptr ? callSortComparator(L, comparator, left, right)
                                              : defaultSortLess(L, left, right);
    if (result && comparator != nullptr) {
        if (comparisons >= comparisonLimit) {
            L->error("table.sort: comparison limit exceeded");
        }
        ++comparisons;
        L->consumeNativeWork();
        if (callSortComparator(L, comparator, right, left)) {
            L->error("invalid order function for sorting");
        }
    }
    return result;
}

static void safeMergeSort(LuaState* L, LuaVector<Value>& values, Function* comparator) {
    const usize size = values.size();
    usize comparisons = 0;
    if (size < 2) {
        if (size == 1 && comparator != nullptr && checkedSortLess(L, comparator, values[0], values[0], comparisons)) {
            L->error("invalid order function for sorting");
        }
        return;
    }

    /**
     * @brief 排序前拒绝两种最常见的无效比较器。
     *
     * 即使有状态比较器在这些检查后改变结果，下方归并实现仍保持内存安全。
     */
    if (comparator != nullptr) {
        for (const Value& value : values) {
            if (checkedSortLess(L, comparator, value, value, comparisons)) {
                L->error("invalid order function for sorting");
            }
        }
    }

    LuaVector<Value> scratch(values.get_allocator());
    scratch.resize(size);
    for (usize width = 1; width < size; width = width > size / 2 ? size : width * 2) {
        for (usize left = 0; left < size; left += width * 2) {
            const usize middle = std::min(left + width, size);
            const usize right = std::min(left + width * 2, size);
            usize first = left;
            usize second = middle;
            usize output = left;

            while (first < middle && second < right) {
                if (checkedSortLess(L, comparator, values[second], values[first], comparisons)) {
                    scratch[output++] = values[second++];
                } else {
                    scratch[output++] = values[first++];
                }
            }
            while (first < middle) {
                scratch[output++] = values[first++];
            }
            while (second < right) {
                scratch[output++] = values[second++];
            }
        }
        values.swap(scratch);
    }
}

static Value callForeachCallback(LuaState* L, Function* callback, const Value& key, const Value& value) {
    usize savedCI = L->getCurrentCI();
    usize savedTop = L->getAbsoluteTop();
    L->consumeNativeWork(savedTop == 0 ? 1 : static_cast<u64>(savedTop));
    LuaVector<Value> savedStack(LuaStdAllocator<Value>(L->getGlobalState().getAllocator()));
    savedStack.reserve(savedTop);
    for (usize i = 0; i < savedTop; ++i) {
        savedStack.push_back(L->getStack().at(i));
    }

    try {
        L->pushFunction(callback);
        L->pushValue(key);
        L->pushValue(value);
        RuntimeServices services(L->getGlobalState());
        VM::call(services, L, 2, 1);
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
// table.insert(table, value)——在末尾插入
        i32 len = getTableLength(table);
        L->consumeNativeWork();
        Value value = L->at(2);
        table->set(Value(static_cast<f64>(len + 1)), value);
    } else {
// table.insert(table, pos, value)——在指定位置插入
        i32 pos = getIntegerArg(L, 2, "insert");
        Value value = L->at(3);
        i32 len = getTableLength(table);
        if (pos < 1 || pos > len + 1) {
            L->error("bad argument #2 to 'table.insert' (position out of bounds)");
        }

        const u64 shifts = static_cast<u64>(len - pos + 1);
        L->consumeNativeWork(shifts + 1);

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
        L->consumeNativeWork();
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
        L->consumeNativeWork();
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

    i32 pos = (nargs >= 2) ? getIntegerArg(L, 2, "remove") : len;

    if (pos < 1 || pos > len) {
        L->pushNil();
        return 1;
    }

    // 获取要移除的值
    Value removed = table->get(Value(static_cast<f64>(pos)));

    L->consumeNativeWork(static_cast<u64>(len - pos + 1));

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
    TableConcatText separator;
    if (nargs >= 2 && !L->isNil(2)) {
        if (!tableConcatText(L->at(2), separator)) {
            L->error("table.concat: separator must be a string or number");
        }
    }

    // 获取起始和结束索引
    i32 i = (nargs >= 3) ? getIntegerArg(L, 3, "concat") : 1;
    i32 j = (nargs >= 4) ? getIntegerArg(L, 4, "concat") : getTableLength(table);

    // 连接字符串
    LuaVector<char> result(LuaStdAllocator<char>(L->getGlobalState().getAllocator()));
    const ResourcePolicy& resources = L->getGlobalState().getResourcePolicy();
    const usize outputLimit = std::min(resources.maxStringBytes, resources.maxOutputBytes);
    const auto append = [&](StrView text) {
        if (text.size() > outputLimit || result.size() > outputLimit - text.size()) {
            L->error("table.concat: result exceeds resource limit");
        }
        L->consumeNativeWork(text.empty() ? 1 : static_cast<u64>(text.size()));
        result.insert(result.end(), text.begin(), text.end());
    };

    for (i32 idx = i; idx <= j;) {
        if (idx > i && !separator.view.empty()) {
            append(separator.view);
        }

        const Value value = table->get(Value(static_cast<f64>(idx)));
        TableConcatText text;
        if (!tableConcatText(value, text)) {
            L->error("table.concat: invalid value (must be string or number)");
        }
        append(text.view);
        if (idx == j) {
            break;
        }
        ++idx;
    }

    // 返回连接后的字符串
    const StrView resultView = result.empty() ? StrView("") : StrView(result.data(), result.size());
    L->pushString(L->getGlobalState().getStringPool().intern(resultView));
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
    if (static_cast<usize>(len) > L->getGlobalState().getResourcePolicy().maxSortElements) {
        L->error("table.sort: element limit exceeded");
    }
    Function* comparator = nullptr;

    if (nargs >= 2) {
        if (!L->at(2).isFunction()) {
            L->error("bad argument #2 to 'table.sort' (function expected)");
        }
        comparator = L->at(2).asFunction();
    }

    // 提取数组部分到 vector
    LuaVector<Value> arr(LuaStdAllocator<Value>(L->getGlobalState().getAllocator()));
    arr.reserve(len);
    for (i32 i = 1; i <= len; i++) {
        if ((i & 255) == 1) {
            L->consumeNativeWork(static_cast<u64>(std::min(256, len - i + 1)));
        }
        arr.push_back(table->get(Value(static_cast<f64>(i))));
    }

    safeMergeSort(L, arr, comparator);

    // 将排序后的值写回表
    for (i32 i = 0; i < len; i++) {
        if ((i & 255) == 0) {
            L->consumeNativeWork(static_cast<u64>(std::min(256, len - i)));
        }
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
        L->consumeNativeWork();
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

    L->consumeNativeWork(nargs == 0 ? 1 : static_cast<u64>(nargs));

    // 创建新表
    Table* result = L->getGlobalState().getGC().create<Table>();

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
    i32 i = (nargs >= 2 && !L->at(2).isNil()) ? getIntegerArg(L, 2, "unpack") : 1;
    i32 j = (nargs >= 3 && !L->at(3).isNil()) ? getIntegerArg(L, 3, "unpack") : getTableLength(table);

    const i64 count64 = j >= i ? static_cast<i64>(j) - static_cast<i64>(i) + 1 : 0;
    const usize returnLimit = L->getGlobalState().getResourcePolicy().maxReturnValues;
    if (count64 > static_cast<i64>(std::numeric_limits<i32>::max()) ||
        static_cast<u64>(count64) > static_cast<u64>(returnLimit)) {
        L->error("table.unpack: result count exceeds resource limit");
    }
    const usize countSlots = static_cast<usize>(count64);
    if (countSlots > std::numeric_limits<usize>::max() - L->getAbsoluteTop()) {
        throw StackOverflowError("stack overflow: resource stack slot limit exceeded");
    }
    L->getStack().checkLimit(L->getAbsoluteTop() + countSlots);
    L->consumeNativeWork(count64 == 0 ? 1 : static_cast<u64>(count64));

    // 将表元素压入栈
    const i32 count = static_cast<i32>(count64);
    for (i64 offset = 0; offset < count64; ++offset) {
        const i32 idx = static_cast<i32>(static_cast<i64>(i) + offset);
        Value v = table->get(Value(static_cast<f64>(idx)));
        L->pushValue(v);
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
    i32 f = getIntegerArg(L, 2, "move");
    i32 e = getIntegerArg(L, 3, "move");
    i32 t = getIntegerArg(L, 4, "move");

    // 获取目标表（默认为源表）
    Table* a2 = (nargs >= 5 && L->isTable(5)) ? L->at(5).asTable() : a1;

    // 移动元素
    if (f <= e) {
        const i64 count = static_cast<i64>(e) - static_cast<i64>(f) + 1;
        const i64 targetEnd = static_cast<i64>(t) + count - 1;
        if (targetEnd > std::numeric_limits<i32>::max() || targetEnd < std::numeric_limits<i32>::min()) {
            L->error("table.move: destination range out of bounds");
        }
        L->consumeNativeWork(static_cast<u64>(count));

        // 正向移动
        if (a1 == a2 && t > f && t <= e) {
            // 从后向前复制，避免覆盖
            for (i64 offset = count; offset-- > 0;) {
                Value v = a1->get(Value(static_cast<f64>(static_cast<i64>(f) + offset)));
                a2->set(Value(static_cast<f64>(static_cast<i64>(t) + offset)), v);
            }
        } else {
            // 从前向后复制
            for (i64 offset = 0; offset < count; ++offset) {
                Value v = a1->get(Value(static_cast<f64>(static_cast<i64>(f) + offset)));
                a2->set(Value(static_cast<f64>(static_cast<i64>(t) + offset)), v);
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

    L->requireStandardLibrary("table");
    TableLibModule module;
    module.registerFunctions(L);
    module.initialize(L);
}

} // namespace Lua
