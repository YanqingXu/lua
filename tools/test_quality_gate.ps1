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

function Assert-FileNotContains {
    param(
        [string]$RelativePath,
        [string[]]$Patterns
    )

    $path = Join-RepoPath $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing required quality gate file: $RelativePath"
    }

    foreach ($pattern in $Patterns) {
        if (Select-String -LiteralPath $path -Pattern $pattern -Quiet) {
            throw "$RelativePath contains forbidden pattern: $pattern"
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
    "check_opcode_coverage_matrix\.ps1",
    "check_value_result_variant_only\.ps1",
    "check_c_style_patterns\.ps1",
    "check_doc_drift\.ps1",
    "clang-format",
    "clang-tidy",
    "MSBuild",
    "failed with exit code"
)

Assert-FileContains "tools/add_source.ps1" @(
    "LUA_CORE_SOURCES",
    "LUA_REPL_SOURCES",
    "LUA_TEST_SOURCES",
    "lua_bytecode",
    "AllowMissing",
    "DryRun",
    "Quiet"
)

Assert-FileContains "tools/check_opcode_coverage_matrix.ps1" @(
    "enum\\s\+class\\s\+OpCode",
    "OpcodeMetadata",
    "mayInvokeMetamethod",
    "opcode_coverage_matrix\.md",
    "Duplicate matrix row",
    "Opcode matrix order mismatch",
    "CoverageContract",
    "--list",
    "not registered"
)

Assert-FileContains "tests/unit/vm/opcode_coverage_contract.json" @(
    '"schemaVersion":\s*1',
    '"tests":',
    '"coverage":',
    '"positive":',
    '"boundary":',
    '"metamethod":',
    'VM Dispatch::Data Move Handlers Execute Directly'
)

Assert-FileContains "tools/check_value_result_variant_only.ps1" @(
    "ValueResult",
    "variant-only",
    "forbiddenPatterns",
    "setPayload"
)

Assert-FileContains "tools/check_c_style_patterns.ps1" @(
    "C-style pattern guard",
    "forbiddenPatterns",
    "WarningOnly",
    "TestScope",
    "return nullptr",
    "BaselinePath",
    "UpdateBaseline",
    "textHash",
    "rationale"
)

Assert-FileNotContains "tools/check_c_style_patterns.ps1" @(
    "AllowedCount",
    "AllowedMatches"
)

Assert-FileContains "tools/c_style_allowlist.json" @(
    '"schemaVersion":\s+1',
    '"rule":',
    '"path":',
    '"line":',
    '"textHash":',
    '"rationale":'
)

Assert-FileContains "src/compiler/codegen/codegen_types.hpp" @(
    "using Variant = std::variant",
    "ValueResult\(\) = default",
    "explicit ValueResult\(Variant value\)"
)

Assert-FileContains "tests/unit/vm/opcode_coverage_matrix.md" @(
    "VM Opcode Coverage Matrix",
    "\| MOVE \|",
    "\| VARARG \|",
    "PR-54 Verification Standard",
    "opcode_coverage_contract.json",
    "Suite::Test"
)

Assert-FileContains "tools/check_doc_drift.ps1" @(
    "Get-TestRunSummary",
    "Registered Tests:\s*",
    "Total Results:\s*",
    "Assert-DocHasCurrentTestCounts",
    "Get-CurrentDocumentationPaths",
    "live-facts:start",
    "Assert-VerifiedAgainstPathsExist",
    "Assert-LiveFactsVerificationFreshness",
    "git.Source -C",
    "verified_against",
    "modern-cpp-teaching-audit-report"
)

$docDriftScript = Get-Content -LiteralPath (Join-RepoPath "tools/check_doc_drift.ps1") -Raw
$statusDoc = Get-Content -LiteralPath (Join-RepoPath "docs/status/project-status.md") -Raw
$testCountMatch = [regex]::Match($statusDoc, "(\d+)\D+registered tests\D+(\d+)\D+assertion results")
if (-not $testCountMatch.Success) {
    throw "docs/status/project-status.md is missing parseable test count facts"
}

foreach ($staleCount in @($testCountMatch.Groups[1].Value, $testCountMatch.Groups[2].Value)) {
    if ($docDriftScript -match "(?<!\d)$staleCount(?!\d)") {
        throw "tools/check_doc_drift.ps1 must parse test counts dynamically instead of hard-coding $staleCount"
    }
}

function Invoke-CStylePositionBaselineSmokeTest {
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lua_c_style_quality_" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path (Join-Path $tempRoot "src") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $tempRoot "tests") -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $tempRoot "tools") -Force | Out-Null

    try {
        $probePath = Join-Path $tempRoot "src\probe.cpp"
        [System.IO.File]::WriteAllText($probePath, @"
void* findProbe(bool missing) {
    if (missing) {
        return nullptr;
    }
    return reinterpret_cast<void*>(1);
}
"@)

        $guard = Join-RepoPath "tools/check_c_style_patterns.ps1"
        & powershell -NoProfile -ExecutionPolicy Bypass -File $guard -Root $tempRoot -TestScope All -UpdateBaseline
        if ($LASTEXITCODE -ne 0) {
            throw "C-style baseline smoke could not generate its initial baseline"
        }

        [System.IO.File]::WriteAllText($probePath, @"
void* findProbe(bool missing) {
    if (!missing) {
        return reinterpret_cast<void*>(1);
    }

    return nullptr;
}
"@)

        $previousErrorActionPreference = $ErrorActionPreference
        try {
            $ErrorActionPreference = "Continue"
            & powershell -NoProfile -ExecutionPolicy Bypass -File $guard -Root $tempRoot -TestScope Product *> $null
            $movedMatchExitCode = $LASTEXITCODE
        } finally {
            $ErrorActionPreference = $previousErrorActionPreference
        }
        if ($movedMatchExitCode -eq 0) {
            throw "C-style position baseline must reject moving a match while preserving its count"
        }

        & powershell -NoProfile -ExecutionPolicy Bypass -File $guard -Root $tempRoot -TestScope All -UpdateBaseline
        if ($LASTEXITCODE -ne 0) {
            throw "C-style baseline smoke could not accept an intentional baseline update"
        }
        & powershell -NoProfile -ExecutionPolicy Bypass -File $guard -Root $tempRoot -TestScope Product | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "C-style position baseline did not pass after the intentional update"
        }
    } finally {
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force
        }
    }
}

Invoke-CStylePositionBaselineSmokeTest

function Invoke-AddSourceSmokeTest {
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lua_add_source_quality_" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $tempRoot | Out-Null

    try {
        foreach ($relative in @(
            "CMakeLists.txt",
            "lua.vcxproj",
            "lua.vcxproj.filters",
            "lua_bytecode.vcxproj",
            "lua_bytecode.vcxproj.filters",
            "lua_test.vcxproj",
            "lua_test.vcxproj.filters"
        )) {
            Copy-Item -LiteralPath (Join-RepoPath $relative) -Destination (Join-Path $tempRoot $relative)
        }

        New-Item -ItemType Directory -Path (Join-Path $tempRoot "src\compiler\probe") -Force | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $tempRoot "src\bytecode") -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $tempRoot "src\compiler\probe\sample.cpp") -Value "// sample" -NoNewline
        Set-Content -LiteralPath (Join-Path $tempRoot "src\compiler\probe\sample.hpp") -Value "// sample" -NoNewline
        Set-Content -LiteralPath (Join-Path $tempRoot "src\bytecode\probe_tool.cpp") -Value "// sample" -NoNewline

        $scriptPath = Join-RepoPath "tools/add_source.ps1"
        & $scriptPath -Root $tempRoot -SourcePath "src\compiler\probe\sample.cpp", "src\compiler\probe\sample.hpp" -Target Core -Quiet
        & $scriptPath -Root $tempRoot -SourcePath "src\compiler\probe\sample.cpp", "src\compiler\probe\sample.hpp" -Target Core -Quiet
        & $scriptPath -Root $tempRoot -SourcePath "src\bytecode\probe_tool.cpp" -Target @("Bytecode", "Test") -Quiet

        $cmake = Get-Content -LiteralPath (Join-Path $tempRoot "CMakeLists.txt") -Raw
        $coreProject = Get-Content -LiteralPath (Join-Path $tempRoot "lua.vcxproj") -Raw
        $coreFilters = Get-Content -LiteralPath (Join-Path $tempRoot "lua.vcxproj.filters") -Raw
        $bytecodeProject = Get-Content -LiteralPath (Join-Path $tempRoot "lua_bytecode.vcxproj") -Raw
        $bytecodeFilters = Get-Content -LiteralPath (Join-Path $tempRoot "lua_bytecode.vcxproj.filters") -Raw
        $testProject = Get-Content -LiteralPath (Join-Path $tempRoot "lua_test.vcxproj") -Raw

        foreach ($required in @(
            "src/compiler/probe/sample.cpp",
            "src/bytecode/probe_tool.cpp"
        )) {
            if ($cmake -notmatch [regex]::Escape($required)) {
                throw "tools/add_source.ps1 smoke missing CMake entry: $required"
            }
        }

        if (([regex]::Matches($cmake, [regex]::Escape("src/compiler/probe/sample.cpp"))).Count -ne 1) {
            throw "tools/add_source.ps1 must be idempotent for repeated CMake entries"
        }

        foreach ($required in @(
            "src\compiler\probe\sample.cpp",
            "src\compiler\probe\sample.hpp"
        )) {
            if ($coreProject -notmatch [regex]::Escape($required)) {
                throw "tools/add_source.ps1 smoke missing core project entry: $required"
            }
            if ($coreFilters -notmatch [regex]::Escape($required)) {
                throw "tools/add_source.ps1 smoke missing core filter entry: $required"
            }
        }

        if ($bytecodeProject -notmatch [regex]::Escape("src\bytecode\probe_tool.cpp") -or
            $bytecodeFilters -notmatch [regex]::Escape("src\bytecode")) {
            throw "tools/add_source.ps1 smoke missing bytecode project/filter entry"
        }

        if ($testProject -notmatch [regex]::Escape("src\bytecode\probe_tool.cpp")) {
            throw "tools/add_source.ps1 smoke missing test project entry"
        }
    } finally {
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force
        }
    }
}

Invoke-AddSourceSmokeTest

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
