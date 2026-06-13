param(
    [Parameter(Mandatory = $true)]
    [string]$Query,
    [string]$Root = "",
    [string]$IndexPath = "docs\ai\generated\rag-index.jsonl",
    [int]$TopK = 8,
    [int]$MaxTerms = 200
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($Root)) {
    $scriptDir = if (-not [string]::IsNullOrWhiteSpace($PSScriptRoot)) {
        $PSScriptRoot
    } else {
        Split-Path -Parent $MyInvocation.MyCommand.Path
    }
    $Root = (Resolve-Path (Join-Path $scriptDir "..")).Path
}

function Join-RepoPath {
    param([string]$RelativePath)
    return Join-Path $Root $RelativePath
}

function Convert-ToRepoPath {
    param([string]$Path)

    $fullRoot = [System.IO.Path]::GetFullPath($Root).TrimEnd('\', '/')
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if ($fullPath.StartsWith($fullRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return ($fullPath.Substring($fullRoot.Length).TrimStart('\', '/') -replace "\\", "/")
    }

    return ($fullPath -replace "\\", "/")
}

$StopWords = @{
    "the" = $true; "and" = $true; "for" = $true; "with" = $true; "that" = $true
    "this" = $true; "from" = $true; "into" = $true; "are" = $true; "was" = $true
    "were" = $true; "has" = $true; "have" = $true; "not" = $true; "but" = $true
    "you" = $true; "your" = $true; "will" = $true; "can" = $true; "all" = $true
    "auto" = $true; "const" = $true; "return" = $true; "void" = $true; "true" = $true
    "false" = $true; "class" = $true; "struct" = $true; "public" = $true; "private" = $true
    "include" = $true; "namespace" = $true
}

function Get-Tokens {
    param([string]$Text)

    $tokens = [System.Collections.Generic.List[string]]::new()
    $lower = $Text.ToLowerInvariant()

    foreach ($match in [regex]::Matches($lower, "[a-z_][a-z0-9_]{1,}|[0-9]{2,}")) {
        $token = $match.Value
        if ($token.Length -le 80 -and -not $StopWords.ContainsKey($token)) {
            $tokens.Add($token) | Out-Null
        }
    }

    foreach ($match in [regex]::Matches($Text, "[\u4e00-\u9fff]+")) {
        $segment = $match.Value
        if ($segment.Length -le 4) {
            $tokens.Add($segment) | Out-Null
        }
        for ($i = 0; $i -lt $segment.Length - 1; $i++) {
            $tokens.Add($segment.Substring($i, 2)) | Out-Null
        }
        for ($i = 0; $i -lt $segment.Length - 2; $i++) {
            $tokens.Add($segment.Substring($i, 3)) | Out-Null
        }
    }

    return @($tokens)
}

function ConvertTo-SparseVector {
    param(
        [string]$Text,
        [int]$Limit
    )

    $counts = @{}
    foreach ($token in Get-Tokens $Text) {
        if (-not $counts.ContainsKey($token)) {
            $counts[$token] = 0
        }
        $counts[$token] += 1
    }

    if ($counts.Count -eq 0) {
        return [ordered]@{}
    }

    $norm = 0.0
    foreach ($value in $counts.Values) {
        $norm += [double]$value * [double]$value
    }
    $norm = [Math]::Sqrt($norm)
    if ($norm -eq 0.0) {
        $norm = 1.0
    }

    $vector = [ordered]@{}
    $topTerms = $counts.GetEnumerator() |
        Sort-Object -Property @{ Expression = "Value"; Descending = $true }, @{ Expression = "Key"; Descending = $false } |
        Select-Object -First $Limit

    foreach ($entry in $topTerms) {
        $vector[$entry.Key] = [Math]::Round(([double]$entry.Value / $norm), 6)
    }

    return $vector
}

function Get-VectorValue {
    param(
        [object]$Vector,
        [string]$Term
    )

    $property = $Vector.PSObject.Properties[$Term]
    if ($null -eq $property) {
        return 0.0
    }

    return [double]$property.Value
}

function Get-Snippet {
    param([string]$Text)

    $line = (($Text -split "\r?\n") | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -First 1)
    if ($null -eq $line) {
        return ""
    }

    $trimmed = $line.Trim()
    if ($trimmed.Length -gt 160) {
        return $trimmed.Substring(0, 157) + "..."
    }
    return $trimmed
}

$absoluteIndexPath = if ([System.IO.Path]::IsPathRooted($IndexPath)) { $IndexPath } else { Join-RepoPath $IndexPath }
if (-not (Test-Path -LiteralPath $absoluteIndexPath)) {
    throw "Missing RAG index: $(Convert-ToRepoPath $absoluteIndexPath). Run tools\build_rag_index.ps1 first."
}

$queryVector = ConvertTo-SparseVector -Text $Query -Limit $MaxTerms
if ($queryVector.Count -eq 0) {
    throw "Query produced no searchable tokens."
}

$results = [System.Collections.Generic.List[object]]::new()

foreach ($line in [System.IO.File]::ReadLines($absoluteIndexPath)) {
    if ([string]::IsNullOrWhiteSpace($line)) {
        continue
    }

    $chunk = $line | ConvertFrom-Json
    $score = 0.0
    foreach ($term in $queryVector.Keys) {
        $score += [double]$queryVector[$term] * (Get-VectorValue -Vector $chunk.vector -Term $term)
    }

    if ($score -gt 0.0) {
        $weightedScore = $score * [Math]::Pow([double]$chunk.boost, 2.0)
        $results.Add([pscustomobject][ordered]@{
            score = [Math]::Round($weightedScore, 6)
            path = $chunk.path
            start_line = $chunk.start_line
            end_line = $chunk.end_line
            title = $chunk.title
            boost = $chunk.boost
            tags = ($chunk.tags -join ",")
            snippet = Get-Snippet $chunk.text
        }) | Out-Null
    }
}

$topResults = @($results | Sort-Object -Property @{ Expression = "score"; Descending = $true }, @{ Expression = "path"; Descending = $false } | Select-Object -First $TopK)

if ($topResults.Count -eq 0) {
    Write-Host "[WARN] No matching chunks"
    exit 0
}

$topResults | Format-Table -AutoSize
