param(
    [Parameter(Mandatory = $true)]
    [string]$BuildDirectory,

    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,

    [Parameter(Mandatory = $true)]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$RuntimeIdentifier,

    [string]$Configuration = "Release",

    [string]$Commit = ""
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

if ($Version -notmatch '^[0-9]+\.[0-9]+\.[0-9]+(?:-rc\.[0-9]+)?$') {
    throw "Version must be a semantic version or -rc.N candidate"
}
if ($RuntimeIdentifier -notmatch '^[a-z0-9_-]+$') {
    throw "RuntimeIdentifier contains unsupported characters"
}

$repository = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$build = (Resolve-Path -LiteralPath $BuildDirectory).Path
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$output = (Resolve-Path -LiteralPath $OutputDirectory).Path
$archiveBase = "lua-cpp-$Version-$RuntimeIdentifier"
$stage = Join-Path $output ".stage-$archiveBase"
$packageRoot = Join-Path $stage $archiveBase

if (Test-Path -LiteralPath $stage) {
    $resolvedStage = (Resolve-Path -LiteralPath $stage).Path
    if (-not $resolvedStage.StartsWith($output + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove a staging directory outside the output directory"
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}
New-Item -ItemType Directory -Path $packageRoot | Out-Null

try {
    & cmake --install $build --config $Configuration --prefix $packageRoot
    if ($LASTEXITCODE -ne 0) {
        throw "cmake --install failed with exit code $LASTEXITCODE"
    }

    Copy-Item -LiteralPath (Join-Path $repository "CHANGELOG.md") -Destination $packageRoot
    Copy-Item -LiteralPath (Join-Path $repository "SECURITY.md") -Destination $packageRoot
    $releaseDocs = Join-Path $packageRoot "share/lua_cpp/release"
    New-Item -ItemType Directory -Force -Path $releaseDocs | Out-Null
    Copy-Item -LiteralPath (Join-Path $repository "docs/release/release-checklist.md") -Destination $releaseDocs
    Copy-Item -LiteralPath (Join-Path $repository "docs/release/rc-notes-0.1.0.md") -Destination $releaseDocs

    if ([string]::IsNullOrWhiteSpace($Commit)) {
        $Commit = (& git -C $repository rev-parse HEAD).Trim()
        if ($LASTEXITCODE -ne 0) {
            throw "Unable to resolve release commit"
        }
    }
    $python = Get-Command py -ErrorAction SilentlyContinue
    if ($null -eq $python) {
        $python = Get-Command python3 -ErrorAction SilentlyContinue
    }
    if ($null -eq $python) {
        $python = Get-Command python -ErrorAction Stop
    }
    $sbomInside = Join-Path $packageRoot "share/lua_cpp/sbom.spdx.json"
    $pythonArguments = @()
    if ($python.Name -eq "py.exe" -or $python.Name -eq "py") {
        $pythonArguments += "-3"
    }
    $pythonArguments += @(
        (Join-Path $repository "tools/generate_sbom.py"),
        "--root", $packageRoot,
        "--output", $sbomInside,
        "--name", $archiveBase,
        "--version", $Version,
        "--commit", $Commit
    )
    & $python.Source @pythonArguments
    if ($LASTEXITCODE -ne 0) {
        throw "SBOM generation failed with exit code $LASTEXITCODE"
    }

    $archive = Join-Path $output "$archiveBase.zip"
    if (Test-Path -LiteralPath $archive) {
        Remove-Item -LiteralPath $archive -Force
    }
    Push-Location $stage
    try {
        & cmake -E tar cf $archive --format=zip $archiveBase
        if ($LASTEXITCODE -ne 0) {
            throw "Archive creation failed with exit code $LASTEXITCODE"
        }
    } finally {
        Pop-Location
    }

    $externalSbom = Join-Path $output "$archiveBase.spdx.json"
    Copy-Item -LiteralPath $sbomInside -Destination $externalSbom -Force
    $checksumPath = Join-Path $output "$archiveBase.SHA256SUMS"
    $checksumLines = @()
    foreach ($artifact in @($archive, $externalSbom)) {
        $hash = (Get-FileHash -LiteralPath $artifact -Algorithm SHA256).Hash.ToLowerInvariant()
        $checksumLines += "$hash  $([IO.Path]::GetFileName($artifact))"
    }
    $checksumLines | Set-Content -LiteralPath $checksumPath -Encoding ascii

    [ordered]@{
        schemaVersion     = 1
        version           = $Version
        runtimeIdentifier = $RuntimeIdentifier
        commit            = $Commit
        archive           = [IO.Path]::GetFileName($archive)
        sbom              = [IO.Path]::GetFileName($externalSbom)
        checksums         = [IO.Path]::GetFileName($checksumPath)
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output "$archiveBase.manifest.json") -Encoding utf8

    $validatorArguments = @()
    if ($python.Name -eq "py.exe" -or $python.Name -eq "py") {
        $validatorArguments += "-3"
    }
    $validatorArguments += @(
        (Join-Path $repository "tools/validate_release_artifacts.py"),
        "--archive", $archive,
        "--sbom", $externalSbom,
        "--checksums", $checksumPath,
        "--manifest", (Join-Path $output "$archiveBase.manifest.json"),
        "--expected-version", $Version,
        "--expected-rid", $RuntimeIdentifier,
        "--expected-commit", $Commit
    )
    & $python.Source @validatorArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Release artifact validation failed with exit code $LASTEXITCODE"
    }
} finally {
    if (Test-Path -LiteralPath $stage) {
        Remove-Item -LiteralPath $stage -Recurse -Force
    }
}

Write-Host "Release package created: $archiveBase"
