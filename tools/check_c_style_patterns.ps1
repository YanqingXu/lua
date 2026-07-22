param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [ValidateSet("Product", "Tests", "All")]
    [string]$TestScope = "Product",
    [string]$BaselinePath = "tools\c_style_allowlist.json",
    [switch]$UpdateBaseline,
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
        if (Test-Path -LiteralPath $sourceRoot) {
            Get-ChildItem -LiteralPath $sourceRoot -Recurse -File | Where-Object {
                $_.Extension -in @(".cpp", ".hpp", ".h")
            }
        }
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

function Resolve-BaselinePath {
    param([string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }
    return Join-Path $Root $Path
}

function Test-SkippableCommentLine {
    param([string]$Line)

    $trimmed = $Line.TrimStart()
    return $trimmed.StartsWith("//") -or $trimmed.StartsWith("/*") -or $trimmed.StartsWith("*")
}

function Get-TextHash {
    param([string]$Text)

    $sha256 = [System.Security.Cryptography.SHA256]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Text)
        return ([System.BitConverter]::ToString($sha256.ComputeHash($bytes))).Replace("-", "").ToLowerInvariant()
    } finally {
        $sha256.Dispose()
    }
}

function Get-MatchKey {
    param(
        [string]$Rule,
        [string]$Path,
        [int]$Line,
        [string]$TextHash
    )

    return ("{0}|{1}|{2}|{3}" -f $Rule, $Path.ToLowerInvariant(), $Line, $TextHash)
}

