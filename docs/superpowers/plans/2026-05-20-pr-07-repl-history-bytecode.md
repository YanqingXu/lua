# PR-07: REPL History and Meta Commands

## Scope

Implement the first REPL ergonomics slice from roadmap stage 4.4:

- Persistent REPL history stored in `.lua_history`.
- `.help` meta command.
- `.bytecode <expr|chunk>` meta command.

Do not include tab completion, colors, `.ast`, or `.gc` in this PR.

## Design

Keep `REPL::run()` as the only interactive entry point and add a small testable support surface in
`src/repl.hpp`:

- `MetaCommandKind` and `MetaCommand` describe parsed dot commands.
- `parseMetaCommand()` classifies first-line REPL input without executing Lua code.
- `printHelp()` emits stable help text.
- `recordHistory()`, `loadHistory()`, and `saveHistory()` manage history persistence.
- `printBytecode()` compiles a REPL expression or chunk and delegates formatting to the existing
  bytecode printer.
- `runMetaCommand()` dispatches parsed meta commands.

The existing `GarbageCollector`, parser, VM, and public REPL entry points remain compatible.

## Build Updates

- Add `src/repl.cpp` and `tests/unit/app/test_repl_commands.cpp` to the `lua_test` target.
- Add `src/bytecode/bytecode_printer.cpp` to the `lua_app` Visual Studio project so `repl.cpp`
  can call the printer in the app build.
- Keep CMake and Visual Studio project files in sync.

## Tests

Add focused unit tests for:

- Meta command parsing.
- Help text contents.
- History record/save/load round trip.
- `.bytecode` success for an expression and usage failure for an empty argument.

Verification sequence:

1. Run the targeted REPL command test while it is red.
2. Implement the minimal REPL support functions and main-loop dispatch.
3. Build `lua_test` and `lua_app`.
4. Run the targeted REPL command tests.
5. Smoke-test `lua_app -i` with `.help`, `.bytecode 1 + 2`, and `exit` from stdin.
6. Run the full unit test suite.
