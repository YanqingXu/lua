# Runtime Debugger Core

This directory is the protocol-independent part of the YanLua debugger. It owns VM safepoints, session and step
state, source/breakpoint binding, paused-state handles, stack/variable inspection, and the restricted read-only
evaluator.

Editor-facing code is intentionally kept in the separate
[`YanqingXu/LuaDebugger`](https://github.com/YanqingXu/LuaDebugger) repository:

- `LuaDebugger/src/debug_adapter`: Debug Adapter Protocol transport and session mapping;
- `LuaDebugger/editors/vscode`: VS Code extension;
- `LuaDebugger/tests`: adapter protocol and end-to-end tests;
- `LuaDebugger/docs`: debugger architecture and user documentation.

Runtime code must not include DAP JSON types, editor APIs, or transport concerns. The adapter may depend on
`lua_core` and communicates with this directory only through `IDebugRuntime`, stable IDs, value snapshots, and
debug events.

The debugger is a post-0.1 feature line. CMake development builds enable it by default; setting
`LUA_CPP_BUILD_DEBUGGER=OFF` removes its implementation, tests, fuzzers, and benchmark. Official 0.1.x Runtime
Preview packages use that disabled configuration, so these internal C++ interfaces are not part of the 0.1 SDK or
ABI commitment.
