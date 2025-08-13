#include "debug_info.hpp"
#include <iostream>
#include <sstream>

namespace Lua {

    // DebugInfoManager implementation
    void DebugInfoManager::mapInstruction(usize instructionAddr, i32 line, i32 column) {
        DebugSourceLocation location(currentFile_, line, column, currentFunction_);
        instructionMap_[instructionAddr] = location;
    }

    void DebugInfoManager::mapInstruction(usize instructionAddr, const DebugSourceLocation& location) {
        instructionMap_[instructionAddr] = location;
    }

    void DebugInfoManager::mapFunction(usize functionAddr, const std::string& functionName) {
        functionNames_[functionAddr] = functionName;
    }

    DebugSourceLocation DebugInfoManager::getSourceLocation(usize instructionAddr) const {
        auto it = instructionMap_.find(instructionAddr);
        if (it != instructionMap_.end()) {
            return it->second;
        }
        return DebugSourceLocation(); // Invalid location
    }

    std::string DebugInfoManager::getFunctionName(usize functionAddr) const {
        auto it = functionNames_.find(functionAddr);
        if (it != functionNames_.end()) {
            return it->second;
        }
        return ""; // Unknown function
    }

    bool DebugInfoManager::hasDebugInfo(usize instructionAddr) const {
        return instructionMap_.find(instructionAddr) != instructionMap_.end();
    }

    void DebugInfoManager::clear() {
        instructionMap_.clear();
        functionNames_.clear();
        currentFile_.clear();
        currentFunction_.clear();
    }

    void DebugInfoManager::dumpDebugInfo() const {
        std::cout << "=== Debug Info Dump ===" << std::endl;
        std::cout << "Instruction mappings: " << instructionMap_.size() << std::endl;
        
        for (const auto& pair : instructionMap_) {
            std::cout << "  0x" << std::hex << pair.first << std::dec 
                      << " -> " << pair.second.toString() << std::endl;
        }
        
        std::cout << "Function mappings: " << functionNames_.size() << std::endl;
        for (const auto& pair : functionNames_) {
            std::cout << "  0x" << std::hex << pair.first << std::dec 
                      << " -> " << pair.second << std::endl;
        }
        std::cout << "======================" << std::endl;
    }

    // DebugCallStack implementation
    void DebugCallStack::pushFrame(const DebugSourceLocation& location, usize instructionAddr) {
        DebugFrame frame(location, instructionAddr);
        frames_.push_back(frame);
    }

    void DebugCallStack::pushFrame(const DebugFrame& frame) {
        frames_.push_back(frame);
    }

    void DebugCallStack::popFrame() {
        if (!frames_.empty()) {
            frames_.pop_back();
        }
    }

    const DebugFrame* DebugCallStack::getCurrentFrame() const {
        if (frames_.empty()) {
            return nullptr;
        }
        return &frames_.back();
    }

    const DebugFrame* DebugCallStack::getFrame(size_t index) const {
        if (index >= frames_.size()) {
            return nullptr;
        }
        return &frames_[frames_.size() - 1 - index]; // Reverse order (top to bottom)
    }

    void DebugCallStack::setLocalVariable(const std::string& name, const std::string& value) {
        if (!frames_.empty()) {
            frames_.back().localVariables[name] = value;
        }
    }

    std::string DebugCallStack::getLocalVariable(const std::string& name) const {
        if (!frames_.empty()) {
            const auto& locals = frames_.back().localVariables;
            auto it = locals.find(name);
            if (it != locals.end()) {
                return it->second;
            }
        }
        return "";
    }

    std::vector<std::string> DebugCallStack::generateStackTrace() const {
        std::vector<std::string> trace;
        
        for (size_t i = 0; i < frames_.size(); ++i) {
            const DebugFrame& frame = frames_[frames_.size() - 1 - i]; // Reverse order
            trace.push_back(frame.toString());
        }
        
        return trace;
    }

    std::string DebugCallStack::getFormattedStackTrace() const {
        std::ostringstream oss;
        oss << "Stack trace:\n";
        
        std::vector<std::string> trace = generateStackTrace();
        for (size_t i = 0; i < trace.size(); ++i) {
            oss << "  " << i << ": " << trace[i] << "\n";
        }
        
        return oss.str();
    }

} // namespace Lua
