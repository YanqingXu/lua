param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$Executable = "bin\lua_app.exe",
    [int]$TimeoutSeconds = 300,
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

& $integrityScript -Root $Root

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("lua51_official_strict_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tempRoot | Out-Null

$outcome = "failed"
$exitCode = $null
$stdoutText = ""
$stderrText = ""
$completionSentinelPresent = $false
$elapsed = [System.Diagnostics.Stopwatch]::StartNew()
$processElapsed = $null
$stageMarkers = @(
    [pscustomobject]@{ script = "main.lua"; pattern = '^testing lua\.c options$' },
    [pscustomobject]@{ script = "gc.lua"; pattern = '^testing garbage collection$' },
    [pscustomobject]@{ script = "db.lua"; pattern = '^testing debug library and debug information$' },
    [pscustomobject]@{ script = "calls.lua"; pattern = '^testing functions and calls$' },
    [pscustomobject]@{ script = "strings.lua"; pattern = '^testing strings and string library$' },
    [pscustomobject]@{ script = "literals.lua"; pattern = '^testing scanner$' },
    [pscustomobject]@{ script = "attrib.lua"; pattern = '^testing require$' },
    [pscustomobject]@{ script = "locals.lua"; pattern = '^testing local variables plus some extra stuff$' },
    [pscustomobject]@{ script = "constructs.lua"; pattern = '^testing syntax$' },
    [pscustomobject]@{ script = "code.lua"; pattern = '^testing code generation and optimizations$' },
    [pscustomobject]@{ script = "big.lua"; pattern = '^testing string length overflow$' },
    [pscustomobject]@{ script = "nextvar.lua"; pattern = '^testing tables, next, and for$' },
    [pscustomobject]@{ script = "pm.lua"; pattern = '^testing pattern matching$' },
    [pscustomobject]@{ script = "api.lua"; pattern = 'testC not active: skipping API tests' },
    [pscustomobject]@{ script = "events.lua"; pattern = '^testing metatables$' },
    [pscustomobject]@{ script = "vararg.lua"; pattern = '^testing vararg$' },
    [pscustomobject]@{ script = "closure.lua"; pattern = '^testing closures and coroutines$' },
    [pscustomobject]@{ script = "errors.lua"; pattern = '^testing errors$' },
    [pscustomobject]@{ script = "math.lua"; pattern = '^testing numbers and math lib$' },
    [pscustomobject]@{ script = "sort.lua"; pattern = '^testing sort$' },
    [pscustomobject]@{ script = "verybig.lua"; pattern = '^testing large programs \(>64k\)$' },
    [pscustomobject]@{ script = "files.lua"; pattern = '^testing i/o$' }
)
$stageStarts = [System.Collections.Generic.List[object]]::new()
$stageStarts.Add([pscustomobject]@{ script = "all.lua"; startSeconds = 0.0; marker = "process start" })
$seenStages = @{ "all.lua" = $true }

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
    $processElapsed = [System.Diagnostics.Stopwatch]::StartNew()

    # Consume redirected output while the process runs. Besides avoiding pipe
    # back-pressure, this gives the strict lane timestamped script milestones
    # without changing a byte of the SHA-locked upstream sources.
    $stdoutLines = [System.Collections.Generic.List[string]]::new()
    $stderrLines = [System.Collections.Generic.List[string]]::new()
    $stdoutEof = $false
    $stderrEof = $false
    $stdoutTask = $process.StandardOutput.ReadLineAsync()
    $stderrTask = $process.StandardError.ReadLineAsync()
    $timedOut = $false

    while (-not ($process.HasExited -and $stdoutEof -and $stderrEof)) {
        while (-not $stdoutEof -and $stdoutTask.IsCompleted) {
            $line = $stdoutTask.Result
            if ($null -eq $line) {
                $stdoutEof = $true
                break
            }

            $stdoutLines.Add($line)
            foreach ($marker in $stageMarkers) {
                if (-not $seenStages.ContainsKey($marker.script) -and $line -match $marker.pattern) {
                    $seenStages[$marker.script] = $true
                    $stageStarts.Add([pscustomobject]@{
                        script = $marker.script
                        startSeconds = [math]::Round($processElapsed.Elapsed.TotalSeconds, 3)
                        marker = $line.Trim()
                    })
                    break
                }
            }
            $stdoutTask = $process.StandardOutput.ReadLineAsync()
        }

        while (-not $stderrEof -and $stderrTask.IsCompleted) {
            $line = $stderrTask.Result
            if ($null -eq $line) {
                $stderrEof = $true
                break
            }
            $stderrLines.Add($line)
            $stderrTask = $process.StandardError.ReadLineAsync()
        }

        if (-not $process.HasExited -and $processElapsed.Elapsed.TotalSeconds -ge $TimeoutSeconds) {
            $timedOut = $true
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        }

        if (-not ($process.HasExited -and $stdoutEof -and $stderrEof)) {
            Start-Sleep -Milliseconds 5
        }
    }

    $process.WaitForExit()
    $processElapsed.Stop()
    $stdoutText = $stdoutLines -join [Environment]::NewLine
    $stderrText = $stderrLines -join [Environment]::NewLine
    $completionSentinelPresent = $stdoutLines.Contains("final OK !!!")
    if ($timedOut) {
        $outcome = "timeout"
    } else {
        $exitCode = $process.ExitCode
        $outcome = if ($exitCode -eq 0 -and $completionSentinelPresent) { "passed" } else { "failed" }
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
    if ($null -ne $processElapsed -and $processElapsed.IsRunning) {
        $processElapsed.Stop()
    }
    if (Test-Path -LiteralPath $tempRoot) {
        Remove-Item -LiteralPath $tempRoot -Recurse -Force
    }
}

$stageProfile = @()
$profileElapsedSeconds = if ($null -ne $processElapsed) {
    $processElapsed.Elapsed.TotalSeconds
} else {
    $elapsed.Elapsed.TotalSeconds
}
for ($i = 0; $i -lt $stageStarts.Count; ++$i) {
    $start = [double]$stageStarts[$i].startSeconds
    $end = if ($i + 1 -lt $stageStarts.Count) {
        [double]$stageStarts[$i + 1].startSeconds
    } else {
        $profileElapsedSeconds
    }
    $stageProfile += [ordered]@{
        script = $stageStarts[$i].script
        startSeconds = [math]::Round($start, 3)
        elapsedUntilNextStageSeconds = [math]::Round([math]::Max(0.0, $end - $start), 3)
        marker = $stageStarts[$i].marker
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
    processElapsedSeconds = [math]::Round($profileElapsedSeconds, 3)
    completionSentinelPresent = $completionSentinelPresent
    stageProfile = $stageProfile
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

if ($outcome -ne "passed") {
    throw "official-strict did not pass: $outcome"
}

Write-Host "[OK] official-strict passed"
