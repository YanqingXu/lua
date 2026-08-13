param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$PowerShellExecutable = ""
)

$ErrorActionPreference = "Stop"

function Join-RepoPath {
    param([string]$RelativePath)
    return Join-Path $Root $RelativePath
}

function Resolve-PowerShellExecutable {
    param([string]$RequestedExecutable)

    if (-not [string]::IsNullOrWhiteSpace($RequestedExecutable)) {
        $requestedCommand = Get-Command $RequestedExecutable -ErrorAction SilentlyContinue
        if ($requestedCommand) {
            return $requestedCommand.Source
        }

        if (Test-Path -LiteralPath $RequestedExecutable -PathType Leaf) {
            return (Resolve-Path -LiteralPath $RequestedExecutable).Path
        }

        throw "Requested PowerShell executable does not exist: $RequestedExecutable"
    }

    try {
        $currentProcessPath = (Get-Process -Id $PID -ErrorAction Stop).Path
        $currentProcessName = [System.IO.Path]::GetFileNameWithoutExtension($currentProcessPath)
        if ($currentProcessName -in @("pwsh", "powershell") -and
            (Test-Path -LiteralPath $currentProcessPath -PathType Leaf)) {
            return $currentProcessPath
        }
    } catch {
        # Fall through to command discovery for restricted hosts.
    }

    $powerShellCommand = Get-Command pwsh, powershell -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $powerShellCommand) {
        throw "Unable to resolve a PowerShell executable (pwsh or powershell)"
    }
    return $powerShellCommand.Source
}

function Get-PowerShellFileArguments {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments = @()
    )

    $invokeArguments = @("-NoProfile", "-NonInteractive")
    if ($env:OS -eq "Windows_NT") {
        $invokeArguments += @("-ExecutionPolicy", "Bypass")
    }
    $invokeArguments += @("-File", $ScriptPath)
    $invokeArguments += @($Arguments)
    return $invokeArguments
}

function Invoke-PowerShellFileCapture {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments = @()
    )

    $invokeArguments = @(Get-PowerShellFileArguments -ScriptPath $ScriptPath -Arguments $Arguments)
    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $global:LASTEXITCODE = -1
        $output = & $script:powerShellExecutablePath @invokeArguments 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    $lines = @($output | ForEach-Object { $_.ToString() })
    return [pscustomobject]@{
        ExitCode = $exitCode
        Lines = $lines
        Text = $lines -join "`n"
    }
}

function Invoke-PowerShellFile {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments = @()
    )

    $result = Invoke-PowerShellFileCapture -ScriptPath $ScriptPath -Arguments $Arguments
    foreach ($line in $result.Lines) {
        Write-Host $line
    }
    if ($result.ExitCode -ne 0) {
        throw "PowerShell script '$ScriptPath' failed with exit code $($result.ExitCode)"
    }
}

$powerShellExecutablePath = Resolve-PowerShellExecutable $PowerShellExecutable

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

function Assert-FileTextMatches {
    param(
        [string]$RelativePath,
        [string[]]$Patterns
    )

    $path = Join-RepoPath $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing required quality gate file: $RelativePath"
    }

    $text = [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)
    foreach ($pattern in $Patterns) {
        if (-not [regex]::IsMatch($text, $pattern)) {
            throw "$RelativePath is missing required whole-file pattern: $pattern"
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
    "-portability-avoid-pragma-once",
    "WarningsAsErrors:",
    "bugprone-unused-return-value\.AllowCastToVoid",
    "value:\s*'true'"
)

Assert-FileTextMatches ".clang-tidy" @(
    '(?ms)^Checks:\s*>.*?-portability-avoid-pragma-once,.*?^WarningsAsErrors:',
    '(?m)^WarningsAsErrors:.*-portability-avoid-pragma-once'
)

Assert-FileContains "tools/run_quality_gate.ps1" @(
    "\[switch\]\`$Strict",
    "\[string\]\`$FormatBase",
    "check_opcode_coverage_matrix\.ps1",
    "check_value_result_variant_only\.ps1",
    "check_c_style_patterns\.ps1",
    "check_test_signal_integrity\.ps1",
    "check_lua51_official_sources\.ps1",
    "check_lua51_public_api_contract\.py",
    "check_doc_drift\.ps1",
    "clang-format",
    "Test-OwnedCppSourcePath",
    'tests/lua/official/',
    '"lua_test/"',
    "clang-tidy",
    "LUA_TEST_BUILD_GIT_SHA=",
    "MSBuild",
    "failed with exit code",
    "Strict quality gate requires clang-format",
    "Strict quality gate requires clang-tidy",
    "Strict quality gate requires MSBuild\.exe",
    "Strict quality gate requires test executable",
    "Strict changed clang-format scope is empty on a clean worktree",
    "pass -FormatBase <revision> or use -FormatScope All",
    "LuaTestBuildGitSha",
    "check_test_binary_sha\.ps1",
    "Resolve-PowerShellExecutable",
    "Invoke-PowerShellFile",
    "\[string\]\`$TestExecutable"
)

Assert-FileNotContains "tools/run_quality_gate.ps1" @(
    "& powershell"
)

Assert-FileContains "tools/check_test_binary_sha.ps1" @(
    "ExpectedSha must be exactly 40 hexadecimal characters",
    "--build-info",
    "reported Build Git SHA: unknown",
    "does not match expected SHA"
)

Invoke-PowerShellFile -ScriptPath (Join-RepoPath "tools/test_test_binary_sha.ps1")

Assert-FileContains "tools/check_test_signal_integrity.ps1" @(
    "unconditional-assert-true",
    "skip-output-as-success",
    "test_signal_allowlist\.json",
    "Stale or text-changed allowlist entry",
    "expiresOn",
    "Test signal integrity passed"
)

Assert-FileContains "tests/quality/test_signal_allowlist.json" @(
    '"schemaVersion":\s*1',
    '"textHash":',
    '"rationale":',
    '"expiresOn":'
)

Assert-FileContains "tests/unit/framework/test_framework.hpp" @(
    "inline void SKIP_EXPECTED",
    "inline void SKIP_UNEXPECTED"
)
Assert-FileNotContains "tests/unit/framework/test_framework.hpp" @(
    "#define SKIP_EXPECTED",
    "#define SKIP_UNEXPECTED"
)

Invoke-PowerShellFile -ScriptPath (Join-RepoPath "tools/test_test_signal_integrity.ps1")
Invoke-PowerShellFile -ScriptPath (Join-RepoPath "tools/test_release_identity.ps1")

Assert-FileContains "tools/write_workflow_evidence.py" @(
    "lua-cpp\.workflow-evidence/v1",
    "evidence-metadata\.json",
    "runtime-soak-evidence",
    "long-fuzz-evidence",
    "candidate_sha",
    "run_attempt"
)

Assert-FileContains "tools/verify_release_evidence.py" @(
    "lua-cpp\.release-evidence/v2",
    "EXPECTED_CI_JOBS",
    "EXPECTED_NIGHTLY_JOBS",
    "evidence-metadata\.json",
    "candidate_sha",
    "run_attempt",
    "governance",
    "source_readiness",
    "abi_version"
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
    "not registered",
    "Get-CppFunctionEvidence",
    "Test-ProducerEvidence",
    "Test-HandlerEvidence",
    "actualHandlerRegistrations",
    "makeHandlerTable"
)

Assert-FileContains "tests/unit/vm/opcode_coverage_contract.json" @(
    '"schemaVersion":\s*2',
    '"tests":',
    '"coverage":',
    '"producer":',
    '"handler":',
    '"registrar":',
    '"symbol":',
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
    "using Variant =",
    "std::variant<",
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
    "Get-PublicApiContractSummary",
    "Assert-DocHasCurrentPublicApiCounts",
    "check_lua51_public_api_contract\.py",
    "verified_against",
    "Non-technical documentation path"
)

$docDriftScript = Get-Content -LiteralPath (Join-RepoPath "tools/check_doc_drift.ps1") -Raw
$readme = Get-Content -LiteralPath (Join-RepoPath "README.md") -Raw
$testCountMatch = [regex]::Match($readme, "(\d+)\D+registered tests\D+(\d+)\D+assertion results")
if (-not $testCountMatch.Success) {
    throw "README.md is missing parseable test count facts"
}

foreach ($staleCount in @($testCountMatch.Groups[1].Value, $testCountMatch.Groups[2].Value)) {
    if ($docDriftScript -match "(?<!\d)$staleCount(?!\d)") {
        throw "tools/check_doc_drift.ps1 must parse test counts dynamically instead of hard-coding $staleCount"
    }
}

function Invoke-NativeFailurePropagationSmokeTest {
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lua_quality_exit_" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $tempRoot | Out-Null

    try {
        $failingPowerShell = Join-Path $tempRoot "failing_powershell.ps1"
        [System.IO.File]::WriteAllText(
            $failingPowerShell,
            "exit 23`n",
            [System.Text.UTF8Encoding]::new($false)
        )

        $failureMessage = $null
        try {
            & (Join-RepoPath "tools/run_quality_gate.ps1") -SkipBuild -SkipClangTidy -FormatScope Off `
                -PowerShellExecutable $failingPowerShell
        } catch {
            $failureMessage = $_.Exception.Message
        }

        if ($failureMessage -notmatch "check_value_result_variant_only\.ps1.*failed with exit code 23") {
            throw "Quality gate did not propagate a native child exit code: $failureMessage"
        }
    } finally {
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force
        }
    }
}

Invoke-NativeFailurePropagationSmokeTest

function Invoke-StrictMissingToolSmokeTest {
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lua_quality_strict_" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $tempRoot | Out-Null
    $previousPath = $env:PATH

    try {
        $env:PATH = $tempRoot
        $failureMessage = $null
        try {
            & (Join-RepoPath "tools/run_quality_gate.ps1") -Strict -SkipBuild -SkipClangTidy -FormatScope Changed
        } catch {
            $failureMessage = $_.Exception.Message
        }

        if ($failureMessage -notmatch "Strict quality gate requires clang-format on PATH") {
            throw "Strict quality gate silently skipped a missing required tool: $failureMessage"
        }
    } finally {
        $env:PATH = $previousPath
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force
        }
    }
}

Invoke-StrictMissingToolSmokeTest

function Invoke-ClangTidyCompileDefinitionSmokeTest {
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
        "lua_quality_clang_tidy_definition_" + [guid]::NewGuid().ToString("N")
    )
    New-Item -ItemType Directory -Path $tempRoot | Out-Null
    $capturePath = Join-Path $tempRoot "clang_tidy_arguments.txt"
    $previousPath = $env:PATH
    $previousCapturePath = $env:LUA_QUALITY_CLANG_TIDY_CAPTURE

    try {
        [System.IO.File]::WriteAllText(
            (Join-Path $tempRoot "clang-format.ps1"),
            "exit 0`n",
            [System.Text.UTF8Encoding]::new($false)
        )
        [System.IO.File]::WriteAllText(
            (Join-Path $tempRoot "clang-tidy.ps1"),
            "[System.IO.File]::WriteAllLines(`$env:LUA_QUALITY_CLANG_TIDY_CAPTURE, [string[]]`$args)`nexit 43`n",
            [System.Text.UTF8Encoding]::new($false)
        )
        $env:PATH = $tempRoot + [System.IO.Path]::PathSeparator + $previousPath
        $env:LUA_QUALITY_CLANG_TIDY_CAPTURE = $capturePath

        $failureMessage = $null
        try {
            & (Join-RepoPath "tools/run_quality_gate.ps1") -Strict -SkipBuild -FormatScope All
        } catch {
            $failureMessage = $_.Exception.Message
        }

        if ($failureMessage -notmatch "clang-tidy failed.*exit code 43") {
            throw "clang-tidy compile-definition fixture did not reach the capture command: $failureMessage"
        }
        if (-not (Test-Path -LiteralPath $capturePath -PathType Leaf)) {
            throw "clang-tidy compile-definition fixture did not capture command arguments"
        }

        $capturedArguments = [System.IO.File]::ReadAllText($capturePath, [System.Text.Encoding]::UTF8)
        if ($capturedArguments -notmatch 'LUA_TEST_BUILD_GIT_SHA=__FILE__') {
            throw "clang-tidy smoke is missing the LUA_TEST_BUILD_GIT_SHA definition: $capturedArguments"
        }
    } finally {
        $env:PATH = $previousPath
        if ($null -eq $previousCapturePath) {
            Remove-Item Env:LUA_QUALITY_CLANG_TIDY_CAPTURE -ErrorAction SilentlyContinue
        } else {
            $env:LUA_QUALITY_CLANG_TIDY_CAPTURE = $previousCapturePath
        }
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force
        }
    }
}

