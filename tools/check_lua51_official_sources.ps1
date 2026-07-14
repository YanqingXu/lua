param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$SuitePath = "tests/lua/official",
    [string]$ManifestPath = "tests/compatibility/lua51-official-sources.json",
    [switch]$UpdateManifest
)

$ErrorActionPreference = "Stop"

function Resolve-RootedPath {
    param([string]$Path)
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }
    return Join-Path $Root $Path
}

$suiteRoot = (Resolve-Path -LiteralPath (Resolve-RootedPath $SuitePath)).Path
$manifestFile = Resolve-RootedPath $ManifestPath
$suitePrefix = $suiteRoot.TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar

$actualEntries = @(Get-ChildItem -LiteralPath $suiteRoot -Recurse -Force -File | Sort-Object FullName | ForEach-Object {
    [ordered]@{
        path = $_.FullName.Substring($suitePrefix.Length).Replace("\", "/")
        sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
})

if ($UpdateManifest) {
    $parent = Split-Path -Parent $manifestFile
    if (-not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    $document = [ordered]@{
        schemaVersion = 1
        upstream = "Lua 5.1 official test suite"
        sourcePolicy = "Strict runs execute an exact temporary copy; repository sources are never rewritten."
        entries = $actualEntries
    }
    $json = $document | ConvertTo-Json -Depth 4
    [System.IO.File]::WriteAllText($manifestFile, $json + [Environment]::NewLine, [System.Text.UTF8Encoding]::new($false))
    Write-Host "[UPDATED] Lua 5.1 official source manifest: $($actualEntries.Count) files"
    return
}

if (-not (Test-Path -LiteralPath $manifestFile)) {
    throw "Missing official source manifest: $ManifestPath"
}

$manifest = Get-Content -LiteralPath $manifestFile -Raw | ConvertFrom-Json
if ($manifest.schemaVersion -ne 1) {
    throw "Unsupported official source manifest schemaVersion: $($manifest.schemaVersion)"
}

$expectedByPath = @{}
foreach ($entry in @($manifest.entries)) {
    if ($expectedByPath.ContainsKey($entry.path)) {
        throw "Duplicate official source manifest path: $($entry.path)"
    }
    $expectedByPath[$entry.path] = $entry.sha256
}

$actualByPath = @{}
foreach ($entry in $actualEntries) {
    $actualByPath[$entry.path] = $entry.sha256
}

$failures = @()
foreach ($path in $expectedByPath.Keys) {
    if (-not $actualByPath.ContainsKey($path)) {
        $failures += "missing: $path"
    } elseif ($actualByPath[$path] -ne $expectedByPath[$path]) {
        $failures += "hash mismatch: $path"
    }
}
foreach ($path in $actualByPath.Keys) {
    if (-not $expectedByPath.ContainsKey($path)) {
        $failures += "unexpected: $path"
    }
}

if ($failures.Count -gt 0) {
    Write-Host "[FAIL] Lua 5.1 official sources differ from the strict manifest:"
    $failures | ForEach-Object { Write-Host " - $_" }
    throw "Lua 5.1 official source integrity check failed"
}

Write-Host "[OK] Lua 5.1 official source manifest verified: $($actualEntries.Count) files"
