#pragma once

#include "../../common/types.hpp"
#include "../../vm/lua_state.hpp"
#include "../../vm/value.hpp"
#include "../../vm/lua_coroutine.hpp"
#include "../core/lib_registry.hpp"

namespace Lua {
    namespace StandardLibrary {

        /**
         * @brief Lua coroutine library implementation using C++20 coroutines
         *
         * This library provides Lua 5.1 compatible coroutine functions:
         * - coroutine.create(f) - Create a new coroutine
         * - coroutine.resume(co, ...) - Resume a coroutine
         * - coroutine.yield(...) - Yield from current coroutine
         * - coroutine.status(co) - Get coroutine status
         * - coroutine.running() - Get current running coroutine
         */
        class CoroutineLib {
        public:
            /**
             * @brief Initialize coroutine library in the given state
             * @param state Lua state to initialize library in
             */
            static void initialize(LuaState* state);

        private:
            // Lua coroutine API functions (Lua 5.1 multi-return style)
            static i32 luaCreate(LuaState* state);
            static i32 luaResume(LuaState* state);
            static i32 luaYield(LuaState* state);
            static i32 luaStatus(LuaState* state);
            static i32 luaRunning(LuaState* state);

            // Helper functions
            static Str statusToString(CoroutineStatus status);
            static Value createCoroutineUserdata(LuaCoroutine* coro);
            static LuaCoroutine* extractCoroutineFromUserdata(const Value& value);
        };

        /**
         * @brief Coroutine userdata type for wrapping C++20 coroutines
         */
        struct CoroutineUserdata {
            LuaCoroutine* coroutine;

            explicit CoroutineUserdata(LuaCoroutine* coro) : coroutine(coro) {}
        };
    }
}
