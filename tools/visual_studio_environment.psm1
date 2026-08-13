$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Get-LuaCppDefaultVsWherePath {
    $programFilesX86 = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)", "Process")
    if ([string]::IsNullOrWhiteSpace($programFilesX86)) {
        return ""
    }
    return Join-Path $programFilesX86 "Microsoft Visual Studio/Installer/vswhere.exe"
}

function Test-LuaCppVisualStudioRoot {
    param([Parameter(Mandatory = $true)][string]$InstallationPath)

    if ([string]::IsNullOrWhiteSpace($InstallationPath)) {
        return $false
    }
    $developerCommand = Join-Path $InstallationPath "Common7/Tools/VsDevCmd.bat"
    return Test-Path -LiteralPath $developerCommand -PathType Leaf
}

function Find-LuaCppVisualStudioInstallation {
    param(
        [Parameter(Mandatory = $true)]
        [string]$CMakeExecutable,

        [string]$VsWhereExecutable = ""
    )

    $vswhere = $VsWhereExecutable
    if ([string]::IsNullOrWhiteSpace($vswhere)) {
        $vswhere = Get-LuaCppDefaultVsWherePath
    }

    $diagnostics = [Collections.Generic.List[string]]::new()
    if (-not [string]::IsNullOrWhiteSpace($vswhere) -and
        (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        $global:LASTEXITCODE = 0
        try {
            $output = @(& $vswhere @(
                "-latest",
                "-products", "*",
                "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                "-property", "installationPath"
            ))
            $exitCode = $global:LASTEXITCODE
            if ($exitCode -ne 0) {
                $diagnostics.Add("vswhere exited with code $exitCode")
            } else {
                foreach ($candidateLine in $output) {
                    $candidate = ([string]$candidateLine).Trim()
                    if (Test-LuaCppVisualStudioRoot -InstallationPath $candidate) {
                        return [IO.Path]::GetFullPath($candidate)
                    }
                }
                $diagnostics.Add("vswhere returned no installation with Common7/Tools/VsDevCmd.bat")
            }
        } catch {
            $diagnostics.Add("vswhere failed: $($_.Exception.Message)")
        }
    } else {
        $diagnostics.Add("vswhere is unavailable")
    }

    $normalizedCMake = [IO.Path]::GetFullPath($CMakeExecutable)
    $common7Marker = [IO.Path]::DirectorySeparatorChar + "Common7" +
        [IO.Path]::DirectorySeparatorChar
    $markerIndex = $normalizedCMake.IndexOf(
        $common7Marker,
        [StringComparison]::OrdinalIgnoreCase
    )
    if ($markerIndex -ge 0) {
        $bundledInstallation = $normalizedCMake.Substring(0, $markerIndex)
        if (Test-LuaCppVisualStudioRoot -InstallationPath $bundledInstallation) {
            return $bundledInstallation
        }
        $diagnostics.Add("bundled CMake path has no Common7/Tools/VsDevCmd.bat")
    } else {
        $diagnostics.Add("CMake is not bundled under a Visual Studio Common7 directory")
    }

    throw "Unable to locate a Visual Studio installation with the x64 C++ tools. " +
        ($diagnostics -join "; ") + ". CMake: $normalizedCMake"
}

function Import-LuaCppVisualStudioDeveloperEnvironment {
    param(
        [Parameter(Mandatory = $true)]
        [string]$CMakeExecutable,

        [string]$VsWhereExecutable = ""
    )

    if ($env:OS -ne "Windows_NT" -or
        $null -ne (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        return
    }

    $findArguments = @{ CMakeExecutable = $CMakeExecutable }
    if (-not [string]::IsNullOrWhiteSpace($VsWhereExecutable)) {
        $findArguments.VsWhereExecutable = $VsWhereExecutable
    }
    $visualStudio = Find-LuaCppVisualStudioInstallation @findArguments
    $developerCommand = Join-Path $visualStudio "Common7/Tools/VsDevCmd.bat"
    if ([string]::IsNullOrWhiteSpace($env:ComSpec) -or
        -not (Test-Path -LiteralPath $env:ComSpec -PathType Leaf)) {
        throw "Windows command processor is unavailable; cannot import $developerCommand"
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
    if ($null -eq (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        throw "Visual Studio developer environment did not expose cl.exe"
    }
}

Export-ModuleMember -Function @(
    "Find-LuaCppVisualStudioInstallation",
    "Import-LuaCppVisualStudioDeveloperEnvironment"
)
