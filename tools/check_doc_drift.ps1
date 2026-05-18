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
    "docs/PROJECT_STATUS.md",
    "docs/DEVELOPMENT_GUIDE.md",
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
    "docs/history/exprdesc.md"
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

$guideLines = $guide -split "\r?\n"
for ($i = 0; $i -lt $guideLines.Count; $i++) {
    $line = $guideLines[$i]
    if ($line -match "^\s*(cmake|ctest)\b" -or $line -match "cmake\s+-B" -or $line -match "cd\s+build\s*&&\s*ctest") {
        Add-Failure $failures "DEVELOPMENT_GUIDE.md presents CMake/CTest as an executable current command at line $($i + 1): $line"
    }
}

$statusDoc = Read-Text "docs/PROJECT_STATUS.md"
foreach ($required in @("Visual Studio", "MSBuild", ".vcxproj", "CMake", "CTest", "not current", "414", "1634")) {
    if ($statusDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "PROJECT_STATUS.md is missing required fact: $required"
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
