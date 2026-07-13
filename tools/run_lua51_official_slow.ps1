param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$LuaApp = "",
    [ValidateSet("sort", "verybig")]
    [string[]]$Case = @("sort", "verybig"),
    [int]$TimeoutSeconds = 300,
    [switch]$ExpectVerybigTimeout
)

$ErrorActionPreference = "Stop"

function Resolve-ToolPath {
    param([string]$Path, [string]$DefaultRelative)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return Join-Path $Root $DefaultRelative
    }

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    return Join-Path $Root $Path
}

$luaAppPath = Resolve-ToolPath $LuaApp "bin\lua_app.exe"
if (-not (Test-Path -LiteralPath $luaAppPath)) {
    throw "Missing lua_app executable: $luaAppPath. Build lua_app.vcxproj first."
}

$suiteDir = Join-Path $Root "tests/lua/official"
if (-not (Test-Path -LiteralPath (Join-Path $suiteDir "all.lua"))) {
    throw "Missing official Lua 5.1 suite directory: $suiteDir"
}

$integrityScript = Join-Path $Root "tools/check_lua51_official_sources.ps1"
& $integrityScript -Root $Root

$driver = Join-Path ([System.IO.Path]::GetTempPath()) "lua51-official-slow-driver.lua"
$temporaryArtifacts = [System.Collections.Generic.List[string]]::new()
$temporaryArtifacts.Add($driver) | Out-Null
$driverSource = @'
gcinfo = gcinfo or function()
    return collectgarbage("count")
end

local function roundtrip_dofile(name)
    local f = assert(loadfile(name))
    local binary = string.dump(f)
    f = assert(loadstring(binary))
    return f()
end

local case = assert(arg and arg[1], "missing slow official case")
if case == "sort" then
    roundtrip_dofile("sort.lua")
elseif case == "verybig" then
    assert(roundtrip_dofile("verybig.lua") == 10)
else
    error("unknown slow official case: " .. tostring(case))
end
'@

Set-Content -LiteralPath $driver -Value $driverSource -Encoding ASCII

$previous = Get-Location
try {
    Set-Location -LiteralPath $suiteDir

    foreach ($caseName in $Case) {
        $timer = [System.Diagnostics.Stopwatch]::StartNew()
        $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
        $startInfo.FileName = $luaAppPath
        $startInfo.Arguments = "`"$driver`" $caseName"
        $startInfo.WorkingDirectory = $suiteDir
        $startInfo.UseShellExecute = $false
        $startInfo.CreateNoWindow = $true
        $startInfo.RedirectStandardOutput = $true
        $startInfo.RedirectStandardError = $true
        $process = [System.Diagnostics.Process]::new()
        $process.StartInfo = $startInfo
        if (-not $process.Start()) {
            throw "could not start slow official case: $caseName"
        }
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()

        $completed = $process.WaitForExit($TimeoutSeconds * 1000)
        if (-not $completed) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            $process.WaitForExit()
            $null = $stdoutTask.Result
            $null = $stderrTask.Result
            $timer.Stop()
            if ($caseName -eq "verybig" -and $ExpectVerybigTimeout) {
                Write-Host "[XFAIL] verybig dump/undump exceeded ${TimeoutSeconds}s"
                continue
            }
            throw "slow official case timed out: $caseName (${TimeoutSeconds}s)"
        }

        $process.WaitForExit()
        $stdoutText = $stdoutTask.Result
        $stderrText = $stderrTask.Result
        $timer.Stop()
        if ($process.ExitCode -ne 0) {
            throw "slow official case failed: $caseName (exit $($process.ExitCode)): $stderrText"
        }
        if ($caseName -eq "verybig" -and $ExpectVerybigTimeout) {
            throw "verybig unexpectedly passed; remove its XFAIL and promote it to a required slow gate"
        }

        $seconds = [Math]::Round($timer.Elapsed.TotalSeconds, 2)
        Write-Host "[OK] $caseName dump/undump slow gate passed in ${seconds}s"
    }
} finally {
    Set-Location -LiteralPath $previous
    foreach ($artifact in $temporaryArtifacts) {
        Remove-Item -LiteralPath $artifact -ErrorAction SilentlyContinue
    }
    & $integrityScript -Root $Root
}
