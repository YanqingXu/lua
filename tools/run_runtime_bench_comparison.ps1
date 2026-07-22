param(
    [Parameter(Mandatory = $true)]
    [string]$BaseExecutable,

    [Parameter(Mandatory = $true)]
    [string]$HeadExecutable,

    [Parameter(Mandatory = $true)]
    [string]$BaseSha,

    [Parameter(Mandatory = $true)]
    [string]$HeadSha,

    [string]$OutputDirectory = "build/benchmark-comparison",

    [string]$PolicyPath = "tests/compatibility/runtime-benchmark-regression-policy.json"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$baseProgram = (Resolve-Path -LiteralPath $BaseExecutable).Path
$headProgram = (Resolve-Path -LiteralPath $HeadExecutable).Path
$policy = Get-Content -Raw -LiteralPath (Resolve-Path -LiteralPath $PolicyPath).Path | ConvertFrom-Json
if ($policy.schemaVersion -ne 2 -or $policy.minimumRunsPerRevision -lt 3) {
    throw "Invalid runtime benchmark regression policy"
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$resolvedOutput = (Resolve-Path -LiteralPath $OutputDirectory).Path
$baseResults = @()
$headResults = @()
$runs = @()
$previousSha = $env:GITHUB_SHA
$taskset = if ($IsLinux) { Get-Command taskset -ErrorAction SilentlyContinue } else { $null }
$benchmarkCpu = $null
if ($null -ne $taskset) {
    $affinity = (& $taskset.Source -pc $PID 2>&1 | Out-String)
    if ($LASTEXITCODE -eq 0 -and $affinity -match ':\s*(\d+)') {
        $benchmarkCpu = $Matches[1]
    }
}

try {
    for ($pair = 0; $pair -lt [int]$policy.minimumRunsPerRevision; $pair++) {
        $order = if (($pair % 2) -eq 0) { @("base", "head") } else { @("head", "base") }
        foreach ($revision in $order) {
            $isBase = $revision -eq "base"
            $executable = if ($isBase) { $baseProgram } else { $headProgram }
            $sha = if ($isBase) { $BaseSha } else { $HeadSha }
            $resultPath = Join-Path $resolvedOutput ("{0}-{1}.json" -f $revision, ($pair + 1))
            $env:GITHUB_SHA = $sha
            $startedAt = [DateTime]::UtcNow.ToString("o")
            if ($null -ne $benchmarkCpu) {
                & $taskset.Source --cpu-list $benchmarkCpu $executable --profile ci --json $resultPath
            } else {
                & $executable --profile ci --json $resultPath
            }
            if ($LASTEXITCODE -ne 0) {
                throw "$revision benchmark pair $pair failed with exit code $LASTEXITCODE"
            }
            & (Join-Path $PSScriptRoot "check_runtime_bench.ps1") -ResultPath $resultPath
            if ($LASTEXITCODE -ne 0) {
                throw "$revision benchmark contract validation failed"
            }
            if ($isBase) {
                $baseResults += $resultPath
            } else {
                $headResults += $resultPath
            }
            $runs += [ordered]@{
                pair       = $pair
                revision   = $revision
                sha        = $sha
                resultPath = $resultPath
                startedAt  = $startedAt
                endedAt    = [DateTime]::UtcNow.ToString("o")
            }
        }
    }

    $manifestPath = Join-Path $resolvedOutput "run-order.json"
    [ordered]@{
        schemaVersion = 1
        runnerPid     = $PID
        baseSha       = $BaseSha
        headSha       = $HeadSha
        runs          = $runs
    } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifestPath -Encoding utf8

    & (Join-Path $PSScriptRoot "check_runtime_bench_comparison.ps1") `
        -BaseResultPath $baseResults `
        -HeadResultPath $headResults `
        -RunManifestPath $manifestPath `
        -PolicyPath $PolicyPath `
        -OutputPath (Join-Path $resolvedOutput "comparison.json")
    if ($LASTEXITCODE -ne 0) {
        throw "runtime benchmark comparison validation failed"
    }
} finally {
    $env:GITHUB_SHA = $previousSha
}
