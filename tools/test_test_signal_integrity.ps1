param(
    [string]$Checker = (Join-Path $PSScriptRoot "check_test_signal_integrity.ps1")
)

$ErrorActionPreference = "Stop"

function Write-Utf8Text {
    param(
        [string]$Path,
        [string]$Text
    )

    $parent = Split-Path -Parent $Path
    if (-not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    [System.IO.File]::WriteAllText($Path, $Text, [System.Text.UTF8Encoding]::new($false))
}

function Get-TextHash {
    param([string]$Text)

    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text.Trim())
        return ([System.BitConverter]::ToString($sha256.ComputeHash($bytes))).Replace("-", "").ToLowerInvariant()
    } finally {
        $sha256.Dispose()
    }
}

function Get-LineNumber {
    param(
        [string]$Text,
        [string]$Needle
    )

    $index = $Text.IndexOf($Needle, [System.StringComparison]::Ordinal)
    if ($index -lt 0) {
        throw "Fixture text is missing expected needle: $Needle"
    }
    return ([regex]::Matches($Text.Substring(0, $index), "`n")).Count + 1
}

function New-AllowlistEntry {
    param(
        [string]$Path,
        [int]$Line,
        [string]$Invocation,
        [string]$ExpiresOn
    )

    return [ordered]@{
        rule = "unconditional-assert-true"
        path = $Path
        line = $Line
        textHash = Get-TextHash $Invocation
        rationale = "Fixture compile-time probe publishes its preceding static_assert result."
        expiresOn = $ExpiresOn
    }
}

function Initialize-Fixture {
    param(
        [string]$FixtureRoot,
        [string]$Source,
        [object[]]$Entries = @()
    )

    Write-Utf8Text -Path (Join-Path $FixtureRoot "tests\unit\fixture.cpp") -Text $Source
    $allowlist = [ordered]@{
        schemaVersion = 1
        entries = @($Entries)
    }
    $json = $allowlist | ConvertTo-Json -Depth 8
    Write-Utf8Text -Path (Join-Path $FixtureRoot "tests\quality\test_signal_allowlist.json") `
        -Text ($json + "`n")
}

function Invoke-Checker {
    param([string]$FixtureRoot)

    $powerShell = (Get-Process -Id $PID).Path
    if (-not $powerShell -or -not (Test-Path -LiteralPath $powerShell -PathType Leaf)) {
        $powerShellCommand = Get-Command pwsh, powershell -ErrorAction SilentlyContinue |
            Select-Object -First 1
        if (-not $powerShellCommand) {
            throw "Unable to resolve the current PowerShell executable"
        }
        $powerShell = $powerShellCommand.Source
    }

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = & $powerShell -NoProfile -ExecutionPolicy Bypass -File $Checker `
            -Root $FixtureRoot -AllowlistPath "tests\quality\test_signal_allowlist.json" 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    return [pscustomobject]@{
        ExitCode = $exitCode
        Output = ($output | ForEach-Object { $_.ToString() }) -join "`n"
    }
}

function Assert-Succeeded {
    param(
        [pscustomobject]$Result,
        [string]$Scenario
    )

    if ($Result.ExitCode -ne 0) {
        throw "$Scenario should succeed, but exited $($Result.ExitCode):`n$($Result.Output)"
    }
}

function Assert-FailedWith {
    param(
        [pscustomobject]$Result,
        [string]$Pattern,
        [string]$Scenario
    )

    if ($Result.ExitCode -eq 0) {
        throw "$Scenario should fail"
    }
    if ($Result.Output -notmatch $Pattern) {
        throw "$Scenario failed without expected diagnostic '$Pattern':`n$($Result.Output)"
    }
}

