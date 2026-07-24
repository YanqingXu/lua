param(
    [switch]$AllowDirty,
    [string]$ExpectedVersion = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repository = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$cmake = Get-Content -Raw -LiteralPath (Join-Path $repository "CMakeLists.txt")
$versionHeader = Get-Content -Raw -LiteralPath (Join-Path $repository "src/lua_cpp_version.h")
if ($cmake -notmatch 'project\(lua_cpp VERSION ([0-9]+\.[0-9]+\.[0-9]+)') {
    throw "Unable to read the CMake project version"
}
$cmakeVersion = $Matches[1]
if ($versionHeader -notmatch '#define LUA_CPP_VERSION "([^"]+)"') {
    throw "Unable to read LUA_CPP_VERSION"
}
$headerVersion = $Matches[1]
if ($cmakeVersion -ne $headerVersion) {
    throw "CMake version $cmakeVersion does not match public header version $headerVersion"
}
if (-not [string]::IsNullOrWhiteSpace($ExpectedVersion)) {
    $baseVersion = $ExpectedVersion -replace '-rc\.[0-9]+$', ''
    if ($baseVersion -ne $cmakeVersion) {
        throw "Expected version $ExpectedVersion does not match project version $cmakeVersion"
    }
}

$required = @(
    "CHANGELOG.md",
    "SECURITY.md",
    "docs/release/release-checklist.md",
    "docs/release/rc-notes-0.1.0.md",
    ".github/workflows/release.yml",
    "tests/coverage/component-thresholds.json",
    "tests/compatibility/runtime-benchmark-absolute-policy.json",
    "tools/generate_sbom.py",
    "tools/validate_release_artifacts.py",
    "tools/package_release.ps1"
)
foreach ($relative in $required) {
    if (-not (Test-Path -LiteralPath (Join-Path $repository $relative) -PathType Leaf)) {
        throw "Release readiness file is missing: $relative"
    }
}

if (-not $AllowDirty) {
    $status = @(& git -C $repository status --porcelain)
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to inspect git status"
    }
    if ($status.Count -ne 0) {
        throw "Release readiness requires a clean worktree"
    }
}

$python = Get-Command py -ErrorAction SilentlyContinue
if ($null -eq $python) {
    $python = Get-Command python3 -ErrorAction SilentlyContinue
}
if ($null -eq $python) {
    $python = Get-Command python -ErrorAction Stop
}
$arguments = @()
if ($python.Name -eq "py.exe" -or $python.Name -eq "py") {
    $arguments += "-3"
}
$arguments += (Join-Path $repository "tools/check_lua51_public_api_contract.py")
& $python.Source @arguments
if ($LASTEXITCODE -ne 0) {
    throw "Public API contract failed release readiness"
}

Write-Host "Release readiness source checks passed."
Write-Host "  Version: $cmakeVersion"
Write-Host "  ABI: 0"
if ($AllowDirty) {
    Write-Host "  Worktree cleanliness: explicitly skipped"
}
