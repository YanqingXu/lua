param(
    [switch]$AllowDirty,
    [string]$ExpectedVersion = "",
    [string]$ExpectedCommit = "",
    [string]$EvidenceRepository = "",
    [string]$EvidenceOutput = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repository = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
Import-Module (Join-Path $PSScriptRoot "release_identity.psm1") -Force
$evidencePath = $null
if (-not [string]::IsNullOrWhiteSpace($EvidenceOutput)) {
    if ($AllowDirty) {
        throw "Normalized source-readiness evidence cannot be generated with -AllowDirty"
    }
    $evidencePath = if ([System.IO.Path]::IsPathRooted($EvidenceOutput)) {
        [System.IO.Path]::GetFullPath($EvidenceOutput)
    }
    else {
        [System.IO.Path]::GetFullPath((Join-Path $repository $EvidenceOutput))
    }
    if (Test-Path -LiteralPath $evidencePath -PathType Leaf) {
        Remove-Item -LiteralPath $evidencePath -Force
    }
}
$cmake = Get-Content -Raw -LiteralPath (Join-Path $repository "CMakeLists.txt")
$versionHeader = Get-Content -Raw -LiteralPath (Join-Path $repository "src/lua_cpp_version.h")
$releaseIdentity = Get-LuaCppReleaseIdentity `
    -CMakeText $cmake `
    -VersionHeaderText $versionHeader
$cmakeVersion = $releaseIdentity.ProjectVersion
$abiVersion = $releaseIdentity.AbiVersion
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
    "tools/verify_release_governance.py",
    "tools/verify_source_readiness_evidence.py",
    "tools/verify_release_evidence.py",
    "tools/build_release_body.py",
    "tools/release_identity.psm1",
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

$candidateSha = (& git -C $repository rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $candidateSha -notmatch '^[0-9a-f]{40}$') {
    throw "Unable to resolve the exact source commit"
}
if (
    -not [string]::IsNullOrWhiteSpace($ExpectedCommit) -and
    $candidateSha -cne $ExpectedCommit
) {
    throw "Expected commit $ExpectedCommit does not match source commit $candidateSha"
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
Write-Host "  ABI: $abiVersion"
if ($AllowDirty) {
    Write-Host "  Worktree cleanliness: explicitly skipped"
}

if ($null -ne $evidencePath) {
    if ([string]::IsNullOrWhiteSpace($ExpectedVersion)) {
        throw "Normalized source-readiness evidence requires -ExpectedVersion"
    }
    if ($ExpectedCommit -notmatch '^[0-9a-f]{40}$') {
        throw "Normalized source-readiness evidence requires a full lowercase -ExpectedCommit"
    }
    if ($EvidenceRepository -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') {
        throw "Normalized source-readiness evidence requires owner/name -EvidenceRepository"
    }
    $evidence = [ordered]@{
        schema = "lua-cpp.source-readiness-evidence/v1"
        generated_at = [DateTime]::UtcNow.ToString(
            "yyyy-MM-ddTHH:mm:ssZ",
            [System.Globalization.CultureInfo]::InvariantCulture
        )
        repository = $EvidenceRepository
        candidate_sha = $candidateSha
        version = $ExpectedVersion
        project_version = $cmakeVersion
        abi_version = $abiVersion
        checks = [ordered]@{
            source_version_consistent = "passed"
            required_files_present = "passed"
            worktree_clean = "passed"
            public_api_contract = "passed"
        }
    }
    $evidenceDirectory = Split-Path -Parent $evidencePath
    New-Item -ItemType Directory -Force -Path $evidenceDirectory | Out-Null
    $temporaryPath = Join-Path $evidenceDirectory (
        ".$([System.IO.Path]::GetFileName($evidencePath)).$([guid]::NewGuid().ToString('N')).tmp"
    )
    try {
        $json = ($evidence | ConvertTo-Json -Depth 4) + [Environment]::NewLine
        $utf8NoBom = [System.Text.UTF8Encoding]::new($false)
        [System.IO.File]::WriteAllText($temporaryPath, $json, $utf8NoBom)
        [System.IO.File]::Move($temporaryPath, $evidencePath)
    }
    finally {
        if (Test-Path -LiteralPath $temporaryPath -PathType Leaf) {
            Remove-Item -LiteralPath $temporaryPath -Force
        }
    }
    Write-Host "  Evidence: $evidencePath"
}
