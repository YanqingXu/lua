#pragma once

#include "lib_common.hpp"
#include "lib_utils.hpp"
#include "../common/types.hpp"
#include "../vm/state.hpp"

namespace Lua {
    
    // Base library module class
    class BaseLib : public LibModule {
    public:
        // LibModule interface implementation
        const std::string& getName() const override {
            static const std::string name = "base";
            return name;
        }
        
        void registerModule(State* state) override;
        
        const std::string& getVersion() const override {
            static const std::string version = "1.0.0";
            return version;
        }
        
    private:
        // Base library functions
        static Value print(State* state, int nargs);
        static Value tonumber(State* state, int nargs);
        static Value tostring(State* state, int nargs);
        static Value type(State* state, int nargs);
        static Value ipairs(State* state, int nargs);
        static Value pairs(State* state, int nargs);
        static Value next(State* state, int nargs);
        static Value getmetatable(State* state, int nargs);
        static Value setmetatable(State* state, int nargs);
        static Value rawget(State* state, int nargs);
        static Value rawset(State* state, int nargs);
        static Value rawlen(State* state, int nargs);
        static Value rawequal(State* state, int nargs);
        static Value pcall(State* state, int nargs);
        static Value xpcall(State* state, int nargs);
        static Value error(State* state, int nargs);
        static Value lua_assert(State* state, int nargs);
        static Value select(State* state, int nargs);
        static Value unpack(State* state, int nargs);
    };
    
    // Legacy function for backward compatibility
    void registerBaseLib(State* state);
}
