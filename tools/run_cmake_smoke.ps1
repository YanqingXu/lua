param(
    [string]$BuildDir = "build\cmake",
    [string]$Configuration = "Debug",
    [string]$Generator = "",
    [string]$Platform = "x64",
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Get-CommandOrNull {
    param([string]$Name)
    return Get-Command $Name -ErrorAction SilentlyContinue
}

function Find-VisualStudioTool {
    param([string]$Name)

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere)) {
        return $null
    }

    $installPath = & $vswhere -latest -products * -property installationPath
    if (-not $installPath) {
        return $null
    }

    $candidate = Join-Path $installPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\$Name"
    if (Test-Path -LiteralPath $candidate) {
        return $candidate
    }

    return $null
}

function Find-Tool {
    param([string]$Name)

    $command = Get-CommandOrNull $Name
    if ($command) {
        return $command.Source
    }

    $exeName = if ($Name.EndsWith(".exe")) { $Name } else { "$Name.exe" }
    return Find-VisualStudioTool $exeName
}

function Invoke-Step {
    param(
        [string]$Name,
        [scriptblock]$Body
    )

    Write-Host ""
    Write-Host "==> $Name"
    & $Body
}

function Invoke-NativeCommand {
    param(
        [string]$FilePath,
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $FilePath $($Arguments -join ' ')"
    }
}

$cmake = Find-Tool "cmake"
if (-not $cmake) {
    Write-Host "[SKIP] CMake was not found on PATH or in the Visual Studio installation."
    exit 0
}

$ctest = Find-Tool "ctest"
if (-not $ctest) {
    Write-Host "[SKIP] CTest was not found on PATH or in the Visual Studio installation."
    exit 0
}

$buildPath = if ([System.IO.Path]::IsPathRooted($BuildDir)) {
    $BuildDir
} else {
    Join-Path $root $BuildDir
}

if ($Clean -and (Test-Path -LiteralPath $buildPath)) {
    $resolvedRoot = (Resolve-Path -LiteralPath $root).Path.TrimEnd('\', '/')
    $resolvedBuild = (Resolve-Path -LiteralPath $buildPath).Path
    if (-not $resolvedBuild.StartsWith($resolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a build directory outside the repository: $resolvedBuild"
    }

    Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
}

Push-Location $root
try {
    Invoke-Step "Configure CMake" {
        $configureArgs = @("-S", $root, "-B", $buildPath)
        if ($Generator) {
            $configureArgs += @("-G", $Generator)
            if ($Platform) {
                $configureArgs += @("-A", $Platform)
            }
        }

        Invoke-NativeCommand $cmake $configureArgs
    }

    Invoke-Step "Build CMake targets" {
        Invoke-NativeCommand $cmake @("--build", $buildPath, "--config", $Configuration)
    }

    Invoke-Step "Run CTest" {
        Invoke-NativeCommand $ctest @("--test-dir", $buildPath, "-C", $Configuration, "--output-on-failure")
    }
} finally {
    Pop-Location
}
