#include "compiler/codegen/codegen.hpp"
#include "compiler/parser/parser.hpp"
#include "core/function.hpp"
#include "debugger/debug_runtime.hpp"
#include "runtime/runtime_services.hpp"
#include "vm/state/lua_state.hpp"
#include "vm/vm.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>

using namespace Lua;
using namespace Lua::Debugger;

namespace {

Proto* compileFixture(RuntimeServices& services) {
    constexpr StrView source = "local shared = {name = 'Yan', [1] = 42}\n"
                               "local marker = 'ready'\n"
                               "return shared, marker\n";
    Parser parser{Str(source), services};
    auto parsed = parser.parse();
    if (!parsed) {
        throw parsed.error();
    }
    CodeGenerator codegen(services);
    return codegen.generate(*parsed, "@fuzz/debugger_expression.lua");
}

class ExpressionFixture {
public:
    ExpressionFixture() : worker_([this]() { run(); }) {
        {
            std::unique_lock lock(mutex_);
            if (!condition_.wait_for(lock, std::chrono::seconds(5), [this]() { return ready_ || failure_; })) {
                lock.unlock();
                stop();
                throw std::runtime_error("expression fuzzer owner thread did not initialize");
            }
            if (failure_) {
                const std::exception_ptr failure = failure_;
                lock.unlock();
                stop();
                std::rethrow_exception(failure);
            }
        }

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (std::chrono::steady_clock::now() < deadline) {
            DebugController* controller = nullptr;
            bool executionDone = false;
            std::exception_ptr failure;
            {
                std::lock_guard lock(mutex_);
                controller = controller_;
                executionDone = executionDone_;
                failure = failure_;
            }
            if (failure) {
                stop();
                std::rethrow_exception(failure);
            }
            if (executionDone || controller == nullptr) {
                break;
            }
            if (controller->snapshot().state == DebugSessionState::Suspended) {
                auto frames = controller->stackTrace(ThreadId{1}, 0, 1);
                if (frames && !frames->empty()) {
                    frame_ = frames->front().id;
                    return;
                }
                break;
            }
            std::this_thread::yield();
        }
        stop();
        throw std::runtime_error("expression fuzzer fixture did not reach a paused Lua frame");
    }

    ~ExpressionFixture() {
        stop();
    }

    ExpressionFixture(const ExpressionFixture&) = delete;
    ExpressionFixture& operator=(const ExpressionFixture&) = delete;

    void evaluate(const std::uint8_t* data, std::size_t size) {
        DebugController* controller = nullptr;
        {
            std::lock_guard lock(mutex_);
            controller = controller_;
        }
        if (controller == nullptr) {
            throw std::runtime_error("expression fuzzer lost its debugger controller");
        }
        const StrView expression(reinterpret_cast<const char*>(data), size);
        (void)controller->evaluate(frame_, expression);
    }

private:
    void run() noexcept {
        EngineContext context;
        DebugController& controller = context.globalState().enableDebugger();
        RuntimeServices services = context.services();
        UPtr<LuaState> state;
        DebugSession session;
        try {
            state = LuaState::create(context);
            Proto* proto = compileFixture(services);
            Function* function = new Function(proto);
            function->setEnv(state->getGlobalTable());
            services.gc.registerObject(function);

            const SourceId source = controller.registerFilePath("fuzz/debugger_expression.lua");
            SourceBreakpoint breakpoint;
            breakpoint.line = 3;
            if (!controller.setBreakpoints(source, std::span<const SourceBreakpoint>(&breakpoint, 1))) {
                throw std::runtime_error("could not configure expression fuzzer breakpoint");
            }
            auto attached = controller.attachSession();
            if (!attached) {
                throw std::runtime_error("could not attach expression fuzzer session");
            }
            session = std::move(*attached);
            if (!controller.configurationDone()) {
                throw std::runtime_error("could not configure expression fuzzer session");
            }
            {
                std::lock_guard lock(mutex_);
                controller_ = &controller;
                ready_ = true;
            }
            condition_.notify_all();
            VM::execute(services, state.get(), function);
        } catch (...) {
            std::lock_guard lock(mutex_);
            if (!release_) {
                failure_ = std::current_exception();
            }
        }

        {
            std::unique_lock lock(mutex_);
            executionDone_ = true;
            condition_.notify_all();
            condition_.wait(lock, [this]() { return release_; });
        }
        session.disconnect(DisconnectAction::ContinueExecution);
        context.globalState().disableDebugger(DisconnectAction::ContinueExecution);
        state.reset();
        context.gc().clearAll(context.strings());
    }

    void stop() noexcept {
        if (!worker_.joinable()) {
            return;
        }
        DebugController* controller = nullptr;
        {
            std::lock_guard lock(mutex_);
            release_ = true;
            controller = controller_;
        }
        if (controller != nullptr) {
            const DebugSessionSnapshot snapshot = controller->snapshot();
            if (snapshot.state == DebugSessionState::Suspended) {
                (void)controller->continueExecution(ThreadId{1});
            } else if (snapshot.state != DebugSessionState::Terminated) {
                (void)controller->terminateExecution();
            }
        }
        condition_.notify_all();
        worker_.join();
        controller_ = nullptr;
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    DebugController* controller_ = nullptr;
    std::exception_ptr failure_;
    bool ready_ = false;
    bool executionDone_ = false;
    bool release_ = false;
    FrameId frame_;
    std::thread worker_;
};

class FixtureManager {
public:
    void evaluate(const std::uint8_t* data, std::size_t size) {
        if (!fixture_ || inputs_ >= 1000) {
            fixture_.reset();
            fixture_ = std::make_unique<ExpressionFixture>();
            inputs_ = 0;
        }
        fixture_->evaluate(data, size);
        ++inputs_;
    }

private:
    std::unique_ptr<ExpressionFixture> fixture_;
    std::size_t inputs_ = 0;
};

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    static FixtureManager manager;
    manager.evaluate(data, size);
    return 0;
}
