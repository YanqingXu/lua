#include "lib_utils.hpp"
#include "../vm/state.hpp"
#include "../vm/value.hpp"
#include "../vm/table.hpp"
#include <sstream>
#include <stdexcept>

namespace Lua {
namespace LibUtils {

    // ArgChecker functions are implemented as inline functions in the header file
    

    

    
    // Convert functions are implemented as inline functions in the header file
    
    // String functions are implemented as inline functions in the header file
    
    // TableUtils functions are implemented as inline functions in the header file
    
    // Error namespace implementation
    namespace Error {
        void throwError(State* state, const std::string& message, int level) {
            // This is a simplified implementation
            // In a real implementation, you'd use the state's error handling mechanism
            throw std::runtime_error(message);
        }
        
        void throwTypeError(State* state, int argIndex, const std::string& expectedType, const std::string& actualType) {
            std::ostringstream oss;
            oss << "bad argument #" << argIndex << " (" << expectedType << " expected, got " << actualType << ")";
            throwError(state, oss.str(), 1);
        }
        
        void throwArgError(State* state, int argIndex, const std::string& message) {
            std::ostringstream oss;
            oss << "bad argument #" << argIndex << " (" << message << ")";
            throwError(state, oss.str(), 1);
        }
        
        std::string formatError(const std::string& functionName, const std::string& message) {
            return functionName + ": " + message;
        }
    }

} // namespace LibUtils
} // namespace Lua