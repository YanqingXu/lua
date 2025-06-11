#pragma once

#include "../common/types.hpp"
#include "lib_common.hpp"
#include "lib_utils.hpp"
#include "../vm/value.hpp"

namespace Lua {

    // Forward declarations
    class State;
    class Function;

    /**
     * @brief Lua字符串库模块
     * 
     * 提供字符串操作相关的函数，包括：
     * - string.len: 获取字符串长度
     * - string.sub: 字符串截取
     * - string.upper: 转换为大写
     * - string.lower: 转换为小写
     * - string.find: 查找子字符串
     * - string.gsub: 全局替换
     * - string.match: 模式匹配
     * - string.gmatch: 全局模式匹配
     * - string.rep: 字符串重复
     * - string.reverse: 字符串反转
     * - string.format: 格式化字符串
     * - string.char: 字符编码转字符
     * - string.byte: 字符转编码
     */
    class StringLib : public LibModule {
    public:
        // LibModule interface implementation
        const Str& getName() const override { 
            static const Str name = "string";
            return name;
        }
        const Str& getVersion() const override { 
            static const Str version = "1.0.0";
            return version;
        }
        void registerModule(State* state) override;
        
        // String library functions
        static Value len(State* state, int nargs);
        static Value sub(State* state, int nargs);
        static Value upper(State* state, int nargs);
        static Value lower(State* state, int nargs);
        static Value find(State* state, int nargs);
        static Value gsub(State* state, int nargs);
        static Value match(State* state, int nargs);
        static Value gmatch(State* state, int nargs);
        static Value rep(State* state, int nargs);
        static Value reverse(State* state, int nargs);
        static Value format(State* state, int nargs);
        static Value char_func(State* state, int nargs);  // 'char' is a keyword
        static Value byte_func(State* state, int nargs);  // 'byte' might conflict
        
    private:
        // Helper functions for pattern matching
        static bool matchPattern(const Str& str, const Str& pattern, size_t& start, size_t& end);
        static Str replacePattern(const Str& str, const Str& pattern, const Str& replacement);
        static Vec<Str> findAllMatches(const Str& str, const Str& pattern);
        
        // Helper functions for string formatting
        static Str formatString(const Str& format, const Vec<Value>& args);
        static Str escapePattern(const Str& pattern);
    };
    
    // Legacy registration function for backward compatibility
    void registerStringLib(State* state);

} // namespace Lua