Invoke-ClangTidyCompileDefinitionSmokeTest

function Invoke-StrictEmptyChangedScopeSmokeTest {
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lua_quality_empty_changed_" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $tempRoot | Out-Null
    $previousPath = $env:PATH
    $previousBaseRef = $env:GITHUB_BASE_REF

    try {
        [System.IO.File]::WriteAllText(
            (Join-Path $tempRoot "clang-format.ps1"),
            "exit 0`n",
            [System.Text.UTF8Encoding]::new($false)
        )
        [System.IO.File]::WriteAllText(
            (Join-Path $tempRoot "git.ps1"),
            "exit 0`n",
            [System.Text.UTF8Encoding]::new($false)
        )
        $env:PATH = $tempRoot + [System.IO.Path]::PathSeparator + $previousPath
        Remove-Item Env:GITHUB_BASE_REF -ErrorAction SilentlyContinue

        $failureMessage = $null
        try {
            & (Join-RepoPath "tools/run_quality_gate.ps1") -Strict -SkipBuild -SkipClangTidy -FormatScope Changed
        } catch {
            $failureMessage = $_.Exception.Message
        }

        if ($failureMessage -notmatch "Strict changed clang-format scope is empty on a clean worktree") {
            throw "Strict changed-scope gate did not reject an unbounded clean worktree: $failureMessage"
        }
    } finally {
        $env:PATH = $previousPath
        if ($null -eq $previousBaseRef) {
            Remove-Item Env:GITHUB_BASE_REF -ErrorAction SilentlyContinue
        } else {
            $env:GITHUB_BASE_REF = $previousBaseRef
        }
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force
        }
    }
}

Invoke-StrictEmptyChangedScopeSmokeTest

