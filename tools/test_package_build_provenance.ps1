$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repositoryRoot = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
$modulePath = Join-Path $PSScriptRoot "build_provenance.psm1"
$cmakeModulePath = (Join-Path $repositoryRoot "cmake/WriteBuildProvenance.cmake").Replace('\', '/')
Import-Module $modulePath -Force

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Program,

        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [Parameter(Mandatory = $true)]
        [string]$Scenario
    )

    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Scenario failed with exit code $LASTEXITCODE"
    }
}

function Assert-Rejected {
    param(
        [Parameter(Mandatory = $true)]
        [scriptblock]$Action,

        [Parameter(Mandatory = $true)]
        [string]$Pattern,

        [Parameter(Mandatory = $true)]
        [string]$Scenario
    )

    try {
        & $Action
    } catch {
        if ($_.Exception.Message -notmatch $Pattern) {
            throw "$Scenario failed for the wrong reason: $($_.Exception.Message)"
        }
        return
    }
    throw "$Scenario unexpectedly succeeded"
}

function Get-FixtureHead {
    param([Parameter(Mandatory = $true)][string]$Source)

    $head = (& git -C $Source rev-parse --verify HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $head -notmatch '^[0-9a-fA-F]{40}$') {
        throw "Unable to resolve fixture HEAD"
    }
    return $head.ToLowerInvariant()
}

function Get-SupportedRuntimeIdentifier {
    param(
        [Parameter(Mandatory = $true)]
        [PSCustomObject]$Provenance
    )

    if ($Provenance.pointer_size -ne 8) {
        return $null
    }
    $systemName = $Provenance.system_name.ToLowerInvariant()
    $processor = $Provenance.system_processor.ToLowerInvariant()
    $x64Processors = @("amd64", "x86_64", "x86-64", "x64")
    if ($systemName -eq "windows" -and $processor -cin $x64Processors) {
        return "windows-x64"
    }
    if ($systemName -eq "linux" -and $processor -cin $x64Processors) {
        return "linux-x64"
    }
    if ($systemName -eq "darwin" -and
        $processor -cin @("aarch64", "arm64")) {
        return "macos-arm64"
    }
    return $null
}

