Set-StrictMode -Version Latest

function Resolve-LuaCppNormalizedDirectory {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    try {
        $resolved = (Resolve-Path -LiteralPath $Path -ErrorAction Stop).Path
    } catch {
        throw "$Label does not exist or cannot be resolved: $Path"
    }
    $full = [IO.Path]::GetFullPath($resolved)
    $root = [IO.Path]::GetPathRoot($full)
    while ($full.Length -gt $root.Length -and
           ($full.EndsWith([IO.Path]::DirectorySeparatorChar) -or
            $full.EndsWith([IO.Path]::AltDirectorySeparatorChar))) {
        $full = $full.Substring(0, $full.Length - 1)
    }
    return $full.Replace('\', '/')
}

function Assert-LuaCppCanonicalProvenanceFields {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RawJson,

        [Parameter(Mandatory = $true)]
        [string[]]$ExpectedFields
    )

    foreach ($field in $ExpectedFields) {
        $fieldPattern = '"' + [regex]::Escape($field) + '"\s*:'
        $fieldOccurrences = [regex]::Matches($RawJson, $fieldPattern).Count
        if ($fieldOccurrences -ne 1) {
            throw "Build provenance field '$field' must occur exactly once with its canonical name"
        }
    }
}

function Assert-LuaCppRuntimeIdentifierTargetIdentity {
    param(
        [Parameter(Mandatory = $true)]
        [PSCustomObject]$Provenance,

        [Parameter(Mandatory = $true)]
        [string]$RuntimeIdentifier
    )

    $expectedSystemName = ""
    $acceptedProcessors = @()
    $acceptedGeneratorPlatforms = @()
    switch -CaseSensitive ($RuntimeIdentifier) {
        "windows-x64" {
            $expectedSystemName = "Windows"
            $acceptedProcessors = @("amd64", "x86_64", "x86-64", "x64")
            $acceptedGeneratorPlatforms = @("amd64", "x86_64", "x86-64", "x64")
            break
        }
        "linux-x64" {
            $expectedSystemName = "Linux"
            $acceptedProcessors = @("amd64", "x86_64", "x86-64", "x64")
            $acceptedGeneratorPlatforms = @("amd64", "x86_64", "x86-64", "x64")
            break
        }
        "macos-arm64" {
            $expectedSystemName = "Darwin"
            $acceptedProcessors = @("aarch64", "arm64")
            $acceptedGeneratorPlatforms = @("aarch64", "arm64")
            break
        }
        default {
            throw "Unsupported RuntimeIdentifier '$RuntimeIdentifier'; expected exactly windows-x64, linux-x64, or macos-arm64"
        }
    }

    if (-not [string]::Equals(
            $Provenance.system_name,
            $expectedSystemName,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Build provenance target system '$($Provenance.system_name)' does not match RuntimeIdentifier '$RuntimeIdentifier'"
    }
    $normalizedProcessor = $Provenance.system_processor.ToLowerInvariant()
    if ($normalizedProcessor -cnotin $acceptedProcessors) {
        throw "Build provenance target processor '$($Provenance.system_processor)' does not match RuntimeIdentifier '$RuntimeIdentifier'"
    }
    if ($Provenance.pointer_size -ne 8) {
        throw "Build provenance pointer size '$($Provenance.pointer_size)' does not match 64-bit RuntimeIdentifier '$RuntimeIdentifier'"
    }

    if (-not [string]::IsNullOrEmpty($Provenance.generator_platform)) {
        $normalizedGeneratorPlatform = $Provenance.generator_platform.ToLowerInvariant()
        if ($normalizedGeneratorPlatform -cnotin $acceptedGeneratorPlatforms) {
            throw "Build provenance generator platform '$($Provenance.generator_platform)' does not match RuntimeIdentifier '$RuntimeIdentifier'"
        }
    }
}

function Test-LuaCppBuildProvenance {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildDirectory,

        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string]$Commit,

        [string]$Configuration = "",

        [string]$RuntimeIdentifier = ""
    )

    if ($Commit -notmatch '^[0-9a-fA-F]{40}$') {
        throw "Expected package commit must be exactly 40 hexadecimal characters"
    }
    $expectedCommit = $Commit.ToLowerInvariant()
    $normalizedBuild = Resolve-LuaCppNormalizedDirectory `
        -Path $BuildDirectory `
        -Label "Build directory"
    $normalizedRepository = Resolve-LuaCppNormalizedDirectory `
        -Path $Repository `
        -Label "Repository"
    $provenancePath = Join-Path $normalizedBuild "lua-cpp-build-provenance.json"
    if (-not (Test-Path -LiteralPath $provenancePath -PathType Leaf)) {
        throw "Build provenance is missing: $provenancePath"
    }

    try {
        $rawProvenance = Get-Content -Raw -LiteralPath $provenancePath
        $provenance = $rawProvenance | ConvertFrom-Json -ErrorAction Stop
    } catch {
        throw "Build provenance is not valid JSON: $provenancePath"
    }
    if ($null -eq $provenance -or $provenance -isnot [PSCustomObject]) {
        throw "Build provenance root must be a JSON object"
    }

    $expectedFields = @(
        "schema",
        "source_git_status",
        "source_git_sha",
        "source_directory",
        "binary_directory",
        "generator",
        "generator_platform",
        "generator_toolset",
        "system_name",
        "system_processor",
        "pointer_size",
        "cmake_version",
        "build_type",
        "configuration_types",
        "configured_at"
    )
    $actualFields = @($provenance.PSObject.Properties.Name)
    $fieldDifference = @(Compare-Object `
        -ReferenceObject $expectedFields `
        -DifferenceObject $actualFields)
    if ($fieldDifference.Count -ne 0) {
        throw "Build provenance field set does not match lua-cpp.build-provenance/v2"
    }
    Assert-LuaCppCanonicalProvenanceFields `
        -RawJson $rawProvenance `
        -ExpectedFields $expectedFields

    if ($provenance.schema -ne "lua-cpp.build-provenance/v2") {
        throw "Build provenance schema mismatch"
    }
    if ($provenance.source_git_status -ne "exact") {
        throw "Build provenance has no exact Git source identity"
    }
    if ($provenance.source_git_sha -isnot [string] -or
        $provenance.source_git_sha -notmatch '^[0-9a-fA-F]{40}$') {
        throw "Build provenance source Git SHA is not exactly 40 hexadecimal characters"
    }
    $provenanceCommit = $provenance.source_git_sha.ToLowerInvariant()
    if ($provenanceCommit -ne $expectedCommit) {
        throw "Build provenance SHA $provenanceCommit does not match package commit $expectedCommit"
    }

    foreach ($field in @("source_directory", "binary_directory", "generator",
            "system_name", "system_processor", "cmake_version")) {
        if ($provenance.$field -isnot [string] -or
            [string]::IsNullOrWhiteSpace($provenance.$field)) {
            throw "Build provenance field '$field' is missing"
        }
    }
    foreach ($field in @("generator_platform", "generator_toolset")) {
        if ($provenance.$field -isnot [string]) {
            throw "Build provenance field '$field' must be a string"
        }
    }
    if ($provenance.build_type -isnot [string]) {
        throw "Build provenance build_type must be a string"
    }
    if (($provenance.pointer_size -isnot [int]) -and
        ($provenance.pointer_size -isnot [long])) {
        throw "Build provenance pointer_size must be an integer"
    }
    if ($provenance.pointer_size -le 0) {
        throw "Build provenance pointer_size must be positive"
    }
    if ($rawProvenance -notmatch '"configuration_types"\s*:\s*\[') {
        throw "Build provenance configuration_types must be an array"
    }
    $configurationTypes = @($provenance.configuration_types)
    $seenConfigurationTypes = @{}
    foreach ($configurationType in $configurationTypes) {
        if ($configurationType -isnot [string] -or
            [string]::IsNullOrWhiteSpace($configurationType)) {
            throw "Build provenance configuration_types contains an invalid value"
        }
        if ($seenConfigurationTypes.ContainsKey($configurationType)) {
            throw "Build provenance configuration_types contains a duplicate value"
        }
        $seenConfigurationTypes[$configurationType] = $true
    }
    if ($configurationTypes.Count -gt 0) {
        if (-not [string]::IsNullOrEmpty($provenance.build_type)) {
            throw "Build provenance mixes single-config and multi-config metadata"
        }
        if (-not [string]::IsNullOrWhiteSpace($Configuration) -and
            $Configuration -cnotin $configurationTypes) {
            throw "Requested configuration '$Configuration' is not present in build provenance configuration_types"
        }
    } else {
        if ([string]::IsNullOrWhiteSpace($provenance.build_type)) {
            throw "Build provenance single-config build_type is missing"
        }
        if (-not [string]::IsNullOrWhiteSpace($Configuration) -and
            $provenance.build_type -cne $Configuration) {
            throw "Build provenance build_type '$($provenance.build_type)' does not match requested configuration '$Configuration'"
        }
    }

    $normalizedProvenanceSource = Resolve-LuaCppNormalizedDirectory `
        -Path $provenance.source_directory `
        -Label "Build provenance source directory"
    $normalizedProvenanceBuild = Resolve-LuaCppNormalizedDirectory `
        -Path $provenance.binary_directory `
        -Label "Build provenance binary directory"
    $comparison = if ($env:OS -eq "Windows_NT") {
        [StringComparison]::OrdinalIgnoreCase
    } else {
        [StringComparison]::Ordinal
    }
    if (-not [string]::Equals(
            $normalizedProvenanceSource,
            $normalizedRepository,
            $comparison)) {
        throw "Build provenance source directory does not match the package repository"
    }
    if (-not [string]::Equals(
            $normalizedProvenanceBuild,
            $normalizedBuild,
            $comparison)) {
        throw "Build provenance binary directory does not match the selected build directory"
    }
    if ($provenance.cmake_version -notmatch '^[0-9]+\.[0-9]+(?:\.[0-9]+)?') {
        throw "Build provenance CMake version is invalid"
    }
    if ($rawProvenance -notmatch
        '"configured_at"\s*:\s*"(?<configured_at>[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z)"') {
        throw "Build provenance configure timestamp is missing or not canonical UTC"
    }
    $configuredAtText = $Matches["configured_at"]
    $configuredAt = [DateTimeOffset]::MinValue
    if (-not [DateTimeOffset]::TryParse(
            $configuredAtText,
            [Globalization.CultureInfo]::InvariantCulture,
            [Globalization.DateTimeStyles]::AssumeUniversal,
            [ref]$configuredAt) -or
        $configuredAt.Offset -ne [TimeSpan]::Zero) {
        throw "Build provenance configure timestamp is invalid"
    }

    if (-not [string]::IsNullOrEmpty($RuntimeIdentifier)) {
        Assert-LuaCppRuntimeIdentifierTargetIdentity `
            -Provenance $provenance `
            -RuntimeIdentifier $RuntimeIdentifier
    }

    return $provenance
}