function Get-BaselineRationale {
    param([pscustomobject]$Match)

    if ($Match.Rule -eq "bare delete" -and $Match.Path -eq "src\gc\garbage_collector.cpp") {
        return "GC destroyObject is the sole intrusive-list ownership destruction boundary."
    }
    if ($Match.Rule -eq "C allocator free" -and $Match.Path -eq "src\core\userdata.cpp") {
        return "UserdataBufferDeleter pairs the platform C allocation boundary with RAII cleanup."
    }
    if ($Match.Path.StartsWith("tests\", [System.StringComparison]::OrdinalIgnoreCase)) {
        return "Existing test advisory; migrate manual or C-style fixtures when the test is touched."
    }
    if ($Match.WarningOnly) {
        return "Existing product advisory; modernize when the surrounding compatibility boundary is touched."
    }
    return "Existing product compatibility baseline; remove or modernize when the location is touched."
}

$forbiddenPatterns = @(
    @{
        Name = "NULL macro"
        Pattern = "(?<![A-Za-z0-9_])NULL(?![A-Za-z0-9_])"
        SkipCommentLines = $true
    },
    @{
        Name = "bare new"
        Pattern = "(?<![\w:])new\s+(?:[A-Za-z_][\w:<>]*)(?:\s*\*)?\s*(?:\(|\{|\[)"
        SkipCommentLines = $true
    },
    @{
        Name = "bare delete"
        Pattern = "(?<![\w:])delete\s+(?:\[\]\s*)?[A-Za-z_][\w_]*(?:->\w+|\.\w+)?\s*;"
        SkipCommentLines = $true
    },
    @{
        Name = "C allocator free"
        Pattern = "\b(?:std::)?free\s*\("
        SkipCommentLines = $true
    },
    @{
        Name = "simple #define"
        Pattern = "^\s*#\s*define\b"
        SkipCommentLines = $false
    },
    @{
        Name = "void pointer C-style cast"
        Pattern = "\(\s*(?:const\s+)?void\s*\*\s*\)\s*[A-Za-z_(&*]"
        SkipCommentLines = $true
    },
    @{
        Name = "return nullptr"
        Pattern = "\breturn\s+nullptr\s*;"
        SkipCommentLines = $true
    }
)

$warningPatterns = @(
    @{
        Name = "char pointer parse cursor"
        Pattern = "\bchar\s*\*\s*(?:end|endptr)\b"
        WarningOnly = $true
        SkipCommentLines = $true
    },
    @{
        Name = "const char pointer array"
        Pattern = "const\s+char\s*\*\s+const\s+\w+\s*\["
        WarningOnly = $true
        SkipCommentLines = $true
    },
    @{
        Name = "native fixed-size array declaration"
        Pattern = "\b(?:char|u?int\d*_t|i\d+|u\d+|f\d+|LuaNumber|Instruction|Value|MatchCapture)\s+\w+\s*\[[^\]]+\]"
        WarningOnly = $true
        SkipCommentLines = $true
    },
    @{
        Name = "test manual ownership"
        Pattern = "(?<![\w:])(?:new\s+(?:[A-Za-z_][\w:<>]*)(?:\s*\*)?\s*(?:\(|\{|\[)|delete\s+(?:\[\]\s*)?[A-Za-z_][\w_]*(?:->\w+|\.\w+)?\s*;)"
        WarningOnly = $true
        Scopes = @("Tests", "All")
        SkipCommentLines = $true
    }
)

$rules = @($forbiddenPatterns + $warningPatterns)
$sourceFiles = @(Get-SourceFiles -Scope $TestScope)
$matchesByRule = @{}
$allMatches = [System.Collections.Generic.List[object]]::new()

foreach ($rule in $rules) {
    if ($rule.ContainsKey("Scopes") -and -not (@($rule.Scopes) -contains $TestScope)) {
        continue
    }

    $regex = [regex]::new($rule.Pattern)
    $matches = [System.Collections.Generic.List[object]]::new()

    foreach ($file in $sourceFiles) {
        $lineNumber = 0
        foreach ($line in Get-Content -LiteralPath $file.FullName) {
            $lineNumber += 1
            if ($rule.SkipCommentLines -and (Test-SkippableCommentLine $line)) {
                continue
            }

            if ($regex.IsMatch($line)) {
                $text = $line.Trim()
                $item = [pscustomobject]@{
                    Rule = $rule.Name
                    Path = Get-RelativePath $file.FullName
                    Line = $lineNumber
                    Text = $text
                    TextHash = Get-TextHash $text
                    WarningOnly = $rule.ContainsKey("WarningOnly") -and [bool]$rule.WarningOnly
                }
                $matches.Add($item) | Out-Null
                $allMatches.Add($item) | Out-Null
            }
        }
    }

    $matchesByRule[$rule.Name] = @($matches)
}

$resolvedBaselinePath = Resolve-BaselinePath $BaselinePath
if ($UpdateBaseline) {
    if ($TestScope -ne "All") {
        throw "-UpdateBaseline requires -TestScope All so product and test advisory locations stay in one baseline."
    }

    $entries = @($allMatches | Sort-Object Rule, Path, Line, TextHash | ForEach-Object {
        [ordered]@{
            rule = $_.Rule
            path = $_.Path
            line = $_.Line
            textHash = $_.TextHash
            rationale = Get-BaselineRationale $_
        }
    })
    $document = [ordered]@{
        schemaVersion = 1
        generatedBy = "tools/check_c_style_patterns.ps1 -UpdateBaseline -TestScope All"
        entries = $entries
    }
    $json = $document | ConvertTo-Json -Depth 5
    # Keep the generated baseline byte-stable across Windows and Unix runners.
    [System.IO.File]::WriteAllText($resolvedBaselinePath, $json + "`n", [System.Text.UTF8Encoding]::new($false))
    Write-Host "[UPDATED] C-style position baseline: $($entries.Count) entries"
    return
}

if (-not (Test-Path -LiteralPath $resolvedBaselinePath)) {
    throw "Missing C-style position baseline: $BaselinePath. Generate it with -UpdateBaseline -TestScope All."
}

$baseline = Get-Content -LiteralPath $resolvedBaselinePath -Raw | ConvertFrom-Json
if ($baseline.schemaVersion -ne 1) {
    throw "Unsupported C-style baseline schemaVersion: $($baseline.schemaVersion)"
}

$baselineEntries = @($baseline.entries)
$baselineByKey = @{}
foreach ($entry in $baselineEntries) {
    if ([string]::IsNullOrWhiteSpace($entry.rule) -or [string]::IsNullOrWhiteSpace($entry.path) -or
        [string]::IsNullOrWhiteSpace($entry.textHash) -or [string]::IsNullOrWhiteSpace($entry.rationale)) {
        throw "C-style baseline entries require rule/path/line/textHash/rationale."
    }
    $key = Get-MatchKey $entry.rule $entry.path ([int]$entry.line) $entry.textHash
    if ($baselineByKey.ContainsKey($key)) {
        throw "Duplicate C-style baseline entry: $($entry.rule) $($entry.path):$($entry.line)"
    }
    $baselineByKey[$key] = $entry
}

function Test-PathInScope {
    param([string]$Path)

    if ($TestScope -eq "Product") { return $Path.StartsWith("src\", [System.StringComparison]::OrdinalIgnoreCase) }
    if ($TestScope -eq "Tests") { return $Path.StartsWith("tests\", [System.StringComparison]::OrdinalIgnoreCase) }
    return $Path.StartsWith("src\", [System.StringComparison]::OrdinalIgnoreCase) -or
        $Path.StartsWith("tests\", [System.StringComparison]::OrdinalIgnoreCase)
}

$violations = [System.Collections.Generic.List[object]]::new()
foreach ($rule in $rules) {
    if (-not $matchesByRule.ContainsKey($rule.Name)) {
        continue
    }

    $matches = @($matchesByRule[$rule.Name])
    $strictMatches = @($matches)
    $advisoryMatches = @()

    $strictKeys = @{}
    $unknownStrict = @()
    foreach ($match in $strictMatches) {
        $key = Get-MatchKey $match.Rule $match.Path $match.Line $match.TextHash
        $strictKeys[$key] = $true
        if (-not $baselineByKey.ContainsKey($key)) {
            $unknownStrict += $match
        }
    }

    $expectedStrictEntries = @($baselineEntries | Where-Object {
        $_.rule -eq $rule.Name -and (Test-PathInScope $_.path)
    })
    $staleStrict = @($expectedStrictEntries | Where-Object {
        $key = Get-MatchKey $_.rule $_.path ([int]$_.line) $_.textHash
        -not $strictKeys.ContainsKey($key)
    })

    $unknownAdvisory = @()
    $advisoryKeys = @{}
    foreach ($match in $advisoryMatches) {
        $key = Get-MatchKey $match.Rule $match.Path $match.Line $match.TextHash
        $advisoryKeys[$key] = $true
        if (-not $baselineByKey.ContainsKey($key)) {
            $unknownAdvisory += $match
        }
    }
    $expectedAdvisoryEntries = @()
    $staleAdvisory = @($expectedAdvisoryEntries | Where-Object {
        $key = Get-MatchKey $_.rule $_.path ([int]$_.line) $_.textHash
        -not $advisoryKeys.ContainsKey($key)
    })

    if ($unknownStrict.Count -eq 0 -and $staleStrict.Count -eq 0) {
        $suffix = if ($advisoryMatches.Count -gt 0) { "; $($advisoryMatches.Count) advisory, $($unknownAdvisory.Count) new" } else { "" }
        Write-Host ("[OK] {0}: {1} position-baselined{2}" -f $rule.Name, $strictMatches.Count, $suffix)
    } else {
        Write-Host ("[FAIL] {0}: {1} unregistered, {2} stale baseline entries" -f $rule.Name, $unknownStrict.Count, $staleStrict.Count)
        $violations.Add([pscustomobject]@{
            Rule = $rule.Name
            Unknown = $unknownStrict
            Stale = $staleStrict
        }) | Out-Null
    }

    if ($staleAdvisory.Count -gt 0) {
        Write-Host ("[RATCHET] {0}: {1} advisory baseline entries can be removed with -UpdateBaseline" -f $rule.Name, $staleAdvisory.Count)
    }
    if ($ListMatches) {
        foreach ($match in $matches) {
            Write-Host ("  {0}:{1}: {2} [{3}]" -f $match.Path, $match.Line, $match.Text, $match.TextHash)
        }
    }
}

if ($violations.Count -gt 0) {
    Write-Host ""
    Write-Host "C-style position baseline drift was found:"
    foreach ($violation in $violations) {
        foreach ($match in @($violation.Unknown | Select-Object -First 10)) {
            Write-Host ("  unregistered: {0} {1}:{2}: {3}" -f $violation.Rule, $match.Path, $match.Line, $match.Text)
        }
        foreach ($entry in @($violation.Stale | Select-Object -First 10)) {
            Write-Host ("  stale: {0} {1}:{2} [{3}]" -f $violation.Rule, $entry.path, $entry.line, $entry.textHash)
        }
    }
    throw "C-style pattern guard failed. Review the change, then run -UpdateBaseline -TestScope All if it is intentional."
}

Write-Host "[OK] C-style position baseline guard passed"
