#include "coroutine_lib.hpp"
#include "../../vm/function.hpp"
#include "../../vm/userdata.hpp"
#include "../../vm/table.hpp"
#include <iostream>

namespace Lua {
    namespace StandardLibrary {

        void CoroutineLib::initialize(State* state) {
            if (!state) return;

            try {
                // Create coroutine table
                auto coroTable = GCRef<Table>(new Table());

                // Add coroutine functions (multi-return C API)
                LibRegistry::registerTableFunction(state, Value(coroTable), "create", &CoroutineLib::luaCreate);
                LibRegistry::registerTableFunction(state, Value(coroTable), "resume", &CoroutineLib::luaResume);
                LibRegistry::registerTableFunction(state, Value(coroTable), "yield", &CoroutineLib::luaYield);
                LibRegistry::registerTableFunction(state, Value(coroTable), "status", &CoroutineLib::luaStatus);
                LibRegistry::registerTableFunction(state, Value(coroTable), "running", &CoroutineLib::luaRunning);

                // Set global coroutine table
                state->setGlobal("coroutine", Value(coroTable));

            } catch (const std::exception& e) {
                std::cerr << "Error initializing coroutine library: " << e.what() << std::endl;
            }
        }

        i32 CoroutineLib::luaCreate(State* state) {
            // Expect 1 argument: function
            if (!state) return 0;

            if (state->getTop() < 1) {
                // No arguments -> error message
                state->push(Value("coroutine.create: missing function argument"));
                return 1; // return error message
            }

            Value funcValue = state->get(0);
            if (!funcValue.isFunction()) {
                state->push(Value("coroutine.create: argument must be a function"));
                return 1;
            }

            GCRef<Function> func = funcValue.asFunction();

            // Create C++20 coroutine (basic infra; function binding TBD)
            LuaCoroutine* coro = state->createCoroutine(func);
            if (!coro) {
                state->push(Value("coroutine.create: failed to create coroutine"));
                return 1;
            }

            // Wrap in userdata and return it
            Value ud = createCoroutineUserdata(coro);
            state->push(ud);
            return 1;
        }

        i32 CoroutineLib::luaResume(State* state) {
            if (!state) return 0;
            if (state->getTop() < 1) {
                state->push(Value(false));
                state->push(Value("coroutine.resume: missing coroutine argument"));
                return 2;
            }

            Value coroValue = state->get(0);
            LuaCoroutine* coro = extractCoroutineFromUserdata(coroValue);
            if (!coro) {
                state->push(Value(false));
                state->push(Value("coroutine.resume: invalid coroutine"));
                return 2;
            }

            // Collect args after the coroutine parameter
            Vec<Value> args;
            for (int i = 1; i < state->getTop(); i++) {
                args.push_back(state->get(i));
            }

            // Resume coroutine
            CoroutineResult result = state->resumeCoroutine(coro, args);

            if (result.success) {
                state->clearStack();
                state->push(Value(true));
                for (const auto& v : result.values) {
                    state->push(v);
                }
                return static_cast<i32>(1 + result.values.size());
            } else {
                state->clearStack();
                state->push(Value(false));
                state->push(Value(result.errorMessage.empty() ? Str("coroutine error") : result.errorMessage));
                return 2;
            }
        }

        i32 CoroutineLib::luaYield(State* state) {
            if (!state) return 0;

            // Gather all current stack values as yield values
            Vec<Value> values;
            for (int i = 0; i < state->getTop(); i++) {
                values.push_back(state->get(i));
            }

            CoroutineResult result = state->yieldFromCoroutine(values);
            if (!result.success) {
                // Not in coroutine
                state->clearStack();
                state->push(Value("coroutine.yield: not in a coroutine"));
                return 1;
            }

            // On yield, no values are returned to the yielding coroutine itself here
            state->clearStack();
            return 0;
        }

        i32 CoroutineLib::luaStatus(State* state) {
            if (!state) return 0;
            if (state->getTop() < 1) {
                state->push(Value("dead"));
                return 1;
            }

            Value coroValue = state->get(0);
            LuaCoroutine* coro = extractCoroutineFromUserdata(coroValue);
            if (!coro) {
                state->push(Value("dead"));
                return 1;
            }

            CoroutineStatus status = state->getCoroutineStatus(coro);
            state->push(Value(statusToString(status)));
            return 1;
        }

        i32 CoroutineLib::luaRunning(State* state) {
            // Not fully implemented yet; return nil per infra
            (void)state;
            return 0;
        }

        Str CoroutineLib::statusToString(CoroutineStatus status) {
            switch (status) {
                case CoroutineStatus::SUSPENDED: return "suspended";
                case CoroutineStatus::RUNNING: return "running";
                case CoroutineStatus::NORMAL: return "normal";
                case CoroutineStatus::DEAD: return "dead";
                default: return "unknown";
            }
        }

        Value CoroutineLib::createCoroutineUserdata(LuaCoroutine* coro) {
            try {
                // Create full userdata block to hold CoroutineUserdata
                auto userdata = Userdata::createFull(sizeof(CoroutineUserdata));

                // Initialize userdata with coroutine pointer
                auto* coroData = static_cast<CoroutineUserdata*>(userdata->getData());
                new (coroData) CoroutineUserdata(coro);

                return Value(userdata);
            } catch (const std::exception& e) {
                std::cerr << "Error creating coroutine userdata: " << e.what() << std::endl;
                return Value(); // nil
            }
        }

        LuaCoroutine* CoroutineLib::extractCoroutineFromUserdata(const Value& value) {
            try {
                if (!value.isUserdata()) {
                    return nullptr;
                }

                GCRef<Userdata> userdata = value.asUserdata();
                if (!userdata || userdata->getUserDataSize() < sizeof(CoroutineUserdata)) {
                    return nullptr;
                }

                auto* coroData = static_cast<CoroutineUserdata*>(userdata->getData());
                return coroData ? coroData->coroutine : nullptr;
            } catch (const std::exception& e) {
                std::cerr << "Error extracting coroutine from userdata: " << e.what() << std::endl;
                return nullptr;
            }
        }
    }
}