function Invoke-LuaCppReleaseBuildRefresh {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$BuildDirectory,

        [Parameter(Mandatory = $true)]
        [string]$Repository,

        [Parameter(Mandatory = $true)]
        [string]$Commit,

        [Parameter(Mandatory = $true)]
        [string]$Configuration,

        [string]$RuntimeIdentifier = ""
    )

    $normalizedBuild = Resolve-LuaCppNormalizedDirectory `
        -Path $BuildDirectory `
        -Label "Build directory"
    $normalizedRepository = Resolve-LuaCppNormalizedDirectory `
        -Path $Repository `
        -Label "Repository"

    & cmake -S $normalizedRepository -B $normalizedBuild
    if ($LASTEXITCODE -ne 0) {
        throw "Release build reconfigure failed with exit code $LASTEXITCODE"
    }
    & cmake --build $normalizedBuild --config $Configuration --clean-first
    if ($LASTEXITCODE -ne 0) {
        throw "Release clean rebuild failed with exit code $LASTEXITCODE"
    }

    $refreshedCommit = (& git -C $normalizedRepository rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or
        $refreshedCommit -notmatch '^[0-9a-fA-F]{40}$' -or
        $refreshedCommit.ToLowerInvariant() -ne $Commit.ToLowerInvariant()) {
        throw "Repository HEAD changed while refreshing the release build"
    }
    $refreshedStatus = @(& git -C $normalizedRepository status --porcelain)
    if ($LASTEXITCODE -ne 0) {
        throw "Unable to re-check repository worktree after release build refresh"
    }
    if ($refreshedStatus.Count -ne 0) {
        throw "Release build refresh left the repository worktree dirty"
    }

    return Test-LuaCppBuildProvenance `
        -BuildDirectory $normalizedBuild `
        -Repository $normalizedRepository `
        -Commit $Commit `
        -Configuration $Configuration `
        -RuntimeIdentifier $RuntimeIdentifier
}

Export-ModuleMember -Function `
    Test-LuaCppBuildProvenance, `
    Invoke-LuaCppReleaseBuildRefresh
