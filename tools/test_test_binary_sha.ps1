param(
    [string]$Checker = (Join-Path $PSScriptRoot "check_test_binary_sha.ps1")
)

$ErrorActionPreference = "Stop"

function Write-FixtureExecutable {
    param(
        [string]$Path,
        [string[]]$OutputLines,
        [int]$ExitCode = 0
    )

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add('if ($args.Count -ne 1 -or $args[0] -ne "--build-info") { exit 91 }') | Out-Null
    foreach ($outputLine in $OutputLines) {
        $escapedOutput = $outputLine.Replace("'", "''")
        $lines.Add("Write-Output '$escapedOutput'") | Out-Null
    }
    $lines.Add("exit $ExitCode") | Out-Null
    [System.IO.File]::WriteAllText(
        $Path,
        ($lines -join "`n") + "`n",
        [System.Text.UTF8Encoding]::new($false)
    )
}

function Invoke-Checker {
    param(
        [string]$Executable,
        [string]$ExpectedSha
    )

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
            -TestExecutable $Executable -ExpectedSha $ExpectedSha 2>&1
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

$Checker = (Resolve-Path -LiteralPath $Checker).Path
$expectedSha = "0123456789abcdef0123456789abcdef01234567"
$differentSha = "89abcdef0123456789abcdef0123456789abcdef"
$temporaryBase = [System.IO.Path]::GetFullPath([System.IO.Path]::GetTempPath()).TrimEnd("\", "/") +
    [System.IO.Path]::DirectorySeparatorChar
$temporaryRoot = Join-Path $temporaryBase ("lua_test_binary_sha_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null

try {
    $successFixture = Join-Path $temporaryRoot "success.ps1"
    Write-FixtureExecutable -Path $successFixture -OutputLines @("Build Git SHA: $expectedSha")
    $successResult = Invoke-Checker -Executable $successFixture -ExpectedSha $expectedSha.ToUpperInvariant()
    Assert-Succeeded -Result $successResult -Scenario "matching SHA fixture"

    $unknownFixture = Join-Path $temporaryRoot "unknown.ps1"
    Write-FixtureExecutable -Path $unknownFixture -OutputLines @("Build Git SHA: unknown")
    Assert-FailedWith -Result (Invoke-Checker $unknownFixture $expectedSha) `
        -Pattern "reported Build Git SHA: unknown" -Scenario "unknown SHA fixture"

    $mismatchFixture = Join-Path $temporaryRoot "mismatch.ps1"
    Write-FixtureExecutable -Path $mismatchFixture -OutputLines @("Build Git SHA: $differentSha")
    Assert-FailedWith -Result (Invoke-Checker $mismatchFixture $expectedSha) `
        -Pattern "does not match expected SHA" -Scenario "mismatched SHA fixture"

    $malformedFixture = Join-Path $temporaryRoot "malformed.ps1"
    Write-FixtureExecutable -Path $malformedFixture `
        -OutputLines @("Build Git SHA: 0123456789abcdef0123456789abcdef0123456")
    Assert-FailedWith -Result (Invoke-Checker $malformedFixture $expectedSha) `
        -Pattern "must be exactly 40 hexadecimal characters" -Scenario "malformed SHA fixture"

    $nonzeroFixture = Join-Path $temporaryRoot "nonzero.ps1"
    Write-FixtureExecutable -Path $nonzeroFixture -OutputLines @("Build Git SHA: $expectedSha") -ExitCode 23
    Assert-FailedWith -Result (Invoke-Checker $nonzeroFixture $expectedSha) `
        -Pattern "exited with code 23" -Scenario "nonzero build-info fixture"

    Assert-FailedWith -Result (Invoke-Checker $successFixture "not-a-commit") `
        -Pattern "ExpectedSha must be exactly 40 hexadecimal characters" -Scenario "malformed expected SHA fixture"
} finally {
    $resolvedTemporaryRoot = [System.IO.Path]::GetFullPath($temporaryRoot)
    if (-not $resolvedTemporaryRoot.StartsWith($temporaryBase, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove fixture path outside the system temporary directory: $resolvedTemporaryRoot"
    }
    if (Test-Path -LiteralPath $resolvedTemporaryRoot) {
        Remove-Item -LiteralPath $resolvedTemporaryRoot -Recurse -Force
    }
}

Write-Host "[OK] Test binary SHA fixture contract passed"
