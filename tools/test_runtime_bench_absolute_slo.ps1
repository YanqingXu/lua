$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$policyPath = Join-Path (Split-Path -Parent $PSScriptRoot) `
    "tests/compatibility/runtime-benchmark-absolute-policy.json"
$checker = Join-Path $PSScriptRoot "check_runtime_bench_absolute_slo.ps1"
$temporaryDirectory = Join-Path ([System.IO.Path]::GetTempPath()) `
    ("lua-runtime-bench-slo-" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $temporaryDirectory | Out-Null

try {
    $policy = Get-Content -Raw -LiteralPath $policyPath | ConvertFrom-Json
    $metrics = @()
    foreach ($limit in $policy.metrics) {
        $metrics += [ordered]@{
            name      = [string]$limit.name
            direction = [string]$limit.direction
            median    = [double]$limit.threshold
        }
    }
    $result = [ordered]@{
        profile    = "ci"
        build_type = "Release"
        metrics    = $metrics
    }
    $passingPath = Join-Path $temporaryDirectory "passing.json"
    $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $passingPath -Encoding utf8
    & $checker -ResultPath $passingPath -PolicyPath $policyPath

    $failing = Get-Content -Raw -LiteralPath $passingPath | ConvertFrom-Json
    $first = $failing.metrics[0]
    if ($first.direction -eq "higher") {
        $first.median = [double]$first.median / 2.0
    } else {
        $first.median = [double]$first.median * 2.0
    }
    $failingPath = Join-Path $temporaryDirectory "failing.json"
    $failing | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $failingPath -Encoding utf8

    $rejected = $false
    try {
        & $checker -ResultPath $failingPath -PolicyPath $policyPath
    } catch {
        $rejected = $_.Exception.Message -match "absolute SLO failed"
    }
    if (-not $rejected) {
        throw "Absolute SLO contract accepted a threshold violation"
    }
} finally {
    Remove-Item -LiteralPath $temporaryDirectory -Recurse -Force -ErrorAction SilentlyContinue
}

Write-Host "Runtime benchmark absolute SLO contract passed."