function Invoke-FormatScopeOwnershipSmokeTest {
    param(
        [ValidateSet("All", "Changed")]
        [string]$Scope
    )

    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
        "lua_quality_format_scope_" + $Scope.ToLowerInvariant() + "_" + [guid]::NewGuid().ToString("N")
    )
    New-Item -ItemType Directory -Path $tempRoot | Out-Null
    $capturePath = Join-Path $tempRoot "clang-format-arguments.txt"
    $previousPath = $env:PATH
    $previousCapturePath = $env:LUA_QUALITY_FORMAT_CAPTURE
    $previousBaseRef = $env:GITHUB_BASE_REF

    try {
        [System.IO.File]::WriteAllText(
            (Join-Path $tempRoot "clang-format.ps1"),
            @'
[System.IO.File]::WriteAllLines(
    $env:LUA_QUALITY_FORMAT_CAPTURE,
    [string[]]$args,
    [System.Text.UTF8Encoding]::new($false)
)
exit 41
'@,
            [System.Text.UTF8Encoding]::new($false)
        )

        if ($Scope -eq "Changed") {
            [System.IO.File]::WriteAllText(
                (Join-Path $tempRoot "git.ps1"),
                @'
if ($args.Count -gt 0 -and $args[0] -eq "diff") {
    if ($args -notcontains "--cached") {
        Write-Output "src/debug/trace_sink.hpp"
        Write-Output "tests/fuzz/fuzz_parser.cpp"
        Write-Output "tests/lua/official/etc/ltests.c"
        Write-Output "examples/embedding.cpp"
        Write-Output "benchmarks/runtime_bench.cpp"
        Write-Output "lua_test/include/test_framework/test_framework.hpp"
    }
    exit 0
}
if ($args.Count -gt 0 -and $args[0] -eq "ls-files") {
    exit 0
}
exit 0
'@,
                [System.Text.UTF8Encoding]::new($false)
            )
        }

        $env:PATH = $tempRoot + [System.IO.Path]::PathSeparator + $previousPath
        $env:LUA_QUALITY_FORMAT_CAPTURE = $capturePath
        Remove-Item Env:GITHUB_BASE_REF -ErrorAction SilentlyContinue

        $failureMessage = $null
        try {
            & (Join-RepoPath "tools/run_quality_gate.ps1") -Strict -SkipBuild -SkipClangTidy -FormatScope $Scope
        } catch {
            $failureMessage = $_.Exception.Message
        }

        if ($failureMessage -notmatch "clang-format failed with exit code 41") {
            throw "$Scope clang-format scope fixture did not reach the capture command: $failureMessage"
        }
        if (-not (Test-Path -LiteralPath $capturePath -PathType Leaf)) {
            throw "$Scope clang-format scope fixture did not capture any arguments"
        }

        $arguments = @([System.IO.File]::ReadAllLines($capturePath))
        $ownedFixtures = @(
            "src/debug/trace_sink.hpp",
            "tests/fuzz/fuzz_parser.cpp",
            "examples/embedding.cpp",
            "benchmarks/runtime_bench.cpp",
            "lua_test/include/test_framework/test_framework.hpp"
        )
        foreach ($relativePath in $ownedFixtures) {
            $expectedPath = [System.IO.Path]::GetFullPath((Join-RepoPath $relativePath))
            if ($arguments -notcontains $expectedPath) {
                $matchingArguments = @($arguments | Where-Object {
                    $_ -match [regex]::Escape([System.IO.Path]::GetFileName($expectedPath))
                })
                throw "$Scope clang-format scope omitted owned fixture: $relativePath; captured matches: $($matchingArguments -join ', ')"
            }
        }

        $officialPath = [System.IO.Path]::GetFullPath((Join-RepoPath "tests/lua/official/etc/ltests.c"))
        if ($arguments -contains $officialPath) {
            throw "$Scope clang-format scope included read-only upstream fixture: tests/lua/official/etc/ltests.c"
        }

        Write-Host "[OK] $Scope clang-format scope includes owned C/C++ and excludes tests/lua/official"
    } finally {
        $env:PATH = $previousPath
        if ($null -eq $previousCapturePath) {
            Remove-Item Env:LUA_QUALITY_FORMAT_CAPTURE -ErrorAction SilentlyContinue
        } else {
            $env:LUA_QUALITY_FORMAT_CAPTURE = $previousCapturePath
        }
        if ($null -eq $previousBaseRef) {
            Remove-Item Env:GITHUB_BASE_REF -ErrorAction SilentlyContinue
        } else {
            $env:GITHUB_BASE_REF = $previousBaseRef
        }
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force
        }
    }
}

Invoke-FormatScopeOwnershipSmokeTest -Scope All
Invoke-FormatScopeOwnershipSmokeTest -Scope Changed

function Invoke-ShaFailureStopsUnitTestSmokeTest {
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lua_quality_sha_stop_" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $tempRoot | Out-Null
    $fixtureExecutable = Join-Path $tempRoot "lua_test_fixture.ps1"
    $invocationCountPath = Join-Path $tempRoot "unit_invocation_count.txt"
    $previousCountPath = $env:LUA_QUALITY_SHA_PROBE_COUNT
    $previousRegistered = $env:LUA_QUALITY_SHA_PROBE_REGISTERED
    $previousTotal = $env:LUA_QUALITY_SHA_PROBE_TOTAL

    try {
        [System.IO.File]::WriteAllText(
            $fixtureExecutable,
            @'
if ($args.Count -eq 1 -and $args[0] -eq "--build-info") {
    Write-Output "Build Git SHA: 0000000000000000000000000000000000000000"
    exit 0
}

$countPath = $env:LUA_QUALITY_SHA_PROBE_COUNT
$count = 0
if (Test-Path -LiteralPath $countPath) {
    $count = [int]([System.IO.File]::ReadAllText($countPath).Trim())
}
[System.IO.File]::WriteAllText($countPath, ($count + 1).ToString())
Write-Output "Registered Tests: $env:LUA_QUALITY_SHA_PROBE_REGISTERED"
Write-Output "Total Results: $env:LUA_QUALITY_SHA_PROBE_TOTAL"
Write-Output "Failed: 0"
exit 0
'@,
            [System.Text.UTF8Encoding]::new($false)
        )

        $env:LUA_QUALITY_SHA_PROBE_COUNT = $invocationCountPath
        $env:LUA_QUALITY_SHA_PROBE_REGISTERED = $testCountMatch.Groups[1].Value
        $env:LUA_QUALITY_SHA_PROBE_TOTAL = $testCountMatch.Groups[2].Value

        $failureMessage = $null
        try {
            & (Join-RepoPath "tools/run_quality_gate.ps1") -SkipBuild -SkipClangTidy -FormatScope Off `
                -TestExecutable $fixtureExecutable
        } catch {
            $failureMessage = $_.Exception.Message
        }

        if ($failureMessage -notmatch "check_test_binary_sha\.ps1.*failed with exit code 1") {
            throw "Quality gate did not stop on the SHA checker failure: $failureMessage"
        }
        if (-not (Test-Path -LiteralPath $invocationCountPath -PathType Leaf)) {
            throw "SHA stop fixture was not invoked by the documentation contract"
        }

        $invocationCount = [int]([System.IO.File]::ReadAllText($invocationCountPath).Trim())
        if ($invocationCount -ne 1) {
            throw "Unit test executable ran after the SHA checker failed; expected one documentation invocation, got $invocationCount"
        }
        Write-Host "[OK] SHA checker failure stops the unit test executable"
    } finally {
        if ($null -eq $previousCountPath) {
            Remove-Item Env:LUA_QUALITY_SHA_PROBE_COUNT -ErrorAction SilentlyContinue
        } else {
            $env:LUA_QUALITY_SHA_PROBE_COUNT = $previousCountPath
        }
        if ($null -eq $previousRegistered) {
            Remove-Item Env:LUA_QUALITY_SHA_PROBE_REGISTERED -ErrorAction SilentlyContinue
        } else {
            $env:LUA_QUALITY_SHA_PROBE_REGISTERED = $previousRegistered
        }
        if ($null -eq $previousTotal) {
            Remove-Item Env:LUA_QUALITY_SHA_PROBE_TOTAL -ErrorAction SilentlyContinue
        } else {
            $env:LUA_QUALITY_SHA_PROBE_TOTAL = $previousTotal
        }
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force
        }
    }
}

Invoke-ShaFailureStopsUnitTestSmokeTest

function Invoke-OpcodeEvidenceContractSmokeTest {
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lua_opcode_evidence_" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $tempRoot | Out-Null

    try {
        $sourceContract = Join-RepoPath "tests/unit/vm/opcode_coverage_contract.json"
        $probeContract = Join-Path $tempRoot "opcode_coverage_contract.json"
        $checker = Join-RepoPath "tools/check_opcode_coverage_matrix.ps1"

        $producerProbe = Get-Content -LiteralPath $sourceContract -Raw | ConvertFrom-Json
        $producerProbe.coverage[0].producer.function = "MissingProducerForContractTest"
        $producerJson = $producerProbe | ConvertTo-Json -Depth 20
        [System.IO.File]::WriteAllText($probeContract, $producerJson + [Environment]::NewLine,
            [System.Text.UTF8Encoding]::new($false))

        $producerResult = Invoke-PowerShellFileCapture -ScriptPath $checker `
            -Arguments @("-CoverageContract", $probeContract)
        $producerOutput = $producerResult.Text
        $producerExitCode = $producerResult.ExitCode
        if ($producerExitCode -eq 0 -or
            ($producerOutput -join "`n") -notmatch "MissingProducerForContractTest") {
            throw "Opcode checker did not reject a contract with a fabricated CodeGen producer"
        }

        $handlerProbe = Get-Content -LiteralPath $sourceContract -Raw | ConvertFrom-Json
        $handlerProbe.coverage[0].handler.symbol = "missingHandlerForContractTest"
        $handlerJson = $handlerProbe | ConvertTo-Json -Depth 20
        [System.IO.File]::WriteAllText($probeContract, $handlerJson + [Environment]::NewLine,
            [System.Text.UTF8Encoding]::new($false))

        $handlerResult = Invoke-PowerShellFileCapture -ScriptPath $checker `
            -Arguments @("-CoverageContract", $probeContract)
        $handlerOutput = $handlerResult.Text
        $handlerExitCode = $handlerResult.ExitCode
        if ($handlerExitCode -eq 0 -or
            ($handlerOutput -join "`n") -notmatch "missingHandlerForContractTest") {
            throw "Opcode checker did not reject a contract with a fabricated VM handler"
        }
    } finally {
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force
        }
    }
}

