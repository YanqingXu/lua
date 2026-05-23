param(
    [Parameter(Mandatory = $true, Position = 0)]
    [Alias("Path")]
    [string[]]$SourcePath,

    [ValidateSet("Auto", "Core", "Repl", "App", "Bytecode", "Test")]
    [string[]]$Target = @("Auto"),

    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,

    [switch]$AllowMissing,
    [switch]$DryRun,
    [switch]$Quiet
)

$ErrorActionPreference = "Stop"

$script:Changes = [System.Collections.Generic.List[string]]::new()
$script:Noops = [System.Collections.Generic.List[string]]::new()

function Join-RepoPath {
    param([string]$RelativePath)
    return Join-Path $Root $RelativePath
}

function Read-Text {
    param([string]$Path)
    return [System.IO.File]::ReadAllText($Path)
}

function Write-Text {
    param(
        [string]$Path,
        [string]$Text
    )

    if (-not $DryRun) {
        $encoding = [System.Text.UTF8Encoding]::new($false)
        [System.IO.File]::WriteAllText($Path, $Text, $encoding)
    }
}

function Get-Newline {
    param([string]$Text)
    if ($Text -match "`r`n") {
        return "`r`n"
    }
    return "`n"
}

function Escape-Regex {
    param([string]$Text)
    return [regex]::Escape($Text)
}

function Record-Change {
    param([string]$Message)
    $script:Changes.Add($Message) | Out-Null
}

function Record-Noop {
    param([string]$Message)
    $script:Noops.Add($Message) | Out-Null
}

function Write-Status {
    param([string]$Message)
    if (-not $Quiet) {
        Write-Host $Message
    }
}

