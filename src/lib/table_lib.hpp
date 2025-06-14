#ifndef TABLE_LIB_HPP
#define TABLE_LIB_HPP

#include "lib_module.hpp"
#include "error_handling.hpp"
#include "type_conversion.hpp"
#include "../common/types.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include "../vm/table.hpp"
#include <algorithm>
#include <functional>

namespace Lua {
    
    /**
     * Table库实现
     * 提供Lua标准table操作函数
     */
    class TableLib : public LibModule {
    public:
        /**
         * 获取模块名称
         */
        StrView getName() const noexcept override;
        
        /**
         * 注册函数到注册表
         */
        void registerFunctions(FunctionRegistry& registry) override;
        
        /**
         * 可选的初始化逻辑
         */
        void initialize(State* state) override;
        
    public:
        // Table库函数实现
        
        /**
         * table.concat(list [, sep [, start [, end]]])
         * 连接表中的字符串元素
         */
        static Value concat(State* state, i32 nargs);
        
        /**
         * table.insert(list, [pos,] value)
         * 在表中插入元素
         */
        static Value insert(State* state, i32 nargs);
        
        /**
         * table.remove(list [, pos])
         * 从表中移除元素
         */
        static Value remove(State* state, i32 nargs);
        
        /**
         * table.sort(list [, comp])
         * 对表进行排序
         */
        static Value sort(State* state, i32 nargs);
        
        /**
         * table.pack(...)
         * 将参数打包成表
         */
        static Value pack(State* state, i32 nargs);
        
        /**
         * table.unpack(list [, start [, end]])
         * 解包表中的元素
         */
        static Value unpack(State* state, i32 nargs);
        
        /**
         * table.move(a1, f, e, t [, a2])
         * 移动表中的元素
         */
        static Value move(State* state, i32 nargs);
        
        /**
         * table.getn(table)
         * 获取表的长度（兼容性函数）
         */
        static Value getn(State* state, i32 nargs);
        
        /**
         * table.setn(table, n)
         * 设置表的长度（兼容性函数）
         */
        static Value setn(State* state, i32 nargs);
        
        /**
         * table.maxn(table)
         * 获取表中最大的数字索引
         */
        static Value maxn(State* state, i32 nargs);
        
        /**
         * table.foreach(table, func)
         * 遍历表中的所有元素（兼容性函数）
         */
        static Value foreach(State* state, i32 nargs);
        
        /**
         * table.foreachi(table, func)
         * 遍历表中的数组部分（兼容性函数）
         */
        static Value foreachi(State* state, i32 nargs);
        
    private:
        // 辅助函数
        
        /**
         * 获取表的有效长度
         */
        static i32 getTableLength(Table* table);
        
        /**
         * 检查索引是否有效
         */
        static bool isValidIndex(i32 index, i32 length);
        
        /**
         * 将表转换为字符串数组（用于concat）
         */
        static Vec<Str> tableToStringArray(Table* table, i32 start, i32 end);
        
        /**
         * 默认比较函数（用于sort）
         */
        static bool defaultCompare(const Value& a, const Value& b);
        
        /**
         * 使用自定义比较函数进行排序
         */
        static void sortWithComparator(State* state, Table* table, Value comparator);
        
        /**
         * 验证表参数
         */
        static Table* validateTableArg(State* state, i32 argIndex, StrView funcName);
    };
    
    /**
     * 注册table库到状态
     */
    void registerTableLib(State* state);
}

#endif // TABLE_LIB_HPP