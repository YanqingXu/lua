param(
    [string]$Root = "",
    [Parameter(Mandatory = $true)]
    [string]$ReferenceLua,
    [string]$CandidateLua = "bin/lua_app.exe",
    [string]$CasesPath = "tests/compatibility/lua51-differential-cases.json",
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

function Normalize-Output {
    param([string]$Text)
    return ($Text -replace "`r`n", "`n" -replace "`r", "`n")
}

function Invoke-LuaCase {
    param(
        [string]$Executable,
        [string]$Script,
        [int]$Timeout
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Executable
    $startInfo.Arguments = "`"$Script`""
    $startInfo.WorkingDirectory = $Root
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    if (-not $process.Start()) {
        throw "Could not start Lua executable: $Executable"
    }

    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    if (-not $process.WaitForExit($Timeout * 1000)) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        $process.WaitForExit()
        $null = $stdoutTask.Result
        $null = $stderrTask.Result
        return [ordered]@{ outcome = "timeout"; exitStatus = $null; stdout = ""; stderr = "" }
    }

    $process.WaitForExit()
    return [ordered]@{
        outcome = "completed"
        exitStatus = $process.ExitCode
        stdout = Normalize-Output $stdoutTask.Result
        stderr = Normalize-Output $stderrTask.Result
    }
}

$referencePath = Resolve-RootedPath $ReferenceLua
$candidatePath = Resolve-RootedPath $CandidateLua
$casesFile = Resolve-RootedPath $CasesPath
$manifest = Get-Content -LiteralPath $casesFile -Raw | ConvertFrom-Json
if ($manifest.schemaVersion -ne 1) {
    throw "Unsupported differential case schemaVersion: $($manifest.schemaVersion)"
}

$failures = [System.Collections.Generic.List[string]]::new()
$results = [System.Collections.Generic.List[object]]::new()

foreach ($case in @($manifest.cases)) {
    $scriptPath = Resolve-RootedPath $case.script
    $reference = Invoke-LuaCase -Executable $referencePath -Script $scriptPath -Timeout $TimeoutSeconds
    $candidate = Invoke-LuaCase -Executable $candidatePath -Script $scriptPath -Timeout $TimeoutSeconds
    $mismatches = [System.Collections.Generic.List[string]]::new()

    foreach ($field in @("outcome", "exitStatus", "stdout", "stderr")) {
        if ($reference[$field] -ne $candidate[$field]) {
            $mismatches.Add($field) | Out-Null
        }
    }
    if ($mismatches.Count -gt 0) {
        $failures.Add("$($case.id): $($mismatches -join ', ') differ") | Out-Null
    }

    $results.Add([ordered]@{
        id = $case.id
        evidence = @($case.evidence)
        passed = $mismatches.Count -eq 0
        mismatches = @($mismatches)
        reference = $reference
        candidate = $candidate
    }) | Out-Null
}

$document = [ordered]@{
    schemaVersion = 1
    channel = "lua51-differential"
    reference = $referencePath
    candidate = $candidatePath
    passed = $failures.Count -eq 0
    cases = @($results)
}

if ($ResultPath) {
    $resolvedResultPath = if ([System.IO.Path]::IsPathRooted($ResultPath)) { $ResultPath } else { Join-Path $Root $ResultPath }
    $parent = Split-Path -Parent $resolvedResultPath
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $json = $document | ConvertTo-Json -Depth 8
    [System.IO.File]::WriteAllText($resolvedResultPath, $json + [Environment]::NewLine, [System.Text.UTF8Encoding]::new($false))
}

if ($failures.Count -gt 0) {
    Write-Host "[FAIL] Lua 5.1 differential checks failed:"
    $failures | ForEach-Object { Write-Host " - $_" }
    exit 1
}

Write-Host "[OK] Lua 5.1 differential checks passed: $($results.Count) cases"
