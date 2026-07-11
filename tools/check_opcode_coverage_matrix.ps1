param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$CoverageContract = "tests\unit\vm\opcode_coverage_contract.json",
    [string]$TestExecutable = "bin\lua_test.exe"
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

function Resolve-RepoOrAbsolutePath {
    param([string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }
    return Join-RepoPath $Path
}

function Get-RegisteredTestIds {
    param(
        [string]$ExecutablePath,
        [System.Collections.Generic.List[string]]$Failures
    )

    $ids = @{}
    if (-not (Test-Path -LiteralPath $ExecutablePath)) {
        Add-Failure $Failures "Missing test executable for opcode coverage contract: $TestExecutable. Build lua_test.vcxproj first."
        return $ids
    }

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = & $ExecutablePath --list 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    if ($exitCode -ne 0) {
        Add-Failure $Failures "Test registry listing failed: $TestExecutable --list exited with code $exitCode"
        return $ids
    }

    foreach ($line in @($output)) {
        $id = $line.ToString().Trim()
        if ($id -match "^.+::.+$") {
            $ids[$id] = $true
        }
    }
    if ($ids.Count -eq 0) {
        Add-Failure $Failures "No Suite::Test identifiers were parsed from $TestExecutable --list"
    }
    return $ids
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

function Get-OpcodeMetadata {
    param(
        [string]$Path,
        [System.Collections.Generic.List[string]]$Failures
    )

    $text = Get-Content -LiteralPath $Path -Raw
    $metadata = @{}
    $regex = [regex]::new(
        'makeOpcodeMetadata\(\s*OpCode::(?<opcode>[A-Z][A-Z0-9_]*)\s*,\s*"(?<name>[^"]+)"\s*,.*?VM::OpcodeGroup::(?<group>[A-Za-z0-9_]+)\s*,\s*(?<mayInvokeMetamethod>true|false)\s*\)',
        [System.Text.RegularExpressions.RegexOptions]::Singleline
    )

    foreach ($match in $regex.Matches($text)) {
        $opcode = $match.Groups["opcode"].Value
        $metadata[$opcode] = [pscustomobject]@{
            Opcode = $opcode
            Name = $match.Groups["name"].Value
            Group = $match.Groups["group"].Value
            MayInvokeMetamethod = $match.Groups["mayInvokeMetamethod"].Value -eq "true"
        }
    }

    if ($metadata.Count -eq 0) {
        Add-Failure $Failures "No OpcodeMetadata rows parsed from src/compiler/opcode.hpp"
    }

    return $metadata
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

    $rows = [System.Collections.Generic.List[object]]::new()
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
        $row = [pscustomobject]@{
            Opcode = $opcode
            Group = if ($cells.Count -gt 1) { $cells[1] } else { "" }
            PositivePath = if ($cells.Count -gt 2) { $cells[2] } else { "" }
            BoundaryPath = if ($cells.Count -gt 3) { $cells[3] } else { "" }
            MetamethodPath = if ($cells.Count -gt 4) { $cells[4] } else { "" }
            Line = $lineNumber
        }
        $rows.Add($row) | Out-Null

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
$contractPath = Resolve-RepoOrAbsolutePath $CoverageContract
$testExecutablePath = Resolve-RepoOrAbsolutePath $TestExecutable

$expected = @(Get-OpcodeNames -Path $opcodePath)
if ($expected.Count -eq 0) {
    Add-Failure $failures "No opcodes parsed from src/compiler/opcode.hpp"
}

$metadataByOpcode = Get-OpcodeMetadata -Path $opcodePath -Failures $failures

$rows = @(Get-MatrixRows -Path $matrixPath -Failures $failures)
$rowNames = @($rows | ForEach-Object { $_.Opcode })
if ($rows.Count -eq 0) {
    Add-Failure $failures "No opcode rows parsed from tests/unit/vm/opcode_coverage_matrix.md"
}

$expectedSet = @{}
foreach ($opcode in $expected) {
    $expectedSet[$opcode] = $true
}

$seen = @{}
foreach ($opcode in $rowNames) {
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

foreach ($opcode in $rowNames) {
    if (-not $expectedSet.ContainsKey($opcode)) {
        Add-Failure $failures "Matrix row references unknown opcode $opcode"
    }
}

if ($rows.Count -ne $expected.Count) {
    Add-Failure $failures "Opcode matrix row count is $($rows.Count), expected $($expected.Count)"
}

if ($rows.Count -eq $expected.Count) {
    for ($index = 0; $index -lt $expected.Count; $index += 1) {
        if ($rowNames[$index] -ne $expected[$index]) {
            Add-Failure $failures "Opcode matrix order mismatch at row $($index + 1): found $($rowNames[$index]), expected $($expected[$index])"
        }
    }
}

foreach ($row in $rows) {
    if (-not $metadataByOpcode.ContainsKey($row.Opcode)) {
        continue
    }

    $metadata = $metadataByOpcode[$row.Opcode]
    if ($metadata.Name -ne $row.Opcode) {
        Add-Failure $failures "OpcodeMetadata name mismatch for $($row.Opcode): metadata name is $($metadata.Name)"
    }

    if ($row.Group -ne $metadata.Group) {
        Add-Failure $failures "${matrixPath}:$($row.Line) group mismatch for $($row.Opcode): matrix has $($row.Group), metadata has $($metadata.Group)"
    }

    $matrixMarksNotApplicable = $row.MetamethodPath -match "^N/A\b"
    if ($metadata.MayInvokeMetamethod -and $matrixMarksNotApplicable) {
        Add-Failure $failures "${matrixPath}:$($row.Line) metamethod mismatch for $($row.Opcode): metadata says mayInvokeMetamethod=true"
    }
    if (-not $metadata.MayInvokeMetamethod -and -not $matrixMarksNotApplicable) {
        Add-Failure $failures "${matrixPath}:$($row.Line) metamethod mismatch for $($row.Opcode): metadata says mayInvokeMetamethod=false"
    }
}

$contract = $null
if (-not (Test-Path -LiteralPath $contractPath)) {
    Add-Failure $failures "Missing opcode coverage contract: $CoverageContract"
} else {
    try {
        $contract = Get-Content -LiteralPath $contractPath -Raw | ConvertFrom-Json
    } catch {
        Add-Failure $failures "Invalid opcode coverage contract JSON: $($_.Exception.Message)"
    }
}

$registeredTestIds = Get-RegisteredTestIds -ExecutablePath $testExecutablePath -Failures $failures
if ($null -ne $contract) {
    if ($contract.schemaVersion -ne 1) {
        Add-Failure $failures "Unsupported opcode coverage contract schemaVersion: $($contract.schemaVersion)"
    }

    $catalogByKey = @{}
    foreach ($test in @($contract.tests)) {
        if ([string]::IsNullOrWhiteSpace($test.key) -or [string]::IsNullOrWhiteSpace($test.source) -or
            [string]::IsNullOrWhiteSpace($test.id)) {
            Add-Failure $failures "Opcode coverage test catalog entries require key/source/id"
            continue
        }
        if ($catalogByKey.ContainsKey($test.key)) {
            Add-Failure $failures "Duplicate opcode coverage test key: $($test.key)"
            continue
        }
        $catalogByKey[$test.key] = $test

        if (-not (Test-Path -LiteralPath (Join-RepoPath $test.source))) {
            Add-Failure $failures "Opcode coverage test source does not exist: $($test.source)"
        }
        if (-not $registeredTestIds.ContainsKey($test.id)) {
            Add-Failure $failures "Opcode coverage test is not registered: $($test.id)"
        }
    }

    $coverageEntries = @($contract.coverage)
    if ($coverageEntries.Count -ne $expected.Count) {
        Add-Failure $failures "Opcode coverage contract has $($coverageEntries.Count) rows, expected $($expected.Count)"
    }

    $usedTestKeys = @{}
    for ($index = 0; $index -lt $coverageEntries.Count; $index += 1) {
        $coverage = $coverageEntries[$index]
        if ($index -lt $expected.Count -and $coverage.opcode -ne $expected[$index]) {
            Add-Failure $failures "Opcode coverage contract order mismatch at row $($index + 1): found $($coverage.opcode), expected $($expected[$index])"
        }
        if (-not $metadataByOpcode.ContainsKey($coverage.opcode)) {
            Add-Failure $failures "Opcode coverage contract references unknown opcode $($coverage.opcode)"
            continue
        }

        $positive = @($coverage.positive | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        $boundary = @($coverage.boundary | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        $metamethod = @($coverage.metamethod | Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
        if ($positive.Count -eq 0) {
            Add-Failure $failures "Opcode $($coverage.opcode) has no executable positive-path test reference"
        }
        if ($boundary.Count -eq 0) {
            Add-Failure $failures "Opcode $($coverage.opcode) has no executable boundary-path test reference"
        }

        $expectsMetamethod = [bool]$metadataByOpcode[$coverage.opcode].MayInvokeMetamethod
        if ($expectsMetamethod -and $metamethod.Count -eq 0) {
            Add-Failure $failures "Opcode $($coverage.opcode) may invoke metamethods but has no executable metamethod test reference"
        }
        if (-not $expectsMetamethod -and $metamethod.Count -ne 0) {
            Add-Failure $failures "Opcode $($coverage.opcode) cannot invoke metamethods but lists metamethod test references"
        }

        foreach ($key in @($positive + $boundary + $metamethod)) {
            $usedTestKeys[$key] = $true
            if (-not $catalogByKey.ContainsKey($key)) {
                Add-Failure $failures "Opcode $($coverage.opcode) references unknown coverage test key: $key"
            }
        }

        $matrixRow = @($rows | Where-Object { $_.Opcode -eq $coverage.opcode } | Select-Object -First 1)
        if ($matrixRow.Count -eq 1) {
            foreach ($key in $positive) {
                if (-not $catalogByKey.ContainsKey($key)) { continue }
                $test = $catalogByKey[$key]
                $displayId = $test.id.Replace("::", "/")
                if ($matrixRow[0].PositivePath -notmatch [regex]::Escape($test.source) -or
                    $matrixRow[0].PositivePath -notmatch [regex]::Escape($displayId)) {
                    Add-Failure $failures "Matrix positive path for $($coverage.opcode) does not display contract test $($test.id) from $($test.source)"
                }
            }
            foreach ($key in $metamethod) {
                if (-not $catalogByKey.ContainsKey($key)) { continue }
                $test = $catalogByKey[$key]
                $displayId = $test.id.Replace("::", "/")
                if ($matrixRow[0].MetamethodPath -notmatch [regex]::Escape($test.source) -or
                    $matrixRow[0].MetamethodPath -notmatch [regex]::Escape($displayId)) {
                    Add-Failure $failures "Matrix metamethod path for $($coverage.opcode) does not display contract test $($test.id) from $($test.source)"
                }
            }
        }
    }

    foreach ($key in $catalogByKey.Keys) {
        if (-not $usedTestKeys.ContainsKey($key)) {
            Add-Failure $failures "Opcode coverage test catalog entry is unused: $key"
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

Write-Host "[OK] Opcode coverage matrix covers $($expected.Count) opcodes and all referenced tests are registered"
