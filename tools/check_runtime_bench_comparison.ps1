param(
    [Parameter(Mandatory = $true)]
    [string[]]$BaseResultPath,

    [Parameter(Mandatory = $true)]
    [string[]]$HeadResultPath,

    [Parameter(Mandatory = $true)]
    [string]$RunManifestPath,

    [string]$PolicyPath = "tests/compatibility/runtime-benchmark-regression-policy.json",

    [Parameter(Mandatory = $true)]
    [string]$OutputPath,

    [switch]$AllowSameRevision
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Assert-Comparison {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw "Runtime benchmark comparison failed: $Message"
    }
}

function Get-Median {
    param([double[]]$Values)

    Assert-Comparison ($Values.Count -gt 0) "cannot compute a median from an empty sample set"
    [Array]::Sort($Values)
    $middle = [int][Math]::Floor($Values.Count / 2.0)
    if (($Values.Count % 2) -eq 1) {
        return $Values[$middle]
    }
    return ($Values[$middle - 1] + $Values[$middle]) / 2.0
}

function Get-NearestRankPercentile {
    param(
        [double[]]$Values,
        [double]$Percentile
    )

    Assert-Comparison ($Values.Count -gt 0) "cannot compute a percentile from an empty sample set"
    [Array]::Sort($Values)
    $index = [int][Math]::Ceiling($Percentile * $Values.Count) - 1
    return $Values[$index]
}

function Read-Reports {
    param(
        [string[]]$Paths,
        [string]$RevisionName,
        [int]$MinimumRuns
    )

    Assert-Comparison ($Paths.Count -ge $MinimumRuns) `
        "$RevisionName has $($Paths.Count) runs; policy requires at least $MinimumRuns"
    $reports = @()
    foreach ($path in $Paths) {
        $resolved = (Resolve-Path -LiteralPath $path).Path
        $report = Get-Content -Raw -LiteralPath $resolved | ConvertFrom-Json
        Assert-Comparison ($report.schema_version -eq 1) "$RevisionName result has an unsupported schema"
        Assert-Comparison ($report.success -eq $true) "$RevisionName result is not successful"
        Assert-Comparison ($report.profile -eq "ci") "$RevisionName result is not the ci profile"
        Assert-Comparison ($report.build_type -eq "Release") "$RevisionName result is not a Release build"
        Assert-Comparison (-not [string]::IsNullOrWhiteSpace([string]$report.git_sha)) `
            "$RevisionName result is missing git_sha"
        $reports += $report
    }
    return $reports
}

