param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$Executable = "bin\lua_app.exe",
    [int]$TimeoutSeconds = 300,
    [switch]$ExpectFailure,
    [string]$XfailManifest = "tests/compatibility/lua51-official-strict-xfails.json",
    [string]$ResultPath = ""
)

$ErrorActionPreference = "Stop"

function Resolve-RootedPath {
    param([string]$Path)
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }
    return Join-Path $Root $Path
}

$integrityScript = Join-Path $Root "tools/check_lua51_official_sources.ps1"
$manifestPath = Join-Path $Root "tests/compatibility/lua51-official-sources.json"
$suitePath = Join-Path $Root "tests/lua/official"
$executablePath = (Resolve-Path -LiteralPath (Resolve-RootedPath $Executable)).Path

if ($ExpectFailure) {
    $xfailManifestPath = Resolve-RootedPath $XfailManifest
    if (-not (Test-Path -LiteralPath $xfailManifestPath)) {
        throw "Missing official-strict XFAIL manifest: $xfailManifestPath"
    }

    $xfailManifestData = Get-Content -LiteralPath $xfailManifestPath -Raw | ConvertFrom-Json
    $xfailEntries = @($xfailManifestData.xfails)
    if ($xfailManifestData.schemaVersion -ne 1 -or
        $xfailManifestData.channel -ne "official-strict" -or
        $xfailEntries.Count -ne 1 -or
        $xfailEntries[0].id -ne "all.lua-unmodified" -or
        $xfailEntries[0].expectedOutcome -ne "timeout") {
        throw "official-strict requires exactly one timeout-only all.lua XFAIL"
    }
    if ([int]$xfailEntries[0].timeoutSeconds -ne $TimeoutSeconds) {
        throw ("official-strict timeout {0}s does not match the registered XFAIL budget {1}s" -f `
                $TimeoutSeconds, $xfailEntries[0].timeoutSeconds)
    }
}

& $integrityScript -Root $Root

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lua51_official_strict_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempRoot | Out-Null

$outcome = "failed"
$exitCode = $null
$stdoutText = ""
$stderrText = ""
$elapsed = [System.Diagnostics.Stopwatch]::StartNew()

try {
    Copy-Item -Path (Join-Path $suitePath "*") -Destination $tempRoot -Recurse -Force
    & $integrityScript -Root $Root -SuitePath $tempRoot -ManifestPath $manifestPath

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $executablePath
    $startInfo.Arguments = "all.lua"
    $startInfo.WorkingDirectory = $tempRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    if (-not $process.Start()) {
        throw "could not start official-strict"
    }
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()

    $completed = $process.WaitForExit($TimeoutSeconds * 1000)
    if (-not $completed) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        $process.WaitForExit()
        $stdoutText = $stdoutTask.Result
        $stderrText = $stderrTask.Result
        $outcome = "timeout"
    } else {
        $process.WaitForExit()
        $exitCode = $process.ExitCode
        $outcome = if ($exitCode -eq 0) { "passed" } else { "failed" }
    }

    $elapsed.Stop()
    if ($completed) {
        $stdoutText = $stdoutTask.Result
        $stderrText = $stderrTask.Result
    }
} finally {
    & $integrityScript -Root $Root
    if ($elapsed.IsRunning) {
        $elapsed.Stop()
    }
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}

$result = [ordered]@{
    schemaVersion = 1
    channel = "official-strict"
    sourceMode = "unmodified temporary copy"
    outcome = $outcome
    exitCode = $exitCode
    timeoutSeconds = $TimeoutSeconds
    elapsedSeconds = [math]::Round($elapsed.Elapsed.TotalSeconds, 3)
    stdoutTail = (($stdoutText -split "\r?\n") | Select-Object -Last 40) -join [Environment]::NewLine
    stderrTail = (($stderrText -split "\r?\n") | Select-Object -Last 40) -join [Environment]::NewLine
}

if ($ResultPath) {
    $resolvedResultPath = Resolve-RootedPath $ResultPath
    $parent = Split-Path -Parent $resolvedResultPath
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $json = $result | ConvertTo-Json -Depth 4
    [System.IO.File]::WriteAllText($resolvedResultPath, $json + [Environment]::NewLine, [System.Text.UTF8Encoding]::new($false))
}

Write-Host ("official-strict outcome={0}, exit={1}, elapsed={2}s" -f $outcome, $exitCode, $result.elapsedSeconds)
if ($result.stderrTail) {
    Write-Host $result.stderrTail
}

if ($ExpectFailure) {
    if ($outcome -eq "timeout") {
        Write-Host "[XFAIL] official-strict reached the registered timeout compatibility gap"
        exit 0
    }
    if ($outcome -eq "passed") {
        throw "official-strict unexpectedly passed; remove the XFAIL and promote this lane to required"
    }
    throw "official-strict expected the registered timeout XFAIL but ended with $outcome (exit $exitCode)"
}

if ($outcome -ne "passed") {
    throw "official-strict did not pass: $outcome"
}

Write-Host "[OK] official-strict passed"
