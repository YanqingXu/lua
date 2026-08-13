$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

Import-Module (Join-Path $PSScriptRoot "visual_studio_environment.psm1") -Force

function New-LeafFile {
    param([Parameter(Mandatory = $true)][string]$Path)

    $parent = Split-Path -Parent $Path
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
    New-Item -ItemType File -Path $Path -Force | Out-Null
}

function New-FakeVsWhere {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$InstallationPath
    )

    $escapedInstallation = $InstallationPath.Replace("'", "''")
    Set-Content -LiteralPath $Path `
        -Value "Write-Output '$escapedInstallation'" `
        -Encoding utf8
}

function Assert-EqualPath {
    param(
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$Actual,
        [Parameter(Mandatory = $true)][string]$Scenario
    )

    if ([IO.Path]::GetFullPath($Expected) -cne [IO.Path]::GetFullPath($Actual)) {
        throw "$Scenario returned '$Actual' instead of '$Expected'"
    }
}

function Assert-Rejected {
    param(
        [Parameter(Mandatory = $true)][scriptblock]$Action,
        [Parameter(Mandatory = $true)][string]$Pattern,
        [Parameter(Mandatory = $true)][string]$Scenario
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

$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) `
    ("lua_cpp_visual_studio_environment_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
try {
    $standaloneCMake = Join-Path $temporaryRoot "standalone-cmake/bin/cmake.exe"
    New-LeafFile -Path $standaloneCMake

    $vswhereVisualStudio = Join-Path $temporaryRoot "vswhere-visual-studio"
    New-LeafFile -Path (Join-Path $vswhereVisualStudio "Common7/Tools/VsDevCmd.bat")
    $vswhere = Join-Path $temporaryRoot "vswhere.ps1"
    New-FakeVsWhere -Path $vswhere -InstallationPath $vswhereVisualStudio

    $actual = Find-LuaCppVisualStudioInstallation `
        -CMakeExecutable $standaloneCMake `
        -VsWhereExecutable $vswhere
    Assert-EqualPath $vswhereVisualStudio $actual `
        "standalone CMake with a vswhere-discovered Visual Studio"

    $bundledVisualStudio = Join-Path $temporaryRoot "bundled-visual-studio"
    $bundledCMake = Join-Path $bundledVisualStudio `
        "Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
    New-LeafFile -Path $bundledCMake
    New-LeafFile -Path (Join-Path $bundledVisualStudio "Common7/Tools/VsDevCmd.bat")

    $actual = Find-LuaCppVisualStudioInstallation `
        -CMakeExecutable $bundledCMake `
        -VsWhereExecutable $vswhere
    Assert-EqualPath $vswhereVisualStudio $actual "vswhere priority over bundled CMake inference"

    $missingVsWhere = Join-Path $temporaryRoot "missing-vswhere.exe"
    $actual = Find-LuaCppVisualStudioInstallation `
        -CMakeExecutable $bundledCMake `
        -VsWhereExecutable $missingVsWhere
    Assert-EqualPath $bundledVisualStudio $actual "bundled CMake fallback"

    Assert-Rejected {
        Find-LuaCppVisualStudioInstallation `
            -CMakeExecutable $standaloneCMake `
            -VsWhereExecutable $missingVsWhere
    } "Unable to locate.*vswhere is unavailable.*CMake is not bundled" `
        "standalone CMake without vswhere or a Visual Studio developer environment"
} finally {
    if (Test-Path -LiteralPath $temporaryRoot -PathType Container) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

Write-Host "Visual Studio environment discovery contract passed"