Invoke-OpcodeEvidenceContractSmokeTest

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
        Invoke-PowerShellFile -ScriptPath $guard `
            -Arguments @("-Root", $tempRoot, "-TestScope", "All", "-UpdateBaseline")

        [System.IO.File]::WriteAllText($probePath, @"
void* findProbe(bool missing) {
    if (!missing) {
        return reinterpret_cast<void*>(1);
    }

    return nullptr;
}
"@)

        $movedMatchResult = Invoke-PowerShellFileCapture -ScriptPath $guard `
            -Arguments @("-Root", $tempRoot, "-TestScope", "Product")
        $movedMatchExitCode = $movedMatchResult.ExitCode
        if ($movedMatchExitCode -eq 0) {
            throw "C-style position baseline must reject moving a match while preserving its count"
        }

        Invoke-PowerShellFile -ScriptPath $guard `
            -Arguments @("-Root", $tempRoot, "-TestScope", "All", "-UpdateBaseline")
        Invoke-PowerShellFile -ScriptPath $guard `
            -Arguments @("-Root", $tempRoot, "-TestScope", "Product")
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
    'group: ci-\$\{\{ github\.event\.pull_request\.number \|\| github\.ref \}\}',
    "cancel-in-progress: true",
    "windows-latest",
    "ubuntu-latest",
    "configuration: \[Debug, Release\]",
    "compiler: GCC",
    "compiler: Clang",
    "cc: gcc",
    "cxx: g\+\+",
    "cc: clang",
    "cxx: clang\+\+",
    "build_type: Debug",
    "build_type: Release",
    "ctest --test-dir build --output-on-failure",
    "Configure CMake API, native-module, and production-worker evidence",
    "lua_public_api_consumer",
    "lua_public_native_module_host",
    "lua_public_native_module_app",
    "lua_public_native_module_embedding",
    "lua_production_worker",
    "lua_runtime_soak",
    '-L "api-contract\|production-contract\|runtime-failure-contract\|soak-smoke"',
    "LUA_CPP_SANITIZER",
    "sanitizer: \[address, undefined, thread\]",
    "Linux libFuzzer security boundaries",
    "Linux component coverage",
    "CXXFLAGS: -stdlib=libc\+\+",
    "disable_memory_limit: 1",
    "ASAN_OPTIONS: detect_leaks=1:halt_on_error=1",
    "UBSAN_OPTIONS: halt_on_error=1:print_stacktrace=1",
    "clang-format --dry-run --Werror",
    "is_owned_cpp",
    "tests/lua/official/\*",
    "src/\*\|tests/\*\|examples/\*\|benchmarks/\*\|lua_test/\*",
    "run_clang_tidy\.py --build-dir build/lint",
    "Linux runtime benchmark contract",
    'LuaTestBuildGitSha=\$\{\{ github\.sha \}\}',
    "Verify test binary build SHA",
    "check_test_binary_sha\.ps1",
    '-ExpectedSha \$env:GITHUB_SHA',
    "Run tests",
    "write_workflow_evidence\.py",
    "evidence-metadata\.json",
    "LUA_CPP_BUILD_BENCHMARKS=ON",
    "lua_runtime_bench",
    "benchmark-base-source",
    "benchmark-base",
    "benchmark-head",
    "run_runtime_bench_comparison\.ps1",
    "steps\.revisions\.outputs\.base_sha",
    "run_lua51_differential\.ps1",
    "run_lua51_official_slow\.ps1",
    "run_lua51_official_strict\.ps1",
    "tools\\check_doc_drift\.ps1",
    "tools\\run_quality_gate\.ps1",
    "bin\\lua_test\.exe"
)

Assert-FileTextMatches ".github/workflows/ci.yml" @(
    '(?ms)- name:\s*Verify test binary build SHA.*?check_test_binary_sha\.ps1.*?-ExpectedSha\s+\$env:GITHUB_SHA.*?- name:\s*Run tests',
    '(?ms)- name:\s*Upload coverage evidence.*?name:\s*component-coverage.*?if-no-files-found:\s*error',
    '(?ms)- name:\s*Upload benchmark evidence\s+if:\s*success\(\).*?name:\s*runtime-benchmark-evidence.*?if-no-files-found:\s*error',
    '(?ms)is_owned_cpp\(\).*?tests/lua/official/\*\)\s+return 1.*?src/\*\|tests/\*\|examples/\*\|benchmarks/\*\|lua_test/\*\)\s+return 0'
)

Assert-FileContains "src/core/userdata.cpp" @(
    'alignof\(std::max_align_t\) >= kUserdataAlignment',
    'std::malloc\(allocationSize\)'
)

Assert-FileNotContains "src/core/userdata.cpp" @(
    'aligned_alloc',
    '_aligned_malloc'
)

Assert-FileContains "benchmarks/runtime_bench.cpp" @(
    'countVmInstructionHook\(lua_State\*, lua_Debug\*\)',
    'lua_sethook\(state_, countVmInstructionHook, LUA_MASKCOUNT, 1\)',
    'InstructionCountScope counter\(state\)'
)

Assert-FileContains "tools/run_clang_tidy.py" @(
    "compile_commands\.json",
    "PROJECT_ROOTS",
    "bugprone-implicit-widening-of-multiplication-result",
    "bugprone-suspicious-stringview-data-usage",
    "portability-\*",
    "-portability-avoid-pragma-once",
    "warnings-as-errors=\*"
)

Assert-FileContains "CMakeLists.txt" @(
    "project\(lua_cpp VERSION 0\.1\.0",
    "LUA_CPP_BUILD_SHARED",
    "exported_symbols_list",
    "LUA_CPP_SANITIZER",
    "-fsanitize=\$\{LUA_CPP_SANITIZER\}",
    "-fno-omit-frame-pointer",
    "LUA_CPP_BUILD_BENCHMARKS",
    "add_executable\(lua_runtime_bench",
    "NAME runtime_benchmark_contract",
    "lua_embedding_example",
    "example_embedding",
    "lua51_public_api_contract",
    "NAME cmake_package_consumer",
    "NAME release_package_consumer_contract",
    "NAME lua_test_build_sha_contract",
    "check_test_binary_sha\.py",
    "--source-dir",
    "--git-executable",
    "NAME test_binary_sha_checker_contract",
    "test_check_test_binary_sha\.py",
    "NAME release_evidence_verifier_contract",
    "NAME workflow_evidence_writer_contract",
    "NAME test_binary_sha_contract",
    "NAME test_signal_integrity_check",
    "NAME test_signal_integrity_contract",
    "install\(EXPORT LuaCppTargets",
    "install\(FILES LICENSE",
    "configure_package_config_file",
    "write_basic_package_version_file",
    "COMPATIBILITY SameMinorVersion",
    "lua_public_native_module_host",
    "lua_public_native_module_app",
    "lua_public_native_module_embedding",
    "production_worker_success",
    "production_worker_instruction_limit",
    "production_worker_resource_limit",
    "production_worker_allocator_limit",
    "production_worker_json_encoding",
    "verify_worker_json\.py"
)

Assert-FileContains "cmake/ResolveTestBuildGitSha.cmake" @(
    "LUA_TEST_BUILD_GIT_SHA=<40-hex-sha>",
    "Git was not found",
    "must be exactly",
    "40 hexadecimal characters",
    "message\(FATAL_ERROR"
)

Assert-FileContains "src/lua_cpp_version.h" @(
    'LUA_CPP_VERSION "0\.1\.0"',
    'LUA_CPP_ABI_VERSION 0'
)

Assert-FileContains "tests/packaging/consumer/CMakeLists.txt" @(
    'project\(lua_cpp_package_consumer LANGUAGES C CXX\)',
    'LUA_CPP_PACKAGE_ROOT is required',
    'find_package\(LuaCpp 0\.1 CONFIG REQUIRED',
    'NO_DEFAULT_PATH',
    'LuaCpp::Lua',
    'LuaCpp::Shared'
)

