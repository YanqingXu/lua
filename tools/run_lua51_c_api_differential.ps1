param(
    [string]$Root = "",
    [Parameter(Mandatory = $true)]
    [string]$ReferenceProbe,
    [Parameter(Mandatory = $true)]
    [string]$CandidateProbe,
    [int]$TimeoutSeconds = 30,
    [string]$ResultPath = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($Root)) {
    $Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-RootedPath {
    param([string]$Path)
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return (Resolve-Path -LiteralPath $Path).Path
    }
    return (Resolve-Path -LiteralPath (Join-Path $Root $Path)).Path
}

function Invoke-Probe {
    param([string]$Executable)

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Executable
    $startInfo.WorkingDirectory = $Root
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    if (-not $process.Start()) {
        throw "Could not start C API differential probe: $Executable"
    }

    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        $process.WaitForExit()
        throw "C API differential probe timed out: $Executable"
    }

    $process.WaitForExit()
    return [ordered]@{
        exitStatus = $process.ExitCode
        stdout = ($stdoutTask.Result -replace "`r`n", "`n" -replace "`r", "`n")
        stderr = ($stderrTask.Result -replace "`r`n", "`n" -replace "`r", "`n")
    }
}

$referencePath = Resolve-RootedPath $ReferenceProbe
$candidatePath = Resolve-RootedPath $CandidateProbe
$reference = Invoke-Probe -Executable $referencePath
$candidate = Invoke-Probe -Executable $candidatePath
$mismatches = @("exitStatus", "stdout", "stderr") | Where-Object { $reference[$_] -ne $candidate[$_] }
$document = [ordered]@{
    schemaVersion = 1
    channel = "lua51-c-api-differential"
    reference = $referencePath
    candidate = $candidatePath
    passed = $mismatches.Count -eq 0
    mismatches = @($mismatches)
    referenceResult = $reference
    candidateResult = $candidate
}

if ($ResultPath) {
    $resolvedResultPath = if ([System.IO.Path]::IsPathRooted($ResultPath)) { $ResultPath } else { Join-Path $Root $ResultPath }
    $parent = Split-Path -Parent $resolvedResultPath
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $json = $document | ConvertTo-Json -Depth 6
    [System.IO.File]::WriteAllText($resolvedResultPath, $json + [Environment]::NewLine,
        [System.Text.UTF8Encoding]::new($false))
}

if ($mismatches.Count -ne 0) {
    throw "Lua 5.1 C API differential mismatch: $($mismatches -join ', ')"
}

Write-Host "[OK] Lua 5.1 C API differential probe matched: $($candidate.stdout.Trim())"
