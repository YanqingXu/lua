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
    "docs/START_HERE.md",
    "docs/OPTIMIZATION_ROADMAP.md",
    "docs/PROJECT_STATUS.md",
    "docs/DEVELOPMENT_GUIDE.md",
    "docs/glossary.md",
    "docs/walkthroughs/index.md",
    "docs/BYTECODE_GENERATION.md",
    "docs/ARCHITECTURE.md",
    "docs/EXPRESSION_PARSING.md",
    "docs/REGISTER_ALLOCATION.md",
    "docs/VM_TRACE_SYSTEM.md",
    "docs/COROUTINE_DESIGN_ANALYSIS.md",
    "docs/COROUTINE_DESIGN_V2.md",
    "docs/DEBUG.md",
    "docs/PLAN.md",
    "docs/refactor_expdesc_plan.md",
    "docs/refactor_expdesc_pr_checklist.md",
    "docs/refactor_singlepass_cleanup_plan.md",
    "docs/history/exprdesc.md",
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
if ($readme -notmatch "docs/PROJECT_STATUS\.md") {
    Add-Failure $failures "README.md must point readers to docs/PROJECT_STATUS.md"
}

$guide = Read-Text "docs/DEVELOPMENT_GUIDE.md"
if ($guide -notmatch "MSBuild" -or $guide -notmatch "\.vcxproj" -or $guide -notmatch "docs/PROJECT_STATUS\.md") {
    Add-Failure $failures "DEVELOPMENT_GUIDE.md must name the current MSBuild/.vcxproj path and reference docs/PROJECT_STATUS.md"
}

foreach ($required in @("CMakeLists.txt", "tools\run_cmake_smoke.ps1", "CMake", "CTest", "secondary")) {
    if ($guide -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "DEVELOPMENT_GUIDE.md is missing CMake/CTest support fact: $required"
    }
}

$statusDoc = Read-Text "docs/PROJECT_STATUS.md"
foreach ($required in @("Visual Studio", "MSBuild", ".vcxproj", "CMake", "CTest", "secondary", "CMakeLists.txt", "tools/run_cmake_smoke.ps1", "437", "1860", "--report=junit", "RuntimeServices", "Learning Path")) {
    if ($statusDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "PROJECT_STATUS.md is missing required fact: $required"
    }
}

$startHereDoc = Read-Text "docs/START_HERE.md"
foreach ($required in @("PROJECT_STATUS.md", "DEVELOPMENT_GUIDE.md", "walkthroughs/index.md", "glossary.md", "examples/README.md", "bin\lua_test.exe --list")) {
    if ($startHereDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "START_HERE.md is missing learning path reference: $required"
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

$bytecodeDoc = Read-Text "docs/BYTECODE_GENERATION.md"
foreach ($required in @("AST", "SymbolRef", "ValueResult", "CondResult", "LValueRef", "CallResultInfo", "Proto")) {
    if ($bytecodeDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "BYTECODE_GENERATION.md is missing current pipeline term: $required"
    }
}

$historyDoc = Read-Text "docs/history/exprdesc.md"
if ($historyDoc -notmatch "status:\s*historical") {
    Add-Failure $failures "docs/history/exprdesc.md must be marked historical"
}

if ($failures.Count -gt 0) {
    Write-Host "[FAIL] Documentation drift checks failed:"
    foreach ($failure in $failures) {
        Write-Host " - $failure"
    }
    exit 1
}

Write-Host "[OK] Documentation drift checks passed"
