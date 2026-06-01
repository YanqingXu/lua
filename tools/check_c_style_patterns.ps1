param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [switch]$ListMatches
)

$ErrorActionPreference = "Stop"

Write-Host "C-style pattern guard"

function Get-SourceFiles {
    $srcRoot = Join-Path $Root "src"
    if (-not (Test-Path -LiteralPath $srcRoot)) {
        return @()
    }

    return Get-ChildItem -LiteralPath $srcRoot -Recurse -File -Include "*.cpp", "*.hpp", "*.h"
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
        AllowedCount = 2
        SkipCommentLines = $true
    },
    @{
        Name = "bare delete"
        Pattern = "(?<![\w:])delete\s+(?:\[\]\s*)?[A-Za-z_][\w_]*(?:->\w+|\.\w+)?\s*;"
        AllowedCount = 1
        SkipCommentLines = $true
    },
    @{
        Name = "C allocator free"
        Pattern = "\b(?:std::)?free\s*\("
        AllowedCount = 1
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
        AllowedCount = 64
        SkipCommentLines = $true
    }
)

$sourceFiles = @(Get-SourceFiles)
$violations = @()

foreach ($rule in $forbiddenPatterns) {
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
    if ($count -le $allowed) {
        Write-Host ("[OK] {0}: {1}/{2} allowed" -f $rule.Name, $count, $allowed)
    } else {
        Write-Host ("[FAIL] {0}: {1}/{2} allowed" -f $rule.Name, $count, $allowed)
        $violations += [pscustomobject]@{
            Rule = $rule.Name
            Count = $count
            AllowedCount = $allowed
            Matches = $matches
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
    }

    throw "C-style pattern guard failed"
}
