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

function Get-NormalizedRepoPath {
    param([string]$Path)

    $resolvedRoot = (Resolve-Path -LiteralPath $Root).Path.TrimEnd("\", "/")
    $resolvedPath = (Resolve-Path -LiteralPath $Path).Path
    $prefix = $resolvedRoot + [System.IO.Path]::DirectorySeparatorChar
    if ($resolvedPath.StartsWith($prefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $resolvedPath.Substring($prefix.Length).Replace("\", "/")
    }
    return $resolvedPath.Replace("\", "/")
}

function Find-CppClosingBrace {
    param(
        [string]$Text,
        [int]$OpeningBrace
    )

    $depth = 0
    $state = "code"
    $escaped = $false
    for ($index = $OpeningBrace; $index -lt $Text.Length; $index += 1) {
        $character = $Text[$index]
        $next = if ($index + 1 -lt $Text.Length) { $Text[$index + 1] } else { [char]0 }

        if ($state -eq "line-comment") {
            if ($character -eq "`n") { $state = "code" }
            continue
        }
        if ($state -eq "block-comment") {
            if ($character -eq "*" -and $next -eq "/") {
                $state = "code"
                $index += 1
            }
            continue
        }
        if ($state -eq "string" -or $state -eq "character") {
            if ($escaped) {
                $escaped = $false
                continue
            }
            if ($character -eq "\") {
                $escaped = $true
                continue
            }
            if (($state -eq "string" -and $character -eq '"') -or
                ($state -eq "character" -and $character -eq "'")) {
                $state = "code"
            }
            continue
        }

        if ($character -eq "/" -and $next -eq "/") {
            $state = "line-comment"
            $index += 1
            continue
        }
        if ($character -eq "/" -and $next -eq "*") {
            $state = "block-comment"
            $index += 1
            continue
        }
        if ($character -eq '"') {
            $state = "string"
            continue
        }
        if ($character -eq "'") {
            $state = "character"
            continue
        }
        if ($character -eq "{") {
            $depth += 1
        } elseif ($character -eq "}") {
            $depth -= 1
            if ($depth -eq 0) {
                return $index
            }
        }
    }

    return -1
}

function Get-CppFunctionEvidence {
    param(
        [string]$Path,
        [string]$Function,
        [string]$SignatureContains,
        [System.Collections.Generic.List[string]]$Failures,
        [string]$Context
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        Add-Failure $Failures "$Context source does not exist: $Path"
        return $null
    }

    $text = Get-Content -LiteralPath $Path -Raw
    $escapedFunction = [regex]::Escape($Function)
    $pattern = "(?m)^[\t ]*[^\r\n;{}]*$escapedFunction\s*\([^\r\n;{}]*\)[^\r\n;{}]*\{"
    $candidates = @([regex]::Matches($text, $pattern))
    if (-not [string]::IsNullOrWhiteSpace($SignatureContains)) {
        $candidates = @($candidates | Where-Object { $_.Value.Contains($SignatureContains) })
    }

    if ($candidates.Count -ne 1) {
        $qualifier = if ([string]::IsNullOrWhiteSpace($SignatureContains)) { "" } else { " containing '$SignatureContains'" }
        Add-Failure $Failures "$Context must resolve exactly one function '$Function'$qualifier in $Path; found $($candidates.Count)"
        return $null
    }

    $match = $candidates[0]
    $relativeOpening = $match.Value.LastIndexOf("{")
    $opening = $match.Index + $relativeOpening
    $closing = Find-CppClosingBrace -Text $text -OpeningBrace $opening
    if ($closing -lt $opening) {
        Add-Failure $Failures "$Context function '$Function' has no balanced body in $Path"
        return $null
    }

    return [pscustomobject]@{
        Signature = $match.Value.Substring(0, $relativeOpening).Trim()
        Body = $text.Substring($opening, $closing - $opening + 1)
    }
}

function Get-CodegenSourceFiles {
    param(
        [string]$ScanRoot,
        [System.Collections.Generic.List[string]]$Failures,
        [string]$Context
    )

    if ([string]::IsNullOrWhiteSpace($ScanRoot)) {
        Add-Failure $Failures "$Context requires scanRoot"
        return @()
    }

    $normalizedRoot = $ScanRoot.Replace("\", "/").TrimEnd("/")
    if ($normalizedRoot -ne "src/compiler/codegen") {
        Add-Failure $Failures "$Context must scan the complete src/compiler/codegen tree: $ScanRoot"
        return @()
    }

    $rootPath = Join-RepoPath $ScanRoot
    if (-not (Test-Path -LiteralPath $rootPath -PathType Container)) {
        Add-Failure $Failures "$Context scanRoot does not exist: $ScanRoot"
        return @()
    }

    return @(Get-ChildItem -LiteralPath $rootPath -Recurse -File | Where-Object {
        $_.Extension -in @(".cpp", ".hpp", ".cc", ".hh")
    })
}

function Get-PatternOccurrences {
    param(
        [System.IO.FileInfo[]]$Files,
        [string]$Pattern
    )

    $occurrences = [System.Collections.Generic.List[object]]::new()
    $regex = [regex]::new($Pattern)
    foreach ($file in $Files) {
        $text = Get-Content -LiteralPath $file.FullName -Raw
        foreach ($match in $regex.Matches($text)) {
            $occurrences.Add([pscustomobject]@{
                Source = Get-NormalizedRepoPath $file.FullName
                Index = $match.Index
                Value = $match.Value
            }) | Out-Null
        }
    }
    return @($occurrences)
}

function Test-IntentionallyNotProducedEvidence {
    param(
        [string]$Opcode,
        [pscustomobject]$Producer,
        [hashtable]$CatalogByKey,
        [hashtable]$UsedTestKeys,
        [System.Collections.Generic.List[string]]$Failures
    )

    $context = "Opcode $Opcode intentionally-not-produced evidence"
    if ([string]::IsNullOrWhiteSpace($Producer.reason) -or [string]::IsNullOrWhiteSpace($Producer.test) -or
        [string]::IsNullOrWhiteSpace($Producer.testFunction)) {
        Add-Failure $Failures "$context requires reason/test/testFunction"
        return
    }

    $files = @(Get-CodegenSourceFiles -ScanRoot $Producer.scanRoot -Failures $Failures -Context $context)
    if ($files.Count -eq 0) { return }

    $opcodePattern = "\bOpCode::" + [regex]::Escape($Opcode) + "\b"
    $actualReferences = @(Get-PatternOccurrences -Files $files -Pattern $opcodePattern)
    $allowedReferences = @($Producer.allowedReferences)
    $expectedReferenceCount = 0

    foreach ($allowed in $allowedReferences) {
        if ([string]::IsNullOrWhiteSpace($allowed.source) -or [string]::IsNullOrWhiteSpace($allowed.function) -or
            [string]::IsNullOrWhiteSpace($allowed.normalizesTo) -or $null -eq $allowed.count -or
            [int]$allowed.count -lt 1) {
            Add-Failure $Failures "$context allowedReferences entries require source/function/count >= 1/normalizesTo"
            continue
        }

        $normalizedSource = ([string]$allowed.source).Replace("\", "/")
        $expectedReferenceCount += [int]$allowed.count
        $sourceOccurrences = @($actualReferences | Where-Object { $_.Source -eq $normalizedSource })
        if ($sourceOccurrences.Count -ne [int]$allowed.count) {
            Add-Failure $Failures "$context expected $($allowed.count) OpCode::$Opcode reference(s) in $normalizedSource; found $($sourceOccurrences.Count)"
        }

        $functionEvidence = Get-CppFunctionEvidence -Path (Join-RepoPath $allowed.source) `
            -Function $allowed.function -SignatureContains $allowed.signatureContains -Failures $Failures -Context $context
        if ($null -eq $functionEvidence) { continue }
        $functionReferenceCount = [regex]::Matches($functionEvidence.Body, $opcodePattern).Count
        if ($functionReferenceCount -ne [int]$allowed.count) {
            Add-Failure $Failures "$context expected all $($allowed.count) reference(s) inside $($allowed.function); found $functionReferenceCount"
        }

        $target = [regex]::Escape([string]$allowed.normalizesTo)
        $normalizationPattern = "(?s)if\s*\([^)]*OpCode::$Opcode[^)]*\)\s*\{[^}]*\bop\s*=\s*OpCode::$target\b"
        if ($functionEvidence.Body -notmatch $normalizationPattern) {
            Add-Failure $Failures "$context function $($allowed.function) does not normalize OpCode::$Opcode to OpCode::$($allowed.normalizesTo)"
        }
    }

    if ($actualReferences.Count -ne $expectedReferenceCount) {
        $locations = @($actualReferences | ForEach-Object { $_.Source } | Sort-Object -Unique) -join ", "
        Add-Failure $Failures "$context found $($actualReferences.Count) codegen reference(s), expected $expectedReferenceCount; locations: $locations"
    }

    if ($allowedReferences.Count -gt 0 -and $null -eq $Producer.noProductionCalls) {
        Add-Failure $Failures "$context with allowed normalization references requires noProductionCalls proof"
    } elseif ($null -ne $Producer.noProductionCalls) {
        $callProof = $Producer.noProductionCalls
        if ([string]::IsNullOrWhiteSpace($callProof.function) -or [string]::IsNullOrWhiteSpace($callProof.definitionSource)) {
            Add-Failure $Failures "$context noProductionCalls requires function/definitionSource"
        } elseif ($null -eq $callProof.expectedCalls -or [int]$callProof.expectedCalls -ne 0) {
            Add-Failure $Failures "$context noProductionCalls expectedCalls must be exactly 0"
        } else {
            $cppFiles = @($files | Where-Object { $_.Extension -in @(".cpp", ".cc") })
            $callPattern = "\b" + [regex]::Escape([string]$callProof.function) + "\s*\("
            $callOccurrences = @(Get-PatternOccurrences -Files $cppFiles -Pattern $callPattern)
            $definitionEvidence = Get-CppFunctionEvidence -Path (Join-RepoPath $callProof.definitionSource) `
                -Function $callProof.function -SignatureContains $callProof.signatureContains -Failures $Failures -Context $context
            $expectedMatches = 1
            if ($null -ne $definitionEvidence -and $callOccurrences.Count -ne $expectedMatches) {
                Add-Failure $Failures "$context expected $($callProof.expectedCalls) production call(s) to $($callProof.function); found $($callOccurrences.Count - 1)"
            }
        }
    }

    $testKey = [string]$Producer.test
    $UsedTestKeys[$testKey] = $true
    if (-not $CatalogByKey.ContainsKey($testKey)) {
        Add-Failure $Failures "$context references unknown test key: $testKey"
        return
    }

    $test = $CatalogByKey[$testKey]
    $testEvidence = Get-CppFunctionEvidence -Path (Join-RepoPath $test.source) -Function $Producer.testFunction `
        -SignatureContains "" -Failures $Failures -Context "$context test"
    if ($null -eq $testEvidence) { return }

    if ($expectedReferenceCount -eq 0) {
        $absenceAssertion = "(?s)ASSERT_FALSE\s*\([^;]*hasOpcode\s*\([^;]*OpCode::$Opcode\b[^;]*\)"
        if ($testEvidence.Body -notmatch $absenceAssertion) {
            Add-Failure $Failures "$context test does not assert that generated bytecode omits OpCode::$Opcode"
        }
    } else {
        $targets = @($allowedReferences | ForEach-Object { $_.normalizesTo } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Sort-Object -Unique)
        if ($testEvidence.Body -notmatch $opcodePattern) {
            Add-Failure $Failures "$context test does not exercise OpCode::$Opcode"
        }
        foreach ($targetOpcode in $targets) {
            $target = [regex]::Escape([string]$targetOpcode)
            $normalizationAssertion = "(?s)ASSERT_EQ\s*\([^;]*OpCode::$target\b[^;]*GET_OPCODE\s*\("
            if ($testEvidence.Body -notmatch $normalizationAssertion) {
                Add-Failure $Failures "$context test does not assert normalization to OpCode::$targetOpcode"
            }
        }
    }
}

function Test-ProducerEvidence {
    param(
        [pscustomobject]$Coverage,
        [hashtable]$CatalogByKey,
        [hashtable]$UsedTestKeys,
        [System.Collections.Generic.List[string]]$Failures
    )

    $opcode = [string]$Coverage.opcode
    $producer = $Coverage.producer
    if ($null -eq $producer -or [string]::IsNullOrWhiteSpace($producer.kind)) {
        Add-Failure $Failures "Opcode $opcode requires producer kind evidence"
        return
    }

    if ([string]$producer.kind -eq "intentionally-not-produced") {
        Test-IntentionallyNotProducedEvidence -Opcode $opcode -Producer $producer -CatalogByKey $CatalogByKey `
            -UsedTestKeys $UsedTestKeys -Failures $Failures
        return
    }

    if ([string]::IsNullOrWhiteSpace($producer.source) -or [string]::IsNullOrWhiteSpace($producer.function) -or
        [string]::IsNullOrWhiteSpace($producer.emitter)) {
        Add-Failure $Failures "Opcode $opcode producer requires source/function/emitter evidence"
        return
    }
    if (-not ([string]$producer.source).Replace("\", "/").StartsWith("src/compiler/codegen/")) {
        Add-Failure $Failures "Opcode $opcode producer must be inside src/compiler/codegen: $($producer.source)"
        return
    }

    $sourcePath = Join-RepoPath $producer.source
    $evidence = Get-CppFunctionEvidence -Path $sourcePath -Function $producer.function `
        -SignatureContains $producer.signatureContains -Failures $Failures -Context "Opcode $opcode producer"
    if ($null -eq $evidence) { return }

    $emitterPattern = [regex]::Escape([string]$producer.emitter) + "\s*\("
    if ($evidence.Body -notmatch $emitterPattern) {
        Add-Failure $Failures "Opcode $opcode producer function $($producer.function) does not invoke $($producer.emitter)"
    }

    $opcodePattern = "\bOpCode::" + [regex]::Escape($opcode) + "\b"
    switch ([string]$producer.kind) {
        "direct" {
            $directEmissionPattern = $emitterPattern + "[^;]*" + $opcodePattern
            if ($evidence.Body -notmatch $directEmissionPattern) {
                Add-Failure $Failures "Opcode $opcode direct producer function $($producer.function) does not pass OpCode::$opcode to $($producer.emitter)"
            }
        }
        "selected" {
            if ([string]::IsNullOrWhiteSpace($producer.variable)) {
                Add-Failure $Failures "Opcode $opcode selected producer requires a variable"
                break
            }
            $variable = [regex]::Escape([string]$producer.variable)
            $selectionPattern = "\b$variable\s*=\s*OpCode::$opcode\b"
            if ($evidence.Body -notmatch $selectionPattern) {
                Add-Failure $Failures "Opcode $opcode producer does not select OpCode::$opcode through $($producer.variable)"
            }
            $flowPattern = $emitterPattern + "[^;]*\b$variable\b"
            if ($evidence.Body -notmatch $flowPattern) {
                Add-Failure $Failures "Opcode $opcode selected producer does not pass $($producer.variable) to $($producer.emitter)"
            }
        }
        default {
            Add-Failure $Failures "Opcode $opcode has unsupported producer kind: $($producer.kind)"
        }
    }
}

function Test-HandlerEvidence {
    param(
        [pscustomobject]$Coverage,
        [hashtable]$ActualRegistrations,
        [pscustomobject]$CentralRegistry,
        [System.Collections.Generic.List[string]]$Failures
    )

    $opcode = [string]$Coverage.opcode
    $handler = $Coverage.handler
    if ($null -eq $handler -or [string]::IsNullOrWhiteSpace($handler.source) -or
        [string]::IsNullOrWhiteSpace($handler.registrar) -or [string]::IsNullOrWhiteSpace($handler.symbol)) {
        Add-Failure $Failures "Opcode $opcode requires handler source/registrar/symbol evidence"
        return
    }

    $normalizedSource = ([string]$handler.source).Replace("\", "/")
    if (-not $normalizedSource.StartsWith("src/vm/vm_handlers/")) {
        Add-Failure $Failures "Opcode $opcode handler must be inside src/vm/vm_handlers: $normalizedSource"
        return
    }
    if (-not $ActualRegistrations.ContainsKey($opcode) -or @($ActualRegistrations[$opcode]).Count -ne 1) {
        Add-Failure $Failures "Opcode $opcode must have exactly one actual VM handler registration"
        return
    }

    $actual = @($ActualRegistrations[$opcode])[0]
    if ($actual.Source -ne $normalizedSource -or $actual.Symbol -ne [string]$handler.symbol) {
        Add-Failure $Failures "Opcode $opcode handler contract differs from actual registration: $($actual.Source) -> $($actual.Symbol)"
    }

    $sourcePath = Join-RepoPath $handler.source
    $registrarEvidence = Get-CppFunctionEvidence -Path $sourcePath -Function $handler.registrar `
        -SignatureContains "" -Failures $Failures -Context "Opcode $opcode handler registrar"
    if ($null -ne $registrarEvidence) {
        $assignmentPattern = "table\s*\[\s*opcodeIndex\s*\(\s*OpCode::$opcode\s*\)\s*\]\s*\.handler\s*=\s*" +
            [regex]::Escape([string]$handler.symbol) + "\s*;"
        if ($registrarEvidence.Body -notmatch $assignmentPattern) {
            Add-Failure $Failures "Opcode $opcode is not assigned to $($handler.symbol) inside $($handler.registrar)"
        }
    }

    $symbolEvidence = Get-CppFunctionEvidence -Path $sourcePath -Function $handler.symbol `
        -SignatureContains "" -Failures $Failures -Context "Opcode $opcode handler symbol"
    if ($null -ne $symbolEvidence -and $symbolEvidence.Signature -notmatch "\bHandlerStatus\b") {
        Add-Failure $Failures "Opcode $opcode handler symbol $($handler.symbol) does not return HandlerStatus"
    }

    if ($null -ne $CentralRegistry) {
        $registrarCall = "handlers::" + [regex]::Escape([string]$handler.registrar) + "\s*\(\s*table\s*\)"
        if ($CentralRegistry.Body -notmatch $registrarCall) {
            Add-Failure $Failures "Opcode $opcode registrar $($handler.registrar) is not called by makeHandlerTable"
        }
    }
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

$actualHandlerRegistrations = @{}
$handlerDirectory = Join-RepoPath "src\vm\vm_handlers"
$handlerRegistrationPattern = [regex]::new(
    'table\s*\[\s*opcodeIndex\s*\(\s*OpCode::(?<opcode>[A-Z][A-Z0-9_]*)\s*\)\s*\]\s*\.handler\s*=\s*(?<symbol>[A-Za-z_][A-Za-z0-9_]*)\s*;'
)
foreach ($file in @(Get-ChildItem -LiteralPath $handlerDirectory -File -Filter "vm_handlers_*.cpp")) {
    $source = Get-Content -LiteralPath $file.FullName -Raw
    $relativeSource = Get-NormalizedRepoPath $file.FullName
    foreach ($match in $handlerRegistrationPattern.Matches($source)) {
        $opcode = $match.Groups["opcode"].Value
        if (-not $actualHandlerRegistrations.ContainsKey($opcode)) {
            $actualHandlerRegistrations[$opcode] = [System.Collections.Generic.List[object]]::new()
        }
        $actualHandlerRegistrations[$opcode].Add([pscustomobject]@{
            Source = $relativeSource
            Symbol = $match.Groups["symbol"].Value
        }) | Out-Null
    }
}
foreach ($opcode in $expected) {
    if (-not $actualHandlerRegistrations.ContainsKey($opcode)) {
        Add-Failure $failures "No VM handler registration parsed for opcode $opcode"
    } elseif (@($actualHandlerRegistrations[$opcode]).Count -ne 1) {
        Add-Failure $failures "VM handler registration count for opcode $opcode is $(@($actualHandlerRegistrations[$opcode]).Count), expected 1"
    }
}
foreach ($opcode in $actualHandlerRegistrations.Keys) {
    if (-not $expectedSet.ContainsKey($opcode)) {
        Add-Failure $failures "VM handler registry references unknown opcode $opcode"
    }
}

$centralRegistry = Get-CppFunctionEvidence -Path (Join-RepoPath "src\vm\vm_handlers.cpp") `
    -Function "makeHandlerTable" -SignatureContains "" -Failures $failures -Context "VM handler registry"

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
    if ($contract.schemaVersion -ne 2) {
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

        Test-ProducerEvidence -Coverage $coverage -CatalogByKey $catalogByKey -UsedTestKeys $usedTestKeys `
            -Failures $failures
        Test-HandlerEvidence -Coverage $coverage -ActualRegistrations $actualHandlerRegistrations `
            -CentralRegistry $centralRegistry -Failures $failures

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

$notProducedCount = @($contract.coverage | Where-Object { $_.producer.kind -eq "intentionally-not-produced" }).Count
$producedCount = $expected.Count - $notProducedCount
$successMessage = "[OK] Opcode coverage matrix covers $($expected.Count) opcodes: " +
    "$producedCount verified CodeGen producers, $notProducedCount verified intentionally-not-produced entries, " +
    "VM handler registrations, and registered tests"
Write-Host $successMessage