Assert-FileContains "src/lib/iolib.cpp" @(
    'LuaString line\(LuaStdAllocator<char>',
    'LuaString buffer\(LuaStdAllocator<char>',
    'LuaString content\(LuaStdAllocator<char>',
    'LuaString token\(LuaStdAllocator<char>'
)

Assert-FileNotContains "src/lib/iolib.cpp" @(
    'std::string line;',
    'std::string buffer;',
    'std::string content;',
    'std::string token;'
)

Assert-FileContains "tools/check_runtime_bench.ps1" @(
    'schema_version must be 1',
    'numeric CI evidence must come from a Release build',
    'closure_count must remain exactly 100000',
    'gc_pause_sample_count',
    'gc_pause_p50_us',
    'gc_pause_p95_us',
    'gc_pause_p99_us',
    'gc_pause_max_us',
    'heap_growth_bytes_per_million_frames',
    'allocator_live_after_close',
    'check_runtime_bench_absolute_slo\.ps1'
)

Assert-FileContains "tools/check_runtime_bench_absolute_slo.ps1" @(
    'Runtime benchmark absolute SLO failed',
    'policy\.schemaVersion',
    'metric.*below minimum',
    'metric.*above maximum'
)

Assert-FileContains "tools/run_runtime_bench_comparison.ps1" @(
    'schemaVersion -ne 3',
    'minimumRunsPerRevision',
    'confirmationRunsPerRevision',
    'maximumRunsPerRevision',
    'runtimeInputDiffPaths',
    'runtimeInputsEquivalent',
    'comparison-initial\.json',
    'confirmationTriggered',
    'confirmationRecommended',
    '@\("base", "head"\)',
    '@\("head", "base"\)',
    'check_runtime_bench\.ps1',
    'check_runtime_bench_comparison\.ps1',
    'run-order\.json',
    'comparison\.json'
)

Assert-FileContains "tools/check_runtime_bench_comparison.ps1" @(
    'alternating-base-head-on-the-same-runner',
    'runnerPid',
    'gc_pause_p99_us',
    'median-of-run-medians',
    'median-of-paired-run-regressions',
    'pooled-nearest-rank-p99',
    'confirm-mixed-paired-threshold-outcomes',
    'maximumRegressionRatio',
    'regressionRatio',
    'pairedOutcomeMixed',
    'confirmationTriggered',
    'confirmationRecommended',
    'equivalent-runtime-inputs',
    'success\s*=\s*\$effectiveFailures\.Count -eq 0'
)

Assert-FileContains "tests/compatibility/runtime-benchmark-regression-policy.json" @(
    '"schemaVersion": 3',
    '"minimumRunsPerRevision": 3',
    '"confirmationRunsPerRevision": 2',
    '"maximumRunsPerRevision": 5',
    '"sampleAggregation": "median-of-run-medians"',
    '"regressionAggregation": "median-of-paired-run-regressions"',
    '"gcPauseAggregation": "pooled-nearest-rank-p99"',
    '"noisePolicy": "confirm-mixed-paired-threshold-outcomes"',
    '"runtimeInputPaths"',
    '"vm_instructions_per_second"',
    '"cpp_to_lua_ns_per_call"',
    '"lua_to_cpp_ns_per_call"',
    '"coroutine_resume_yield_ns"',
    '"closure_upvalue_lifecycle_per_second"',
    '"gc_pause_p99_us"'
)

Assert-FileContains "tests/coverage/component-thresholds.json" @(
    '"schemaVersion":\s*1',
    '"bytecode_verifier"',
    '"c_api"',
    '"gc_phases"',
    '"opcode_handlers"',
    '"parser_codegen"',
    '"sandbox_denied_paths"'
)

Assert-FileContains ".github/workflows/nightly.yml" @(
    "Nightly endurance",
    'nightly-endurance-\$\{\{ github\.event_name \}\}-\$\{\{ github\.ref \}\}',
    "cancel-in-progress: true",
    "runtime-soak",
    "lua_runtime_soak",
    "native_module_iterations",
    "Validate endurance inputs",
    "SOAK_MINUTES >= 45",
    "SOAK_MINUTES <= 80",
    "NATIVE_MODULE_ITERATIONS >= 1000",
    "long-fuzz",
    "timeout-minutes: 160",
    "Validate fuzz input",
    "FUZZ_SECONDS_PER_TARGET >= 600",
    "FUZZ_SECONDS_PER_TARGET <= 1200",
    "max_total_time",
    "write_workflow_evidence\.py",
    "evidence-metadata\.json",
    "runtime-soak-evidence",
    "long-fuzz-evidence",
    "worker-fault-matrix",
    "verify_worker_fault_matrix\.py",
    "--enforce-process-limits",
    "--candidate-sha",
    "worker-fault-matrix\.json"
)

Assert-FileTextMatches ".github/workflows/nightly.yml" @(
    '(?ms)- name:\s*Upload endurance evidence\s+if:\s*success\(\).*?name:\s*runtime-soak-evidence.*?if-no-files-found:\s*error',
    '(?ms)- name:\s*Upload fuzz evidence\s+if:\s*success\(\).*?name:\s*long-fuzz-evidence.*?if-no-files-found:\s*error',
    '(?ms)- name:\s*Upload worker fault evidence\s+if:\s*success\(\).*?name:\s*worker-fault-matrix-\$\{\{ matrix\.artifact_suffix \}\}.*?if-no-files-found:\s*error'
)

Assert-FileContains ".github/workflows/release.yml" @(
    "Release candidate packages",
    "LUA_RELEASE_GOVERNANCE_ATTESTATION",
    "Verify exact-SHA release evidence",
    "actions: read",
    "test_verify_release_governance\.py",
    "test_verify_source_readiness_evidence\.py",
    "test_verify_release_evidence\.py",
    "test_build_release_body\.py",
    "verify_release_governance\.py",
    "verify_release_evidence\.py",
    "governance-evidence\.json",
    "source-readiness-evidence\.json",
    "name: release-evidence",
    "needs: \[governance, verify_evidence\]",
    "needs: \[governance, packages, consume_packages, verify_evidence, verify_tag\]",
    "check_release_readiness\.ps1",
    "package_release\.ps1",
    "Downloaded SDK consumer",
    "Deep-validate downloaded package",
    "Build and run isolated static and shared consumers",
    "verify_release_package_consumer\.py",
    "Publish immutable candidate",
    "release-evidence\.json",
    "build_release_body\.py",
    "release-body\.md",
    "gh run download",
    "sha256sum --",
    "release create"
)

Assert-FileNotContains ".github/workflows/release.yml" @(
    "LUA_RELEASE_GOVERNANCE_APPROVED"
)

Assert-FileTextMatches ".github/workflows/release.yml" @(
    '(?ms)- name:\s*Checkout candidate.*?ref:\s*\$\{\{\s*needs\.verify_evidence\.outputs\.candidate_sha\s*\}\}',
    '(?ms)- name:\s*Checkout release notes.*?ref:\s*\$\{\{\s*needs\.verify_evidence\.outputs\.candidate_sha\s*\}\}',
    '(?ms)package_release\.ps1.*?-Commit\s+\$\{\{\s*needs\.verify_evidence\.outputs\.candidate_sha\s*\}\}',
    '(?ms)verify_tag:\s*.*?if:\s*github\.event_name == ''push'' && startsWith\(github\.ref, ''refs/tags/''\)',
    '(?ms)publish:\s*.*?if:\s*github\.event_name == ''push'' && startsWith\(github\.ref, ''refs/tags/''\)'
)

Assert-FileContains "tools/verify_release_governance.py" @(
    "lua-cpp\.release-governance-attestation/v1",
    "lua-cpp\.release-governance-evidence/v1",
    "approved_by",
    "independent_reviewer",
    "time-limited-waiver",
    "protected-ruleset",
    "candidate-only",
    "CONTROL_NAMES"
)

Assert-FileContains "tools/verify_source_readiness_evidence.py" @(
    "lua-cpp\.source-readiness-evidence/v1",
    "candidate_sha",
    "project_version",
    "abi_version",
    "public_api_contract"
)

Assert-FileContains "tools/release_identity.psm1" @(
    "LUA_CPP_VERSION_MAJOR",
    "LUA_CPP_VERSION_MINOR",
    "LUA_CPP_VERSION_PATCH",
    "LUA_CPP_VERSION",
    "LUA_CPP_ABI_VERSION",
    "SOVERSION",
    "Get-LuaCppReleaseIdentity"
)

