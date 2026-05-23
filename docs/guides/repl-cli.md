---
status: current
verified_against: src/app/app_options.hpp; src/app/app_options.cpp; src/main.cpp; src/repl.hpp; src/repl.cpp; src/repl/; tests/unit/app/test_app_options.cpp; tests/unit/app/test_repl_commands.cpp
last_checked: 2026-05-23
applies_to: lua_app command-line and REPL behavior
---

# REPL And CLI Guide

`lua_app.exe` is the interpreter executable. It can run a script, enter the REPL, print version/help, and optionally write a VM trace.

## Command Line

```powershell
bin\lua_app.exe [options] [script [args]]
```

Options currently parsed by `AppOptions`:

| Option | Behavior |
|---|---|
| `-v` | Show version and exit |
| `-h` | Show usage and exit |
| `-i` | Enter REPL when no script is provided |
| `--trace <file>` | Write VM execution trace as JSONL |
| `--trace-diff <file>` | Write VM execution trace with `changedRegisters` instead of full register snapshots |

The first non-option argument is treated as the script path. Arguments after it are left as script arguments.

Precedence:

1. `-v`
2. `-h`
3. script mode
4. explicit REPL mode
5. default behavior

With no script and no `-i`, the current default behavior enters the REPL.

## Script Mode

```powershell
bin\lua_app.exe examples\hello.lua
bin\lua_app.exe --trace bin\hello.jsonl examples\hello.lua
bin\lua_app.exe --trace-diff bin\hello-diff.jsonl examples\hello.lua
```

Script mode reads the file through `readWholeFile()`, parses it, generates a `Proto`, and executes it through the VM.

## REPL Mode

```powershell
bin\lua_app.exe
bin\lua_app.exe -i
```

The REPL initializes:

- `_VERSION`
- `_PROMPT`
- `_PROMPT2`
- `exit()`

Supported REPL behavior:

- `exit` or `quit` on the first line exits
- `exit()` exits through the registered function
- Ctrl+D / EOF exits
- Ctrl+C cancels current input where supported
- default prompts are line-numbered as `lua:1> ` for the first line and `lua:2>> ` for continuation lines
- custom multi-line prompts use `_PROMPT2` literally
- `=expr` is transformed into `return expr` and prints returned values
- ordinary input is parsed as statements and does not auto-print expression values
- Tab completion covers meta commands, globals, and loaded library fields such as `string.sub`
- colorized error output is enabled only for terminal REPL sessions; redirected output stays plain

Supported meta commands:

- `.help` prints the command list
- `.bytecode <expr|chunk>` parses and compiles the input, then prints the compact Proto bytecode
- `.ast <expr|chunk>` parses the input and prints a tree view of the AST
- `.gc [stats|collect|strategy|help]` prints GC statistics, runs collection through the active `GCStrategy`, or shows the strategy boundary

Both `.bytecode` and `.ast` first try the argument as a chunk. If that fails and the input was not explicitly written as `=expr`, they retry it as `return <expr>`. The `.ast` output labels this fallback as `mode: expression`; normal chunks are labeled `mode: chunk`.

The `.gc` command intentionally uses the active collector through `RuntimeServices.gc`.
It reports the current strategy and lists the available `mark-sweep` and incremental teaching-placeholder strategies.

GC strategy can also be queried or switched from Lua:

```lua
collectgarbage("strategy")
collectgarbage("strategy", "mark-sweep")
collectgarbage("strategy", "incremental")
```

The incremental strategy currently delegates to the same mark-sweep collection phases so the live-object semantics stay equivalent while the future write-barrier and scheduling work remains explicit.

Tab completion is intentionally conservative. It completes from the end of the current line, uses the current `LuaState` global table for global names, and walks dotted table paths for loaded library fields.

Colorized errors use ANSI red output through `ErrorColorMode::Auto`.
On Windows, the REPL enables virtual terminal processing before emitting colored errors.

## Prompt Customization

Inside the REPL:

```lua
_PROMPT = "lua> "
_PROMPT2 = "...> "
```

The default `_PROMPT` / `_PROMPT2` values opt into line-numbered prompts.
Assigning custom prompt strings keeps those strings unchanged.

## Trace Output

`--trace <file>` installs `JsonTraceSink` through `VM::setTraceSink()`. `--trace-diff <file>` additionally enables VM diff mode so instruction events contain `changedRegisters`. See `docs/vm/trace-system.md`.

## Related Tests

```powershell
bin\lua_test.exe --filter "AppOptions"
bin\lua_test.exe --filter "REPL Commands"
bin\lua_test.exe --filter "VM Trace Debug"
```