function Resolve-RepoSource {
    param([string]$Path)

    $rootPath = (Resolve-Path -LiteralPath $Root).Path.TrimEnd('\', '/')
    $candidate = if ([System.IO.Path]::IsPathRooted($Path)) {
        [System.IO.Path]::GetFullPath($Path)
    } else {
        [System.IO.Path]::GetFullPath((Join-Path $rootPath $Path))
    }

    if (-not $candidate.StartsWith($rootPath, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to add a path outside the repository: $Path"
    }

    if (-not $AllowMissing -and -not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "Source file does not exist: $Path (use -AllowMissing only for planned files)"
    }

    $relative = $candidate.Substring($rootPath.Length).TrimStart('\', '/')
    $msbuild = $relative -replace '/', '\'
    $cmake = $relative -replace '\\', '/'
    $extension = [System.IO.Path]::GetExtension($relative).ToLowerInvariant()
    $kind = switch ($extension) {
        ".cpp" { "ClCompile"; break }
        ".cc" { "ClCompile"; break }
        ".cxx" { "ClCompile"; break }
        ".c" { "ClCompile"; break }
        ".hpp" { "ClInclude"; break }
        ".hh" { "ClInclude"; break }
        ".hxx" { "ClInclude"; break }
        ".h" { "ClInclude"; break }
        default { throw "Unsupported source extension '$extension': $Path" }
    }

    return [pscustomobject]@{
        FullPath = $candidate
        Relative = $relative
        MsBuild = $msbuild
        CMake = $cmake
        Kind = $kind
    }
}

function Resolve-SourceTargets {
    param([string]$MsBuildPath)

    if ($Target -contains "Auto" -and $Target.Count -gt 1) {
        throw "-Target Auto cannot be combined with explicit targets"
    }

    if ($Target -notcontains "Auto") {
        return @($Target | Sort-Object -Unique)
    }

    if ($MsBuildPath -like "tests\unit\*") {
        return @("Test")
    }
    if ($MsBuildPath -like "src\repl\*" -or $MsBuildPath -eq "src\repl.cpp" -or
        $MsBuildPath -eq "src\repl.hpp") {
        return @("Repl")
    }
    if ($MsBuildPath -like "src\app\*" -or $MsBuildPath -eq "src\main.cpp") {
        return @("App")
    }
    if ($MsBuildPath -like "src\bytecode\*") {
        return @("Bytecode")
    }
    if ($MsBuildPath -like "src\*") {
        return @("Core")
    }

    throw "Cannot infer target for '$MsBuildPath'; pass -Target explicitly"
}

function Get-CMakeContainer {
    param([string]$TargetName)

    switch ($TargetName) {
        "Core" { return [pscustomobject]@{ Kind = "List"; Name = "LUA_CORE_SOURCES" } }
        "Repl" { return [pscustomobject]@{ Kind = "List"; Name = "LUA_REPL_SOURCES" } }
        "Test" { return [pscustomobject]@{ Kind = "List"; Name = "LUA_TEST_SOURCES" } }
        "App" { return [pscustomobject]@{ Kind = "Executable"; Name = "lua_app" } }
        "Bytecode" { return [pscustomobject]@{ Kind = "Executable"; Name = "lua_bytecode" } }
        default { throw "Unknown target: $TargetName" }
    }
}

function Get-ProjectFiles {
    param([string]$TargetName)

    switch ($TargetName) {
        "Core" { return @("lua.vcxproj") }
        "Repl" { return @("lua_app.vcxproj", "lua_test.vcxproj") }
        "App" { return @("lua_app.vcxproj") }
        "Bytecode" { return @("lua_bytecode.vcxproj") }
        "Test" { return @("lua_test.vcxproj") }
        default { throw "Unknown target: $TargetName" }
    }
}

function Get-FilterName {
    param([string]$MsBuildPath)

    $directory = Split-Path -Path $MsBuildPath -Parent
    if ([string]::IsNullOrWhiteSpace($directory)) {
        return ""
    }

    if ($directory -eq "tests\unit\framework") {
        return "framework"
    }

    if ($directory -like "tests\unit\*") {
        $rest = $directory.Substring("tests\unit\".Length)
        return "tests\$rest"
    }

    return $directory
}

function New-DeterministicGuidText {
    param([string]$Text)

    $md5 = [System.Security.Cryptography.MD5]::Create()
    try {
        $hash = $md5.ComputeHash([System.Text.Encoding]::UTF8.GetBytes("lua-add-source:$Text"))
    } finally {
        $md5.Dispose()
    }

    $hex = -join ($hash | ForEach-Object { $_.ToString("x2") })
    return "{$($hex.Substring(0, 8))-$($hex.Substring(8, 4))-$($hex.Substring(12, 4))-$($hex.Substring(16, 4))-$($hex.Substring(20, 12))}"
}

function Get-FilterChain {
    param([string]$FilterName)

    if ([string]::IsNullOrWhiteSpace($FilterName)) {
        return @()
    }

    $parts = $FilterName -split "\\"
    $chain = @()
    for ($i = 0; $i -lt $parts.Count; ++$i) {
        $chain += (($parts[0..$i]) -join "\")
    }
    return $chain
}

function Add-CMakeListEntry {
    param(
        [string]$ListName,
        [string]$Entry
    )

    $path = Join-RepoPath "CMakeLists.txt"
    $text = Read-Text $path
    $pattern = "(?ms)set\($(Escape-Regex $ListName)\r?\n.*?\r?\n\s*\)"
    $match = [regex]::Match($text, $pattern)
    if (-not $match.Success) {
        throw "Could not find CMake list '$ListName'"
    }

    if ($match.Value -match "(?m)^\s*$(Escape-Regex $Entry)\s*$") {
        Record-Noop "CMakeLists.txt already contains $Entry in $ListName"
        return
    }

    $newline = Get-Newline $text
    $closing = [regex]::Match($match.Value, "\r?\n\s*\)$")
    if (-not $closing.Success) {
        throw "Could not find closing ')' for CMake list '$ListName'"
    }

    $insertAt = $match.Index + $match.Length - $closing.Length
    $updated = $text.Insert($insertAt, "$newline    $Entry")
    Write-Text $path $updated
    Record-Change "CMakeLists.txt: added $Entry to $ListName"
}

function Add-CMakeExecutableEntry {
    param(
        [string]$ExecutableName,
        [string]$Entry
    )

    $path = Join-RepoPath "CMakeLists.txt"
    $text = Read-Text $path
    $pattern = "(?ms)add_executable\($(Escape-Regex $ExecutableName)\r?\n.*?\r?\n\s*\)"
    $match = [regex]::Match($text, $pattern)
    if (-not $match.Success) {
        throw "Could not find CMake executable '$ExecutableName'"
    }

    if ($match.Value -match "(?m)^\s*$(Escape-Regex $Entry)\s*$") {
        Record-Noop "CMakeLists.txt already contains $Entry in $ExecutableName"
        return
    }

    $newline = Get-Newline $text
    $closing = [regex]::Match($match.Value, "\r?\n\s*\)$")
    if (-not $closing.Success) {
        throw "Could not find closing ')' for CMake executable '$ExecutableName'"
    }

    $insertAt = $match.Index + $match.Length - $closing.Length
    $updated = $text.Insert($insertAt, "$newline        $Entry")
    Write-Text $path $updated
    Record-Change "CMakeLists.txt: added $Entry to $ExecutableName"
}

function Add-CMakeEntry {
    param(
        [string]$TargetName,
        [string]$Entry
    )

    $container = Get-CMakeContainer $TargetName
    if ($container.Kind -eq "List") {
        Add-CMakeListEntry $container.Name $Entry
    } else {
        Add-CMakeExecutableEntry $container.Name $Entry
    }
}

function Add-ProjectItem {
    param(
        [string]$ProjectFile,
        [string]$ItemKind,
        [string]$IncludePath
    )

    $path = Join-RepoPath $ProjectFile
    $text = Read-Text $path
    $itemPattern = "<$ItemKind\s+Include=`"$(Escape-Regex $IncludePath)`"(\s*/|>)"
    if ($text -match $itemPattern) {
        Record-Noop "${ProjectFile} already contains $IncludePath"
        return
    }

    $newline = Get-Newline $text
    $groupPattern = "(?ms)<ItemGroup>\r?\n(?:(?!</ItemGroup>).)*<$ItemKind\b(?:(?!</ItemGroup>).)*</ItemGroup>"
    $match = [regex]::Match($text, $groupPattern)
    $entry = "    <$ItemKind Include=`"$IncludePath`" />"

    if ($match.Success) {
        $closing = [regex]::Match($match.Value, "\r?\n\s*</ItemGroup>")
        $insertAt = $match.Index + $match.Length - $closing.Length
        $updated = $text.Insert($insertAt, "$newline$entry")
    } else {
        $projectClose = [regex]::Match($text, "\r?\n</Project>\s*$")
        if (-not $projectClose.Success) {
            throw "Could not find project close tag in $ProjectFile"
        }
        $block = "$newline  <ItemGroup>$newline$entry$newline  </ItemGroup>"
        $updated = $text.Insert($projectClose.Index, $block)
    }

    Write-Text $path $updated
    Record-Change "${ProjectFile}: added $IncludePath as $ItemKind"
}

function Add-FilterDeclarations {
    param(
        [string]$FiltersFile,
        [string]$FilterName
    )

    $chain = @(Get-FilterChain $FilterName)
    if ($chain.Count -eq 0) {
        return
    }

    $path = Join-RepoPath $FiltersFile
    $text = Read-Text $path
    $newline = Get-Newline $text
    $entries = @()

    foreach ($filter in $chain) {
        if ($text -notmatch "<Filter\s+Include=`"$(Escape-Regex $filter)`"(\s*/|>)") {
            $guid = New-DeterministicGuidText "$FiltersFile/$filter"
            $entries += "    <Filter Include=`"$filter`">$newline      <UniqueIdentifier>$guid</UniqueIdentifier>$newline    </Filter>"
        }
    }

    if ($entries.Count -eq 0) {
        return
    }

    $block = ($entries -join $newline)
    $groupPattern = "(?ms)<ItemGroup>\r?\n(?:(?!</ItemGroup>).)*<Filter\b(?:(?!</ItemGroup>).)*</ItemGroup>"
    $match = [regex]::Match($text, $groupPattern)

    if ($match.Success) {
        $closing = [regex]::Match($match.Value, "\r?\n\s*</ItemGroup>")
        $insertAt = $match.Index + $match.Length - $closing.Length
        $updated = $text.Insert($insertAt, "$newline$block")
    } else {
        $projectOpen = [regex]::Match($text, "<Project[^>]*>\r?\n")
        if (-not $projectOpen.Success) {
            throw "Could not find project open tag in $FiltersFile"
        }
        $newGroup = "  <ItemGroup>$newline$block$newline  </ItemGroup>$newline"
        $updated = $text.Insert($projectOpen.Index + $projectOpen.Length, $newGroup)
    }

    Write-Text $path $updated
    Record-Change "${FiltersFile}: added filter declaration(s) for $FilterName"
}

function Add-FilterItem {
    param(
        [string]$FiltersFile,
        [string]$ItemKind,
        [string]$IncludePath,
        [string]$FilterName
    )

    $path = Join-RepoPath $FiltersFile
    $text = Read-Text $path
    $itemPattern = "<$ItemKind\s+Include=`"$(Escape-Regex $IncludePath)`"(\s*/|>)"
    if ($text -match $itemPattern) {
        Record-Noop "${FiltersFile} already contains $IncludePath"
        return
    }

    Add-FilterDeclarations $FiltersFile $FilterName
    $text = Read-Text $path
    $newline = Get-Newline $text

    if ([string]::IsNullOrWhiteSpace($FilterName)) {
        $entry = "    <$ItemKind Include=`"$IncludePath`" />"
    } else {
        $entry = "    <$ItemKind Include=`"$IncludePath`">$newline      <Filter>$FilterName</Filter>$newline    </$ItemKind>"
    }

    $groupPattern = "(?ms)<ItemGroup>\r?\n(?:(?!</ItemGroup>).)*<$ItemKind\b(?:(?!</ItemGroup>).)*</ItemGroup>"
    $match = [regex]::Match($text, $groupPattern)
    if ($match.Success) {
        $closing = [regex]::Match($match.Value, "\r?\n\s*</ItemGroup>")
        $insertAt = $match.Index + $match.Length - $closing.Length
        $updated = $text.Insert($insertAt, "$newline$entry")
    } else {
        $projectClose = [regex]::Match($text, "\r?\n</Project>\s*$")
        if (-not $projectClose.Success) {
            throw "Could not find project close tag in $FiltersFile"
        }
        $block = "$newline  <ItemGroup>$newline$entry$newline  </ItemGroup>"
        $updated = $text.Insert($projectClose.Index, $block)
    }

    Write-Text $path $updated
    Record-Change "${FiltersFile}: added $IncludePath with filter '$FilterName'"
}

function Add-VisualStudioEntry {
    param(
        [string]$ProjectFile,
        [string]$ItemKind,
        [string]$IncludePath
    )

    Add-ProjectItem $ProjectFile $ItemKind $IncludePath

    $filtersFile = "$ProjectFile.filters"
    $filtersPath = Join-RepoPath $filtersFile
    if (Test-Path -LiteralPath $filtersPath) {
        $filterName = Get-FilterName $IncludePath
        Add-FilterItem $filtersFile $ItemKind $IncludePath $filterName
    }
}

foreach ($sourcePath in $SourcePath) {
    $source = Resolve-RepoSource $sourcePath
    $sourceTargets = Resolve-SourceTargets $source.MsBuild

    foreach ($targetName in $sourceTargets) {
        if ($source.Kind -eq "ClCompile") {
            Add-CMakeEntry $targetName $source.CMake
        }

        foreach ($projectFile in Get-ProjectFiles $targetName) {
            Add-VisualStudioEntry $projectFile $source.Kind $source.MsBuild
        }
    }
}

foreach ($message in $script:Changes) {
    $prefix = if ($DryRun) { "[DRY] " } else { "[ADD] " }
    Write-Status "$prefix$message"
}

foreach ($message in $script:Noops) {
    Write-Status "[OK] $message"
}

if ($script:Changes.Count -eq 0) {
    Write-Status "[OK] no project files needed changes"
}