Assert-FileContains "tools/check_release_readiness.ps1" @(
    "Get-LuaCppReleaseIdentity",
    "ExpectedCommit",
    "EvidenceRepository",
    "lua-cpp\.source-readiness-evidence/v1",
    "abi_version"
)

Assert-FileContains "tools/build_release_body.py" @(
    "require_approved=True",
    "expired at release publication",
    "source_readiness",
    "abi_version",
    "expected_version"
)

Assert-FileContains "tools/package_release.ps1" @(
    "generate_sbom\.py",
    "validate_release_artifacts\.py",
    "verify_release_package_consumer\.py",
    "SHA256SUMS",
    "schemaVersion",
    "rev-parse HEAD",
    "Commit.*does not match repository HEAD",
    "status --porcelain",
    "Release packaging requires a clean worktree"
)

Assert-FileContains "CMakeLists.txt" @(
    "WriteBuildProvenance\.cmake",
    "lua-cpp-build-provenance\.json",
    "build_provenance_cmake_contract",
    "package_build_provenance_contract",
    "release_body_consumer_contract",
    "release_governance_verifier_contract",
    "source_readiness_evidence_contract",
    "release_identity_contract",
    "LUA_CPP_ABI_VERSION",
    'SOVERSION \$\{LUA_CPP_ABI_VERSION\}'
)

Assert-FileContains "cmake/BuildProvenance.json.in" @(
    "lua-cpp\.build-provenance/v2",
    '"system_name"',
    '"system_processor"',
    '"pointer_size": @_pointer_size_json@'
)

Assert-FileContains "cmake/WriteBuildProvenance.cmake" @(
    "CMAKE_SYSTEM_NAME",
    "CMAKE_SYSTEM_PROCESSOR",
    "CMAKE_SIZEOF_VOID_P",
    "_pointer_size_json"
)

Assert-FileContains "tests/cmake/test_build_provenance.cmake" @(
    "lua-cpp\.build-provenance/v2",
    "LUA_CPP_EXPECTED_SYSTEM_NAME",
    "LUA_CPP_EXPECTED_SYSTEM_PROCESSOR",
    "LUA_CPP_EXPECTED_POINTER_SIZE",
    "_pointer_size_kind.*NUMBER"
)

Assert-FileContains "tools/build_provenance.psm1" @(
    "lua-cpp\.build-provenance/v2",
    "source_git_status",
    "source_git_sha",
    "system_name",
    "system_processor",
    "pointer_size",
    "build_type",
    "configuration_types",
    "windows-x64",
    "linux-x64",
    "macos-arm64",
    "Unsupported RuntimeIdentifier",
    "source directory does not match",
    "binary directory does not match",
    "Invoke-LuaCppReleaseBuildRefresh",
    "--clean-first",
    "worktree dirty"
)

Assert-FileContains "tools/package_release.ps1" @(
    "build_provenance\.psm1",
    "Test-LuaCppBuildProvenance",
    "Invoke-LuaCppReleaseBuildRefresh",
    '-Configuration\s+\$Configuration',
    '-RuntimeIdentifier\s+\$RuntimeIdentifier',
    'windows-x64',
    'linux-x64',
    'macos-arm64',
    'Unsupported RuntimeIdentifier'
)

Assert-FileNotContains "tools/package_release.ps1" @(
    'RuntimeIdentifier\s+-notmatch'
)

Assert-FileContains "tools/test_package_build_provenance.ps1" @(
    "visual_studio_environment\.psm1",
    "Import-LuaCppVisualStudioDeveloperEnvironment",
    "Visual Studio explicit x64 provenance",
    "Linux RID impersonating a Windows target",
    "macOS RID impersonating a Windows target",
    "32-bit build as x64",
    "processor alias suffix impersonation",
    "missing target identity field",
    "forged pointer-size type",
    "legacy v1 provenance shape"
)

Assert-FileContains "tools/visual_studio_environment.psm1" @(
    "Find-LuaCppVisualStudioInstallation",
    "Import-LuaCppVisualStudioDeveloperEnvironment",
    "Microsoft\.VisualStudio\.Component\.VC\.Tools\.x86\.x64",
    "Common7/Tools/VsDevCmd\.bat",
    "CMake is not bundled under a Visual Studio Common7 directory"
)

Assert-FileContains "tools/test_visual_studio_environment.ps1" @(
    "standalone CMake with a vswhere-discovered Visual Studio",
    "vswhere priority over bundled CMake inference",
    "bundled CMake fallback",
    "standalone CMake without vswhere"
)

Assert-FileContains "tools/build_release_body.py" @(
    "payload_evidence",
    "required_timed_steps",
    "authoritative_runtime_inputs",
    "metric_regressions_recomputed",
    "observed_failure_metrics",
    "absolute_slo",
    "base_ancestry",
    "release checksum file set mismatch"
)

Assert-FileContains "tools/validate_release_artifacts.py" @(
    "archive member uses an unsafe path",
    "release checksum manifest does not exactly match",
    "external SBOM is not byte-identical",
    "SBOM file set mismatch"
)

Assert-FileContains "tools/verify_release_package_consumer.py" @(
    "NO_PACKAGE_REGISTRY:BOOL=TRUE",
    "CMAKE_PREFIX_PATH:STRING=",
    "--clean-first",
    "--show-only=json-v1",
    "downloaded package consumer test set mismatch",
    "consumer resolved LuaCpp outside the extracted package"
)

Assert-FileContains "tools/test_verify_release_package_consumer.py" @(
    "path traversal archive",
    "symlink archive",
    "escaped CMake cache binding",
    "missing shared consumer test",
    "consumer build failure",
    "consumer runtime test failure"
)

