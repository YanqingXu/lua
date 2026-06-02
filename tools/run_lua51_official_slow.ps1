param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$LuaApp = "",
    [ValidateSet("sort", "verybig")]
    [string[]]$Case = @("sort", "verybig")
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

$suiteDir = Join-Path $Root "tests\lua\official"
if (-not (Test-Path -LiteralPath (Join-Path $suiteDir "all.lua"))) {
    throw "Missing official Lua 5.1 suite directory: $suiteDir"
}

$driver = Join-Path ([System.IO.Path]::GetTempPath()) "lua51-official-slow-driver.lua"
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
        $elapsed = Measure-Command {
            & $luaAppPath $driver $caseName
            if ($LASTEXITCODE -ne 0) {
                throw "slow official case failed: $caseName (exit $LASTEXITCODE)"
            }
        }

        $seconds = [Math]::Round($elapsed.TotalSeconds, 2)
        Write-Host "[OK] $caseName dump/undump slow gate passed in ${seconds}s"
    }
} finally {
    Set-Location -LiteralPath $previous
    Remove-Item -LiteralPath $driver -ErrorAction SilentlyContinue
}
