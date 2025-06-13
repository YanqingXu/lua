#pragma once

#include "../../common/types.hpp"
#include <string>
#include <ostream>

// 前向声明
namespace Lua {
    struct Token;
}

namespace Lua {
    /**
     * @brief 源码位置信息类
     * 
     * 用于存储AST节点在源代码中的位置信息，包括文件名、行号、列号。
     * 这些信息对于错误报告、调试信息生成和IDE集成至关重要。
     */
    class SourceLocation {
    private:
        Str filename_;      // 文件名
        int line_;          // 行号 (从1开始)
        int column_;        // 列号 (从1开始)
        
    public:
        /**
         * @brief 默认构造函数
         * 创建一个无效的位置信息
         */
        SourceLocation() : filename_("<unknown>"), line_(0), column_(0) {}
        
        /**
         * @brief 构造函数
         * @param filename 文件名
         * @param line 行号 (从1开始)
         * @param column 列号 (从1开始)
         */
        SourceLocation(const Str& filename, int line, int column)
            : filename_(filename), line_(line), column_(column) {}
        
        /**
         * @brief 从Token创建位置信息
         * @param token 词法分析器的Token
         * @param filename 文件名
         * 注意：此方法需要在包含lexer.hpp后才能使用
         */
        static SourceLocation fromToken(const struct Token& token, const Str& filename = "<input>");
        
        /**
         * @brief 从行号和列号创建位置信息的便利方法
         * @param line 行号
         * @param column 列号
         * @param filename 文件名
         */
        static SourceLocation fromLineColumn(int line, int column, const Str& filename = "<input>") {
            return SourceLocation(filename, line, column);
        }
        
        // Getters
        const Str& getFilename() const { return filename_; }
        int getLine() const { return line_; }
        int getColumn() const { return column_; }
        
        // Setters
        void setFilename(const Str& filename) { filename_ = filename; }
        void setLine(int line) { line_ = line; }
        void setColumn(int column) { column_ = column; }
        
        /**
         * @brief 检查位置信息是否有效
         * @return 如果行号和列号都大于0则返回true
         */
        bool isValid() const {
            return line_ > 0 && column_ > 0;
        }
        
        /**
         * @brief 创建一个范围位置信息
         * @param start 开始位置
         * @param end 结束位置
         * @return 表示范围的位置信息（使用开始位置的坐标）
         */
        static SourceLocation makeRange(const SourceLocation& start, const SourceLocation& end) {
            // 对于范围，我们使用开始位置的坐标
            // 未来可以扩展为包含结束位置的信息
            return start;
        }
        
        /**
         * @brief 格式化位置信息为字符串
         * @return 格式化的位置字符串，如 "file.lua:10:5"
         */
        Str toString() const {
            if (!isValid()) {
                return filename_ + ":?:?";
            }
            return filename_ + ":" + std::to_string(line_) + ":" + std::to_string(column_);
        }
        
        /**
         * @brief 比较操作符
         */
        bool operator==(const SourceLocation& other) const {
            return filename_ == other.filename_ && 
                   line_ == other.line_ && 
                   column_ == other.column_;
        }
        
        bool operator!=(const SourceLocation& other) const {
            return !(*this == other);
        }
        
        /**
         * @brief 小于操作符，用于排序
         * 按文件名、行号、列号的顺序比较
         */
        bool operator<(const SourceLocation& other) const {
            if (filename_ != other.filename_) {
                return filename_ < other.filename_;
            }
            if (line_ != other.line_) {
                return line_ < other.line_;
            }
            return column_ < other.column_;
        }
        
        /**
         * @brief 小于等于操作符
         */
        bool operator<=(const SourceLocation& other) const {
            return *this < other || *this == other;
        }
        
        /**
         * @brief 大于操作符
         */
        bool operator>(const SourceLocation& other) const {
            return other < *this;
        }
        
        /**
         * @brief 大于等于操作符
         */
        bool operator>=(const SourceLocation& other) const {
            return !(*this < other);
        }
    };
    
    /**
     * @brief 输出流操作符
     * 允许直接将SourceLocation输出到流中
     */
    inline std::ostream& operator<<(std::ostream& os, const SourceLocation& loc) {
        return os << loc.toString();
    }
    
    /**
     * @brief 源码范围信息类
     * 
     * 表示源码中的一个范围，包含开始和结束位置。
     * 用于表示跨越多行或多列的语法结构。
     */
    class SourceRange {
    private:
        SourceLocation start_;  // 开始位置
        SourceLocation end_;    // 结束位置
        
    public:
        /**
         * @brief 默认构造函数
         */
        SourceRange() = default;
        
        /**
         * @brief 构造函数
         * @param start 开始位置
         * @param end 结束位置
         */
        SourceRange(const SourceLocation& start, const SourceLocation& end)
            : start_(start), end_(end) {}
        
        /**
         * @brief 从单个位置创建范围
         * @param location 位置信息
         */
        explicit SourceRange(const SourceLocation& location)
            : start_(location), end_(location) {}
        
        // Getters
        const SourceLocation& getStart() const { return start_; }
        const SourceLocation& getEnd() const { return end_; }
        
        // Setters
        void setStart(const SourceLocation& start) { start_ = start; }
        void setEnd(const SourceLocation& end) { end_ = end; }
        
        /**
         * @brief 检查范围是否有效
         */
        bool isValid() const {
            return start_.isValid() && end_.isValid();
        }
        
        /**
         * @brief 检查位置是否在范围内
         * @param location 要检查的位置
         * @return 如果位置在范围内返回true
         */
        bool contains(const SourceLocation& location) const {
            return start_ <= location && location <= end_;
        }
        
        /**
         * @brief 格式化范围信息为字符串
         */
        Str toString() const {
            if (start_ == end_) {
                return start_.toString();
            }
            return start_.toString() + "-" + std::to_string(end_.getLine()) + ":" + std::to_string(end_.getColumn());
        }
    };
    
    /**
     * @brief 输出流操作符
     */
    inline std::ostream& operator<<(std::ostream& os, const SourceRange& range) {
        return os << range.toString();
    }
}