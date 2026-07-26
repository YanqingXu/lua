param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$AllowlistPath = "tests\quality\test_signal_allowlist.json",
    [switch]$ListMatches
)

$ErrorActionPreference = "Stop"

function Get-NormalizedPath {
    param([string]$Path)

    $normalized = $Path.Replace("/", "\")
    if ($normalized.StartsWith(".\", [System.StringComparison]::Ordinal)) {
        return $normalized.Substring(2)
    }
    return $normalized
}

function Resolve-RepoPath {
    param([string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }
    return Join-Path $Root $Path
}

function Get-RelativePath {
    param([string]$Path)

    $resolvedRoot = (Resolve-Path -LiteralPath $Root).Path.TrimEnd("\", "/")
    $resolvedPath = (Resolve-Path -LiteralPath $Path).Path
    $rootPrefix = $resolvedRoot + [System.IO.Path]::DirectorySeparatorChar
    if (-not $resolvedPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Test signal scan path is outside the repository root: $resolvedPath"
    }
    return Get-NormalizedPath $resolvedPath.Substring($rootPrefix.Length)
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

function Get-CodeMask {
    param([string]$Text)

    $builder = [System.Text.StringBuilder]::new($Text.Length)
    $index = 0
    $state = "code"

    while ($index -lt $Text.Length) {
        $current = $Text[$index]
        $next = if ($index + 1 -lt $Text.Length) { $Text[$index + 1] } else { [char]0 }

        if ($state -eq "line-comment") {
            if ($current -eq "`n") {
                [void]$builder.Append("`n")
                $state = "code"
            } else {
                [void]$builder.Append(" ")
            }
            $index += 1
            continue
        }

        if ($state -eq "block-comment") {
            if ($current -eq "*" -and $next -eq "/") {
                [void]$builder.Append("  ")
                $index += 2
                $state = "code"
                continue
            }
            [void]$builder.Append($(if ($current -eq "`n") { "`n" } else { " " }))
            $index += 1
            continue
        }

        if ($state -eq "string" -or $state -eq "character") {
            $closingCharacter = if ($state -eq "string") { '"' } else { "'" }
            if ($current -eq "\") {
                [void]$builder.Append(" ")
                $index += 1
                if ($index -lt $Text.Length) {
                    [void]$builder.Append($(if ($Text[$index] -eq "`n") { "`n" } else { " " }))
                    $index += 1
                }
                continue
            }

            [void]$builder.Append($(if ($current -eq "`n") { "`n" } else { " " }))
            $index += 1
            if ($current -eq $closingCharacter) {
                $state = "code"
            }
            continue
        }

        if ($current -eq "/" -and $next -eq "/") {
            [void]$builder.Append("  ")
            $index += 2
            $state = "line-comment"
            continue
        }
        if ($current -eq "/" -and $next -eq "*") {
            [void]$builder.Append("  ")
            $index += 2
            $state = "block-comment"
            continue
        }

        if ($current -eq "R" -and $next -eq '"') {
            $delimiterEnd = $Text.IndexOf("(", $index + 2, [System.StringComparison]::Ordinal)
            if ($delimiterEnd -ge 0 -and $delimiterEnd - $index -le 18) {
                $delimiter = $Text.Substring($index + 2, $delimiterEnd - ($index + 2))
                if ($delimiter -notmatch "[\s()\\]") {
                    $rawEndMarker = ")" + $delimiter + '"'
                    $rawEnd = $Text.IndexOf($rawEndMarker, $delimiterEnd + 1, [System.StringComparison]::Ordinal)
                    if ($rawEnd -ge 0) {
                        $rawLength = $rawEnd + $rawEndMarker.Length - $index
                        for ($rawOffset = 0; $rawOffset -lt $rawLength; $rawOffset += 1) {
                            $rawCharacter = $Text[$index + $rawOffset]
                            [void]$builder.Append($(if ($rawCharacter -eq "`n") { "`n" } else { " " }))
                        }
                        $index += $rawLength
                        continue
                    }
                }
            }
        }

        if ($current -eq '"') {
            [void]$builder.Append(" ")
            $index += 1
            $state = "string"
            continue
        }
        if ($current -eq "'") {
            [void]$builder.Append(" ")
            $index += 1
            $state = "character"
            continue
        }

        [void]$builder.Append($current)
        $index += 1
    }

    return $builder.ToString()
}

function Get-ClosingParenthesis {
    param(
        [string]$Mask,
        [int]$OpenIndex
    )

    $depth = 0
    for ($index = $OpenIndex; $index -lt $Mask.Length; $index += 1) {
        if ($Mask[$index] -eq "(") {
            $depth += 1
        } elseif ($Mask[$index] -eq ")") {
            $depth -= 1
            if ($depth -eq 0) {
                return $index
            }
        }
    }
    return -1
}

function Get-MacroArguments {
    param(
        [string]$Text,
        [string]$Mask,
        [int]$OpenIndex,
        [int]$CloseIndex
    )

    $arguments = [System.Collections.Generic.List[object]]::new()
    $argumentStart = $OpenIndex + 1
    $parenthesisDepth = 0
    $bracketDepth = 0
    $braceDepth = 0

    for ($index = $argumentStart; $index -lt $CloseIndex; $index += 1) {
        switch ($Mask[$index]) {
            "(" { $parenthesisDepth += 1 }
            ")" { $parenthesisDepth -= 1 }
            "[" { $bracketDepth += 1 }
            "]" { $bracketDepth -= 1 }
            "{" { $braceDepth += 1 }
            "}" { $braceDepth -= 1 }
            "," {
                if ($parenthesisDepth -eq 0 -and $bracketDepth -eq 0 -and $braceDepth -eq 0) {
                    $arguments.Add([pscustomobject]@{
                            Start = $argumentStart
                            Length = $index - $argumentStart
                            Text = $Text.Substring($argumentStart, $index - $argumentStart)
                            Mask = $Mask.Substring($argumentStart, $index - $argumentStart)
                        }) | Out-Null
                    $argumentStart = $index + 1
                }
            }
        }
    }

    $arguments.Add([pscustomobject]@{
            Start = $argumentStart
            Length = $CloseIndex - $argumentStart
            Text = $Text.Substring($argumentStart, $CloseIndex - $argumentStart)
            Mask = $Mask.Substring($argumentStart, $CloseIndex - $argumentStart)
        }) | Out-Null
    return @($arguments)
}

function Test-UnconditionalTrue {
    param([string]$ConditionMask)

    $compact = [regex]::Replace($ConditionMask, "\s+", "")
    return $compact -match "^\(*true\)*$"
}

function Get-LineNumber {
    param(
        [string]$Text,
        [int]$Index
    )

    if ($Index -le 0) {
        return 1
    }
    return ([regex]::Matches($Text.Substring(0, $Index), "`n")).Count + 1
}

function Get-StatementEnd {
    param(
        [string]$Mask,
        [int]$StartIndex
    )

    $parenthesisDepth = 0
    $bracketDepth = 0
    $braceDepth = 0
    for ($index = $StartIndex; $index -lt $Mask.Length; $index += 1) {
        switch ($Mask[$index]) {
            "(" { $parenthesisDepth += 1 }
            ")" { $parenthesisDepth -= 1 }
            "[" { $bracketDepth += 1 }
            "]" { $bracketDepth -= 1 }
            "{" { $braceDepth += 1 }
            "}" { $braceDepth -= 1 }
            ";" {
                if ($parenthesisDepth -eq 0 -and $bracketDepth -eq 0 -and $braceDepth -eq 0) {
                    return $index
                }
            }
        }
    }
    return -1
}

function Test-ContainsSkipLiteral {
    param([string]$Text)

    $stringLiteralPattern = '(?is)(?:u8|u|U|L)?"(?:\\.|[^"\\])*(?:\[SKIP\]|\bskipped\b)(?:\\.|[^"\\])*"'
    return [regex]::IsMatch($Text, $stringLiteralPattern)
}

function New-SignalMatch {
    param(
        [string]$Rule,
        [string]$Path,
        [int]$Line,
        [string]$Text
    )

    $normalizedText = $Text.Trim()
    return [pscustomobject]@{
        Rule = $Rule
        Path = Get-NormalizedPath $Path
        Line = $Line
        Text = $normalizedText
        TextHash = Get-TextHash $normalizedText
    }
}

function Get-MatchesForFile {
    param([System.IO.FileInfo]$File)

    $relativePath = Get-RelativePath $File.FullName
    $rawText = [System.IO.File]::ReadAllText($File.FullName, [System.Text.Encoding]::UTF8)
    $text = $rawText.Replace("`r`n", "`n").Replace("`r", "`n")
    $mask = Get-CodeMask $text
    $matches = [System.Collections.Generic.List[object]]::new()

    foreach ($macroMatch in [regex]::Matches($mask, "\bASSERT_TRUE\s*\(")) {
        $openIndex = $mask.IndexOf("(", $macroMatch.Index, [System.StringComparison]::Ordinal)
        $closeIndex = Get-ClosingParenthesis -Mask $mask -OpenIndex $openIndex
        if ($closeIndex -lt 0) {
            throw "Unterminated ASSERT_TRUE invocation in ${relativePath}:$(Get-LineNumber $text $macroMatch.Index)"
        }

        $arguments = @(Get-MacroArguments -Text $text -Mask $mask -OpenIndex $openIndex -CloseIndex $closeIndex)
        if ($arguments.Count -ge 2 -and (Test-UnconditionalTrue $arguments[1].Mask)) {
            $invocation = $text.Substring($macroMatch.Index, $closeIndex - $macroMatch.Index + 1)
            $matches.Add((New-SignalMatch -Rule "unconditional-assert-true" -Path $relativePath `
                        -Line (Get-LineNumber $text $macroMatch.Index) -Text $invocation)) | Out-Null
        }
    }

    $outputPattern = '(?<![\w:])std::(?:cout|cerr|clog)\b|(?<![\w:])(?:std::)?(?:printf|fprintf|puts)\s*\('
    $seenOutputStatements = @{}
    foreach ($outputMatch in [regex]::Matches($mask, $outputPattern)) {
        $statementEnd = Get-StatementEnd -Mask $mask -StartIndex $outputMatch.Index
        if ($statementEnd -lt 0) {
            continue
        }
        $statement = $text.Substring($outputMatch.Index, $statementEnd - $outputMatch.Index + 1)
        if (-not (Test-ContainsSkipLiteral $statement)) {
            continue
        }

        $identity = "$($outputMatch.Index)|$statementEnd"
        if ($seenOutputStatements.ContainsKey($identity)) {
            continue
        }
        $seenOutputStatements[$identity] = $true
        $matches.Add((New-SignalMatch -Rule "skip-output-as-success" -Path $relativePath `
                    -Line (Get-LineNumber $text $outputMatch.Index) -Text $statement)) | Out-Null
    }

    return @($matches)
}

function Get-MatchKey {
    param(
        [string]$Rule,
        [string]$Path,
        [int]$Line,
        [string]$TextHash
    )

    return ("{0}|{1}|{2}|{3}" -f $Rule, (Get-NormalizedPath $Path).ToLowerInvariant(), $Line,
        $TextHash.ToLowerInvariant())
}

function Get-AllowlistIdentity {
    param(
        [string]$Rule,
        [string]$Path,
        [int]$Line
    )

    return ("{0}|{1}|{2}" -f $Rule, (Get-NormalizedPath $Path).ToLowerInvariant(), $Line)
}

$resolvedRoot = (Resolve-Path -LiteralPath $Root).Path
$Root = $resolvedRoot
$unitRoot = Join-Path $Root "tests\unit"
if (-not (Test-Path -LiteralPath $unitRoot -PathType Container)) {
    throw "Missing test source root: tests\unit"
}

$sourceFiles = @(Get-ChildItem -LiteralPath $unitRoot -Recurse -File -Filter "*.cpp" | Sort-Object FullName)
$allMatches = [System.Collections.Generic.List[object]]::new()
foreach ($file in $sourceFiles) {
    foreach ($match in @(Get-MatchesForFile $file)) {
        $allMatches.Add($match) | Out-Null
    }
}

if ($ListMatches) {
    foreach ($match in @($allMatches | Sort-Object Rule, Path, Line)) {
        $singleLineText = [regex]::Replace($match.Text, "\s+", " ")
        Write-Host ("[MATCH] {0} {1}:{2} [{3}] {4}" -f $match.Rule, $match.Path, $match.Line,
            $match.TextHash, $singleLineText)
    }
}

$resolvedAllowlistPath = Resolve-RepoPath $AllowlistPath
if (-not (Test-Path -LiteralPath $resolvedAllowlistPath -PathType Leaf)) {
    throw "Missing test signal allowlist: $AllowlistPath"
}

try {
    $allowlist = [System.IO.File]::ReadAllText($resolvedAllowlistPath, [System.Text.Encoding]::UTF8) | ConvertFrom-Json
} catch {
    throw "Invalid test signal allowlist JSON: $($_.Exception.Message)"
}

$failures = [System.Collections.Generic.List[string]]::new()
if ($allowlist.schemaVersion -ne 1) {
    $failures.Add("Unsupported test signal allowlist schemaVersion: $($allowlist.schemaVersion)") | Out-Null
}

$allowedRules = @("unconditional-assert-true", "skip-output-as-success")
$allowlistByKey = @{}
$allowlistIdentities = @{}
$allowlistEntries = @($allowlist.entries)
$today = (Get-Date).Date

foreach ($entry in $allowlistEntries) {
    $rule = [string]$entry.rule
    $path = Get-NormalizedPath ([string]$entry.path)
    $line = [int]$entry.line
    $textHash = ([string]$entry.textHash).ToLowerInvariant()
    $rationale = [string]$entry.rationale
    $expiresOn = [string]$entry.expiresOn

    $entryValid = $true
    if ($rule -notin $allowedRules) {
        $failures.Add("Unknown allowlist rule '$rule' at ${path}:$line") | Out-Null
        $entryValid = $false
    }
    if ($path -notmatch "^tests\\unit\\.+\.cpp$" -or $path -match "[*?\[\]]" -or
        [System.IO.Path]::IsPathRooted($path) -or $path.Split("\") -contains "..") {
        $failures.Add("Allowlist path must name one exact tests/unit C++ file: '$path'") | Out-Null
        $entryValid = $false
    }
    if ($line -le 0) {
        $failures.Add("Allowlist line must be positive for '$path'") | Out-Null
        $entryValid = $false
    }
    if ($textHash -notmatch "^[0-9a-f]{64}$") {
        $failures.Add("Allowlist textHash must be lowercase SHA-256 for ${path}:$line") | Out-Null
        $entryValid = $false
    }
    if ([string]::IsNullOrWhiteSpace($rationale) -or $rationale.Trim().Length -lt 16) {
        $failures.Add("Allowlist rationale must be specific and auditable for ${path}:$line") | Out-Null
        $entryValid = $false
    }

    $expiration = [datetime]::MinValue
    if (-not [datetime]::TryParseExact($expiresOn, "yyyy-MM-dd",
            [System.Globalization.CultureInfo]::InvariantCulture,
            [System.Globalization.DateTimeStyles]::None, [ref]$expiration)) {
        $failures.Add("Allowlist expiresOn must use YYYY-MM-DD for ${path}:$line") | Out-Null
        $entryValid = $false
    } elseif ($expiration.Date -lt $today) {
        $failures.Add("Allowlist entry expired on $expiresOn for $rule ${path}:$line") | Out-Null
    }

    if (-not $entryValid) {
        continue
    }

    $identity = Get-AllowlistIdentity -Rule $rule -Path $path -Line $line
    if ($allowlistIdentities.ContainsKey($identity)) {
        $failures.Add("Duplicate test signal allowlist entry: $rule ${path}:$line") | Out-Null
        continue
    }
    $allowlistIdentities[$identity] = $true

    $key = Get-MatchKey -Rule $rule -Path $path -Line $line -TextHash $textHash
    $allowlistByKey[$key] = $entry
}

$currentMatchKeys = @{}
foreach ($match in $allMatches) {
    $key = Get-MatchKey -Rule $match.Rule -Path $match.Path -Line $match.Line -TextHash $match.TextHash
    $currentMatchKeys[$key] = $true
    if (-not $allowlistByKey.ContainsKey($key)) {
        $singleLineText = [regex]::Replace($match.Text, "\s+", " ")
        $failures.Add("Unregistered $($match.Rule) at $($match.Path):$($match.Line): $singleLineText") | Out-Null
    }
}

foreach ($key in $allowlistByKey.Keys) {
    if (-not $currentMatchKeys.ContainsKey($key)) {
        $entry = $allowlistByKey[$key]
        $failures.Add(
            "Stale or text-changed allowlist entry: $($entry.rule) $($entry.path):$($entry.line) [$($entry.textHash)]"
        ) | Out-Null
    }
}

if ($failures.Count -gt 0) {
    Write-Host "Test signal integrity failures:"
    foreach ($failure in $failures) {
        Write-Host "  $failure"
    }
    throw "Test signal integrity check failed with $($failures.Count) issue(s)"
}

Write-Host ("[OK] Test signal integrity passed: {0} C++ files, {1} exact allowlist entries, no unexpected skip markers" `
        -f $sourceFiles.Count, $allowlistEntries.Count)
