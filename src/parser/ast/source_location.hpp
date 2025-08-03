#pragma once

#include "../../common/types.hpp"
#include <string>
#include <ostream>

// Forward declaration
namespace Lua {
    struct Token;
}

namespace Lua {
    /**
     * @brief Source code location class
     * 
     * Stores position information in source code for AST nodes,
     * including filename, line number, and column number.
     * This information is crucial for error reporting, debugging, and IDE integration.
     */
    class SourceLocation {
    private:
        Str filename_;      // Filename
        int line_;          // Line number (starting from 1)
        int column_;        // Column number (starting from 1)
        
    public:
        /**
         * @brief Default constructor
         * Creates an invalid location
         */
        SourceLocation() : filename_("<unknown>"), line_(0), column_(0) {}
        
        /**
         * @brief Constructor
         * @param filename File name
         * @param line Line number (starting from 1)
         * @param column Column number (starting from 1)
         */
        SourceLocation(const Str& filename, int line, int column)
            : filename_(filename), line_(line), column_(column) {}
        
        /**
         * @brief Create location from a Token
         * @param token Lexer token
         * @param filename File name
         * Note: This method requires including lexer.hpp
         */
        static SourceLocation fromToken(const struct Token& token, const Str& filename = "<input>");
        
        /**
         * @brief Convenience method to create location from line and column
         * @param line Line number
         * @param column Column number
         * @param filename File name
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
         * @brief Check if location is valid
         * @return True if both line and column are greater than 0
         */
        bool isValid() const {
            return line_ > 0 && column_ > 0;
        }
        
        /**
         * @brief Create a location range
         * @param start Start location
         * @param end End location
         * @return A location representing the range (uses the start location)
         */
        static SourceLocation makeRange(const SourceLocation& start, const SourceLocation& end) {
            // For ranges, we use the start location
            // Can be extended in future to include end location
            return start;
        }
        
        /**
         * @brief Format location as string
         * @return Formatted string like "file.lua:10:5"
         */
        Str toString() const {
            if (!isValid()) {
                return filename_ + ":?:?";
            }
            return filename_ + ":" + std::to_string(line_) + ":" + std::to_string(column_);
        }

        /**
         * @brief Format location in Lua 5.1 style
         * @return Formatted string like "filename:line:" (no column, colon at end)
         */
        Str toLua51String() const {
            Str displayName = filename_;

            // Convert special filenames to Lua 5.1 standard
            if (displayName.empty() || displayName == "<input>" || displayName == "<unknown>") {
                displayName = "stdin";
            }

            if (!isValid()) {
                return displayName + ":?:";
            }

            return displayName + ":" + std::to_string(line_) + ":";
        }
        
        /**
         * @brief Comparison operator
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
         * @brief Less-than operator for sorting
         * Compares by filename, then line, then column
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
         * @brief Less-than-or-equal operator
         */
        bool operator<=(const SourceLocation& other) const {
            return *this < other || *this == other;
        }
        
        /**
         * @brief Greater-than operator
         */
        bool operator>(const SourceLocation& other) const {
            return other < *this;
        }
        
        /**
         * @brief Greater-than-or-equal operator
         */
        bool operator>=(const SourceLocation& other) const {
            return !(*this < other);
        }
    };
    
    /**
     * @brief Output stream operator
     * Allows printing SourceLocation to output stream
     */
    inline std::ostream& operator<<(std::ostream& os, const SourceLocation& loc) {
        return os << loc.toString();
    }
    
    /**
     * @brief Source code range class
     * 
     * Represents a range in source code with start and end locations.
     * Used to represent syntax elements spanning multiple lines or columns.
     */
    class SourceRange {
    private:
        SourceLocation start_;  // Start location
        SourceLocation end_;    // End location
        
    public:
        /**
         * @brief Default constructor
         */
        SourceRange() = default;
        
        /**
         * @brief Constructor
         * @param start Start location
         * @param end End location
         */
        SourceRange(const SourceLocation& start, const SourceLocation& end)
            : start_(start), end_(end) {}
        
        /**
         * @brief Create range from a single location
         * @param location Location information
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
         * @brief Check if range is valid
         */
        bool isValid() const {
            return start_.isValid() && end_.isValid();
        }
        
        /**
         * @brief Check if location is within the range
         * @param location Location to check
         * @return True if location is within the range
         */
        bool contains(const SourceLocation& location) const {
            return start_ <= location && location <= end_;
        }
        
        /**
         * @brief Format range as string
         */
        Str toString() const {
            if (start_ == end_) {
                return start_.toString();
            }
            return start_.toString() + "-" + std::to_string(end_.getLine()) + ":" + std::to_string(end_.getColumn());
        }
    };
    
    /**
     * @brief Output stream operator
     */
    inline std::ostream& operator<<(std::ostream& os, const SourceRange& range) {
        return os << range.toString();
    }
}
