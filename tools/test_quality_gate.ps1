param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

function Join-RepoPath {
    param([string]$RelativePath)
    return Join-Path $Root $RelativePath
}

function Assert-FileContains {
    param(
        [string]$RelativePath,
        [string[]]$Patterns
    )

    $path = Join-RepoPath $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing required quality gate file: $RelativePath"
    }

    foreach ($pattern in $Patterns) {
        if (-not (Select-String -LiteralPath $path -Pattern $pattern -Quiet)) {
            throw "$RelativePath is missing required pattern: $pattern"
        }
    }
}

Assert-FileContains ".clang-format" @(
    "BasedOnStyle:\s*LLVM",
    "ColumnLimit:\s*120",
    "SortIncludes:\s*Never"
)

Assert-FileContains ".clang-tidy" @(
    "bugprone-\*",
    "performance-\*",
    "readability-\*",
    "WarningsAsErrors:"
)

Assert-FileContains "tools/run_quality_gate.ps1" @(
    "check_doc_drift\.ps1",
    "clang-format",
    "clang-tidy",
    "MSBuild"
)

Assert-FileContains "tools/check_doc_drift.ps1" @(
    "Get-TestRunSummary",
    "Registered Tests:\s*",
    "Total Results:\s*",
    "Assert-DocHasCurrentTestCounts"
)

$docDriftScript = Get-Content -LiteralPath (Join-RepoPath "tools/check_doc_drift.ps1") -Raw
foreach ($staleCount in @("513", "2497")) {
    if ($docDriftScript -match "(?<!\d)$staleCount(?!\d)") {
        throw "tools/check_doc_drift.ps1 must parse test counts dynamically instead of hard-coding $staleCount"
    }
}

Assert-FileContains ".github/workflows/ci.yml" @(
    "pull_request",
    "windows-latest",
    "tools\\check_doc_drift\.ps1",
    "tools\\run_quality_gate\.ps1",
    "bin\\lua_test\.exe"
)

Assert-FileContains "docs/status/project-status.md" @(
    "clang-format",
    "clang-tidy",
    "GitHub Actions",
    "tools/run_quality_gate\.ps1"
)

Write-Host "[OK] quality gate configuration tests passed"
