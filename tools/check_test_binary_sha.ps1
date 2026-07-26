param(
    [Parameter(Mandatory = $true)]
    [string]$TestExecutable,

    [Parameter(Mandatory = $true)]
    [string]$ExpectedSha
)

$ErrorActionPreference = "Stop"

if ($ExpectedSha -notmatch "^[0-9a-fA-F]{40}$") {
    throw "ExpectedSha must be exactly 40 hexadecimal characters"
}

if (-not [System.IO.Path]::IsPathRooted($TestExecutable)) {
    $TestExecutable = Join-Path (Get-Location).Path $TestExecutable
}
$resolvedExecutable = [System.IO.Path]::GetFullPath($TestExecutable)
if (-not (Test-Path -LiteralPath $resolvedExecutable -PathType Leaf)) {
    throw "Test executable does not exist: $resolvedExecutable"
}

$previousErrorActionPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = "Continue"
    $buildInfoOutput = @(& $resolvedExecutable --build-info 2>&1)
    $buildInfoExitCode = $LASTEXITCODE
} finally {
    $ErrorActionPreference = $previousErrorActionPreference
}

$buildInfoText = ($buildInfoOutput | ForEach-Object { $_.ToString() }) -join "`n"
if ($buildInfoExitCode -ne 0) {
    throw "Test executable --build-info exited with code $buildInfoExitCode`: $buildInfoText"
}

$buildInfoLines = @($buildInfoText.Replace("`r`n", "`n").Replace("`r", "`n") -split "`n")
$shaLines = @($buildInfoLines | Where-Object { $_ -match "^Build Git SHA:" })
if ($shaLines.Count -ne 1) {
    throw "Test executable must report exactly one 'Build Git SHA:' line; found $($shaLines.Count): $buildInfoText"
}

$reportedValue = $shaLines[0].Substring("Build Git SHA:".Length).Trim()
if ($reportedValue -eq "unknown") {
    throw "Test executable reported Build Git SHA: unknown"
}
if ($shaLines[0] -notmatch "^Build Git SHA: ([0-9a-fA-F]{40})$") {
    throw "Test executable Build Git SHA must be exactly 40 hexadecimal characters: '$($shaLines[0])'"
}

$actualSha = $Matches[1].ToLowerInvariant()
$normalizedExpectedSha = $ExpectedSha.ToLowerInvariant()
if ($actualSha -ne $normalizedExpectedSha) {
    throw "Test executable build SHA $actualSha does not match expected SHA $normalizedExpectedSha"
}

Write-Host "[OK] Test executable build SHA matches expected SHA: $actualSha"
