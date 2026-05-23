param(
    [switch]$FormatFix,
    [ValidateSet("Changed", "All", "Off")]
    [string]$FormatScope = "Changed",
    [switch]$SkipClangTidy,
    [switch]$SkipBuild,
    [string]$Configuration = "Debug",
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Invoke-Step {
    param(
        [string]$Name,
        [scriptblock]$Body
    )

    Write-Host ""
    Write-Host "==> $Name"
    & $Body
}

function Get-CommandOrNull {
    param([string]$Name)
    return Get-Command $Name -ErrorAction SilentlyContinue
}

function Get-SourceFiles {
    $roots = @("src", "tests")
    $extensions = @("*.cpp", "*.hpp", "*.h")

    foreach ($dir in $roots) {
        $path = Join-Path $root $dir
        if (-not (Test-Path -LiteralPath $path)) {
            continue
        }

        foreach ($ext in $extensions) {
            Get-ChildItem -LiteralPath $path -Recurse -File -Filter $ext
        }
    }
}

function Get-ChangedSourceFiles {
    $git = Get-CommandOrNull "git"
    if (-not $git) {
        return @()
    }

    $patterns = @("src/", "tests/")
    $names = @()

    if ($env:GITHUB_BASE_REF) {
        $baseRef = "origin/$($env:GITHUB_BASE_REF)"
        & $git.Source fetch origin $env:GITHUB_BASE_REF --depth=1 | Out-Null
        $names = & $git.Source diff --name-only --diff-filter=ACMRT "$baseRef...HEAD" -- src tests
    } else {
        $names = & $git.Source diff --name-only --diff-filter=ACMRT HEAD -- src tests
        $staged = & $git.Source diff --cached --name-only --diff-filter=ACMRT -- src tests
        $names = @($names) + @($staged)
    }

    $names |
        Where-Object {
            $name = $_
            ($patterns | Where-Object { $name.StartsWith($_) }).Count -gt 0 -and
                ($name.EndsWith(".cpp") -or $name.EndsWith(".hpp") -or $name.EndsWith(".h"))
        } |
        Sort-Object -Unique |
        ForEach-Object {
            $path = Join-Path $root $_
            if (Test-Path -LiteralPath $path) {
                Get-Item -LiteralPath $path
            }
        }
}

function Get-ClangTidyFiles {
    $preferred = @(
        "src/compiler/codegen_types.hpp",
        "src/compiler/register_allocator.hpp",
        "src/compiler/codegen_context.hpp",
        "src/io/input_stream.cpp",
        "src/debug/value_serializer.cpp",
        "tests/unit/framework/test_runner.cpp"
    )

    foreach ($relative in $preferred) {
        $path = Join-Path $root $relative
        if (Test-Path -LiteralPath $path) {
            Get-Item -LiteralPath $path
        }
    }
}

function Find-MSBuild {
    $cmd = Get-CommandOrNull "MSBuild.exe"
    if ($cmd) {
        return $cmd.Source
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        if ($installPath) {
            $candidate = Join-Path $installPath "MSBuild\Current\Bin\MSBuild.exe"
            if (Test-Path -LiteralPath $candidate) {
                return $candidate
            }
        }
    }

    return $null
}

Push-Location $root
try {
    Invoke-Step "clang-format" {
        $clangFormat = Get-CommandOrNull "clang-format"
        if (-not $clangFormat) {
            Write-Host "[SKIP] clang-format was not found on PATH"
            return
        }

        if ($FormatScope -eq "Off") {
            Write-Host "[SKIP] clang-format skipped by -FormatScope Off"
            return
        }

        $files = if ($FormatScope -eq "All") {
            @(Get-SourceFiles)
        } else {
            @(Get-ChangedSourceFiles)
        }

        if ($files.Count -eq 0) {
            Write-Host "[SKIP] no C++ source files found for clang-format scope: $FormatScope"
            return
        }

        if ($FormatFix) {
            & $clangFormat.Source -i --style=file @($files.FullName)
        } else {
            & $clangFormat.Source --dry-run --Werror --style=file @($files.FullName)
        }
    }

    Invoke-Step "clang-tidy smoke" {
        if ($SkipClangTidy) {
            Write-Host "[SKIP] clang-tidy skipped by flag"
            return
        }

        $clangTidy = Get-CommandOrNull "clang-tidy"
        if (-not $clangTidy) {
            Write-Host "[SKIP] clang-tidy was not found on PATH"
            return
        }

        $files = @(Get-ClangTidyFiles)
        if ($files.Count -eq 0) {
            Write-Host "[SKIP] no clang-tidy smoke files found"
            return
        }

        foreach ($file in $files) {
            & $clangTidy.Source $file.FullName -- -std=c++20 "-I$root\src" "-I$root\tests\unit\framework" "-I$root\lua_test\include"
        }
    }

    Invoke-Step "opcode coverage matrix" {
        & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root "tools\check_opcode_coverage_matrix.ps1")
    }

    Invoke-Step "MSBuild lua_test" {
        if ($SkipBuild) {
            Write-Host "[SKIP] MSBuild skipped by flag"
            return
        }

        $msbuild = Find-MSBuild
        if (-not $msbuild) {
            Write-Host "[SKIP] MSBuild.exe was not found"
            return
        }

        & $msbuild (Join-Path $root "lua_test.vcxproj") /m "/p:Configuration=$Configuration" "/p:Platform=$Platform"
    }

    Invoke-Step "documentation drift" {
        & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root "tools\check_doc_drift.ps1")
    }

    Invoke-Step "unit tests" {
        if ($SkipBuild) {
            Write-Host "[SKIP] unit tests skipped with -SkipBuild"
            return
        }

        $testExe = Join-Path $root "bin\lua_test.exe"
        if (-not (Test-Path -LiteralPath $testExe)) {
            Write-Host "[SKIP] bin\lua_test.exe was not found"
            return
        }

        & $testExe
    }
} finally {
    Pop-Location
}
