param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

$legacyFields = @(
    "kind",
    "immediate",
    "access",
    "reg",
    "constIndex",
    "aux",
    "instructionPc",
    "boolValue",
    "numberValue",
    "ownsRegister",
    "isMultiResult",
    "isSingleValue"
)

$allowedSuppressionFiles = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
@(
    "src/compiler/codegen/codegen_types.hpp",
    "src/common/diagnostics.hpp"
) | ForEach-Object { [void]$allowedSuppressionFiles.Add($_) }

$allowedProbeFiles = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
@(
    "src/compiler/codegen/codegen_types.hpp",
    "tests/unit/compiler/test_codegen_result_types.cpp",
    "tests/unit/compiler/test_expression_emitter.cpp"
) | ForEach-Object { [void]$allowedProbeFiles.Add($_) }

$fieldPattern = ($legacyFields | ForEach-Object { [regex]::Escape($_) }) -join "|"
$declarationPattern = '\b(?:const\s+)?ValueResult\s*(?:[&*]\s*)?([A-Za-z_]\w*)\b'

function Get-RelativeRepoPath {
    param([string]$Path)
    $rootPath = (Resolve-Path -LiteralPath $Root).Path.TrimEnd("\", "/") + [System.IO.Path]::DirectorySeparatorChar
    $filePath = (Resolve-Path -LiteralPath $Path).Path
    $rootUri = [System.Uri]::new($rootPath)
    $fileUri = [System.Uri]::new($filePath)
    return [System.Uri]::UnescapeDataString($rootUri.MakeRelativeUri($fileUri).ToString())
}

function Get-CxxFiles {
    foreach ($dir in @("src", "tests")) {
        $path = Join-Path $Root $dir
        if (-not (Test-Path -LiteralPath $path)) {
            continue
        }

        Get-ChildItem -LiteralPath $path -Recurse -File |
            Where-Object { $_.Extension -in @(".cpp", ".hpp", ".h") }
    }
}

function Count-Char {
    param(
        [string]$Text,
        [char]$Char
    )

    $count = 0
    foreach ($c in $Text.ToCharArray()) {
        if ($c -eq $Char) {
            $count++
        }
    }
    return $count
}

$violations = New-Object System.Collections.Generic.List[string]
$unexpectedSuppressions = New-Object System.Collections.Generic.List[string]
$legacyAccessCount = 0

foreach ($file in Get-CxxFiles) {
    $relative = Get-RelativeRepoPath $file.FullName
    $lines = Get-Content -LiteralPath $file.FullName
    $tracked = New-Object System.Collections.Generic.List[object]
    $suppressionDepth = 0
    $braceDepth = 0

    for ($index = 0; $index -lt $lines.Count; $index++) {
        $line = $lines[$index]
        $lineNo = $index + 1

        if ($line -match '\bLUA_SUPPRESS_DEPRECATED_DECLARATIONS_BEGIN\b') {
            $suppressionDepth++
            if (-not $allowedSuppressionFiles.Contains($relative)) {
                $unexpectedSuppressions.Add("${relative}:${lineNo}: unexpected deprecation suppression fence")
            }
        }

        if (($line -match '\bValueResultLegacyMirrorProbe\b') -and (-not $allowedProbeFiles.Contains($relative))) {
            $violations.Add("${relative}:${lineNo}: ValueResultLegacyMirrorProbe is only for legacy drift characterization")
        }

        foreach ($match in [regex]::Matches($line, $declarationPattern)) {
            $name = $match.Groups[1].Value
            if ($name -eq "Visitor") {
                continue
            }

            $declDepth = $braceDepth
            if (($line -match '\{') -and ($line -notmatch ';')) {
                $declDepth = $braceDepth + 1
            }

            $tracked.Add([pscustomobject]@{
                Name = $name
                Depth = $declDepth
            })
        }

        for ($entryIndex = 0; $entryIndex -lt $tracked.Count; $entryIndex++) {
            $entry = $tracked[$entryIndex]
            $accessPattern = "\b$([regex]::Escape($entry.Name))\s*\.\s*($fieldPattern)\b"
            foreach ($access in [regex]::Matches($line, $accessPattern)) {
                $legacyAccessCount++
                if ($suppressionDepth -eq 0) {
                    $field = $access.Groups[1].Value
                    $violations.Add("${relative}:${lineNo}: ValueResult legacy field '$field' must use payload()/visit() or legacyFields()")
                }
            }
        }

        if ($line -match '\bLUA_SUPPRESS_DEPRECATED_DECLARATIONS_END\b') {
            if ($suppressionDepth -eq 0) {
                $violations.Add("${relative}:${lineNo}: suppression fence ended without a matching begin")
            } else {
                $suppressionDepth--
            }
        }

        $braceDepth += (Count-Char $line '{')
        $braceDepth -= (Count-Char $line '}')
        if ($braceDepth -lt 0) {
            $braceDepth = 0
        }

        for ($i = $tracked.Count - 1; $i -ge 0; $i--) {
            if ($tracked[$i].Depth -gt $braceDepth) {
                $tracked.RemoveAt($i)
            }
        }
    }

    if ($suppressionDepth -ne 0) {
        $violations.Add("${relative}: suppression fence was not closed")
    }
}

if ($unexpectedSuppressions.Count -gt 0) {
    $unexpectedSuppressions | ForEach-Object { Write-Error $_ }
    throw "Unexpected deprecation suppression fences found"
}

if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Error $_ }
    throw "ValueResult legacy field access check failed"
}

Write-Host "[OK] ValueResult legacy field fence holds ($legacyAccessCount allowed fenced accesses)"
