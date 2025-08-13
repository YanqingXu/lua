#include "types.hpp"
#include <sstream>
#include <iomanip>

namespace Lua {

    std::string LuaException::getFormattedMessage() const {
        std::ostringstream oss;
        
        // Basic error message
        oss << message_;
        
        // Add location information if available
        if (!filename_.empty() && line_ >= 0) {
            oss << "\n  at " << filename_;
            if (line_ >= 0) {
                oss << ":" << line_;
                if (column_ >= 0) {
                    oss << ":" << column_;
                }
            }
        }
        
        // Add function name if available
        if (!functionName_.empty()) {
            oss << "\n  in function '" << functionName_ << "'";
        }
        
        // Add context information if available
        if (!contextInfo_.empty()) {
            oss << "\n  context: " << contextInfo_;
        }
        
        return oss.str();
    }

    std::string LuaException::getStackTrace() const {
        if (callStack_.empty()) {
            return "";
        }
        
        std::ostringstream oss;
        oss << "\nStack trace:\n";
        
        for (size_t i = 0; i < callStack_.size(); ++i) {
            oss << "  " << std::setw(2) << i << ": " << callStack_[i] << "\n";
        }
        
        return oss.str();
    }

    const char* LuaException::what() const noexcept {
        try {
            if (formattedMessage_.empty()) {
                formattedMessage_ = getFormattedMessage();
                
                // Add stack trace if available
                std::string stackTrace = getStackTrace();
                if (!stackTrace.empty()) {
                    formattedMessage_ += stackTrace;
                }
            }
            return formattedMessage_.c_str();
        } catch (...) {
            // Fallback to basic message if formatting fails
            return message_.c_str();
        }
    }

} // namespace Lua