function Invoke-RuntimeBenchmarkComparisonSmokeTest {
    $tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) `
        ("lua_runtime_benchmark_comparison_" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $tempRoot | Out-Null
    try {
        function New-SyntheticBenchmarkReport {
            param(
                [string]$Sha,
                [double]$CppToLua
            )

            return [ordered]@{
                schema_version      = 1
                success             = $true
                profile             = "ci"
                build_type          = "Release"
                git_sha             = $Sha
                compiler            = "synthetic-compiler"
                os                  = "synthetic-os"
                workload            = [ordered]@{ timing_samples = 3; closure_samples = 1 }
                gc_pause_samples_us = @(1.0, 2.0, 3.0, 4.0, 5.0)
                metrics             = @(
                    [ordered]@{ name = "vm_instructions_per_second"; direction = "higher"; samples = @(100.0, 100.0, 100.0) },
                    [ordered]@{ name = "cpp_to_lua_ns_per_call"; direction = "lower"; samples = @($CppToLua, $CppToLua, $CppToLua) },
                    [ordered]@{ name = "lua_to_cpp_ns_per_call"; direction = "lower"; samples = @(100.0, 100.0, 100.0) },
                    [ordered]@{ name = "coroutine_resume_yield_ns"; direction = "lower"; samples = @(100.0, 100.0, 100.0) },
                    [ordered]@{ name = "closure_upvalue_lifecycle_per_second"; direction = "higher"; samples = @(100.0) },
                    [ordered]@{ name = "gc_pause_p99_us"; direction = "lower"; samples = @(5.0) }
                )
            }
        }

        $allBasePaths = @()
        $allHeadPaths = @()
        for ($run = 1; $run -le 5; $run++) {
            $basePath = Join-Path $tempRoot "base-$run.json"
            $headPath = Join-Path $tempRoot "head-$run.json"
            New-SyntheticBenchmarkReport -Sha "base-sha" -CppToLua 100.0 |
                ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $basePath -Encoding utf8
            New-SyntheticBenchmarkReport -Sha "head-sha" -CppToLua 100.0 |
                ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $headPath -Encoding utf8
            $allBasePaths += $basePath
            $allHeadPaths += $headPath
        }

        function Set-SyntheticCppToLuaValues {
            param(
                [double[]]$BaseValues,
                [double[]]$HeadValues
            )

            if ($BaseValues.Count -ne $HeadValues.Count) {
                throw "synthetic base/head value counts differ"
            }
            for ($run = 0; $run -lt $BaseValues.Count; $run++) {
                foreach ($revision in @("base", "head")) {
                    $isBase = $revision -eq "base"
                    $value = if ($isBase) { $BaseValues[$run] } else { $HeadValues[$run] }
                    $sha = if ($isBase) { "base-sha" } else { "head-sha" }
                    $report = New-SyntheticBenchmarkReport -Sha $sha -CppToLua $value
                    $path = if ($isBase) { $allBasePaths[$run] } else { $allHeadPaths[$run] }
                    $report | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $path -Encoding utf8
                }
            }
        }

        $manifestPath = Join-Path $tempRoot "run-order.json"
        function Write-SyntheticManifest {
            param(
                [int]$PairCount,
                [bool]$RuntimeInputsEquivalent
            )

            $runs = @()
            for ($pair = 0; $pair -lt $PairCount; $pair++) {
                $order = if (($pair % 2) -eq 0) { @("base", "head") } else { @("head", "base") }
                foreach ($revision in $order) {
                    $isBase = $revision -eq "base"
                    $startedAt = [DateTimeOffset]::FromUnixTimeSeconds(2 * $runs.Count).UtcDateTime
                    $runs += [ordered]@{
                        pair       = $pair
                        revision   = $revision
                        sha        = if ($isBase) { "base-sha" } else { "head-sha" }
                        resultPath = if ($isBase) { $allBasePaths[$pair] } else { $allHeadPaths[$pair] }
                        startedAt  = $startedAt.ToString("o")
                        endedAt    = $startedAt.AddSeconds(1).ToString("o")
                    }
                }
            }
            [ordered]@{
                schemaVersion            = 2
                runnerPid               = $PID
                baseSha                 = "base-sha"
                headSha                 = "head-sha"
                runtimeInputPaths        = @("CMakeLists.txt", "cmake", "src")
                runtimeInputDiffPaths    = if ($RuntimeInputsEquivalent) { @() } else { @("src/runtime.cpp") }
                runtimeInputsEquivalent = $RuntimeInputsEquivalent
                confirmationTriggered   = $PairCount -gt 3
                runs                     = $runs
            } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifestPath -Encoding utf8
        }

        $comparisonPath = Join-Path $tempRoot "comparison.json"
        $basePaths = @($allBasePaths[0..2])
        $headPaths = @($allHeadPaths[0..2])
        Set-SyntheticCppToLuaValues -BaseValues @(100.0, 100.0, 100.0) `
            -HeadValues @(100.0, 100.0, 100.0)
        Write-SyntheticManifest -PairCount 3 -RuntimeInputsEquivalent $false
        & (Join-RepoPath "tools/check_runtime_bench_comparison.ps1") `
            -BaseResultPath $basePaths `
            -HeadResultPath $headPaths `
            -RunManifestPath $manifestPath `
            -PolicyPath (Join-RepoPath "tests/compatibility/runtime-benchmark-regression-policy.json") `
            -OutputPath $comparisonPath
        $comparison = Get-Content -Raw -LiteralPath $comparisonPath | ConvertFrom-Json
        if ($comparison.schemaVersion -ne 3 -or $comparison.success -ne $true -or
            $comparison.decision -ne "thresholds-passed" -or
            $comparison.regressionAggregation -ne "median-of-paired-run-regressions" -or
            $comparison.metrics.Count -ne 6) {
            throw "base-vs-head benchmark checker rejected stable synthetic evidence"
        }

        $baseMedians = @(100.0, 200.0, 300.0)
        $headMedians = @(90.0, 300.0, 270.0)
        Set-SyntheticCppToLuaValues -BaseValues $baseMedians -HeadValues $headMedians
        & (Join-RepoPath "tools/check_runtime_bench_comparison.ps1") `
            -BaseResultPath $basePaths `
            -HeadResultPath $headPaths `
            -RunManifestPath $manifestPath `
            -PolicyPath (Join-RepoPath "tests/compatibility/runtime-benchmark-regression-policy.json") `
            -OutputPath $comparisonPath
        $comparison = Get-Content -Raw -LiteralPath $comparisonPath | ConvertFrom-Json
        $cppToLua = $comparison.metrics | Where-Object { $_.name -eq "cpp_to_lua_ns_per_call" }
        if ($comparison.success -ne $true -or [Math]::Abs($cppToLua.regressionRatio + 0.1) -gt 0.000001 -or
            $cppToLua.pairedRunCount -ne 3 -or $cppToLua.pairedRegressionRatios.Count -ne 3 -or
            $comparison.confirmationRecommended -ne $false) {
            throw "base-vs-head benchmark checker did not preserve paired comparison under runner drift"
        }

        Set-SyntheticCppToLuaValues -BaseValues @(100.0, 100.0, 100.0) `
            -HeadValues @(100.0, 170.0, 137.0)
        Write-SyntheticManifest -PairCount 3 -RuntimeInputsEquivalent $false
        & (Join-RepoPath "tools/check_runtime_bench_comparison.ps1") `
            -BaseResultPath $basePaths `
            -HeadResultPath $headPaths `
            -RunManifestPath $manifestPath `
            -PolicyPath (Join-RepoPath "tests/compatibility/runtime-benchmark-regression-policy.json") `
            -OutputPath $comparisonPath `
            -DoNotThrowOnRegression
        $comparison = Get-Content -Raw -LiteralPath $comparisonPath | ConvertFrom-Json
        $cppToLua = $comparison.metrics | Where-Object { $_.name -eq "cpp_to_lua_ns_per_call" }
        if ($comparison.success -ne $false -or $comparison.decision -ne "confirmation-required" -or
            $comparison.confirmationRecommended -ne $true -or
            $comparison.mixedFailingMetrics -notcontains "cpp_to_lua_ns_per_call" -or
            $cppToLua.pairedRunsWithinLimit -ne 1 -or $cppToLua.pairedRunsOverLimit -ne 2) {
            throw "base-vs-head benchmark checker did not request confirmation for asymmetric noise"
        }

        Write-SyntheticManifest -PairCount 3 -RuntimeInputsEquivalent $true
        & (Join-RepoPath "tools/check_runtime_bench_comparison.ps1") `
            -BaseResultPath $basePaths `
            -HeadResultPath $headPaths `
            -RunManifestPath $manifestPath `
            -PolicyPath (Join-RepoPath "tests/compatibility/runtime-benchmark-regression-policy.json") `
            -OutputPath $comparisonPath
        $comparison = Get-Content -Raw -LiteralPath $comparisonPath | ConvertFrom-Json
        if ($comparison.success -ne $true -or $comparison.decision -ne "equivalent-runtime-inputs" -or
            $comparison.runtimeInputsEquivalent -ne $true -or
            $comparison.observedThresholdFailures.Count -lt 1 -or $comparison.failures.Count -ne 0) {
            throw "equivalent runtime inputs did not deterministically neutralize hosted-runner noise"
        }

        Set-SyntheticCppToLuaValues -BaseValues @(100.0, 100.0, 100.0, 100.0, 100.0) `
            -HeadValues @(100.0, 170.0, 137.0, 100.0, 100.0)
        $basePaths = @($allBasePaths)
        $headPaths = @($allHeadPaths)
        Write-SyntheticManifest -PairCount 5 -RuntimeInputsEquivalent $false
        & (Join-RepoPath "tools/check_runtime_bench_comparison.ps1") `
            -BaseResultPath $basePaths `
            -HeadResultPath $headPaths `
            -RunManifestPath $manifestPath `
            -PolicyPath (Join-RepoPath "tests/compatibility/runtime-benchmark-regression-policy.json") `
            -OutputPath $comparisonPath
        $comparison = Get-Content -Raw -LiteralPath $comparisonPath | ConvertFrom-Json
        $cppToLua = $comparison.metrics | Where-Object { $_.name -eq "cpp_to_lua_ns_per_call" }
        if ($comparison.success -ne $true -or $comparison.runsPerRevision -ne 5 -or
            [Math]::Abs($cppToLua.regressionRatio) -gt 0.000001 -or
            $comparison.confirmationTriggered -ne $true -or
            $comparison.confirmationRecommended -ne $false) {
            throw "confirmation pairs did not resolve the observed asymmetric-noise pattern"
        }

        Set-SyntheticCppToLuaValues -BaseValues @(100.0, 100.0, 100.0) `
            -HeadValues @(130.0, 130.0, 130.0)
        $basePaths = @($allBasePaths[0..2])
        $headPaths = @($allHeadPaths[0..2])
        Write-SyntheticManifest -PairCount 3 -RuntimeInputsEquivalent $false
        $rejected = $false
        try {
            & (Join-RepoPath "tools/check_runtime_bench_comparison.ps1") `
                -BaseResultPath $basePaths `
                -HeadResultPath $headPaths `
                -RunManifestPath $manifestPath `
                -PolicyPath (Join-RepoPath "tests/compatibility/runtime-benchmark-regression-policy.json") `
                -OutputPath $comparisonPath
        } catch {
            $rejected = $_.Exception.Message -match "cpp_to_lua_ns_per_call"
        }
        if (-not $rejected) {
            throw "base-vs-head benchmark checker accepted a 30% C++ to Lua regression"
        }
        $comparison = Get-Content -Raw -LiteralPath $comparisonPath | ConvertFrom-Json
        if ($comparison.success -ne $false -or $comparison.confirmationRecommended -ne $false -or
            $comparison.decision -ne "thresholds-failed") {
            throw "persistent benchmark regression did not emit terminal failure evidence"
        }
    } finally {
        if (Test-Path -LiteralPath $tempRoot) {
            Remove-Item -LiteralPath $tempRoot -Recurse -Force
        }
    }
}

