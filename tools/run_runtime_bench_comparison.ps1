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
if ($policy.schemaVersion -ne 3 -or $policy.minimumRunsPerRevision -lt 3 -or
    $policy.confirmationRunsPerRevision -lt 2 -or
    $policy.maximumRunsPerRevision -ne
        ($policy.minimumRunsPerRevision + $policy.confirmationRunsPerRevision)) {
    throw "Invalid runtime benchmark regression policy"
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$resolvedOutput = (Resolve-Path -LiteralPath $OutputDirectory).Path
$generatedEvidenceNames = @("comparison.json", "comparison-initial.json", "run-order.json")
for ($run = 1; $run -le [int]$policy.maximumRunsPerRevision; $run++) {
    $generatedEvidenceNames += @("base-$run.json", "head-$run.json")
}
foreach ($name in $generatedEvidenceNames) {
    $path = Join-Path $resolvedOutput $name
    if (Test-Path -LiteralPath $path) {
        Remove-Item -LiteralPath $path -Force
    }
}
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

$runtimeInputPaths = @($policy.runtimeInputPaths | ForEach-Object { [string]$_ })
if ($runtimeInputPaths.Count -eq 0) {
    throw "Runtime benchmark policy has no runtime input paths"
}
$gitDiffArguments = @("diff", "--name-only", $BaseSha, $HeadSha, "--") + $runtimeInputPaths
$runtimeInputDiffPaths = @(& git $gitDiffArguments)
if ($LASTEXITCODE -ne 0) {
    throw "Unable to compare runtime benchmark inputs"
}
$runtimeInputDiffPaths = @($runtimeInputDiffPaths | Where-Object {
        -not [string]::IsNullOrWhiteSpace([string]$_)
    })
$runtimeInputsEquivalent = $runtimeInputDiffPaths.Count -eq 0

function Invoke-BenchmarkPair {
    param([int]$Pair)

    $order = if (($Pair % 2) -eq 0) { @("base", "head") } else { @("head", "base") }
    foreach ($revision in $order) {
        $isBase = $revision -eq "base"
        $executable = if ($isBase) { $baseProgram } else { $headProgram }
        $sha = if ($isBase) { $BaseSha } else { $HeadSha }
        $resultPath = Join-Path $resolvedOutput ("{0}-{1}.json" -f $revision, ($Pair + 1))
        $env:GITHUB_SHA = $sha
        $startedAt = [DateTime]::UtcNow.ToString("o")
        if ($null -ne $benchmarkCpu) {
            & $taskset.Source --cpu-list $benchmarkCpu $executable --profile ci --json $resultPath
        } else {
            & $executable --profile ci --json $resultPath
        }
        if ($LASTEXITCODE -ne 0) {
            throw "$revision benchmark pair $Pair failed with exit code $LASTEXITCODE"
        }
        & (Join-Path $PSScriptRoot "check_runtime_bench.ps1") -ResultPath $resultPath
        if ($LASTEXITCODE -ne 0) {
            throw "$revision benchmark contract validation failed"
        }
        if ($isBase) {
            $script:baseResults += $resultPath
        } else {
            $script:headResults += $resultPath
        }
        $script:runs += [ordered]@{
            pair       = $Pair
            revision   = $revision
            sha        = $sha
            resultPath = $resultPath
            startedAt  = $startedAt
            endedAt    = [DateTime]::UtcNow.ToString("o")
        }
    }
}

function Write-RunManifest {
    param(
        [string]$Path,
        [bool]$ConfirmationTriggered
    )

    [ordered]@{
        schemaVersion            = 2
        runnerPid               = $PID
        baseSha                 = $BaseSha
        headSha                 = $HeadSha
        runtimeInputPaths        = $runtimeInputPaths
        runtimeInputDiffPaths    = $runtimeInputDiffPaths
        runtimeInputsEquivalent = $runtimeInputsEquivalent
        confirmationTriggered   = $ConfirmationTriggered
        runs                     = $runs
    } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $Path -Encoding utf8
}

$comparisonChecker = Join-Path $PSScriptRoot "check_runtime_bench_comparison.ps1"
$manifestPath = Join-Path $resolvedOutput "run-order.json"
$comparisonPath = Join-Path $resolvedOutput "comparison.json"
$initialComparisonPath = Join-Path $resolvedOutput "comparison-initial.json"

try {
    for ($pair = 0; $pair -lt [int]$policy.minimumRunsPerRevision; $pair++) {
        Invoke-BenchmarkPair -Pair $pair
    }

    Write-RunManifest -Path $manifestPath -ConfirmationTriggered $false

    & $comparisonChecker `
        -BaseResultPath $baseResults `
        -HeadResultPath $headResults `
        -RunManifestPath $manifestPath `
        -PolicyPath $PolicyPath `
        -OutputPath $comparisonPath `
        -DoNotThrowOnRegression
    $comparison = Get-Content -Raw -LiteralPath $comparisonPath | ConvertFrom-Json

    if ($comparison.confirmationRecommended -eq $true) {
        Write-Host "Runtime benchmark evidence is inconsistent; collecting confirmation pairs."
        Copy-Item -LiteralPath $comparisonPath -Destination $initialComparisonPath -Force
        $firstConfirmationPair = [int]$policy.minimumRunsPerRevision
        for ($pair = $firstConfirmationPair; $pair -lt [int]$policy.maximumRunsPerRevision; $pair++) {
            Invoke-BenchmarkPair -Pair $pair
        }
        Write-RunManifest -Path $manifestPath -ConfirmationTriggered $true
        & $comparisonChecker `
            -BaseResultPath $baseResults `
            -HeadResultPath $headResults `
            -RunManifestPath $manifestPath `
            -PolicyPath $PolicyPath `
            -OutputPath $comparisonPath
    } elseif ($comparison.success -ne $true) {
        & $comparisonChecker `
            -BaseResultPath $baseResults `
            -HeadResultPath $headResults `
            -RunManifestPath $manifestPath `
            -PolicyPath $PolicyPath `
            -OutputPath $comparisonPath
    }
} finally {
    $env:GITHUB_SHA = $previousSha
}
