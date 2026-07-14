param(
    [Parameter(Mandatory = $true)]
    [string]$ResultPath
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Assert-Contract {
    param(
        [bool]$Condition,
        [string]$Message
    )

    if (-not $Condition) {
        throw "Runtime benchmark contract failed: $Message"
    }
}

function Assert-FiniteNumber {
    param(
        [object]$Value,
        [string]$Name,
        [bool]$AllowNegative = $false
    )

    $number = [double]$Value
    Assert-Contract (-not [double]::IsNaN($number)) "$Name is NaN"
    Assert-Contract (-not [double]::IsInfinity($number)) "$Name is infinite"
    if (-not $AllowNegative) {
        Assert-Contract ($number -gt 0.0) "$Name must be positive"
    }
}

function Assert-NearlyEqual {
    param(
        [object]$Actual,
        [object]$Expected,
        [string]$Name
    )

    $actualNumber = [double]$Actual
    $expectedNumber = [double]$Expected
    Assert-Contract (-not [double]::IsNaN($actualNumber)) "$Name is NaN"
    Assert-Contract (-not [double]::IsInfinity($actualNumber)) "$Name is infinite"
    $scale = [Math]::Max(1.0, [Math]::Max([Math]::Abs($actualNumber), [Math]::Abs($expectedNumber)))
    Assert-Contract ([Math]::Abs($actualNumber - $expectedNumber) -le (1.0e-9 * $scale)) `
        "$Name does not match its raw evidence (actual=$actualNumber, expected=$expectedNumber)"
}

function Get-SortedNumbers {
    param([object[]]$Values)

    Assert-Contract ($Values.Count -gt 0) "cannot derive a statistic from an empty sample set"
    [double[]]$numbers = @($Values | ForEach-Object { [double]$_ })
    [Array]::Sort($numbers)
    return $numbers
}

function Get-Median {
    param([object[]]$Values)

    [double[]]$numbers = @(Get-SortedNumbers -Values $Values)
    $middle = [int][Math]::Floor($numbers.Count / 2.0)
    if (($numbers.Count % 2) -eq 1) {
        return $numbers[$middle]
    }
    return ($numbers[$middle - 1] + $numbers[$middle]) / 2.0
}

function Get-NearestRankPercentile {
    param(
        [object[]]$Values,
        [double]$Percentile
    )

    Assert-Contract (($Percentile -gt 0.0) -and ($Percentile -le 1.0)) `
        "percentile must be in (0, 1]"
    [double[]]$numbers = @(Get-SortedNumbers -Values $Values)
    $index = [int][Math]::Ceiling($Percentile * $numbers.Count) - 1
    return $numbers[$index]
}

function Get-HeapSlope {
    param([object[]]$Checkpoints)

    Assert-Contract ($Checkpoints.Count -ge 3) "heap stability needs at least three checkpoints"
    $begin = [int][Math]::Floor($Checkpoints.Count / 5.0)
    $count = $Checkpoints.Count - $begin
    $meanFrame = 0.0
    $meanBytes = 0.0
    for ($index = $begin; $index -lt $Checkpoints.Count; $index++) {
        $meanFrame += [double]$Checkpoints[$index].frame
        $meanBytes += [double]$Checkpoints[$index].allocator_live_bytes
    }
    $meanFrame /= $count
    $meanBytes /= $count

    $numerator = 0.0
    $denominator = 0.0
    for ($index = $begin; $index -lt $Checkpoints.Count; $index++) {
        $frameOffset = [double]$Checkpoints[$index].frame - $meanFrame
        $byteOffset = [double]$Checkpoints[$index].allocator_live_bytes - $meanBytes
        $numerator += $frameOffset * $byteOffset
        $denominator += $frameOffset * $frameOffset
    }
    Assert-Contract ($denominator -gt 0.0) "heap checkpoint frames do not span an interval"
    return ($numerator / $denominator) * 1.0e6
}

$resolvedResult = (Resolve-Path -LiteralPath $ResultPath).Path
$result = Get-Content -Raw -LiteralPath $resolvedResult | ConvertFrom-Json

Assert-Contract ($result.schema_version -eq 1) "schema_version must be 1"
Assert-Contract ($result.success -eq $true) "success must be true"
Assert-Contract ($result.profile -eq "ci") "profile must be ci"
Assert-Contract ($result.build_type -eq "Release") "numeric CI evidence must come from a Release build"
Assert-Contract (-not [string]::IsNullOrWhiteSpace([string]$result.compiler)) "compiler metadata is missing"
Assert-Contract ($result.compiler -ne "unknown") "compiler metadata must identify the compiler"
Assert-Contract (-not [string]::IsNullOrWhiteSpace([string]$result.os)) "OS metadata is missing"
Assert-Contract ($result.os -ne "unknown") "OS metadata must identify the operating system"
Assert-Contract (-not [string]::IsNullOrWhiteSpace([string]$result.git_sha)) "git_sha metadata is missing"
if (-not [string]::IsNullOrWhiteSpace($env:GITHUB_SHA)) {
    Assert-Contract ($result.git_sha -eq $env:GITHUB_SHA) "git_sha does not match GITHUB_SHA"
}

$expectedWorkload = [ordered]@{
    timing_samples     = 3
    parse_iterations   = 1
    vm_iterations      = 100000
    cpp_to_lua_calls   = 2000
    lua_to_cpp_calls   = 20000
    coroutine_yields   = 1000
    table_iterations   = 50000
    closure_samples    = 1
    closure_count      = 100000
    gc_pause_frames    = 10000
    gc_step_size       = 4
    heap_warmup_frames = 1000
    heap_frames        = 20000
}

foreach ($entry in $expectedWorkload.GetEnumerator()) {
    $property = $result.workload.PSObject.Properties[$entry.Key]
    Assert-Contract ($null -ne $property) "workload.$($entry.Key) is missing"
    Assert-Contract ([int64]$property.Value -eq [int64]$entry.Value) `
        "workload.$($entry.Key) must be $($entry.Value)"
}
Assert-Contract ($result.sample_count -eq $result.workload.timing_samples) `
    "sample_count does not match workload.timing_samples"
Assert-Contract ($result.closure_count -eq 100000) "closure_count must remain exactly 100000"
Assert-Contract ($result.closure_count -eq $result.workload.closure_count) `
    "closure_count does not match workload.closure_count"
Assert-Contract ($result.gc_pause_sample_count -eq $result.workload.gc_pause_frames) `
    "gc_pause_sample_count does not match workload.gc_pause_frames"
Assert-Contract ($result.gc_pause_samples_us.Count -eq $result.gc_pause_sample_count) `
    "gc_pause_sample_count does not match the raw sample array"
Assert-Contract ($result.gc_step_size -eq $result.workload.gc_step_size) `
    "gc_step_size does not match workload.gc_step_size"
Assert-Contract ($result.gc_cycles -gt 0) "fixed-budget GC completed no collection cycle"
Assert-Contract ($result.heap_gc_cycles -gt 0) "heap stability workload completed no collection cycle"

$ordinarySamples = [int]$result.workload.timing_samples
$closureSamples = [int]$result.workload.closure_samples
$metricContract = [ordered]@{
    parse_compile_mib_per_second          = [pscustomobject]@{ Unit = "MiB/s"; Direction = "higher"; Samples = $ordinarySamples }
    vm_instructions_per_second            = [pscustomobject]@{ Unit = "instructions/s"; Direction = "higher"; Samples = $ordinarySamples }
    cpp_to_lua_ns_per_call                = [pscustomobject]@{ Unit = "ns/call"; Direction = "lower"; Samples = $ordinarySamples }
    lua_to_cpp_ns_per_call                = [pscustomobject]@{ Unit = "ns/call"; Direction = "lower"; Samples = $ordinarySamples }
    coroutine_resume_yield_ns             = [pscustomobject]@{ Unit = "ns/round-trip"; Direction = "lower"; Samples = $ordinarySamples }
    table_operations_per_second           = [pscustomobject]@{ Unit = "operations/s"; Direction = "higher"; Samples = $ordinarySamples }
    closure_upvalue_lifecycle_per_second   = [pscustomobject]@{ Unit = "closures/s"; Direction = "higher"; Samples = $closureSamples }
    allocation_mib_per_second             = [pscustomobject]@{ Unit = "MiB/s"; Direction = "higher"; Samples = $closureSamples }
    gc_pause_p50_us                       = [pscustomobject]@{ Unit = "us"; Direction = "lower"; Samples = 1 }
    gc_pause_p95_us                       = [pscustomobject]@{ Unit = "us"; Direction = "lower"; Samples = 1 }
    gc_pause_p99_us                       = [pscustomobject]@{ Unit = "us"; Direction = "lower"; Samples = 1 }
    gc_pause_max_us                       = [pscustomobject]@{ Unit = "us"; Direction = "lower"; Samples = 1 }
    heap_growth_bytes_per_million_frames  = [pscustomobject]@{ Unit = "bytes/1M-frames"; Direction = "lower"; Samples = 1 }
}

Assert-Contract ($result.metrics.Count -eq $metricContract.Count) `
    "metric count does not match the CI contract"
$metricsByName = @{}
foreach ($metric in $result.metrics) {
    Assert-Contract ($metricContract.Contains($metric.name)) "unexpected metric '$($metric.name)'"
    Assert-Contract (-not $metricsByName.ContainsKey($metric.name)) "duplicate metric '$($metric.name)'"
    $expectedMetric = $metricContract[$metric.name]
    Assert-Contract ($metric.unit -eq $expectedMetric.Unit) `
        "metric '$($metric.name)' has unit '$($metric.unit)', expected '$($expectedMetric.Unit)'"
    Assert-Contract ($metric.direction -eq $expectedMetric.Direction) `
        "metric '$($metric.name)' has direction '$($metric.direction)', expected '$($expectedMetric.Direction)'"
    Assert-Contract ($metric.samples.Count -eq $expectedMetric.Samples) `
        "metric '$($metric.name)' has $($metric.samples.Count) samples, expected $($expectedMetric.Samples)"

    $allowNegative = $metric.name -eq "heap_growth_bytes_per_million_frames"
    Assert-FiniteNumber $metric.median "$($metric.name).median" $allowNegative
    foreach ($sample in $metric.samples) {
        Assert-FiniteNumber $sample "$($metric.name).sample" $allowNegative
    }
    $recomputedMedian = Get-Median -Values @($metric.samples)
    Assert-NearlyEqual $metric.median $recomputedMedian "$($metric.name).median"
    $metricsByName[$metric.name] = $metric
}

foreach ($name in $metricContract.Keys) {
    Assert-Contract ($metricsByName.ContainsKey($name)) "required metric '$name' is missing"
}

$positivePauseSamples = 0
foreach ($pause in $result.gc_pause_samples_us) {
    $pauseNumber = [double]$pause
    Assert-Contract (-not [double]::IsNaN($pauseNumber)) "gc_pause_samples_us contains NaN"
    Assert-Contract (-not [double]::IsInfinity($pauseNumber)) "gc_pause_samples_us contains infinity"
    Assert-Contract ($pauseNumber -ge 0.0) "gc_pause_samples_us must be non-negative"
    if ($pauseNumber -gt 0.0) {
        $positivePauseSamples++
    }
}
Assert-Contract ($positivePauseSamples -gt 0) "GC pause evidence contains no measurable sample"

$recomputedP50 = Get-NearestRankPercentile -Values @($result.gc_pause_samples_us) -Percentile 0.50
$recomputedP95 = Get-NearestRankPercentile -Values @($result.gc_pause_samples_us) -Percentile 0.95
$recomputedP99 = Get-NearestRankPercentile -Values @($result.gc_pause_samples_us) -Percentile 0.99
$recomputedMaximum = Get-NearestRankPercentile -Values @($result.gc_pause_samples_us) -Percentile 1.00
Assert-NearlyEqual $metricsByName["gc_pause_p50_us"].median $recomputedP50 "gc_pause_p50_us"
Assert-NearlyEqual $metricsByName["gc_pause_p95_us"].median $recomputedP95 "gc_pause_p95_us"
Assert-NearlyEqual $metricsByName["gc_pause_p99_us"].median $recomputedP99 "gc_pause_p99_us"
Assert-NearlyEqual $metricsByName["gc_pause_max_us"].median $recomputedMaximum "gc_pause_max_us"
Assert-Contract (($recomputedP50 -le $recomputedP95) -and ($recomputedP95 -le $recomputedP99) -and `
        ($recomputedP99 -le $recomputedMaximum)) "GC pause percentiles must satisfy P50 <= P95 <= P99 <= max"

Assert-Contract ($result.heap.checkpoint_policy -eq "completed_gc_cycle") `
    "heap checkpoints must be captured at completed GC cycles"
Assert-Contract ($result.heap.checkpoints.Count -ge 3) "heap stability needs at least three checkpoints"
Assert-Contract ($result.heap.baseline_bytes -gt 0) "heap baseline must be positive"
Assert-Contract ($result.heap.final_bytes -gt 0) "heap final value must be positive"
Assert-Contract ($result.heap.allocator_live_after_close -eq 0) `
    "the counting allocator retained bytes after lua_close"
Assert-FiniteNumber $result.heap.growth_bytes_per_million_frames `
    "heap.growth_bytes_per_million_frames" $true

$previousFrame = -1L
foreach ($checkpoint in $result.heap.checkpoints) {
    Assert-Contract ($checkpoint.frame -gt $previousFrame) "heap checkpoint frames must be strictly increasing"
    Assert-Contract ($checkpoint.frame -le $result.workload.heap_frames) `
        "heap checkpoint frame exceeds the workload"
    Assert-Contract ($checkpoint.allocator_live_bytes -gt 0) "heap checkpoint allocator bytes must be positive"
    Assert-Contract ($checkpoint.gc_managed_bytes -gt 0) "heap checkpoint GC bytes must be positive"
    Assert-Contract ($checkpoint.gc_object_count -gt 0) "heap checkpoint object count must be positive"
    $previousFrame = [int64]$checkpoint.frame
}

$firstCheckpoint = $result.heap.checkpoints[0]
$lastCheckpoint = $result.heap.checkpoints[$result.heap.checkpoints.Count - 1]
Assert-Contract ($firstCheckpoint.frame -eq 0) "the first heap checkpoint must be the warmed baseline"
Assert-Contract ($firstCheckpoint.allocator_live_bytes -eq $result.heap.baseline_bytes) `
    "the first heap checkpoint does not match baseline_bytes"
Assert-Contract ($lastCheckpoint.frame -eq $result.workload.heap_frames) `
    "the final heap checkpoint does not match workload.heap_frames"
Assert-Contract ($lastCheckpoint.allocator_live_bytes -eq $result.heap.final_bytes) `
    "the final heap checkpoint does not match final_bytes"

$baselineBytes = [int64]$result.heap.baseline_bytes
$finalBytes = [int64]$result.heap.final_bytes
$expectedAllowedGrowth = [int64][Math]::Max(65536, [Math]::Floor($baselineBytes / 10.0))
$expectedMaxSlope = [int64][Math]::Max(262144, $baselineBytes)
Assert-Contract ($result.heap.allowed_growth_bytes -eq $expectedAllowedGrowth) `
    "heap.allowed_growth_bytes does not match the CI policy"
Assert-Contract ($result.heap.max_growth_bytes_per_million_frames -eq $expectedMaxSlope) `
    "heap.max_growth_bytes_per_million_frames does not match the CI policy"

$recomputedSlope = Get-HeapSlope -Checkpoints @($result.heap.checkpoints)
Assert-NearlyEqual $result.heap.growth_bytes_per_million_frames $recomputedSlope `
    "heap.growth_bytes_per_million_frames"
Assert-NearlyEqual $metricsByName["heap_growth_bytes_per_million_frames"].median $recomputedSlope `
    "heap_growth_bytes_per_million_frames"

$finalSizeStable = ($finalBytes -le $baselineBytes) -or `
    (($finalBytes - $baselineBytes) -le $expectedAllowedGrowth)
$trendStable = $recomputedSlope -le $expectedMaxSlope
$recomputedStable = $finalSizeStable -and $trendStable
Assert-Contract ($result.heap.stable -eq $recomputedStable) `
    "heap.stable does not match the reported sizes and trend policy"
Assert-Contract ($result.heap.stable -eq $true) "heap stability result is false"

Write-Host "Runtime benchmark contract passed."
Write-Host "  Metrics: $($result.metrics.Count)"
Write-Host "  Closures: $($result.closure_count)"
Write-Host "  GC pause samples: $($result.gc_pause_sample_count)"
Write-Host "  Heap checkpoints: $($result.heap.checkpoints.Count)"
