param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

function Join-RepoPath {
    param([string]$RelativePath)
    return Join-Path $Root $RelativePath
}

function Add-Failure {
    param(
        [System.Collections.Generic.List[string]]$Failures,
        [string]$Message
    )
    $Failures.Add($Message) | Out-Null
}

function Read-Text {
    param([string]$RelativePath)
    $path = Join-RepoPath $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing required file: $RelativePath"
    }
    return Get-Content -LiteralPath $path -Raw
}

function Test-FrontMatter {
    param([string]$Text)
    return $Text -match "(?ms)^---\r?\n.*?^status:\s*(current|historical|planned)\s*$.*?^verified_against:\s*.+$.*?^last_checked:\s*\d{4}-\d{2}-\d{2}\s*$.*?^applies_to:\s*.+$.*?^---\s*$"
}

function Get-RepoRelativePath {
    param([string]$Path)

    $fullRoot = (Resolve-Path -LiteralPath $Root).Path.TrimEnd('\', '/')
    $fullPath = (Resolve-Path -LiteralPath $Path).Path
    if ($fullPath.StartsWith($fullRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $fullPath.Substring($fullRoot.Length).TrimStart('\', '/')
    }
    return $fullPath
}

$failures = [System.Collections.Generic.List[string]]::new()

$coreDocs = @(
    "README.md",
    "docs/index.md",
    "docs/roadmap/current.md",
    "docs/status/project-status.md",
    "docs/guides/development.md",
    "docs/guides/repl-cli.md",
    "docs/guides/test-runner.md",
    "docs/guides/bytecode-tool.md",
    "docs/glossary.md",
    "docs/walkthroughs/index.md",
    "docs/projects/lua-lib.md",
    "docs/projects/lua-app.md",
    "docs/projects/lua-test.md",
    "docs/projects/lua-bytecode.md",
    "docs/architecture/gc.md",
    "docs/compiler/bytecode-generation.md",
    "docs/architecture/overview.md",
    "docs/architecture/runtime-services.md",
    "docs/compiler/parser-expression.md",
    "docs/compiler/register-allocation.md",
    "docs/compiler/codegen-responsibility-map.md",
    "docs/vm/instruction-set.md",
    "docs/vm/trace-system.md",
    "docs/stdlib/overview.md",
    "docs/archive/research/deep-research-report.md",
    "docs/archive/coroutine/design-analysis.md",
    "docs/architecture/coroutine.md",
    "docs/archive/debug/debug-notes.md",
    "docs/roadmap/future-architecture.md",
    "docs/archive/refactors/refactor-expdesc-plan.md",
    "docs/archive/refactors/refactor-expdesc-pr-checklist.md",
    "docs/archive/refactors/refactor-singlepass-cleanup-plan.md",
    "docs/archive/history/exprdesc.md",
    "examples/README.md"
)

foreach ($doc in $coreDocs) {
    $path = Join-RepoPath $doc
    if (-not (Test-Path -LiteralPath $path)) {
        Add-Failure $failures "Missing core doc: $doc"
        continue
    }

    $text = Get-Content -LiteralPath $path -Raw
    if (-not (Test-FrontMatter $text)) {
        Add-Failure $failures "Missing or invalid fact header: $doc"
    }
}

foreach ($requiredFile in @("CMakeLists.txt", "tools/run_cmake_smoke.ps1")) {
    $path = Join-RepoPath $requiredFile
    if (-not (Test-Path -LiteralPath $path)) {
        Add-Failure $failures "Missing build support file: $requiredFile"
    }
}

$compilerDir = Join-RepoPath "src/compiler"
if (Test-Path -LiteralPath $compilerDir) {
    $legacyMatches = Get-ChildItem -LiteralPath $compilerDir -Recurse -File |
        Select-String -Pattern "ExprDesc|ExprKind|expdesc" -CaseSensitive

    foreach ($match in $legacyMatches) {
        $relative = Get-RepoRelativePath $match.Path
        Add-Failure $failures "Legacy compiler pipeline reference in ${relative}:$($match.LineNumber): $($match.Line.Trim())"
    }
} else {
    Add-Failure $failures "Missing compiler source directory: src/compiler"
}

$readme = Read-Text "README.md"
if ($readme -notmatch "docs/status/project-status\.md") {
    Add-Failure $failures "README.md must point readers to docs/status/project-status.md"
}

$guide = Read-Text "docs/guides/development.md"
if ($guide -notmatch "MSBuild" -or $guide -notmatch "\.vcxproj" -or $guide -notmatch "docs/status/project-status\.md") {
    Add-Failure $failures "docs/guides/development.md must name the current MSBuild/.vcxproj path and reference docs/status/project-status.md"
}

foreach ($required in @("CMakeLists.txt", "tools\run_cmake_smoke.ps1", "CMake", "CTest", "secondary")) {
    if ($guide -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/guides/development.md is missing CMake/CTest support fact: $required"
    }
}

$statusDoc = Read-Text "docs/status/project-status.md"
$statusDocPath = Join-RepoPath "docs/status/project-status.md"
foreach ($required in @("Visual Studio", "MSBuild", ".vcxproj", "CMake", "CTest", "secondary", "CMakeLists.txt", "tools/run_cmake_smoke.ps1", "504", "2459", "--report=junit", "RuntimeServices", "Learning Path", "lua_bytecode", "decoded instructions", "constant table")) {
    if (-not (Select-String -LiteralPath $statusDocPath -SimpleMatch -Pattern $required -Quiet)) {
        Add-Failure $failures "docs/status/project-status.md is missing required fact: $required"
    }
}

$runtimeServicesDoc = Read-Text "docs/architecture/runtime-services.md"
foreach ($required in @("GlobalState& globalState", "StringPool& strings", "GarbageCollector& gc", "VM::DispatchStrategy* dispatchStrategy")) {
    if ($runtimeServicesDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/architecture/runtime-services.md is missing RuntimeServices field: $required"
    }
}

$startHereDoc = Read-Text "docs/index.md"
foreach ($required in @("docs/status/project-status.md", "docs/guides/development.md", "docs/guides/repl-cli.md", "docs/guides/test-runner.md", "docs/guides/bytecode-tool.md", "docs/projects/lua-lib.md", "docs/projects/lua-app.md", "docs/projects/lua-test.md", "docs/projects/lua-bytecode.md", "docs/vm/instruction-set.md", "walkthroughs/index.md", "glossary.md", "examples/README.md", "bin\lua_test.exe --list")) {
    if ($startHereDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/index.md is missing learning path reference: $required"
    }
}

$glossaryDoc = Read-Text "docs/glossary.md"
foreach ($required in @("RuntimeServices", "GlobalState", "LuaState", "Proto", "SymbolRef", "ValueResult", "CondResult", "LValueRef", "TMS", "RK")) {
    if ($glossaryDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "glossary.md is missing required term: $required"
    }
}

$examplesDoc = Read-Text "examples/README.md"
foreach ($required in @("bin\lua_app.exe", "hello.lua", "control_flow.lua", "tables_and_methods.lua", "metamethods.lua")) {
    if ($examplesDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "examples/README.md is missing example reference: $required"
    }
}

$bytecodeDoc = Read-Text "docs/compiler/bytecode-generation.md"
foreach ($required in @("AST", "SymbolRef", "ValueResult", "CondResult", "LValueRef", "CallResultInfo", "Proto", "BytecodeBuilder")) {
    if ($bytecodeDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/compiler/bytecode-generation.md is missing current pipeline term: $required"
    }
}

$bytecodeToolDoc = Read-Text "docs/guides/bytecode-tool.md"
foreach ($required in @("decoded instructions", "constant references", "constant table", "recursive child Proto output is still pending")) {
    if ($bytecodeToolDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/guides/bytecode-tool.md is missing current bytecode printer fact: $required"
    }
}

$historyDoc = Read-Text "docs/archive/history/exprdesc.md"
if ($historyDoc -notmatch "status:\s*historical") {
    Add-Failure $failures "docs/archive/history/exprdesc.md must be marked historical"
}

if ($failures.Count -gt 0) {
    Write-Host "[FAIL] Documentation drift checks failed:"
    foreach ($failure in $failures) {
        Write-Host " - $failure"
    }
    exit 1
}

Write-Host "[OK] Documentation drift checks passed"