Invoke-RuntimeBenchmarkComparisonSmokeTest

Assert-FileContains "examples/embedding.cpp" @(
    "luaL_loadbuffer",
    "lua_pcall",
    "host_double",
    "embedding result: 42"
)

Assert-FileContains "tools/check_lua51_official_sources.ps1" @(
    "Get-FileHash",
    "SHA256",
    "hash mismatch",
    "unexpected:",
    "lua5.1-tests.tar.gz",
    "lua-5.1.5.tar.gz",
    "nestedPrototypeOpcodes",
    "Lua 5.1.5 luac oracle verified"
)

Assert-FileContains "tools/run_lua51_official_strict.ps1" @(
    "unmodified temporary copy",
    "timeoutSeconds",
    "stageProfile",
    "ReadLineAsync",
    "processElapsedSeconds",
    "completionSentinelPresent",
    "final OK !!!",
    "elapsedUntilNextStageSeconds",
    "check_lua51_official_sources"
)

Assert-FileNotContains "tools/run_lua51_official_strict.ps1" @(
    "ExpectFailure",
    "XfailManifest",
    "XFAIL"
)

Assert-FileContains "tools/run_lua51_official_slow.ps1" @(
    "TimeoutSeconds",
    "check_lua51_official_sources"
)

Assert-FileContains ".github/workflows/ci.yml" @(
    "Official slow verybig gate",
    "-Case verybig",
    "-TimeoutSeconds 300",
    "Official strict evidence",
    "matrix.build_type == 'Release'"
)

Assert-FileNotContains ".github/workflows/ci.yml" @(
    "Official slow verybig XFAIL",
    "-ExpectVerybigTimeout",
    "-ExpectFailure"
)

Assert-FileContains "tools/run_lua51_differential.ps1" @(
    "ReferenceLua",
    "CandidateLua",
    "exitStatus",
    "stdout",
    "stderr",
    "lua51-differential-cases.json"
)

Assert-FileContains "tests/compatibility/lua51-differential-cases.json" @(
    '"returnValueTypes"',
    '"errorCategory"',
    '"gcObservableSideEffect"'
)

Assert-FileContains "tests/compatibility/lua51-official-strict-xfails.json" @(
    '"schemaVersion":\s*1',
    '"channel":\s*"official-strict"',
    '"xfails":\s*\[\s*\]'
)

$strictXfails = Get-Content -LiteralPath (Join-RepoPath "tests/compatibility/lua51-official-strict-xfails.json") `
    -Raw | ConvertFrom-Json
$strictXfailEntries = @($strictXfails.xfails)
if ($strictXfailEntries.Count -ne 0) {
    throw "official-strict must remain a required PASS with no accepted XFAIL entries"
}

Assert-FileContains "tests/compatibility/lua51-official-slow-xfails.json" @(
    '"schemaVersion":\s*1',
    '"channel":\s*"official-slow"',
    '"xfails":\s*\[\s*\]'
)

Assert-FileNotContains "tests/compatibility/lua51-official-slow-xfails.json" @(
    'verybig\.lua-slow-gate',
    'P1-verybig-runtime-budget',
    '"expectedOutcome":\s*"timeout"'
)

$slowXfails = Get-Content -LiteralPath (Join-RepoPath "tests/compatibility/lua51-official-slow-xfails.json") `
    -Raw | ConvertFrom-Json
if (@($slowXfails.xfails).Count -ne 0) {
    throw "official-slow must not retain XFAIL entries after sort.lua and verybig.lua pass the required gate"
}

Assert-FileContains "tests/compatibility/lua51-official-testc-xfails.json" @(
    '"schemaVersion":\s*1',
    '"channel":\s*"official-testc"',
    '"xfails":\s*\[\s*\]'
)

Assert-FileNotContains "tests/compatibility/lua51-official-testc-xfails.json" @(
    'api.lua-stack-shape',
    'code.lua-opcode-sequence',
    'code.lua-lua515-compiler-parity'
)

$testCXfails = Get-Content -LiteralPath (Join-RepoPath "tests/compatibility/lua51-official-testc-xfails.json") `
    -Raw | ConvertFrom-Json
$testCXfailEntries = @($testCXfails.xfails)
if ($testCXfailEntries.Count -ne 0) {
    throw "official-testc must remain a required PASS with no accepted XFAIL entries"
}

Assert-FileContains "tests/unit/official/test_official_suite.cpp" @(
    'applyLua515CodeLuaOracle',
    "end, 'LOADNIL', 'LOADNIL', 'RETURN'",
    'runOfficialTestCScript\("code\.lua", true\)',
    'runOfficialTestCScript\("api\.lua"\)',
    'ASSERT_TRUE\(suite, result\.ok, result\.message\)',
    '"code\.lua with Lua 5\.1\.5 oracle"',
    '"api\.lua with T module"'
)

Assert-FileNotContains "tests/unit/official/test_official_suite.cpp" @(
    ':21: assertion failed',
    'code.lua with T module XFAIL',
    '"api\.lua with T module XFAIL"'
)

Assert-FileContains "tests/compatibility/lua51-official-smoke-deviations.json" @(
    '"strictChannelAllowsSourceRewrites":\s*false',
    'constructs-stress-cap',
    'closure-pressure-cap',
    'gc-wait-caps'
)

Assert-FileContains "README.md" @(
    "clang-format",
    "clang-tidy",
    "GitHub Actions",
    "tools/run_quality_gate\.ps1",
    "f04c890d80a89739eb8dc28ddaeb1ae5e5993273",
    "actions/runs/31661881457",
    "actions/runs/31663816824"
)

Assert-FileNotContains "README.md" @(
    "d8fff0165d46cbf1bcaab692d9fe2b16b4de68f8",
    "actions/runs/31605865677"
)

Assert-FileTextMatches "assessment.md" @(
    '(?m)^baseline_sha: f04c890d80a89739eb8dc28ddaeb1ae5e5993273\r?$',
    '(?m)^last_pushed_checkpoint_sha: f04c890d80a89739eb8dc28ddaeb1ae5e5993273\r?$',
    '(?m)^candidate_sha: pending\r?$'
)

Assert-FileTextMatches "task.md" @(
    '(?m)^candidate_sha: pending\r?$',
    '(?m)^candidate_tree: pending\r?$',
    '(?m)^evidence_baseline_sha: f04c890d80a89739eb8dc28ddaeb1ae5e5993273\r?$'
)

Assert-FileContains "assessment.md" @(
    "actions/runs/31661881457",
    "actions/runs/31663816824",
    "codex/v0\.1\.0-rc1-quality-gate"
)

Assert-FileTextMatches "assessment.md" @(
    '(?m)^\| Nightly \|.*scheduled run.*0.*\|$'
)

Assert-FileNotContains "assessment.md" @(
    "disabled_manually"
)

Assert-FileTextMatches "docs/release/rc-notes-0.1.0.md" @(
    '(?s)17-job.*?scheduled Nightly.*?LLVM 22.*?GitHub Release'
)

Assert-FileNotContains "docs/release/rc-notes-0.1.0.md" @(
    "disabled_manually"
)

Write-Host "[OK] quality gate configuration tests passed"
