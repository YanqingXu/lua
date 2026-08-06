# Runtime Debugger Core

This directory is the protocol-independent part of the YanLua debugger. It owns VM safepoints, session and step
state, source/breakpoint binding, paused-state handles, stack/variable inspection, and the restricted read-only
evaluator.

Editor-facing code is intentionally kept in the sibling `Debugger` repository:

- `Debugger/src/debug_adapter`: Debug Adapter Protocol transport and session mapping;
- `Debugger/editors/vscode`: VS Code extension;
- `Debugger/tests`: adapter protocol and end-to-end tests;
- `Debugger/docs`: debugger architecture and user documentation.

Runtime code must not include DAP JSON types, editor APIs, or transport concerns. The adapter may depend on
`lua_core` and communicates with this directory only through `IDebugRuntime`, stable IDs, value snapshots, and
debug events.