$resolvedChecker = (Resolve-Path -LiteralPath $Checker).Path
$Checker = $resolvedChecker
$temporaryBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath())
$temporaryRoot = Join-Path $temporaryBase ("lua_test_signal_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null

try {
    $cleanSource = @'
void testRealSignal(TestSuite& suite) {
    // ASSERT_TRUE(suite, true, "commented placeholder");
    const char* fixtureText = R"cpp(
        std::cout << "[SKIP] fixture text";
        ASSERT_TRUE(suite, true, "raw fixture placeholder");
    )cpp";
    const bool completed = runFixture();
    ASSERT_TRUE(suite, completed && fixtureText != nullptr, "fixture operation completes");
}
'@
    $cleanRoot = Join-Path $temporaryRoot "clean"
    Initialize-Fixture -FixtureRoot $cleanRoot -Source $cleanSource
    Assert-Succeeded -Result (Invoke-Checker $cleanRoot) -Scenario "real assertion fixture"

    $expectedSkipSource = @'
void testOptionalLocale(TestSuite& suite) {
    SKIP_EXPECTED(suite, "locale collation", "Portuguese locale is unavailable");
}
'@
    $expectedSkipRoot = Join-Path $temporaryRoot "expected-skip"
    Initialize-Fixture -FixtureRoot $expectedSkipRoot -Source $expectedSkipSource
    Assert-Succeeded -Result (Invoke-Checker $expectedSkipRoot) -Scenario "explicit expected skip helper fixture"

    $allowedInvocation = 'ASSERT_TRUE(suite, true, "compile-time signature probe")'
    $allowedSource = @'
void testCompileTimeProbe(TestSuite& suite) {
    static_assert(sizeof(void*) >= sizeof(char*));
    ASSERT_TRUE(suite, true, "compile-time signature probe");
}
'@
    $allowedLine = Get-LineNumber -Text $allowedSource -Needle $allowedInvocation
    $allowedEntry = New-AllowlistEntry -Path "tests/unit/fixture.cpp" -Line $allowedLine `
        -Invocation $allowedInvocation -ExpiresOn "2999-12-31"
    $allowedRoot = Join-Path $temporaryRoot "allowed"
    Initialize-Fixture -FixtureRoot $allowedRoot -Source $allowedSource -Entries @($allowedEntry)
    Assert-Succeeded -Result (Invoke-Checker $allowedRoot) -Scenario "exact allowlist fixture"

    $violationSource = @'
void testFalseGreen(TestSuite& suite) {
    try {
        runFixture();
    } catch (...) {
        std::cout << "[SKIP] parser failed";
        std::cerr << "fixture was skipped after an exception";
        ASSERT_TRUE(suite, true, "exception was treated as success");
    }
}
'@
    $violationRoot = Join-Path $temporaryRoot "violation"
    Initialize-Fixture -FixtureRoot $violationRoot -Source $violationSource
    $violationResult = Invoke-Checker $violationRoot
    Assert-FailedWith -Result $violationResult -Pattern "unconditional-assert-true" `
        -Scenario "unconditional assertion fixture"
    Assert-FailedWith -Result $violationResult -Pattern "skip-output-as-success" `
        -Scenario "manual skip output fixture"
    $skipOutputViolations = ([regex]::Matches($violationResult.Output,
            "Unregistered skip-output-as-success")).Count
    if ($skipOutputViolations -ne 2) {
        throw "manual skip output fixture should report both [SKIP] and skipped forms, got $skipOutputViolations"
    }

    $expiredEntry = New-AllowlistEntry -Path "tests/unit/fixture.cpp" -Line $allowedLine `
        -Invocation $allowedInvocation -ExpiresOn "2000-01-01"
    $expiredRoot = Join-Path $temporaryRoot "expired"
    Initialize-Fixture -FixtureRoot $expiredRoot -Source $allowedSource -Entries @($expiredEntry)
    Assert-FailedWith -Result (Invoke-Checker $expiredRoot) -Pattern "expired on 2000-01-01" `
        -Scenario "expired allowlist fixture"

    $duplicateEntryA = New-AllowlistEntry -Path "tests/unit/fixture.cpp" -Line $allowedLine `
        -Invocation $allowedInvocation -ExpiresOn "2999-12-31"
    $duplicateEntryB = New-AllowlistEntry -Path "tests/unit/fixture.cpp" -Line $allowedLine `
        -Invocation $allowedInvocation -ExpiresOn "2999-12-31"
    $duplicateRoot = Join-Path $temporaryRoot "duplicate"
    Initialize-Fixture -FixtureRoot $duplicateRoot -Source $allowedSource `
        -Entries @($duplicateEntryA, $duplicateEntryB)
    Assert-FailedWith -Result (Invoke-Checker $duplicateRoot) -Pattern "Duplicate test signal allowlist entry" `
        -Scenario "duplicate allowlist fixture"

    $tamperedSource = $allowedSource.Replace(
        '"compile-time signature probe"',
        '"compile-time signature probe text was changed"'
    )
    $tamperedRoot = Join-Path $temporaryRoot "tampered"
    Initialize-Fixture -FixtureRoot $tamperedRoot -Source $tamperedSource -Entries @($allowedEntry)
    $tamperedResult = Invoke-Checker $tamperedRoot
    Assert-FailedWith -Result $tamperedResult -Pattern "Unregistered unconditional-assert-true" `
        -Scenario "tampered allowlist fixture"
    Assert-FailedWith -Result $tamperedResult -Pattern "Stale or text-changed allowlist entry" `
        -Scenario "tampered allowlist fixture"
} finally {
    $resolvedTemporaryRoot = [System.IO.Path]::GetFullPath($temporaryRoot)
    if (-not $resolvedTemporaryRoot.StartsWith($temporaryBase, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove fixture path outside the system temporary directory: $resolvedTemporaryRoot"
    }
    if (Test-Path -LiteralPath $resolvedTemporaryRoot) {
        Remove-Item -LiteralPath $resolvedTemporaryRoot -Recurse -Force
    }
}

Write-Host "[OK] Test signal integrity fixture contract passed"
