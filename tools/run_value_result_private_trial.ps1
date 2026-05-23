param(
    [string]$BuildDir = "build\cmake-value-result-private",
    [string]$Configuration = "Debug",
    [string]$Generator = "",
    [string]$Platform = "x64",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$smokeScript = Join-Path $PSScriptRoot "run_cmake_smoke.ps1"

& $smokeScript `
    -BuildDir $BuildDir `
    -Configuration $Configuration `
    -Generator $Generator `
    -Platform $Platform `
    -ConfigureArgs "-DLUA_VALUE_RESULT_PRIVATE_LEGACY_FIELDS=ON" `
    -Clean:$Clean