function Get-AggregatedMetric {
    param(
        [object[]]$Reports,
        [string]$Name,
        [string]$Direction
    )

    [double[]]$samples = @()
    if ($Name -eq "gc_pause_p99_us") {
        foreach ($report in $Reports) {
            $samples += @($report.gc_pause_samples_us | ForEach-Object { [double]$_ })
        }
        $value = Get-NearestRankPercentile -Values $samples -Percentile 0.99
        return [pscustomobject]@{ Value = $value; SampleCount = $samples.Count }
    }

    foreach ($report in $Reports) {
        $matches = @($report.metrics | Where-Object { $_.name -eq $Name })
        Assert-Comparison ($matches.Count -eq 1) "metric '$Name' must occur exactly once per result"
        Assert-Comparison ($matches[0].direction -eq $Direction) `
            "metric '$Name' direction differs from policy"
        $samples += @($matches[0].samples | ForEach-Object { [double]$_ })
    }
    $value = Get-Median -Values $samples
    return [pscustomobject]@{ Value = $value; SampleCount = $samples.Count }
}

$policy = Get-Content -Raw -LiteralPath (Resolve-Path -LiteralPath $PolicyPath).Path | ConvertFrom-Json
Assert-Comparison ($policy.schemaVersion -eq 1) "policy schemaVersion must be 1"
Assert-Comparison ($policy.executionOrder -eq "alternating-base-head-on-the-same-runner") `
    "policy must require alternating execution on one runner"
$minimumRuns = [int]$policy.minimumRunsPerRevision
Assert-Comparison ($minimumRuns -ge 3) "policy must require at least three runs per revision"

$baseReports = @(Read-Reports -Paths $BaseResultPath -RevisionName "base" -MinimumRuns $minimumRuns)
$headReports = @(Read-Reports -Paths $HeadResultPath -RevisionName "head" -MinimumRuns $minimumRuns)
Assert-Comparison ($baseReports.Count -eq $headReports.Count) "base and head run counts differ"

$baseSha = [string]$baseReports[0].git_sha
$headSha = [string]$headReports[0].git_sha
foreach ($report in $baseReports) {
    Assert-Comparison ($report.git_sha -eq $baseSha) "base results contain multiple revisions"
}
foreach ($report in $headReports) {
    Assert-Comparison ($report.git_sha -eq $headSha) "head results contain multiple revisions"
}
if (-not $AllowSameRevision) {
    Assert-Comparison ($baseSha -ne $headSha) "base and head revisions must differ"
}

$referenceCompiler = [string]$baseReports[0].compiler
$referenceOs = [string]$baseReports[0].os
$referenceWorkload = $baseReports[0].workload | ConvertTo-Json -Compress
foreach ($report in @($baseReports) + @($headReports)) {
    Assert-Comparison ($report.compiler -eq $referenceCompiler) "base and head compilers differ"
    Assert-Comparison ($report.os -eq $referenceOs) "base and head operating systems differ"
    Assert-Comparison (($report.workload | ConvertTo-Json -Compress) -eq $referenceWorkload) `
        "base and head workloads differ"
}

$manifest = Get-Content -Raw -LiteralPath (Resolve-Path -LiteralPath $RunManifestPath).Path | ConvertFrom-Json
Assert-Comparison ($manifest.schemaVersion -eq 1) "run manifest schemaVersion must be 1"
Assert-Comparison ($manifest.baseSha -eq $baseSha) "run manifest base SHA differs from evidence"
Assert-Comparison ($manifest.headSha -eq $headSha) "run manifest head SHA differs from evidence"
Assert-Comparison ([int64]$manifest.runnerPid -gt 0) "run manifest is missing its orchestrator process identity"
$manifestRuns = @($manifest.runs)
Assert-Comparison ($manifestRuns.Count -eq (2 * $baseReports.Count)) "run manifest has the wrong sample count"
$manifestBasePaths = @()
$manifestHeadPaths = @()
$previousStart = [DateTime]::MinValue
for ($pair = 0; $pair -lt $baseReports.Count; $pair++) {
    $first = $manifestRuns[2 * $pair]
    $second = $manifestRuns[(2 * $pair) + 1]
    $expectedFirst = if (($pair % 2) -eq 0) { "base" } else { "head" }
    $expectedSecond = if ($expectedFirst -eq "base") { "head" } else { "base" }
    Assert-Comparison (($first.pair -eq $pair) -and ($second.pair -eq $pair)) `
        "run manifest pair $pair is malformed"
    Assert-Comparison (($first.revision -eq $expectedFirst) -and ($second.revision -eq $expectedSecond)) `
        "run pair $pair does not alternate base/head ordering"
    foreach ($run in @($first, $second)) {
        $expectedSha = if ($run.revision -eq "base") { $baseSha } else { $headSha }
        Assert-Comparison ($run.sha -eq $expectedSha) "run manifest revision SHA is inconsistent"
        $resolvedRunPath = (Resolve-Path -LiteralPath $run.resultPath).Path
        if ($run.revision -eq "base") {
            $manifestBasePaths += $resolvedRunPath
        } else {
            $manifestHeadPaths += $resolvedRunPath
        }
        $startedAt = [DateTime]::Parse([string]$run.startedAt).ToUniversalTime()
        $endedAt = [DateTime]::Parse([string]$run.endedAt).ToUniversalTime()
        Assert-Comparison ($endedAt -ge $startedAt) "run manifest contains a negative duration"
        Assert-Comparison ($startedAt -ge $previousStart) "run manifest timestamps are not execution ordered"
        $previousStart = $startedAt
    }
}
$resolvedBasePaths = @($BaseResultPath | ForEach-Object { (Resolve-Path -LiteralPath $_).Path })
$resolvedHeadPaths = @($HeadResultPath | ForEach-Object { (Resolve-Path -LiteralPath $_).Path })
Assert-Comparison (($manifestBasePaths -join "`n") -eq ($resolvedBasePaths -join "`n")) `
    "run manifest base result paths differ from the compared evidence"
Assert-Comparison (($manifestHeadPaths -join "`n") -eq ($resolvedHeadPaths -join "`n")) `
    "run manifest head result paths differ from the compared evidence"

$metricResults = @()
$failures = @()
$seenMetrics = @{}
foreach ($metricPolicy in @($policy.metrics)) {
    $name = [string]$metricPolicy.name
    $direction = [string]$metricPolicy.direction
    $limit = [double]$metricPolicy.maximumRegressionRatio
    Assert-Comparison (-not $seenMetrics.ContainsKey($name)) "policy contains duplicate metric '$name'"
    $seenMetrics[$name] = $true
    Assert-Comparison (($direction -eq "higher") -or ($direction -eq "lower")) `
        "metric '$name' has an invalid direction"
    Assert-Comparison (($limit -gt 0.0) -and ($limit -lt 1.0)) `
        "metric '$name' has an invalid regression limit"

    $base = Get-AggregatedMetric -Reports $baseReports -Name $name -Direction $direction
    $head = Get-AggregatedMetric -Reports $headReports -Name $name -Direction $direction
    Assert-Comparison (($base.Value -gt 0.0) -and ($head.Value -gt 0.0)) `
        "metric '$name' must remain positive"
    $regression = if ($direction -eq "higher") {
        ($base.Value - $head.Value) / $base.Value
    } else {
        ($head.Value - $base.Value) / $base.Value
    }
    $passed = $regression -le $limit
    if (-not $passed) {
        $failures += "$name regression $([Math]::Round(100.0 * $regression, 2))% exceeds $([Math]::Round(100.0 * $limit, 2))%"
    }
    $metricResults += [ordered]@{
        name                    = $name
        direction               = $direction
        base                    = $base.Value
        head                    = $head.Value
        baseSampleCount         = $base.SampleCount
        headSampleCount         = $head.SampleCount
        regressionRatio         = $regression
        maximumRegressionRatio  = $limit
        passed                  = $passed
    }
}

$evidence = [ordered]@{
    schemaVersion   = 1
    success         = $failures.Count -eq 0
    baseSha         = $baseSha
    headSha         = $headSha
    compiler        = $referenceCompiler
    os              = $referenceOs
    runsPerRevision = $baseReports.Count
    executionOrder  = $policy.executionOrder
    metrics         = $metricResults
    failures        = $failures
}
$outputParent = Split-Path -Parent $OutputPath
if (-not [string]::IsNullOrWhiteSpace($outputParent)) {
    New-Item -ItemType Directory -Force -Path $outputParent | Out-Null
}
$evidence | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $OutputPath -Encoding utf8

if ($failures.Count -gt 0) {
    throw "Runtime benchmark comparison failed: $($failures -join '; ')"
}

Write-Host "Runtime benchmark base-vs-head comparison passed."
Write-Host "  Base: $baseSha"
Write-Host "  Head: $headSha"
Write-Host "  Runs per revision: $($baseReports.Count)"
Write-Host "  Metrics: $($metricResults.Count)"