function Import-VisualStudioDeveloperEnvironment {
    param([Parameter(Mandatory = $true)][string]$CMakeExecutable)

    if ($env:OS -ne "Windows_NT" -or $null -ne (Get-Command cl -ErrorAction SilentlyContinue)) {
        return
    }

    $normalizedCMake = [IO.Path]::GetFullPath($CMakeExecutable)
    $common7Marker = [IO.Path]::DirectorySeparatorChar + "Common7" + [IO.Path]::DirectorySeparatorChar
    $markerIndex = $normalizedCMake.IndexOf($common7Marker, [StringComparison]::OrdinalIgnoreCase)
    if ($markerIndex -lt 0) {
        throw "Cannot locate the Visual Studio root from CMake: $normalizedCMake"
    }
    $visualStudio = $normalizedCMake.Substring(0, $markerIndex)
    $developerCommand = Join-Path $visualStudio "Common7/Tools/VsDevCmd.bat"
    if (-not (Test-Path -LiteralPath $developerCommand -PathType Leaf)) {
        throw "Visual Studio developer environment script is missing: $developerCommand"
    }

    $command = "call `"$developerCommand`" -no_logo -arch=x64 -host_arch=x64 >nul && set"
    $environmentLines = @(& $env:ComSpec /d /s /c $command)
    if ($LASTEXITCODE -ne 0) {
        throw "Visual Studio developer environment setup failed with exit code $LASTEXITCODE"
    }
    foreach ($line in $environmentLines) {
        $separator = $line.IndexOf('=')
        if ($separator -le 0) {
            continue
        }
        $name = $line.Substring(0, $separator)
        $value = $line.Substring($separator + 1)
        [Environment]::SetEnvironmentVariable($name, $value, "Process")
    }
    if ($null -eq (Get-Command cl -ErrorAction SilentlyContinue)) {
        throw "Visual Studio developer environment did not expose cl.exe"
    }
}

$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) `
    ("lua_cpp_build_provenance_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    Assert-Rejected {
        & (Join-Path $PSScriptRoot "package_release.ps1") `
            -BuildDirectory (Join-Path $temporaryRoot "missing-build") `
            -OutputDirectory (Join-Path $temporaryRoot "unused-output") `
            -Version "0.1.0-rc.1" `
            -RuntimeIdentifier "windows-x64-forged"
    } "Unsupported RuntimeIdentifier" "package script arbitrary RID rejection"

    $source = Join-Path $temporaryRoot "source"
    $build = Join-Path $temporaryRoot "build"
    New-Item -ItemType Directory -Path $source | Out-Null
    $fixtureCMake = @"
cmake_minimum_required(VERSION 3.20)
project(lua_cpp_provenance_fixture LANGUAGES C)
find_package(Git QUIET)
include("$cmakeModulePath")
lua_cpp_write_build_provenance(
    OUTPUT "`${CMAKE_CURRENT_BINARY_DIR}/lua-cpp-build-provenance.json"
    SOURCE_DIR "`${CMAKE_CURRENT_SOURCE_DIR}"
    BINARY_DIR "`${CMAKE_CURRENT_BINARY_DIR}"
    GIT_EXECUTABLE "`${GIT_EXECUTABLE}"
)
add_custom_command(
    OUTPUT "`${CMAKE_CURRENT_BINARY_DIR}/built-revision.txt"
    COMMAND "`${CMAKE_COMMAND}" -E copy
        "`${CMAKE_CURRENT_SOURCE_DIR}/revision.txt"
        "`${CMAKE_CURRENT_BINARY_DIR}/built-revision.txt"
    DEPENDS "`${CMAKE_CURRENT_SOURCE_DIR}/revision.txt"
    VERBATIM
)
add_custom_target(fixture_runtime ALL
    DEPENDS "`${CMAKE_CURRENT_BINARY_DIR}/built-revision.txt"
)
install(FILES "`${CMAKE_CURRENT_BINARY_DIR}/built-revision.txt" DESTINATION share)
"@
    Set-Content -LiteralPath (Join-Path $source "CMakeLists.txt") `
        -Value $fixtureCMake `
        -Encoding utf8
    Set-Content -LiteralPath (Join-Path $source "revision.txt") -Value "A" -Encoding ascii

    Invoke-Checked git @("init", $source) "fixture git init"
    Invoke-Checked git @("-C", $source, "add", ".") "fixture git add A"
    Invoke-Checked git @(
        "-C", $source,
        "-c", "user.name=LuaCpp Contract",
        "-c", "user.email=contract@example.invalid",
        "commit", "-m", "fixture A"
    ) "fixture commit A"
    $commitA = Get-FixtureHead $source

    $fixtureConfigureArguments = @(
        "-S", $source,
        "-B", $build,
        "-DBUILD_TESTING=OFF"
    )
    if ($env:OS -ne "Windows_NT") {
        $fixtureConfigureArguments += "-DCMAKE_BUILD_TYPE=Release"
    }
    Invoke-Checked cmake $fixtureConfigureArguments "fixture configure A"
    $positive = Test-LuaCppBuildProvenance `
        -BuildDirectory $build `
        -Repository $source `
        -Commit $commitA `
        -Configuration "Release"
    if ($positive.source_git_sha -ne $commitA) {
        throw "Positive provenance contract returned the wrong SHA"
    }
    $currentRuntimeIdentifier = Get-SupportedRuntimeIdentifier $positive
    if ($env:OS -eq "Windows_NT" -and
        $currentRuntimeIdentifier -ne "windows-x64") {
        throw "Current Windows contract target is not a supported x64 identity"
    }
    if (-not [string]::IsNullOrEmpty($currentRuntimeIdentifier)) {
        $positive = Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitA `
            -Configuration "Release" `
            -RuntimeIdentifier $currentRuntimeIdentifier
    }
    if ($env:OS -eq "Windows_NT" -and
        $positive.generator -match '^Visual Studio ') {
        if (@($positive.configuration_types).Count -eq 0) {
            throw "Visual Studio provenance did not identify a multi-config build"
        }
        $visualStudioX64Build = Join-Path $temporaryRoot "visual-studio-x64-build"
        Invoke-Checked cmake @(
            "-S", $source,
            "-B", $visualStudioX64Build,
            "-G", $positive.generator,
            "-A", "x64",
            "-DBUILD_TESTING=OFF"
        ) "Visual Studio explicit x64 configure"
        $visualStudioX64 = Test-LuaCppBuildProvenance `
            -BuildDirectory $visualStudioX64Build `
            -Repository $source `
            -Commit $commitA `
            -Configuration "Release" `
            -RuntimeIdentifier "windows-x64"
        if ($visualStudioX64.generator_platform -cne "x64") {
            throw "Visual Studio explicit x64 provenance lost its generator platform"
        }
    }

    Set-Content -LiteralPath (Join-Path $source "revision.txt") -Value "B" -Encoding ascii
    Invoke-Checked git @("-C", $source, "add", "revision.txt") "fixture git add B"
    Invoke-Checked git @(
        "-C", $source,
        "-c", "user.name=LuaCpp Contract",
        "-c", "user.email=contract@example.invalid",
        "commit", "-m", "fixture B"
    ) "fixture commit B"
    $commitB = Get-FixtureHead $source
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB
    } "does not match package commit" "stale A build at clean B source"

    Invoke-Checked cmake @("-S", $source, "-B", $build) "fixture reconfigure B"
    $null = Test-LuaCppBuildProvenance `
        -BuildDirectory $build `
        -Repository $source `
        -Commit $commitB `
        -Configuration "Release"

    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -Configuration "Profile"
    } "not present in build provenance configuration_types" "unknown multi-config configuration"

    Set-Content -LiteralPath (Join-Path $source "revision.txt") `
        -Value "DIRTY-BUILD" `
        -Encoding ascii
    Invoke-Checked cmake @("-S", $source, "-B", $build) "dirty fixture reconfigure"
    Invoke-Checked cmake @(
        "--build", $build,
        "--config", "Release"
    ) "dirty fixture build"
    Invoke-Checked git @(
        "-C", $source,
        "restore", "--source=HEAD", "--", "revision.txt"
    ) "restore fixture source to clean HEAD"

    $null = Test-LuaCppBuildProvenance `
        -BuildDirectory $build `
        -Repository $source `
        -Commit $commitB `
        -Configuration "Release"
    $dirtyInstall = Join-Path $temporaryRoot "dirty-install"
    Invoke-Checked cmake @(
        "--install", $build,
        "--config", "Release",
        "--prefix", $dirtyInstall
    ) "install stale dirty build"
    if ((Get-Content -Raw -LiteralPath (
            Join-Path $dirtyInstall "share/built-revision.txt")).Trim() -ne "DIRTY-BUILD") {
        throw "Dirty-build fixture did not reproduce stale install contamination"
    }

    $refreshArguments = @{
        BuildDirectory = $build
        Repository = $source
        Commit = $commitB
        Configuration = "Release"
    }
    if (-not [string]::IsNullOrEmpty($currentRuntimeIdentifier)) {
        $refreshArguments.RuntimeIdentifier = $currentRuntimeIdentifier
    }
    $null = Invoke-LuaCppReleaseBuildRefresh @refreshArguments
    $cleanInstall = Join-Path $temporaryRoot "clean-install"
    Invoke-Checked cmake @(
        "--install", $build,
        "--config", "Release",
        "--prefix", $cleanInstall
    ) "install refreshed clean build"
    if ((Get-Content -Raw -LiteralPath (
            Join-Path $cleanInstall "share/built-revision.txt")).Trim() -ne "B") {
        throw "Release build refresh did not replace stale dirty-build output"
    }

    $cmakeExecutable = (Get-Command cmake -ErrorAction Stop).Source
    $ninja = Get-Command ninja -ErrorAction SilentlyContinue
    if ($null -eq $ninja) {
        $cmakeBundle = Split-Path -Parent (
            Split-Path -Parent (
                Split-Path -Parent $cmakeExecutable
            )
        )
        $bundledNinja = Join-Path $cmakeBundle "Ninja/ninja.exe"
        if (Test-Path -LiteralPath $bundledNinja -PathType Leaf) {
            $ninja = Get-Item -LiteralPath $bundledNinja
        }
    }
    if ($null -eq $ninja -and $env:OS -eq "Windows_NT") {
        $vswhere = Join-Path ${env:ProgramFiles(x86)} `
            "Microsoft Visual Studio/Installer/vswhere.exe"
        if (Test-Path -LiteralPath $vswhere -PathType Leaf) {
            $visualStudio = (& $vswhere -latest -property installationPath).Trim()
            if (-not [string]::IsNullOrWhiteSpace($visualStudio)) {
                $bundledNinja = Join-Path $visualStudio `
                    "Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
                if (Test-Path -LiteralPath $bundledNinja -PathType Leaf) {
                    $ninja = Get-Item -LiteralPath $bundledNinja
                }
            }
        }
    }
    if ($null -eq $ninja) {
        throw "Ninja is required for the single-config provenance contract"
    }
    $ninjaPath = if ($ninja -is [Management.Automation.CommandInfo]) {
        $ninja.Source
    } else {
        $ninja.FullName
    }
    Import-VisualStudioDeveloperEnvironment -CMakeExecutable $cmakeExecutable
    $singleBuild = Join-Path $temporaryRoot "single-config-build"
    Invoke-Checked $cmakeExecutable @(
        "-S", $source,
        "-B", $singleBuild,
        "-G", "Ninja",
        "-DCMAKE_MAKE_PROGRAM=$ninjaPath",
        "-DCMAKE_BUILD_TYPE=Debug"
    ) "single-config Debug configure"
    $null = Test-LuaCppBuildProvenance `
        -BuildDirectory $singleBuild `
        -Repository $source `
        -Commit $commitB `
        -Configuration "Debug"
    if (-not [string]::IsNullOrEmpty($currentRuntimeIdentifier)) {
        $null = Test-LuaCppBuildProvenance `
            -BuildDirectory $singleBuild `
            -Repository $source `
            -Commit $commitB `
            -Configuration "Debug" `
            -RuntimeIdentifier $currentRuntimeIdentifier
    }
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $singleBuild `
            -Repository $source `
            -Commit $commitB `
            -Configuration "Release"
    } "does not match requested configuration" "single-config Debug as Release"

    $provenancePath = Join-Path $build "lua-cpp-build-provenance.json"
    $validJson = Get-Content -Raw -LiteralPath $provenancePath

    $payload = $validJson | ConvertFrom-Json
    $payload.system_name = "wInDoWs"
    $payload.system_processor = "X86_64"
    $payload.pointer_size = 8
    $payload.generator_platform = "AmD64"
    $windowsIdentityJson = $payload | ConvertTo-Json
    Set-Content -LiteralPath $provenancePath `
        -Value $windowsIdentityJson `
        -Encoding utf8
    $null = Test-LuaCppBuildProvenance `
        -BuildDirectory $build `
        -Repository $source `
        -Commit $commitB `
        -Configuration "Release" `
        -RuntimeIdentifier "windows-x64"

    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "linux-x64"
    } "target system.*does not match RuntimeIdentifier" "Linux RID impersonating a Windows target"

    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "macos-arm64"
    } "target system.*does not match RuntimeIdentifier" "macOS RID impersonating a Windows target"

    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-arm64"
    } "Unsupported RuntimeIdentifier" "unknown RID"

    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "Windows-X64"
    } "Unsupported RuntimeIdentifier" "non-canonical RID casing"

    $payload = $windowsIdentityJson | ConvertFrom-Json
    $payload.system_name = "Linux"
    $payload | ConvertTo-Json | Set-Content `
        -LiteralPath $provenancePath `
        -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-x64"
    } "target system.*does not match RuntimeIdentifier" "wrong target operating system"

    $payload = $windowsIdentityJson | ConvertFrom-Json
    $payload.system_processor = "ARM64"
    $payload | ConvertTo-Json | Set-Content `
        -LiteralPath $provenancePath `
        -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-x64"
    } "target processor.*does not match RuntimeIdentifier" "wrong target architecture"

    $payload = $windowsIdentityJson | ConvertFrom-Json
    $payload.system_processor = "amd64-forged"
    $payload | ConvertTo-Json | Set-Content `
        -LiteralPath $provenancePath `
        -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-x64"
    } "target processor.*does not match RuntimeIdentifier" "processor alias suffix impersonation"

    $payload = $windowsIdentityJson | ConvertFrom-Json
    $payload.pointer_size = 4
    $payload | ConvertTo-Json | Set-Content `
        -LiteralPath $provenancePath `
        -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-x64"
    } "pointer size.*does not match 64-bit RuntimeIdentifier" "32-bit build as x64"

    $payload = $windowsIdentityJson | ConvertFrom-Json
    $payload.generator_platform = "Win32"
    $payload | ConvertTo-Json | Set-Content `
        -LiteralPath $provenancePath `
        -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-x64"
    } "generator platform.*does not match RuntimeIdentifier" "contradictory generator platform"

    $payload = $windowsIdentityJson | ConvertFrom-Json
    $payload.PSObject.Properties.Remove("system_processor")
    $payload | ConvertTo-Json | Set-Content `
        -LiteralPath $provenancePath `
        -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-x64"
    } "field set does not match" "missing target identity field"

    $payload = $windowsIdentityJson | ConvertFrom-Json
    $payload.pointer_size = "8"
    $payload | ConvertTo-Json | Set-Content `
        -LiteralPath $provenancePath `
        -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-x64"
    } "pointer_size must be an integer" "forged pointer-size type"

    $payload = $windowsIdentityJson | ConvertFrom-Json
    $payload.system_name = 7
    $payload | ConvertTo-Json | Set-Content `
        -LiteralPath $provenancePath `
        -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-x64"
    } "field 'system_name' is missing" "forged system-name type"

    $payload = $windowsIdentityJson | ConvertFrom-Json
    $payload.schema = "lua-cpp.build-provenance/v1"
    $payload | ConvertTo-Json | Set-Content `
        -LiteralPath $provenancePath `
        -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-x64"
    } "schema mismatch" "legacy v1 provenance shape"

    $duplicateIdentityJson = $windowsIdentityJson -replace `
        '"system_name"\s*:\s*"[^"]+"', `
        '"system_name": "Windows", "system_name": "Windows"'
    Set-Content -LiteralPath $provenancePath `
        -Value $duplicateIdentityJson `
        -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB `
            -RuntimeIdentifier "windows-x64"
    } "must occur exactly once" "duplicate canonical target identity field"

    Set-Content -LiteralPath $provenancePath -Value $validJson -Encoding utf8
    $payload = $validJson | ConvertFrom-Json
    $payload.source_directory = (Join-Path $temporaryRoot "wrong-source")
    New-Item -ItemType Directory -Path $payload.source_directory | Out-Null
    $payload | ConvertTo-Json | Set-Content -LiteralPath $provenancePath -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB
    } "source directory does not match" "wrong source directory"

    Set-Content -LiteralPath $provenancePath -Value $validJson -Encoding utf8
    $payload = $validJson | ConvertFrom-Json
    $payload.configured_at = "2026-07-26T08:42:23+00:00"
    $payload | ConvertTo-Json | Set-Content -LiteralPath $provenancePath -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB
    } "timestamp is missing or not canonical UTC" "non-canonical configure timestamp"

    Set-Content -LiteralPath $provenancePath -Value $validJson -Encoding utf8
    $payload = $validJson | ConvertFrom-Json
    $payload.source_git_status = "unavailable"
    $payload.source_git_sha = "unavailable"
    $payload | ConvertTo-Json | Set-Content -LiteralPath $provenancePath -Encoding utf8
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB
    } "no exact Git source identity" "unavailable Git identity"

    Remove-Item -LiteralPath $provenancePath -Force
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $build `
            -Repository $source `
            -Commit $commitB
    } "Build provenance is missing" "missing provenance"

    $archiveSource = Join-Path $temporaryRoot "source-archive"
    $archiveBuild = Join-Path $temporaryRoot "archive-build"
    New-Item -ItemType Directory -Path $archiveSource | Out-Null
    Set-Content -LiteralPath (Join-Path $archiveSource "CMakeLists.txt") `
        -Value $fixtureCMake `
        -Encoding utf8
    Invoke-Checked cmake @(
        "-S", $archiveSource,
        "-B", $archiveBuild,
        "-DBUILD_TESTING=OFF",
        "-DCMAKE_DISABLE_FIND_PACKAGE_Git=TRUE"
    ) "Git-less source archive configure"
    $archiveProvenance = Get-Content -Raw -LiteralPath `
        (Join-Path $archiveBuild "lua-cpp-build-provenance.json") |
        ConvertFrom-Json
    if ($archiveProvenance.source_git_status -ne "unavailable" -or
        $archiveProvenance.source_git_sha -ne "unavailable") {
        throw "Git-less configure did not emit explicit unavailable provenance"
    }
    Assert-Rejected {
        Test-LuaCppBuildProvenance `
            -BuildDirectory $archiveBuild `
            -Repository $archiveSource `
            -Commit $commitB
    } "no exact Git source identity" "Git-less package rejection"
} finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

Write-Host "Package build provenance contracts passed"
