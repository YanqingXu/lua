param(
    [switch]$FormatFix,
    [ValidateSet("Changed", "All", "Off")]
    [string]$FormatScope = "Changed",
    [string]$FormatBase = "",
    [switch]$SkipClangTidy,
    [switch]$SkipBuild,
    [switch]$Strict,
    [string]$Configuration = "Debug",
    [string]$Platform = "x64",
    [string]$TestExecutable = "",
    [string]$PowerShellExecutable = ""
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
    $global:LASTEXITCODE = 0
    & $Body
    $stepExitCode = $global:LASTEXITCODE
    if ($stepExitCode -ne 0) {
        throw "$Name failed with exit code $stepExitCode"
    }
}

function Get-CommandOrNull {
    param([string]$Name)
    return Get-Command $Name -ErrorAction SilentlyContinue
}

function Resolve-PowerShellExecutable {
    param([string]$RequestedExecutable)

    if (-not [string]::IsNullOrWhiteSpace($RequestedExecutable)) {
        $requestedCommand = Get-Command $RequestedExecutable -ErrorAction SilentlyContinue
        if ($requestedCommand) {
            return $requestedCommand.Source
        }

        if (Test-Path -LiteralPath $RequestedExecutable -PathType Leaf) {
            return (Resolve-Path -LiteralPath $RequestedExecutable).Path
        }

        throw "Requested PowerShell executable does not exist: $RequestedExecutable"
    }

    try {
        $currentProcessPath = (Get-Process -Id $PID -ErrorAction Stop).Path
        $currentProcessName = [System.IO.Path]::GetFileNameWithoutExtension($currentProcessPath)
        if ($currentProcessName -in @("pwsh", "powershell") -and
            (Test-Path -LiteralPath $currentProcessPath -PathType Leaf)) {
            return $currentProcessPath
        }
    } catch {
        # Fall through to command discovery for restricted hosts.
    }

    $powerShellCommand = Get-Command pwsh, powershell -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if (-not $powerShellCommand) {
        throw "Unable to resolve a PowerShell executable (pwsh or powershell)"
    }
    return $powerShellCommand.Source
}

function Invoke-PowerShellFile {
    param(
        [string]$ScriptPath,
        [string[]]$Arguments = @()
    )

    $invokeArguments = @("-NoProfile", "-NonInteractive")
    if ($env:OS -eq "Windows_NT") {
        $invokeArguments += @("-ExecutionPolicy", "Bypass")
    }
    $invokeArguments += @("-File", $ScriptPath)
    $invokeArguments += @($Arguments)

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        # Windows PowerShell can surface a native child's stderr as a
        # non-terminating error. Capture the real process code explicitly.
        $ErrorActionPreference = "Continue"
        $global:LASTEXITCODE = -1
        & $script:powerShellExecutablePath @invokeArguments
        $childExitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if ($childExitCode -ne 0) {
        throw "PowerShell script '$ScriptPath' failed with exit code $childExitCode"
    }
}

function Stop-OrSkipMissingRequirement {
    param(
        [string]$SkipMessage,
        [string]$StrictMessage
    )

    if ($Strict) {
        throw $StrictMessage
    }

    Write-Host "[SKIP] $SkipMessage"
}

