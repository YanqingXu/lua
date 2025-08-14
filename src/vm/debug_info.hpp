#pragma once

#include "../common/types.hpp"
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iostream>

namespace Lua {

    /**
     * @brief Debug source location information
     */
    struct DebugSourceLocation {
        std::string filename;
        i32 line;
        i32 column;
        std::string functionName;

        DebugSourceLocation() : line(-1), column(-1) {}
        DebugSourceLocation(const std::string& file, i32 ln, i32 col = -1, const std::string& func = "")
            : filename(file), line(ln), column(col), functionName(func) {}

        bool isValid() const { return !filename.empty() && line >= 0; }

        std::string toString() const {
            std::ostringstream oss;
            oss << filename;
            if (line >= 0) {
                oss << ":" << line;
                if (column >= 0) {
                    oss << ":" << column;
                }
            }
            if (!functionName.empty()) {
                oss << " in '" << functionName << "'";
            }
            return oss.str();
        }
    };

    /**
     * @brief Debug information manager
     * 
     * Manages mapping between bytecode instructions and source locations
     */
    class DebugInfoManager {
    private:
        // Map from instruction address to source location
        std::unordered_map<usize, DebugSourceLocation> instructionMap_;
        
        // Map from function address to function name
        std::unordered_map<usize, std::string> functionNames_;
        
        // Current source file being processed
        std::string currentFile_;
        
        // Current function being processed
        std::string currentFunction_;

    public:
        DebugInfoManager() = default;
        ~DebugInfoManager() = default;

        // Source file management
        void setCurrentFile(const std::string& filename) { currentFile_ = filename; }
        const std::string& getCurrentFile() const { return currentFile_; }

        // Function management
        void setCurrentFunction(const std::string& functionName) { currentFunction_ = functionName; }
        const std::string& getCurrentFunction() const { return currentFunction_; }

        // Instruction mapping
        void mapInstruction(usize instructionAddr, i32 line, i32 column = -1);
        void mapInstruction(usize instructionAddr, const DebugSourceLocation& location);

        // Function mapping
        void mapFunction(usize functionAddr, const std::string& functionName);

        // Lookup operations
        DebugSourceLocation getSourceLocation(usize instructionAddr) const;
        std::string getFunctionName(usize functionAddr) const;

        // Utility methods
        bool hasDebugInfo(usize instructionAddr) const;
        void clear();
        
        // Statistics
        size_t getInstructionMappingCount() const { return instructionMap_.size(); }
        size_t getFunctionMappingCount() const { return functionNames_.size(); }

        // Debug output
        void dumpDebugInfo() const;
    };

    /**
     * @brief Call stack frame information for debugging
     */
    struct DebugFrame {
        DebugSourceLocation location;
        std::string functionName;
        usize instructionAddr;
        std::unordered_map<std::string, std::string> localVariables;

        DebugFrame() : instructionAddr(0) {}
        DebugFrame(const DebugSourceLocation& loc, usize addr)
            : location(loc), instructionAddr(addr) {}

        std::string toString() const {
            std::ostringstream oss;
            oss << location.toString();
            if (!localVariables.empty()) {
                oss << " [locals: ";
                bool first = true;
                for (const auto& pair : localVariables) {
                    if (!first) oss << ", ";
                    oss << pair.first << "=" << pair.second;
                    first = false;
                }
                oss << "]";
            }
            return oss.str();
        }
    };

    /**
     * @brief Enhanced call stack for debugging
     */
    class DebugCallStack {
    private:
        std::vector<DebugFrame> frames_;
        DebugInfoManager* debugInfo_;

    public:
        explicit DebugCallStack(DebugInfoManager* debugInfo = nullptr) 
            : debugInfo_(debugInfo) {}

        // Frame management
        void pushFrame(const DebugSourceLocation& location, usize instructionAddr);
        void pushFrame(const DebugFrame& frame);
        void popFrame();
        
        // Access
        const DebugFrame* getCurrentFrame() const;
        const DebugFrame* getFrame(size_t index) const;
        size_t getDepth() const { return frames_.size(); }
        bool isEmpty() const { return frames_.empty(); }

        // Local variable tracking
        void setLocalVariable(const std::string& name, const std::string& value);
        std::string getLocalVariable(const std::string& name) const;

        // Stack trace generation
        std::vector<std::string> generateStackTrace() const;
        std::string getFormattedStackTrace() const;

        // Utility
        void clear() { frames_.clear(); }
        void setDebugInfoManager(DebugInfoManager* debugInfo) { debugInfo_ = debugInfo; }
    };

} // namespace Lua
