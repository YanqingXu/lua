param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$SuitePath = "tests/lua/official",
    [string]$ManifestPath = "tests/compatibility/lua51-official-sources.json",
    [string]$LuacPath = "",
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
        schemaVersion = 2
        upstream = "Lua 5.1 official test suite (lua5.1-tests)"
        archive = [ordered]@{
            url = "https://www.lua.org/tests/lua5.1-tests.tar.gz"
            published = "2016-01-18"
            sha256 = "49e4ca6561f82ea605908c5041ab5fad66ed9930fa0686675bd51b02767f18ad"
        }
        referenceCompiler = [ordered]@{
            release = "Lua 5.1.5"
            sourceArchiveUrl = "https://www.lua.org/ftp/lua-5.1.5.tar.gz"
            sourceArchiveSha256 = "2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333"
            oracleFixture = "tests/compatibility/lua51-loadnil-oracle.lua"
            oracleFixtureSha256 = "e57b55964579706ff33bdb66ca6184b331bbd97216f084e76cd56228e4de94d9"
            listingCommand = "luac -l -p tests/compatibility/lua51-loadnil-oracle.lua"
            nestedPrototypeOpcodeSequences = @(
                "LOADNIL/LOADNIL/RETURN",
                "LOADK/LOADBOOL/TEST/JMP/RETURN",
                "LOADNIL/LOADBOOL/TEST/JMP/RETURN"
            )
            upstreamCodeLuaExpectations = @("RETURN", "LOADK/JMP/RETURN", "LOADNIL/JMP/RETURN")
            resolution = "Keep upstream code.lua byte-for-byte; execute exact in-memory oracle corrections for the three release-5.1.5 fixture mismatches before evaluating project compiler parity."
        }
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
if ($manifest.schemaVersion -ne 2) {
    throw "Unsupported official source manifest schemaVersion: $($manifest.schemaVersion)"
}

$expectedArchiveSha = "49e4ca6561f82ea605908c5041ab5fad66ed9930fa0686675bd51b02767f18ad"
$expectedSourceSha = "2640fc56a795f29d28ef15e13c34a47e223960b0240e8cb0a82d9b0738695333"
$expectedFixtureSha = "e57b55964579706ff33bdb66ca6184b331bbd97216f084e76cd56228e4de94d9"
$expectedOpcodeSequences = @(
    "LOADNIL/LOADNIL/RETURN",
    "LOADK/LOADBOOL/TEST/JMP/RETURN",
    "LOADNIL/LOADBOOL/TEST/JMP/RETURN"
)
$expectedUpstreamSequences = @("RETURN", "LOADK/JMP/RETURN", "LOADNIL/JMP/RETURN")

if ($manifest.archive.url -ne "https://www.lua.org/tests/lua5.1-tests.tar.gz" -or
    $manifest.archive.sha256 -ne $expectedArchiveSha -or
    $manifest.referenceCompiler.release -ne "Lua 5.1.5" -or
    $manifest.referenceCompiler.sourceArchiveUrl -ne "https://www.lua.org/ftp/lua-5.1.5.tar.gz" -or
    $manifest.referenceCompiler.sourceArchiveSha256 -ne $expectedSourceSha -or
    (@($manifest.referenceCompiler.nestedPrototypeOpcodeSequences) -join ";") -ne
        ($expectedOpcodeSequences -join ";") -or
    (@($manifest.referenceCompiler.upstreamCodeLuaExpectations) -join ";") -ne
        ($expectedUpstreamSequences -join ";")) {
    throw "Lua 5.1 official archive/compiler provenance is not the locked Lua.org oracle"
}

$oracleFixture = Resolve-RootedPath $manifest.referenceCompiler.oracleFixture
if (-not (Test-Path -LiteralPath $oracleFixture)) {
    throw "Missing Lua 5.1.5 LOADNIL oracle fixture: $($manifest.referenceCompiler.oracleFixture)"
}
$fixtureSha = (Get-FileHash -LiteralPath $oracleFixture -Algorithm SHA256).Hash.ToLowerInvariant()
if ($fixtureSha -ne $expectedFixtureSha -or
    $manifest.referenceCompiler.oracleFixtureSha256 -ne $expectedFixtureSha) {
    throw "Lua 5.1.5 LOADNIL oracle fixture hash mismatch"
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

if ($LuacPath) {
    $resolvedLuac = (Resolve-Path -LiteralPath $LuacPath).Path
    $version = (& $resolvedLuac -v 2>&1 | Out-String).Trim()
    if ($LASTEXITCODE -ne 0 -or $version -notmatch "Lua 5\.1\.5") {
        throw "Reference compiler is not Lua 5.1.5: $version"
    }

    $listing = @(& $resolvedLuac -l -p $oracleFixture 2>&1)
    if ($LASTEXITCODE -ne 0) {
        throw "Lua 5.1.5 luac failed to list the LOADNIL oracle fixture"
    }

    $nestedOpcodeSequences = @()
    $nestedOpcodes = @()
    $inNestedPrototype = $false
    foreach ($line in $listing) {
        if ($line -match '^function <') {
            if ($inNestedPrototype) {
                $nestedOpcodeSequences += ($nestedOpcodes -join "/")
            }
            $inNestedPrototype = $true
            $nestedOpcodes = @()
            continue
        }
        if ($inNestedPrototype -and $line -match '^\s*\d+\s+\[\d+\]\s+([A-Z][A-Z0-9]*)') {
            $nestedOpcodes += $Matches[1]
        }
    }
    if ($inNestedPrototype) {
        $nestedOpcodeSequences += ($nestedOpcodes -join "/")
    }
    if (($nestedOpcodeSequences -join ";") -ne ($expectedOpcodeSequences -join ";")) {
        throw "Lua 5.1.5 luac code oracle mismatch: $($nestedOpcodeSequences -join ';')"
    }
    Write-Host "[OK] Lua 5.1.5 luac oracle verified: $($nestedOpcodeSequences -join '; ')"
}

Write-Host "[OK] Lua 5.1 official source manifest verified: $($actualEntries.Count) files"
