$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repository = (Resolve-Path -LiteralPath (Split-Path -Parent $PSScriptRoot)).Path
Import-Module (Join-Path $PSScriptRoot "release_identity.psm1") -Force

$cmake = Get-Content -Raw -LiteralPath (Join-Path $repository "CMakeLists.txt")
$header = Get-Content -Raw -LiteralPath (Join-Path $repository "src/lua_cpp_version.h")
$identity = Get-LuaCppReleaseIdentity -CMakeText $cmake -VersionHeaderText $header
if ($identity.ProjectVersion -cne "0.1.0" -or $identity.AbiVersion -ne 0) {
    throw "Tracked release identity is not the expected version 0.1.0 / ABI 0"
}

$negativeCases = @(
    @{
        Name = "component mismatch"
        CMake = $cmake
        Header = $header.Replace(
            "#define LUA_CPP_VERSION_PATCH 0",
            "#define LUA_CPP_VERSION_PATCH 9"
        )
    },
    @{
        Name = "header string mismatch"
        CMake = $cmake
        Header = $header.Replace(
            '#define LUA_CPP_VERSION "0.1.0"',
            '#define LUA_CPP_VERSION "9.9.9"'
        )
    },
    @{
        Name = "header ABI mismatch"
        CMake = $cmake
        Header = $header.Replace(
            "#define LUA_CPP_ABI_VERSION 0",
            "#define LUA_CPP_ABI_VERSION 1"
        )
    },
    @{
        Name = "CMake ABI mismatch"
        CMake = $cmake.Replace(
            "set(LUA_CPP_ABI_VERSION 0)",
            "set(LUA_CPP_ABI_VERSION 2)"
        )
        Header = $header
    },
    @{
        Name = "shared SOVERSION bypass"
        CMake = $cmake.Replace(
            'SOVERSION ${LUA_CPP_ABI_VERSION}',
            'SOVERSION ${PROJECT_VERSION_MAJOR}'
        )
        Header = $header
    }
)

foreach ($case in $negativeCases) {
    $failedClosed = $false
    try {
        Get-LuaCppReleaseIdentity `
            -CMakeText $case.CMake `
            -VersionHeaderText $case.Header | Out-Null
    }
    catch {
        $failedClosed = $true
    }
    if (-not $failedClosed) {
        throw "Release identity negative case did not fail closed: $($case.Name)"
    }
}

Write-Host "Release identity contract tests passed ($($negativeCases.Count) negative cases)."
