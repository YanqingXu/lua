param(
    [string]$Root = "",
    [string]$OutputDir = "docs\ai\generated",
    [int]$ChunkLineCount = 80,
    [int]$OverlapLineCount = 12,
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

if ($ChunkLineCount -le 0) {
    throw "ChunkLineCount must be positive."
}

if ($OverlapLineCount -lt 0 -or $OverlapLineCount -ge $ChunkLineCount) {
    throw "OverlapLineCount must be non-negative and smaller than ChunkLineCount."
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

function Normalize-RepoPath {
    param([string]$Path)
    return (($Path -replace "\\", "/").TrimStart("/"))
}

$HighWeightFiles = @{
    "README.md" = 4.0
    "docs/vm/instruction-set.md" = 4.0
    "tools/check_doc_drift.ps1" = 5.0
    "docs/guides/development.md" = 2.5
    "docs/compiler/codegen-responsibility-map.md" = 2.0
    "docs/architecture/runtime-services.md" = 2.0
    "docs/architecture/gc.md" = 2.0
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

function Get-FileBoost {
    param([string]$RelativePath)

    $normalized = Normalize-RepoPath $RelativePath
    if ($HighWeightFiles.ContainsKey($normalized)) {
        return [double]$HighWeightFiles[$normalized]
    }

    return 1.0
}

function Get-RetrievalHints {
    param([string]$RelativePath)

    $normalized = Normalize-RepoPath $RelativePath
    switch ($normalized) {
        "README.md" {
            return "current project facts build tests compatibility quality gate check_doc_drift"
        }
        "docs/vm/instruction-set.md" {
            return "vm instruction set opcode opcodes 38 semantics dispatch bytecode"
        }
        "tools/check_doc_drift.ps1" {
            return "document drift docs drift test counts front matter yaml status last_checked applies_to"
        }
        "docs/guides/development.md" {
            return "development guide code review cpp23 c++23 RAII variant quality gate"
        }
        default {
            return ""
        }
    }
}

function Remove-YamlFrontMatter {
    param([string]$Text)

    if ($Text -notmatch "^\s*---\r?\n") {
        return $Text
    }

    $match = [regex]::Match($Text, "(?ms)^\s*---\r?\n.*?\r?\n---\r?\n")
    if (-not $match.Success) {
        return $Text
    }

    return $Text.Substring($match.Length)
}

function Get-FileKind {
    param([string]$RelativePath)

    if ($RelativePath.StartsWith("src/", [System.StringComparison]::OrdinalIgnoreCase)) {
        return "source"
    }
    if ($RelativePath.StartsWith("tools/", [System.StringComparison]::OrdinalIgnoreCase)) {
        return "tool"
    }
    return "doc"
}

function Get-Language {
    param([string]$RelativePath)

    $extension = [System.IO.Path]::GetExtension($RelativePath).ToLowerInvariant()
    switch ($extension) {
        ".md" { return "markdown" }
        ".ps1" { return "powershell" }
        ".h" { return "cpp" }
        ".hpp" { return "cpp" }
        ".cpp" { return "cpp" }
        default { return $extension.TrimStart(".") }
    }
}

function Get-Tags {
    param([string]$RelativePath)

    $tags = [System.Collections.Generic.List[string]]::new()
    if ($RelativePath -match "compiler|codegen|parser|lexer|opcode") { $tags.Add("compiler") | Out-Null }
    if ($RelativePath -match "(^|/)vm(/|-)") { $tags.Add("vm") | Out-Null }
    if ($RelativePath -match "(^|/)gc(/|-)|garbage") { $tags.Add("gc") | Out-Null }
    if ($RelativePath -match "runtime|core|stdlib|lib/|lib\\|value|table|function|upvalue") { $tags.Add("runtime") | Out-Null }
    if ($RelativePath -match "docs2") { $tags.Add("chinese-docs") | Out-Null }
    if ($RelativePath -match "docs/") { $tags.Add("documentation") | Out-Null }
    if ($RelativePath -match "roadmap|status|drift|quality") { $tags.Add("workflow") | Out-Null }

    if ($tags.Count -eq 0) {
        $tags.Add("repository") | Out-Null
    }

    return @($tags)
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
        [double]$Boost,
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
        $vector[$entry.Key] = [Math]::Round(([double]$entry.Value / $norm) * $Boost, 6)
    }

    return $vector
}

function Get-ChunkTitle {
    param(
        [string[]]$Lines,
        [string]$RelativePath
    )

    foreach ($line in $Lines) {
        if ($line -match "^\s{0,3}#{1,6}\s+(.+?)\s*$") {
            return $matches[1]
        }
    }

    return [System.IO.Path]::GetFileName($RelativePath)
}

$candidateFiles = [System.Collections.Generic.List[System.IO.FileInfo]]::new()

foreach ($rootName in @("src")) {
    $path = Join-RepoPath $rootName
    if (Test-Path -LiteralPath $path) {
        Get-ChildItem -LiteralPath $path -Recurse -File |
            Where-Object { $_.Extension.ToLowerInvariant() -in @(".cpp", ".hpp", ".h") } |
            ForEach-Object { $candidateFiles.Add($_) | Out-Null }
    }
}

foreach ($rootName in @("docs", "docs2")) {
    $path = Join-RepoPath $rootName
    if (Test-Path -LiteralPath $path) {
        Get-ChildItem -LiteralPath $path -Recurse -File -Filter "*.md" |
            Where-Object { (Convert-ToRepoPath $_.FullName) -notlike "docs/ai/generated/*" } |
            ForEach-Object { $candidateFiles.Add($_) | Out-Null }
    }
}

foreach ($toolPath in @(
    "tools/check_doc_drift.ps1",
    "tools/run_quality_gate.ps1",
    "tools/test_quality_gate.ps1",
    "tools/check_opcode_coverage_matrix.ps1",
    "tools/check_value_result_variant_only.ps1",
    "tools/add_source.ps1"
)) {
    $path = Join-RepoPath $toolPath
    if (Test-Path -LiteralPath $path) {
        $candidateFiles.Add((Get-Item -LiteralPath $path)) | Out-Null
    }
}

$filesByPath = @{}
foreach ($file in $candidateFiles) {
    $filesByPath[$file.FullName] = $file
}
$files = @($filesByPath.Values | Sort-Object -Property FullName)

$absoluteOutputDir = Join-RepoPath $OutputDir
[System.IO.Directory]::CreateDirectory($absoluteOutputDir) | Out-Null

$indexPath = Join-Path $absoluteOutputDir "rag-index.jsonl"
$manifestPath = Join-Path $absoluteOutputDir "rag-manifest.json"
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
$writer = [System.IO.StreamWriter]::new($indexPath, $false, $utf8NoBom)

$chunkCount = 0
$step = $ChunkLineCount - $OverlapLineCount
$fileSummaries = [System.Collections.Generic.List[object]]::new()

try {
    foreach ($file in $files) {
        $relativePath = Convert-ToRepoPath $file.FullName
        $boost = Get-FileBoost $relativePath
        $lines = [System.IO.File]::ReadAllLines($file.FullName, $utf8NoBom)
        if ($lines.Count -eq 0) {
            $lines = @("")
        }

        $fileChunkCount = 0
        for ($start = 0; $start -lt $lines.Count; $start += $step) {
            $end = [Math]::Min($start + $ChunkLineCount - 1, $lines.Count - 1)
            $chunkLines = [System.Collections.Generic.List[string]]::new()
            for ($i = $start; $i -le $end; $i++) {
                $chunkLines.Add($lines[$i]) | Out-Null
            }

            $text = ($chunkLines -join "`n").Trim()
            if ([string]::IsNullOrWhiteSpace($text)) {
                continue
            }

            $title = Get-ChunkTitle -Lines @($chunkLines) -RelativePath $relativePath
            $retrievalHints = Get-RetrievalHints $relativePath
            $vectorBody = if ((Get-Language $relativePath) -eq "markdown") { Remove-YamlFrontMatter $text } else { $text }
            $indexedText = @($relativePath, $title, $title, $retrievalHints, $vectorBody) -join "`n"
            $vector = ConvertTo-SparseVector -Text $indexedText -Boost $boost -Limit $MaxTerms
            $chunkId = "{0}#chunk-{1:D4}" -f $relativePath, ($fileChunkCount + 1)
            $chunk = [ordered]@{
                id = $chunkId
                path = $relativePath
                kind = Get-FileKind $relativePath
                language = Get-Language $relativePath
                boost = $boost
                start_line = $start + 1
                end_line = $end + 1
                title = $title
                tags = @(Get-Tags $relativePath)
                retrieval_hints = $retrievalHints
                vector = $vector
                text = $text
            }

            $writer.WriteLine(($chunk | ConvertTo-Json -Depth 20 -Compress))
            $chunkCount += 1
            $fileChunkCount += 1
        }

        $fileSummaries.Add([ordered]@{
            path = $relativePath
            chunks = $fileChunkCount
            boost = $boost
            kind = Get-FileKind $relativePath
        }) | Out-Null
    }
} finally {
    $writer.Dispose()
}

$manifest = [ordered]@{
    generated_at = (Get-Date).ToString("yyyy-MM-ddTHH:mm:ssK")
    generator = "tools/build_rag_index.ps1"
    vector_model = "local-sparse-token-cjk-bigram-v1"
    root = [System.IO.Path]::GetFullPath($Root)
    output_index = Convert-ToRepoPath $indexPath
    output_manifest = Convert-ToRepoPath $manifestPath
    source_roots = @("src", "docs", "docs2")
    included_tools = @(
        "tools/check_doc_drift.ps1",
        "tools/run_quality_gate.ps1",
        "tools/test_quality_gate.ps1",
        "tools/check_opcode_coverage_matrix.ps1",
        "tools/check_value_result_variant_only.ps1",
        "tools/add_source.ps1"
    )
    chunk_line_count = $ChunkLineCount
    overlap_line_count = $OverlapLineCount
    max_terms = $MaxTerms
    file_count = $files.Count
    chunk_count = $chunkCount
    high_weight_files = $HighWeightFiles.GetEnumerator() |
        Sort-Object -Property Key |
        ForEach-Object { [ordered]@{ path = $_.Key; boost = $_.Value } }
    files = @($fileSummaries)
}

[System.IO.File]::WriteAllText($manifestPath, ($manifest | ConvertTo-Json -Depth 20), $utf8NoBom)

Write-Host "[OK] RAG index generated: $(Convert-ToRepoPath $indexPath)"
Write-Host "[OK] RAG manifest generated: $(Convert-ToRepoPath $manifestPath)"
Write-Host "[OK] Indexed $($files.Count) files into $chunkCount chunks"
