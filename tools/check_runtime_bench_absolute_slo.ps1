param(
    [Parameter(Mandatory = $true)]
    [string]$ResultPath,

    [string]$PolicyPath = "tests/compatibility/runtime-benchmark-absolute-policy.json"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Assert-AbsoluteSlo {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw "Runtime benchmark absolute SLO failed: $Message"
    }
}

$result = Get-Content -Raw -LiteralPath (Resolve-Path -LiteralPath $ResultPath).Path | ConvertFrom-Json
$policy = Get-Content -Raw -LiteralPath (Resolve-Path -LiteralPath $PolicyPath).Path | ConvertFrom-Json

Assert-AbsoluteSlo ($policy.schemaVersion -eq 1) "policy schemaVersion must be 1"
Assert-AbsoluteSlo ($policy.scope.profile -eq "ci") "policy profile must be ci"
Assert-AbsoluteSlo ($policy.scope.buildType -eq "Release") "policy buildType must be Release"
Assert-AbsoluteSlo ($result.profile -eq $policy.scope.profile) "result profile does not match policy"
Assert-AbsoluteSlo ($result.build_type -eq $policy.scope.buildType) "result build type does not match policy"
Assert-AbsoluteSlo ($policy.metrics.Count -gt 0) "policy has no metrics"

$metrics = @{}
foreach ($metric in $result.metrics) {
    Assert-AbsoluteSlo (-not $metrics.ContainsKey([string]$metric.name)) "result has duplicate metric '$($metric.name)'"
    $metrics[[string]$metric.name] = $metric
}

$policyNames = @{}
foreach ($limit in $policy.metrics) {
    $name = [string]$limit.name
    $direction = [string]$limit.direction
    $threshold = [double]$limit.threshold
    Assert-AbsoluteSlo (-not [string]::IsNullOrWhiteSpace($name)) "policy contains an unnamed metric"
    Assert-AbsoluteSlo (-not $policyNames.ContainsKey($name)) "policy has duplicate metric '$name'"
    Assert-AbsoluteSlo (($direction -eq "higher") -or ($direction -eq "lower")) `
        "metric '$name' has an invalid direction"
    Assert-AbsoluteSlo ((-not [double]::IsNaN($threshold)) -and
        (-not [double]::IsInfinity($threshold)) -and $threshold -gt 0.0) `
        "metric '$name' has an invalid threshold"
    Assert-AbsoluteSlo ($metrics.ContainsKey($name)) "required metric '$name' is missing"

    $metric = $metrics[$name]
    $actual = [double]$metric.median
    Assert-AbsoluteSlo ($metric.direction -eq $direction) "metric '$name' direction does not match policy"
    Assert-AbsoluteSlo ((-not [double]::IsNaN($actual)) -and (-not [double]::IsInfinity($actual))) `
        "metric '$name' is not finite"
    if ($direction -eq "higher") {
        Assert-AbsoluteSlo ($actual -ge $threshold) `
            "metric '$name' is $actual, below minimum $threshold"
    } else {
        Assert-AbsoluteSlo ($actual -le $threshold) `
            "metric '$name' is $actual, above maximum $threshold"
    }
    $policyNames[$name] = $true
}

Write-Host "Runtime benchmark absolute SLO passed."
Write-Host "  Enforced metrics: $($policy.metrics.Count)"