function Test-OwnedCppSourcePath {
    param([string]$RelativePath)

    $normalizedPath = $RelativePath.Replace("\", "/")
    while ($normalizedPath.StartsWith("./", [System.StringComparison]::Ordinal)) {
        $normalizedPath = $normalizedPath.Substring(2)
    }

    if ($normalizedPath.StartsWith("tests/lua/official/", [System.StringComparison]::OrdinalIgnoreCase)) {
        return $false
    }

    $hasOwnedRoot = @("src/", "tests/", "examples/", "benchmarks/", "lua_test/") |
        Where-Object { $normalizedPath.StartsWith($_, [System.StringComparison]::OrdinalIgnoreCase) } |
        Select-Object -First 1
    if (-not $hasOwnedRoot) {
        return $false
    }

    return [System.IO.Path]::GetExtension($normalizedPath).ToLowerInvariant() -in @(
        ".c",
        ".cc",
        ".cpp",
        ".cxx",
        ".h",
        ".hpp"
    )
}

function Get-SourceFiles {
    $roots = @("src", "tests", "examples", "benchmarks", "lua_test")

    foreach ($dir in $roots) {
        $path = Join-Path $root $dir
        if (-not (Test-Path -LiteralPath $path)) {
            continue
        }

        foreach ($file in Get-ChildItem -LiteralPath $path -Recurse -File) {
            $relativePath = $file.FullName.Substring($root.Length).TrimStart("\", "/")
            if (Test-OwnedCppSourcePath $relativePath) {
                $file
            }
        }
    }
}

function Get-ChangedSourceFiles {
    $git = Get-CommandOrNull "git"
    if (-not $git) {
        Stop-OrSkipMissingRequirement `
            -SkipMessage "git was not found; changed clang-format scope cannot be resolved" `
            -StrictMessage "Strict quality gate requires git to resolve changed source files"
        return @()
    }

    $names = @()
    $baseRef = ""

    if ($env:GITHUB_BASE_REF) {
        $baseRef = "origin/$($env:GITHUB_BASE_REF)"
        & $git.Source fetch origin $env:GITHUB_BASE_REF --depth=1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "git fetch failed with exit code $LASTEXITCODE"
        }
    } elseif (-not [string]::IsNullOrWhiteSpace($FormatBase)) {
        $baseRef = $FormatBase
        & $git.Source rev-parse --verify "$baseRef^{commit}" | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "clang-format base revision does not resolve to a commit: $baseRef"
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($baseRef)) {
        $names = & $git.Source diff --name-only --diff-filter=ACMRT "$baseRef...HEAD" -- `
            src tests examples benchmarks lua_test
        if ($LASTEXITCODE -ne 0) {
            throw "git diff failed with exit code $LASTEXITCODE"
        }
    }

    $working = & $git.Source diff --name-only --diff-filter=ACMRT HEAD -- `
        src tests examples benchmarks lua_test
    if ($LASTEXITCODE -ne 0) {
        throw "git diff failed with exit code $LASTEXITCODE"
    }
    $staged = & $git.Source diff --cached --name-only --diff-filter=ACMRT -- `
        src tests examples benchmarks lua_test
    if ($LASTEXITCODE -ne 0) {
        throw "git diff --cached failed with exit code $LASTEXITCODE"
    }
    $untracked = & $git.Source ls-files --others --exclude-standard -- `
        src tests examples benchmarks lua_test
    if ($LASTEXITCODE -ne 0) {
        throw "git ls-files failed with exit code $LASTEXITCODE"
    }
    $names = @($names) + @($working) + @($staged) + @($untracked)

    if ($Strict -and [string]::IsNullOrWhiteSpace($baseRef) -and $names.Count -eq 0) {
        throw "Strict changed clang-format scope is empty on a clean worktree; pass -FormatBase <revision> or use -FormatScope All"
    }

    $names |
        Where-Object { Test-OwnedCppSourcePath $_ } |
        Sort-Object -Unique |
        ForEach-Object {
            $path = Join-Path $root $_
            if (Test-Path -LiteralPath $path) {
                Get-Item -LiteralPath $path
            }
        }
}

function Get-RepositoryHeadSha {
    $git = Get-CommandOrNull "git"
    if (-not $git) {
        Stop-OrSkipMissingRequirement `
            -SkipMessage "git was not found; test build SHA cannot be resolved" `
            -StrictMessage "Strict quality gate requires git to resolve the test build SHA"
        return $null
    }

    $sha = (& $git.Source -C $root rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $sha -notmatch "^[0-9a-fA-F]{40}$") {
        throw "Unable to resolve the repository HEAD SHA"
    }
    return $sha.ToLowerInvariant()
}

function Resolve-TestExecutablePath {
    if (-not [string]::IsNullOrWhiteSpace($TestExecutable)) {
        if ([System.IO.Path]::IsPathRooted($TestExecutable)) {
            return [System.IO.Path]::GetFullPath($TestExecutable)
        }
        return [System.IO.Path]::GetFullPath((Join-Path $root $TestExecutable))
    }

    $testExecutableName = if ($env:OS -eq "Windows_NT") {
        "lua_test.exe"
    } else {
        "lua_test"
    }
    return Join-Path (Join-Path $root "bin") $testExecutableName
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

    $programFilesX86 = ${env:ProgramFiles(x86)}
    if ([string]::IsNullOrWhiteSpace($programFilesX86)) {
        return $null
    }

    $vswhere = Join-Path $programFilesX86 "Microsoft Visual Studio\Installer\vswhere.exe"
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

$powerShellExecutablePath = Resolve-PowerShellExecutable $PowerShellExecutable
$resolvedTestExecutable = Resolve-TestExecutablePath
$runUnitTests = -not ($SkipBuild -and [string]::IsNullOrWhiteSpace($TestExecutable))

Push-Location $root
try {
    Invoke-Step "clang-format" {
        if ($FormatScope -eq "Off") {
            Write-Host "[SKIP] clang-format skipped by -FormatScope Off"
            return
        }

        $clangFormat = Get-CommandOrNull "clang-format"
        if (-not $clangFormat) {
            Stop-OrSkipMissingRequirement `
                -SkipMessage "clang-format was not found on PATH" `
                -StrictMessage "Strict quality gate requires clang-format on PATH"
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
            $formatArguments = @("-i", "--style=file") + @($files.FullName)
        } else {
            $formatArguments = @("--dry-run", "--Werror", "--style=file") + @($files.FullName)
        }
        & $clangFormat.Source @formatArguments
    }

    Invoke-Step "clang-tidy smoke" {
        if ($SkipClangTidy) {
            Write-Host "[SKIP] clang-tidy skipped by flag"
            return
        }

        $clangTidy = Get-CommandOrNull "clang-tidy"
        if (-not $clangTidy) {
            Stop-OrSkipMissingRequirement `
                -SkipMessage "clang-tidy was not found on PATH" `
                -StrictMessage "Strict quality gate requires clang-tidy on PATH"
            return
        }

        $files = @(Get-ClangTidyFiles)
        if ($files.Count -eq 0) {
            Stop-OrSkipMissingRequirement `
                -SkipMessage "no clang-tidy smoke files found" `
                -StrictMessage "Strict quality gate requires the configured clang-tidy smoke files"
            return
        }

        $sourceInclude = Join-Path $root "src"
        $frameworkInclude = Join-Path $root "tests/unit/framework"
        $testInclude = Join-Path $root "lua_test/include"
        # The standalone smoke does not inherit target compile definitions. A predefined
        # string macro keeps test_runner.cpp parseable without native-shell quote rules.
        $testBuildGitShaDefinition = "-DLUA_TEST_BUILD_GIT_SHA=__FILE__"
        foreach ($file in $files) {
            & $clangTidy.Source $file.FullName -- -std=c++20 $testBuildGitShaDefinition `
                "-I$sourceInclude" "-I$frameworkInclude" "-I$testInclude"
            if ($LASTEXITCODE -ne 0) {
                throw "clang-tidy failed for $($file.FullName) with exit code $LASTEXITCODE"
            }
        }
    }

    Invoke-Step "ValueResult variant-only boundary" {
        Invoke-PowerShellFile -ScriptPath (Join-Path $root "tools/check_value_result_variant_only.ps1")
    }

    Invoke-Step "C-style pattern guard" {
        Invoke-PowerShellFile -ScriptPath (Join-Path $root "tools/check_c_style_patterns.ps1") `
            -Arguments @("-TestScope", "All")
    }

    Invoke-Step "test signal integrity" {
        Invoke-PowerShellFile -ScriptPath (Join-Path $root "tools/check_test_signal_integrity.ps1")
    }

    Invoke-Step "MSBuild lua_test" {
        if ($SkipBuild) {
            Write-Host "[SKIP] MSBuild skipped by flag"
            return
        }

        $msbuild = Find-MSBuild
        if (-not $msbuild) {
            Stop-OrSkipMissingRequirement `
                -SkipMessage "MSBuild.exe was not found" `
                -StrictMessage "Strict quality gate requires MSBuild.exe"
            return
        }

        $headSha = Get-RepositoryHeadSha
        $arguments = @(
            (Join-Path $root "lua_test.vcxproj"),
            "/m",
            "/p:Configuration=$Configuration",
            "/p:Platform=$Platform"
        )
        if (-not [string]::IsNullOrWhiteSpace($headSha)) {
            $arguments += "/p:LuaTestBuildGitSha=$headSha"
        }
        & $msbuild @arguments
    }

    Invoke-Step "opcode coverage matrix and registered test contract" {
        Invoke-PowerShellFile -ScriptPath (Join-Path $root "tools/check_opcode_coverage_matrix.ps1")
    }

    Invoke-Step "Lua 5.1 official source integrity" {
        Invoke-PowerShellFile -ScriptPath (Join-Path $root "tools/check_lua51_official_sources.ps1")
    }

    Invoke-Step "Lua 5.1 public API contract" {
        $python3 = Get-CommandOrNull "python3"
        $py = Get-CommandOrNull "py"
        $python = Get-CommandOrNull "python"
        $checker = Join-Path $root "tools\check_lua51_public_api_contract.py"
        if ($env:OS -eq "Windows_NT" -and $py) {
            & $py.Source -3 $checker
        } elseif ($python3) {
            & $python3.Source $checker
        } elseif ($python) {
            & $python.Source $checker
        } else {
            throw "Python 3 is required for the Lua 5.1 public API contract"
        }
    }

    Invoke-Step "documentation drift" {
        Invoke-PowerShellFile -ScriptPath (Join-Path $root "tools/check_doc_drift.ps1") `
            -Arguments @("-TestExecutable", $resolvedTestExecutable)
    }

    Invoke-Step "test executable build SHA" {
        if (-not $runUnitTests) {
            Write-Host "[SKIP] test executable build SHA skipped with -SkipBuild"
            return
        }

        if (-not (Test-Path -LiteralPath $resolvedTestExecutable -PathType Leaf)) {
            Stop-OrSkipMissingRequirement `
                -SkipMessage "test executable was not found: $resolvedTestExecutable" `
                -StrictMessage "Strict quality gate requires test executable: $resolvedTestExecutable"
            return
        }

        $headSha = Get-RepositoryHeadSha
        if ([string]::IsNullOrWhiteSpace($headSha)) {
            Write-Host "[SKIP] test executable build SHA was not checked because repository HEAD is unavailable"
        } else {
            Invoke-PowerShellFile -ScriptPath (Join-Path $root "tools/check_test_binary_sha.ps1") `
                -Arguments @("-TestExecutable", $resolvedTestExecutable, "-ExpectedSha", $headSha)
        }
    }

    Invoke-Step "unit tests" {
        if (-not $runUnitTests) {
            Write-Host "[SKIP] unit tests skipped with -SkipBuild"
            return
        }

        if (-not (Test-Path -LiteralPath $resolvedTestExecutable -PathType Leaf)) {
            Stop-OrSkipMissingRequirement `
                -SkipMessage "test executable was not found: $resolvedTestExecutable" `
                -StrictMessage "Strict quality gate requires test executable: $resolvedTestExecutable"
            return
        }

        & $resolvedTestExecutable
    }
} finally {
    Pop-Location
}
