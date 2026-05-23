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
    "check_opcode_coverage_matrix\.ps1",
    "check_value_result_legacy_fields\.ps1",
    "check_doc_drift\.ps1",
    "clang-format",
    "clang-tidy",
    "MSBuild"
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
    "opcode_coverage_matrix\.md",
    "Duplicate matrix row",
    "Opcode matrix order mismatch"
)

Assert-FileContains "tools/check_value_result_legacy_fields.ps1" @(
    "ValueResult",
    "legacyFields",
    "ValueResultLegacyMirrorProbe",
    "LUA_SUPPRESS_DEPRECATED_DECLARATIONS_BEGIN",
    "payload\(\)/visit\(\)"
)

Assert-FileContains "tools/run_value_result_private_trial.ps1" @(
    "run_cmake_smoke\.ps1",
    "cmake-value-result-private",
    "LUA_VALUE_RESULT_PRIVATE_LEGACY_FIELDS=ON"
)

Assert-FileContains "CMakeLists.txt" @(
    "option\(LUA_VALUE_RESULT_PRIVATE_LEGACY_FIELDS",
    "target_compile_definitions\(lua_core PUBLIC LUA_VALUE_RESULT_PRIVATE_LEGACY_FIELDS=1\)"
)

Assert-FileContains "src/compiler/codegen/codegen_types.hpp" @(
    "LUA_VALUE_RESULT_PRIVATE_LEGACY_FIELDS",
    "ValueResultLegacyMirrorProbe",
    "overwriteForCharacterization"
)

Assert-FileContains "tests/unit/vm/opcode_coverage_matrix.md" @(
    "VM Opcode Coverage Matrix",
    "\| MOVE \|",
    "\| VARARG \|",
    "PR-54 Verification Standard"
)

Assert-FileContains "tools/check_doc_drift.ps1" @(
    "Get-TestRunSummary",
    "Registered Tests:\s*",
    "Total Results:\s*",
    "Assert-DocHasCurrentTestCounts"
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
