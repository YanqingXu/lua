#pragma once

#include "../common/types.hpp"
#include "lib_common.hpp"
#include "lib_utils.hpp"
#include "../vm/value.hpp"
#include "../vm/table.hpp"
#include <algorithm>
#include <functional>

namespace Lua {

    // Forward declarations
    class State;
    class Function;

    /**
     * @brief Lua表库模块
     * 
     * 提供表操作相关的函数，包括：
     * - table.insert: 向表中插入元素
     * - table.remove: 从表中移除元素
     * - table.concat: 连接表中的元素为字符串
     * - table.sort: 对表进行排序
     * - table.pack: 将参数打包成表
     * - table.unpack: 将表解包为参数
     * - table.move: 移动表中的元素
     * - table.maxn: 获取表中最大数字索引
     */
    class TableLib : public LibModule {
    public:
        // LibModule interface implementation
        const Str& getName() const override { 
            static const Str name = "table";
            return name;
        }
        const Str& getVersion() const override { 
            static const Str version = "1.0.0";
            return version;
        }
        void registerModule(State* state) override;
        
        // Table library functions
        static Value insert(State* state, int nargs);
        static Value remove(State* state, int nargs);
        static Value concat(State* state, int nargs);
        static Value sort(State* state, int nargs);
        static Value pack(State* state, int nargs);
        static Value unpack(State* state, int nargs);
        static Value move(State* state, int nargs);
        static Value maxn(State* state, int nargs);
        
    private:
        // Helper functions
        static int getTableLength(const Table& table);
        static void quickSort(Table& table, int left, int right, const std::function<bool(const Value&, const Value&)>& compare);
        static int partition(Table& table, int left, int right, const std::function<bool(const Value&, const Value&)>& compare);
        static bool defaultCompare(const Value& a, const Value& b);
        
        // Validation helpers
        static bool isValidArrayIndex(const Value& index);
        static int toArrayIndex(const Value& index);
    };

} // namespace Lua
