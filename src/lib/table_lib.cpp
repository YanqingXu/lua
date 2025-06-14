#include "table_lib.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include "../vm/table.hpp"
#include <sstream>
#include <algorithm>
#include <functional>

namespace Lua {
    
    StrView TableLib::getName() const noexcept {
        return "table";
    }
    
    void TableLib::registerFunctions(FunctionRegistry& registry) {
        // 注册table库函数
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "concat", concat);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "insert", insert);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "remove", remove);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "sort", sort);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "pack", pack);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "unpack", unpack);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "move", move);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "getn", getn);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "setn", setn);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "maxn", maxn);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "foreach", foreach);
        REGISTER_NAMESPACED_FUNCTION(registry, "table", "foreachi", foreachi);
    }
    
    void TableLib::initialize(State* state) {
        // 可选的初始化逻辑
    }
    
    Value TableLib::concat(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgRange(nargs, 1, 4, StrView("table.concat"));
            
            Table* table = validateTableArg(state, 1, "table.concat");
            
            // 获取分隔符（默认为空字符串）
            Str separator = "";
            if (nargs >= 2) {
                Value sepVal = state->get(2);
                if (!sepVal.isNil()) {
                    separator = sepVal.asString();
                }
            }
            
            // 获取起始和结束位置
            i32 start = 1;
            i32 end = getTableLength(table);
            
            if (nargs >= 3) {
                Value startVal = state->get(3);
                if (startVal.isNumber()) {
                    start = static_cast<i32>(startVal.asNumber());
                }
            }
            
            if (nargs >= 4) {
                Value endVal = state->get(4);
                if (endVal.isNumber()) {
                    end = static_cast<i32>(endVal.asNumber());
                }
            }
            
            // 验证范围
            if (start > end) {
                state->push(Value(""));
                return 1;
            }
            
            // 连接字符串
            Str result = "";
            for (i32 i = start; i <= end; ++i) {
                Value key = Value(i);
                Value value = table->get(key);
                if (!value.isNil()) {
                    if (i > start && !separator.empty()) {
                        result += separator;
                    }
                    result += value.toString();
                }
            }
            
            state->push(Value(result));
            return 1;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.concat: " << e.what() << std::endl;
            return 0;
        }
    }
    
    Value TableLib::insert(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgRange(nargs, 2, 3, StrView("table.insert"));
            
            Table* table = validateTableArg(state, 1, "table.insert");
            
            i32 pos;
            Value value;
            
            if (nargs == 2) {
                // table.insert(list, value) - 插入到末尾
                pos = getTableLength(table) + 1;
                value = state->get(2);
            } else {
                // table.insert(list, pos, value)
                Value posVal = state->get(2);
                if (!posVal.isNumber()) {
                    throw LibException(LibErrorCode::InvalidArgument, StrView("position must be a number"));
                }
                pos = static_cast<i32>(posVal.asNumber());
                value = state->get(3);
            }
            
            i32 length = getTableLength(table);
            
            // 验证位置
            if (pos < 1 || pos > length + 1) {
                throw LibException(LibErrorCode::OutOfRange, StrView("position out of bounds"));
            }
            
            // 移动元素为新元素腾出空间
            for (i32 i = length; i >= pos; --i) {
                Value key = Value(i);
                Value val = table->get(key);
                if (!val.isNil()) {
                    Value newKey = Value(i + 1);
                    table->set(newKey, val);
                }
            }
            
            // 插入新元素
            Value key = Value(pos);
            table->set(key, value);
            
            return 0;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.insert: " << e.what() << std::endl;
            return 0;
        }
    }
    
    Value TableLib::remove(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgRange(nargs, 1, 2, StrView("table.remove"));
            
            Table* table = validateTableArg(state, 1, "table.remove");
            
            i32 length = getTableLength(table);
            if (length == 0) {
                return 0;
            }
            
            i32 pos = length; // 默认移除最后一个元素
            
            if (nargs >= 2) {
                Value posVal = state->get(2);
                if (!posVal.isNumber()) {
                    throw LibException(LibErrorCode::InvalidArgument, StrView("position must be a number"));
                }
                pos = static_cast<i32>(posVal.asNumber());
            }
            
            // 验证位置
            if (pos < 1 || pos > length) {
                return Value(nullptr);
            }
            
            // 获取要移除的元素
            Value key = Value(pos);
            Value removedValue = table->get(key);
            
            // 移动后续元素
            for (i32 i = pos; i < length; ++i) {
                Value nextKey = Value(i + 1);
                Value nextVal = table->get(nextKey);
                table->set(key, nextVal);
                key = nextKey;
            }
            
            // 移除最后一个元素
            Value lastKey = Value(length);
            table->set(lastKey, Value());
            
            state->push(removedValue);
            return 1;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.remove: " << e.what() << std::endl;
            return 0;
        }
    }
    
    Value TableLib::sort(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgRange(nargs, 1, 2, StrView("table.sort"));
            
            Table* table = validateTableArg(state, 1, "table.sort");
            
            Value comparator = Value();
            if (nargs >= 2) {
                comparator = state->get(2);
                if (!comparator.isNil() && !comparator.isFunction()) {
                    throw LibException(LibErrorCode::InvalidArgument, StrView("comparator must be a function"));
                }
            }
            
            if (comparator.isNil()) {
                // 使用默认比较函数
                i32 length = getTableLength(table);
                Vec<Value> values;
                
                // 收集所有值
                for (i32 i = 1; i <= length; ++i) {
                    Value key = Value(i);
                    Value val = table->get(key);
                    if (!val.isNil()) {
                        values.push_back(val);
                    }
                }
                
                // 排序
                std::sort(values.begin(), values.end(), defaultCompare);
                
                // 写回表
                for (size_t i = 0; i < values.size(); ++i) {
                    Value key = Value(static_cast<i32>(i + 1));
                    table->set(key, values[i]);
                }
                
                // 清除多余的元素
                for (i32 i = static_cast<i32>(values.size()) + 1; i <= length; ++i) {
                    Value key = Value(i);
                    table->set(key, Value());
                }
            } else {
                // 使用自定义比较函数
                sortWithComparator(state, table, comparator);
            }
            
            return 0;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.sort: " << e.what() << std::endl;
            return 0;
        }
    }

    Value TableLib::pack(State* state, i32 nargs) {
        try {
            // 创建新表
            GCRef<Table> resultRef = make_gc_table();
            Table* result = resultRef.get();
            
            // 设置n字段
            Value nKey = Value("n");
            Value nVal = Value(nargs);
            result->set(nKey, nVal);
            
            // 打包所有参数
            for (i32 i = 1; i <= nargs; ++i) {
                Value key = Value(i);
                Value val = state->get(i);
                result->set(key, val);
            }
            
            state->push(Value(result));
            return 1;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.pack: " << e.what() << std::endl;
            return 0;
        }
    }

    Value TableLib::unpack(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgRange(nargs, 1, 3, StrView("table.unpack"));
            
            Table* table = validateTableArg(state, 1, "table.unpack");
            
            i32 start = 1;
            i32 end = getTableLength(table);
            
            if (nargs >= 2) {
                Value startVal = state->get(2);
                if (startVal.isNumber()) {
                    start = static_cast<i32>(startVal.asNumber());
                }
            }
            
            if (nargs >= 3) {
                Value endVal = state->get(3);
                if (endVal.isNumber()) {
                    end = static_cast<i32>(endVal.asNumber());
                }
            }
            
            // 将指定范围的元素推入栈
            i32 count = 0;
            for (i32 i = start; i <= end; ++i) {
                Value key = Value(i);
                Value value = table->get(key);
                state->push(value);
                count++;
            }
            
            return count;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.unpack: " << e.what() << std::endl;
            return 0;
        }
    }

    Value TableLib::move(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgRange(nargs, 4, 5, StrView("table.move"));
            
            Table* a1 = validateTableArg(state, 1, "table.move");
            
            Value fVal = state->get(2);
            Value eVal = state->get(3);
            Value tVal = state->get(4);
            
            if (!fVal.isNumber() || !eVal.isNumber() || !tVal.isNumber()) {
                throw LibException(LibErrorCode::InvalidArgument, StrView("indices must be numbers"));
            }
            
            i32 f = static_cast<i32>(fVal.asNumber());
            i32 e = static_cast<i32>(eVal.asNumber());
            i32 t = static_cast<i32>(tVal.asNumber());
            
            Table* a2 = a1; // 默认目标表是源表
            if (nargs >= 5) {
                a2 = validateTableArg(state, 5, "table.move");
            }
            
            // 执行移动操作
            if (f <= e) {
                i32 n = e - f + 1;
                if (t > f) {
                    // 从后往前移动避免覆盖
                    for (i32 i = n - 1; i >= 0; --i) {
                        Value srcKey = Value(f + i);
                        Value dstKey = Value(t + i);
                        Value val = a1->get(srcKey);
                        a2->set(dstKey, val);
                    }
                } else {
                    // 从前往后移动
                    for (i32 i = 0; i < n; ++i) {
                        Value srcKey = Value(f + i);
                        Value dstKey = Value(t + i);
                        Value val = a1->get(srcKey);
                        a2->set(dstKey, val);
                    }
                }
            }
            
            state->push(Value(a2));
            return 1;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.move: " << e.what() << std::endl;
            return 0;
        }
    }

    Value TableLib::getn(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgCount(nargs, 1, StrView("table.getn"));
            
            Table* table = validateTableArg(state, 1, "table.getn");
            i32 length = getTableLength(table);
            state->push(Value(length));
            return 1;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.getn: " << e.what() << std::endl;
            return 0;
        }
    }

    Value TableLib::setn(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgCount(nargs, 2, StrView("table.setn"));
            
            Table* table = validateTableArg(state, 1, "table.setn");
            
            Value nVal = state->get(2);
            if (!nVal.isNumber()) {
                throw LibException(LibErrorCode::InvalidArgument, StrView("n must be a number"));
            }
            
            // 在Lua 5.1+中，setn是一个空操作，因为表长度是自动计算的
            // 这里保持兼容性，但不做实际操作
            
            return 0;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.setn: " << e.what() << std::endl;
            return 0;
        }
    }

    Value TableLib::maxn(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgCount(nargs, 1, StrView("table.maxn"));
            
            Table* table = validateTableArg(state, 1, "table.maxn");
            
            f64 maxIndex = 0;
            
            // 遍历表找到最大的数字索引
            // 注意：这是一个简化实现，实际需要遍历整个哈希表
            i32 length = getTableLength(table);
            for (i32 i = 1; i <= length * 2; ++i) { // 检查更大的范围
                Value key = Value(i);
                Value val = table->get(key);
                if (!val.isNil()) {
                    maxIndex = std::max(maxIndex, static_cast<f64>(i));
                }
            }
            
            state->push(Value(maxIndex));
            return 1;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.maxn: " << e.what() << std::endl;
            return 0;
        }
    }

    Value TableLib::foreach(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgCount(nargs, 2, StrView("table.foreach"));
            
            Table* table = validateTableArg(state, 1, "table.foreach");
            
            Value func = state->get(2);
            if (!func.isFunction()) {
                throw LibException(LibErrorCode::InvalidArgument, StrView("second argument must be a function"));
            }
            
            // 遍历表的所有元素
            // 注意：这是一个简化实现
            i32 length = getTableLength(table);
            for (i32 i = 1; i <= length; ++i) {
                Value key = Value(i);
                Value val = table->get(key);
                if (!val.isNil()) {
                    // 调用函数 func(key, val)
                    state->push(func);
                    state->push(key);
                    state->push(val);
                    Vec<Value> args = {key, val};
                    Value result = state->call(func, args);
                    
                    // 如果函数返回非nil值，停止遍历
                    if (!result.isNil()) {
                        break;
                    }
                }
            }
            
            return 0;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.foreach: " << e.what() << std::endl;
            return 0;
        }
    }

    Value TableLib::foreachi(State* state, i32 nargs) {
        try {
            ErrorUtils::checkArgCount(nargs, 2, StrView("table.foreachi"));
            
            Table* table = validateTableArg(state, 1, "table.foreachi");
            
            Value func = state->get(2);
            if (!func.isFunction()) {
                throw LibException(LibErrorCode::InvalidArgument, StrView("second argument must be a function"));
            }
            
            // 遍历表的数组部分
            i32 length = getTableLength(table);
            for (i32 i = 1; i <= length; ++i) {
                Value key = Value(i);
                Value val = table->get(key);
                if (!val.isNil()) {
                    // 调用函数 func(index, val)
                    state->push(func);
                    state->push(key);
                    state->push(val);
                    Vec<Value> args = {key, val};
                    Value result = state->call(func, args);
                    
                    // 如果函数返回非nil值，停止遍历
                    if (!result.isNil()) {
                        break;
                    }
                } else {
                    // 遇到nil值停止遍历（数组部分结束）
                    break;
                }
            }
            
            return 0;
            
        } catch (const LibException& e) {
            // 错误处理：直接返回0表示失败
            std::cerr << "Error in table.foreachi: " << e.what() << std::endl;
            return 0;
        }
    }
    
    // 辅助函数实现
    i32 TableLib::getTableLength(Table* table) {
        // 简化实现：返回表的长度
        // 这里应该实现正确的Lua表长度计算
        i32 length = 0;
        for (i32 i = 1; ; ++i) {
            Value key = Value(i);
            Value value = table->get(key);
            if (value.isNil()) {
                break;
            }
            length = i;
        }
        return length;
    }
    
    bool TableLib::isValidIndex(i32 index, i32 length) {
        return index >= 1 && index <= length;
    }
    
    Vec<Str> TableLib::tableToStringArray(Table* table, i32 start, i32 end) {
        Vec<Str> result;
        
        for (i32 i = start; i <= end; ++i) {
            Value key = Value(i);
            Value val = table->get(key);
            
            if (val.isNil()) {
                break; // 遇到nil停止
            }
            
            result.push_back(TypeConverter::toString(val));
        }
        
        return result;
    }
    
    bool TableLib::defaultCompare(const Value& a, const Value& b) {
        // 简单的默认比较实现
        if (a.isNumber() && b.isNumber()) {
            return a.asNumber() < b.asNumber();
        }
        if (a.isString() && b.isString()) {
            return a.asString() < b.asString();
        }
        // 对于其他类型，使用类型优先级
        return a.type() < b.type();
    }
    
    void TableLib::sortWithComparator(State* state, Table* table, Value comparator) {
        i32 length = getTableLength(table);
        Vec<Value> values;
        
        // 收集所有值
        for (i32 i = 1; i <= length; ++i) {
            Value key = Value(i);
            Value val = table->get(key);
            if (!val.isNil()) {
                values.push_back(val);
            }
        }
        
        // 使用自定义比较函数排序
        std::sort(values.begin(), values.end(), [&](const Value& a, const Value& b) {
            state->push(comparator);
            state->push(a);
            state->push(b);
            Vec<Value> args = {a, b};
            Value result = state->call(comparator, args);
            return result.isTruthy();
        });
        
        // 写回表
        for (size_t i = 0; i < values.size(); ++i) {
            Value key = Value(static_cast<i32>(i + 1));
            table->set(key, values[i]);
        }
        
        // 清除多余的元素
        for (i32 i = static_cast<i32>(values.size()) + 1; i <= length; ++i) {
            Value key = Value(i);
            table->set(key, Value());
        }
    }
    
    Table* TableLib::validateTableArg(State* state, i32 argIndex, StrView funcName) {
        Value arg = state->get(argIndex);
        if (!arg.isTable()) {
            Str msg = Str(funcName) + ": argument " + std::to_string(argIndex) + " must be a table";
            throw LibException(LibErrorCode::InvalidArgument, msg);
        }
        return arg.asTable().get();
    }
    
    void registerTableLib(State* state) {
        auto tableLib = std::make_unique<TableLib>();
        // 注册table库函数到全局环境
        FunctionRegistry registry;
        tableLib->registerFunctions(registry);
        
        // 将注册的函数添加到state的全局环境中
        // 这里需要根据实际的State API进行调整
        // 暂时简化实现
    }
}