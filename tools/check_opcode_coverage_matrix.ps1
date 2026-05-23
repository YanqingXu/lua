param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

function Join-RepoPath {
    param([string]$RelativePath)
    return Join-Path $Root $RelativePath
}

function Add-Failure {
    param(
        [System.Collections.Generic.List[string]]$Failures,
        [string]$Message
    )
    $Failures.Add($Message) | Out-Null
}

function Get-OpcodeNames {
    param([string]$Path)

    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Missing opcode header: $Path"
    }

    $text = Get-Content -LiteralPath $Path -Raw
    $match = [regex]::Match(
        $text,
        'enum\s+class\s+OpCode\s*:\s*u8\s*\{(?<body>.*?)\};',
        [System.Text.RegularExpressions.RegexOptions]::Singleline
    )
    if (-not $match.Success) {
        throw "Could not find enum class OpCode in $Path"
    }

    $names = [System.Collections.Generic.List[string]]::new()
    foreach ($line in ($match.Groups["body"].Value -split "\r?\n")) {
        $clean = ($line -replace "//.*$", "").Trim()
        if ([string]::IsNullOrWhiteSpace($clean)) {
            continue
        }

        $nameMatch = [regex]::Match($clean, '^([A-Z][A-Z0-9_]*)\s*,?$')
        if ($nameMatch.Success) {
            $names.Add($nameMatch.Groups[1].Value) | Out-Null
        }
    }

    return @($names)
}

function Get-MatrixRows {
    param(
        [string]$Path,
        [System.Collections.Generic.List[string]]$Failures
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        Add-Failure $Failures "Missing opcode coverage matrix: tests/unit/vm/opcode_coverage_matrix.md"
        return @()
    }

    $rows = [System.Collections.Generic.List[string]]::new()
    $lineNumber = 0
    foreach ($line in (Get-Content -LiteralPath $Path)) {
        $lineNumber += 1
        $trimmed = $line.Trim()
        if (-not $trimmed.StartsWith("|")) {
            continue
        }

        $cells = @($trimmed.Trim("|") -split "\|" | ForEach-Object { $_.Trim() })
        if ($cells.Count -eq 0 -or $cells[0] -eq "Opcode" -or $cells[0] -notmatch "^[A-Z][A-Z0-9_]*$") {
            continue
        }

        $opcode = $cells[0]
        $rows.Add($opcode) | Out-Null

        if ($cells.Count -lt 6) {
            Add-Failure $Failures "${Path}:$lineNumber row for $opcode must have at least 6 columns"
            continue
        }

        for ($index = 0; $index -lt 6; $index += 1) {
            if ([string]::IsNullOrWhiteSpace($cells[$index])) {
                Add-Failure $Failures "${Path}:$lineNumber row for $opcode has an empty required column"
            }
        }
    }

    return @($rows)
}

$failures = [System.Collections.Generic.List[string]]::new()
$opcodePath = Join-RepoPath "src\compiler\opcode.hpp"
$matrixPath = Join-RepoPath "tests\unit\vm\opcode_coverage_matrix.md"

$expected = @(Get-OpcodeNames -Path $opcodePath)
if ($expected.Count -eq 0) {
    Add-Failure $failures "No opcodes parsed from src/compiler/opcode.hpp"
}

$rows = @(Get-MatrixRows -Path $matrixPath -Failures $failures)
if ($rows.Count -eq 0) {
    Add-Failure $failures "No opcode rows parsed from tests/unit/vm/opcode_coverage_matrix.md"
}

$expectedSet = @{}
foreach ($opcode in $expected) {
    $expectedSet[$opcode] = $true
}

$seen = @{}
foreach ($opcode in $rows) {
    if (-not $seen.ContainsKey($opcode)) {
        $seen[$opcode] = 0
    }
    $seen[$opcode] += 1
}

foreach ($entry in $seen.GetEnumerator()) {
    if ($entry.Value -ne 1) {
        Add-Failure $failures "Duplicate matrix row for opcode $($entry.Key)"
    }
}

foreach ($opcode in $expected) {
    if (-not $seen.ContainsKey($opcode)) {
        Add-Failure $failures "Missing matrix row for opcode $opcode"
    }
}

foreach ($opcode in $rows) {
    if (-not $expectedSet.ContainsKey($opcode)) {
        Add-Failure $failures "Matrix row references unknown opcode $opcode"
    }
}

if ($rows.Count -ne $expected.Count) {
    Add-Failure $failures "Opcode matrix row count is $($rows.Count), expected $($expected.Count)"
}

if ($rows.Count -eq $expected.Count) {
    for ($index = 0; $index -lt $expected.Count; $index += 1) {
        if ($rows[$index] -ne $expected[$index]) {
            Add-Failure $failures "Opcode matrix order mismatch at row $($index + 1): found $($rows[$index]), expected $($expected[$index])"
        }
    }
}

if ($failures.Count -gt 0) {
    Write-Host "[FAIL] Opcode coverage matrix check failed:"
    foreach ($failure in $failures) {
        Write-Host " - $failure"
    }
    exit 1
}

Write-Host "[OK] Opcode coverage matrix covers $($expected.Count) opcodes"
