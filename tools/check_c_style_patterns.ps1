param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [ValidateSet("Product", "Tests", "All")]
    [string]$TestScope = "Product",
    [switch]$ListMatches
)

$ErrorActionPreference = "Stop"

Write-Host "C-style pattern guard"

function Get-SourceFiles {
    param([string]$Scope)

    $roots = switch ($Scope) {
        "Product" { @("src") }
        "Tests" { @("tests") }
        "All" { @("src", "tests") }
    }

    foreach ($dir in $roots) {
        $sourceRoot = Join-Path $Root $dir
        if (-not (Test-Path -LiteralPath $sourceRoot)) {
            continue
        }

        Get-ChildItem -LiteralPath $sourceRoot -Recurse -File -Include "*.cpp", "*.hpp", "*.h"
    }
}

function Get-RelativePath {
    param([string]$Path)

    $resolvedRoot = (Resolve-Path -LiteralPath $Root).Path.TrimEnd("\", "/")
    $rootPrefix = $resolvedRoot + [System.IO.Path]::DirectorySeparatorChar
    $resolvedPath = (Resolve-Path -LiteralPath $Path).Path

    if ($resolvedPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $resolvedPath.Substring($rootPrefix.Length).Replace("/", "\")
    }

    return $resolvedPath.Replace("/", "\")
}

function Test-SkippableCommentLine {
    param([string]$Line)

    $trimmed = $Line.TrimStart()
    return $trimmed.StartsWith("//") -or $trimmed.StartsWith("/*") -or $trimmed.StartsWith("*")
}

function Test-AllowedMatch {
    param(
        [pscustomobject]$Match,
        [object[]]$AllowedMatches
    )

    foreach ($allowed in @($AllowedMatches)) {
        if ($Match.Path -eq $allowed.Path -and $Match.Text -eq $allowed.Text) {
            return $true
        }
    }

    return $false
}

$forbiddenPatterns = @(
    @{
        Name = "NULL macro"
        Pattern = "(?<![A-Za-z0-9_])NULL(?![A-Za-z0-9_])"
        AllowedCount = 0
        SkipCommentLines = $true
    },
    @{
        Name = "bare new"
        Pattern = "(?<![\w:])new\s+(?:[A-Za-z_][\w:<>]*)(?:\s*\*)?\s*(?:\(|\{|\[)"
        AllowedCount = 0
        SkipCommentLines = $true
    },
    @{
        Name = "bare delete"
        Pattern = "(?<![\w:])delete\s+(?:\[\]\s*)?[A-Za-z_][\w_]*(?:->\w+|\.\w+)?\s*;"
        AllowedCount = 1
        AllowedMatches = @(
            @{ Path = "src\gc\garbage_collector.cpp"; Text = "delete obj;" }
        )
        SkipCommentLines = $true
    },
    @{
        Name = "C allocator free"
        Pattern = "\b(?:std::)?free\s*\("
        AllowedCount = 1
        AllowedMatches = @(
            @{ Path = "src\core\userdata.cpp"; Text = "std::free(data);" }
        )
        SkipCommentLines = $true
    },
    @{
        Name = "simple #define"
        Pattern = "^\s*#\s*define\b"
        AllowedCount = 51
        SkipCommentLines = $false
    },
    @{
        Name = "void pointer C-style cast"
        Pattern = "\(\s*(?:const\s+)?void\s*\*\s*\)\s*[A-Za-z_(&*]"
        AllowedCount = 0
        SkipCommentLines = $true
    },
    @{
        Name = "return nullptr"
        Pattern = "\breturn\s+nullptr\s*;"
        AllowedCount = 60
        SkipCommentLines = $true
    }
)

$warningPatterns = @(
    @{
        Name = "char pointer parse cursor"
        Pattern = "\bchar\s*\*\s*(?:end|endptr)\b"
        AllowedCount = 0
        WarningOnly = $true
        SkipCommentLines = $true
    },
    @{
        Name = "const char pointer array"
        Pattern = "const\s+char\s*\*\s+const\s+\w+\s*\["
        AllowedCount = 0
        WarningOnly = $true
        SkipCommentLines = $true
    },
    @{
        Name = "native fixed-size array declaration"
        Pattern = "\b(?:char|u?int\d*_t|i\d+|u\d+|f\d+|LuaNumber|Instruction|Value|MatchCapture)\s+\w+\s*\[[^\]]+\]"
        AllowedCount = 0
        WarningOnly = $true
        SkipCommentLines = $true
    },
    @{
        Name = "test manual ownership"
        Pattern = "(?<![\w:])(?:new\s+(?:[A-Za-z_][\w:<>]*)(?:\s*\*)?\s*(?:\(|\{|\[)|delete\s+(?:\[\]\s*)?[A-Za-z_][\w_]*(?:->\w+|\.\w+)?\s*;)"
        AllowedCount = 0
        WarningOnly = $true
        Scopes = @("Tests", "All")
        SkipCommentLines = $true
    }
)

$sourceFiles = @(Get-SourceFiles -Scope $TestScope)
$violations = @()

foreach ($rule in @($forbiddenPatterns + $warningPatterns)) {
    if ($rule.ContainsKey("Scopes") -and -not (@($rule.Scopes) -contains $TestScope)) {
        continue
    }

    $regex = [regex]::new($rule.Pattern)
    $matches = @()

    foreach ($file in $sourceFiles) {
        $lineNumber = 0
        foreach ($line in Get-Content -LiteralPath $file.FullName) {
            $lineNumber++

            if ($rule.SkipCommentLines -and (Test-SkippableCommentLine $line)) {
                continue
            }

            $lineMatches = $regex.Matches($line)
            foreach ($match in $lineMatches) {
                $matches += [pscustomobject]@{
                    Rule = $rule.Name
                    Path = Get-RelativePath $file.FullName
                    Line = $lineNumber
                    Text = $line.Trim()
                }
            }
        }
    }

    $count = $matches.Count
    $allowed = [int]$rule.AllowedCount
    $warningOnly = $rule.ContainsKey("WarningOnly") -and [bool]$rule.WarningOnly
    $strictMatches = @($matches)
    $advisoryMatches = @()

    if (-not $warningOnly) {
        if ($TestScope -eq "Tests") {
            $advisoryMatches = @($matches)
            $strictMatches = @()
            $warningOnly = $true
        } elseif ($TestScope -eq "All") {
            $strictMatches = @($matches | Where-Object { -not $_.Path.StartsWith("tests\", [System.StringComparison]::OrdinalIgnoreCase) })
            $advisoryMatches = @($matches | Where-Object { $_.Path.StartsWith("tests\", [System.StringComparison]::OrdinalIgnoreCase) })
        }
    }

    $strictCount = $strictMatches.Count
    $unknownAllowedMatches = @()
    if ($rule.ContainsKey("AllowedMatches")) {
        foreach ($match in $strictMatches) {
            if (-not (Test-AllowedMatch -Match $match -AllowedMatches $rule.AllowedMatches)) {
                $unknownAllowedMatches += $match
            }
        }
    }

    if ($warningOnly) {
        $label = if ($count -eq 0) { "OK" } else { "WARN" }
        Write-Host ("[{0}] {1}: {2} advisory matches" -f $label, $rule.Name, $count)
    } elseif ($strictCount -le $allowed -and $unknownAllowedMatches.Count -eq 0) {
        $suffix = if ($advisoryMatches.Count -gt 0) { " ($($advisoryMatches.Count) test advisory)" } else { "" }
        Write-Host ("[OK] {0}: {1}/{2} allowed{3}" -f $rule.Name, $strictCount, $allowed, $suffix)
    } else {
        Write-Host ("[FAIL] {0}: {1}/{2} allowed" -f $rule.Name, $strictCount, $allowed)
        $violations += [pscustomobject]@{
            Rule = $rule.Name
            Count = $strictCount
            AllowedCount = $allowed
            Matches = $strictMatches
            UnknownAllowedMatches = $unknownAllowedMatches
        }
    }

    if ($ListMatches -and $matches.Count -gt 0) {
        foreach ($match in $matches) {
            Write-Host ("  {0}:{1}: {2}" -f $match.Path, $match.Line, $match.Text)
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Host ""
    Write-Host "New or unregistered C-style patterns were found:"

    foreach ($violation in $violations) {
        Write-Host ("- {0}: {1} current, {2} allowed" -f $violation.Rule, $violation.Count, $violation.AllowedCount)
        foreach ($match in @($violation.Matches | Select-Object -First 10)) {
            Write-Host ("  {0}:{1}: {2}" -f $match.Path, $match.Line, $match.Text)
        }
        foreach ($match in @($violation.UnknownAllowedMatches | Select-Object -First 10)) {
            Write-Host ("  unregistered allowed-location match: {0}:{1}: {2}" -f $match.Path, $match.Line, $match.Text)
        }
    }

    throw "C-style pattern guard failed"
}
