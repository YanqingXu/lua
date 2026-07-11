param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$TestExecutable = ""
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

function Read-Text {
    param([string]$RelativePath)
    $path = Join-RepoPath $RelativePath
    if (-not (Test-Path -LiteralPath $path)) {
        throw "Missing required file: $RelativePath"
    }
    return [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)
}

function Test-FrontMatter {
    param([string]$Text)
    return $Text -match "(?ms)^---\r?\n.*?^status:\s*(current|historical|planned|active)\s*$.*?^verified_against:\s*.+$.*?^last_checked:\s*\d{4}-\d{2}-\d{2}\s*$.*?^applies_to:\s*.+$.*?^---\s*$"
}

function Get-RepoRelativePath {
    param([string]$Path)

    $fullRoot = (Resolve-Path -LiteralPath $Root).Path.TrimEnd('\', '/')
    $fullPath = [System.IO.Path]::GetFullPath($Path)
    if ($fullPath.StartsWith($fullRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $fullPath.Substring($fullRoot.Length).TrimStart('\', '/')
    }
    return $fullPath
}

function Resolve-TestExecutablePath {
    param([string]$Path)

    if ([string]::IsNullOrWhiteSpace($Path)) {
        return Join-RepoPath "bin\lua_test.exe"
    }

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    return Join-Path $Root $Path
}

function Get-VerifiedAgainstPaths {
    param([string]$Text)

    $match = [regex]::Match($Text, "(?m)^verified_against:\s*(.+?)\s*$")
    if (-not $match.Success) {
        return @()
    }

    return @($match.Groups[1].Value -split ";" | ForEach-Object { $_.Trim() } | Where-Object { $_ })
}

function Assert-VerifiedAgainstPathsExist {
    param(
        [System.Collections.Generic.List[string]]$Failures,
        [string]$DocPath,
        [string]$Text
    )

    foreach ($relativePath in Get-VerifiedAgainstPaths $Text) {
        if ($relativePath -match "^https?://") {
            continue
        }
        $path = Join-RepoPath $relativePath
        if (-not (Test-Path -LiteralPath $path)) {
            Add-Failure $Failures "$DocPath verified_against references missing path: $relativePath"
        }
    }
}

function Convert-CommandOutputToText {
    param([object[]]$Output)

    return ($Output | ForEach-Object { $_.ToString() }) -join "`n"
}

function Get-TestRunSummary {
    param(
        [System.Collections.Generic.List[string]]$Failures,
        [string]$ExecutablePath
    )

    if (-not (Test-Path -LiteralPath $ExecutablePath)) {
        Add-Failure $Failures "Missing test executable for dynamic test count check: $(Get-RepoRelativePath $ExecutablePath). Build lua_test.vcxproj first."
        return $null
    }

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        $output = & $ExecutablePath 2>&1
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    $text = Convert-CommandOutputToText $output

    if ($exitCode -ne 0) {
        Add-Failure $Failures "Dynamic test count check failed: $(Get-RepoRelativePath $ExecutablePath) exited with code $exitCode"
        return $null
    }

    $registered = [regex]::Match($text, "(?m)^Registered Tests:\s*(\d+)\s*$")
    $total = [regex]::Match($text, "(?m)^Total Results:\s*(\d+)\s*$")
    $failed = [regex]::Match($text, "(?m)^Failed:\s*(\d+)\s*$")

    foreach ($matchInfo in @(
        @{ Name = "Registered Tests"; Match = $registered },
        @{ Name = "Total Results"; Match = $total },
        @{ Name = "Failed"; Match = $failed }
    )) {
        if (-not $matchInfo.Match.Success) {
            Add-Failure $Failures "Dynamic test count check could not parse '$($matchInfo.Name)' from $(Get-RepoRelativePath $ExecutablePath) output"
        }
    }

    if (-not ($registered.Success -and $total.Success -and $failed.Success)) {
        return $null
    }

    return [pscustomobject]@{
        RegisteredTests = [int]$registered.Groups[1].Value
        TotalResults = [int]$total.Groups[1].Value
        Failed = [int]$failed.Groups[1].Value
    }
}

function Assert-DocHasCurrentTestCounts {
    param(
        [System.Collections.Generic.List[string]]$Failures,
        [string]$RelativePath,
        [pscustomobject]$Summary
    )

    $registered = [regex]::Escape($Summary.RegisteredTests.ToString())
    $total = [regex]::Escape($Summary.TotalResults.ToString())

    $registeredPattern = "(?<!\d)$registered(?!\d)"
    $totalPattern = "(?<!\d)$total(?!\d)"
    $path = Join-RepoPath $RelativePath

    if (-not (Select-String -LiteralPath $path -Pattern $registeredPattern -Quiet)) {
        Add-Failure $Failures "$RelativePath is missing current registered test count: $($Summary.RegisteredTests) registered / $($Summary.TotalResults) results / $($Summary.Failed) failures"
    }

    if (-not (Select-String -LiteralPath $path -Pattern $totalPattern -Quiet)) {
        Add-Failure $Failures "$RelativePath is missing current assertion result count: $($Summary.RegisteredTests) registered / $($Summary.TotalResults) results / $($Summary.Failed) failures"
    }

    if (-not (Select-String -LiteralPath $path -SimpleMatch -Pattern "$($Summary.Failed) failures" -Quiet)) {
        Add-Failure $Failures "$RelativePath is missing current failure count: $($Summary.RegisteredTests) registered / $($Summary.TotalResults) results / $($Summary.Failed) failures"
    }
}

$failures = [System.Collections.Generic.List[string]]::new()

$coreDocs = @(
    "README.md",
    "docs/index.md",
    "docs/architecture/overview.md",
    "docs/architecture/execution-pipeline/overview.md",
    "docs/compiler/lexer.md",
    "docs/compiler/parser.md",
    "docs/compiler/bytecode-generation.md",
    "docs/compiler/control-flow/overview.md",
    "docs/vm/instruction-set.md",
    "docs/vm/runtime/overview.md",
    "docs/vm/trace-system.md",
    "docs/runtime/value/overview.md",
    "docs/runtime/table/overview.md",
    "docs/runtime/functions/overview.md",
    "docs/runtime/services.md",
    "docs/gc/overview.md",
    "docs/gc/implementation.md",
    "docs/knowledge/source-document-map.md",
    "docs/glossary.md",
    "docs/architecture/patterns.md",
    "docs/compiler/parser.md",
    "docs/compiler/register-allocation.md",
    "docs/compiler/codegen-responsibility-map.md",
    "docs/stdlib/overview.md",
    "docs/stdlib/library-reference/overview.md",
    "docs/compatibility/lua51/overview.md",
    "docs/testing/testing-strategy.md",
    "docs/debugging/overview.md",
    "docs/knowledge/source-map/directory-map.md"
)

foreach ($doc in $coreDocs) {
    $path = Join-RepoPath $doc
    if (-not (Test-Path -LiteralPath $path)) {
        Add-Failure $failures "Missing core doc: $doc"
        continue
    }

    $text = Read-Text $doc
    if (-not (Test-FrontMatter $text)) {
        Add-Failure $failures "Missing or invalid fact header: $doc"
        continue
    }

    Assert-VerifiedAgainstPathsExist -Failures $failures -DocPath $doc -Text $text
}

# Every encyclopedia page is a maintained technical artifact, not only the
# high-traffic pages in $coreDocs.  Requiring fact headers and live anchors for
# the complete tree prevents migrated or newly added pages from becoming
# unverified documentation islands.
$technicalDocs = Get-ChildItem -LiteralPath (Join-RepoPath "docs") -Recurse -File -Filter "*.md"
foreach ($file in $technicalDocs) {
    $doc = (Get-RepoRelativePath $file.FullName).Replace('\', '/')
    $text = [System.IO.File]::ReadAllText($file.FullName, [System.Text.Encoding]::UTF8)
    if (-not (Test-FrontMatter $text)) {
        Add-Failure $failures "Missing or invalid fact header: $doc"
        continue
    }

    Assert-VerifiedAgainstPathsExist -Failures $failures -DocPath $doc -Text $text
}

foreach ($requiredFile in @("CMakeLists.txt", "tools/run_cmake_smoke.ps1", "tools/add_source.ps1")) {
    $path = Join-RepoPath $requiredFile
    if (-not (Test-Path -LiteralPath $path)) {
        Add-Failure $failures "Missing build support file: $requiredFile"
    }
}

$cmakeLists = Read-Text "CMakeLists.txt"
foreach ($required in @("lua_configure_target_warnings", "/W4", "/permissive-", "-Wpedantic", "-Wconversion")) {
    if ($cmakeLists -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "CMakeLists.txt is missing compile warning policy fact: $required"
    }
}

foreach ($projectFile in @("lua.vcxproj", "lua_app.vcxproj", "lua_bytecode.vcxproj", "lua_test.vcxproj")) {
    $projectText = Read-Text $projectFile
    if ($projectText -match "<WarningLevel>Level3</WarningLevel>") {
        Add-Failure $failures "$projectFile must not regress to WarningLevel Level3"
    }
    if ($projectText -notmatch "<WarningLevel>Level4</WarningLevel>") {
        Add-Failure $failures "$projectFile is missing WarningLevel Level4"
    }
}

$compilerDir = Join-RepoPath "src/compiler"
if (Test-Path -LiteralPath $compilerDir) {
    $legacyMatches = Get-ChildItem -LiteralPath $compilerDir -Recurse -File |
        Select-String -Pattern "ExprDesc|ExprKind|expdesc" -CaseSensitive

    foreach ($match in $legacyMatches) {
        $relative = Get-RepoRelativePath $match.Path
        Add-Failure $failures "Legacy compiler pipeline reference in ${relative}:$($match.LineNumber): $($match.Line.Trim())"
    }
} else {
    Add-Failure $failures "Missing compiler source directory: src/compiler"
}

$readme = Read-Text "README.md"
foreach ($required in @("docs/index.md", "docs/compiler/bytecode-generation.md", "docs/vm/instruction-set.md", "docs/runtime/value/overview.md", "docs/gc/implementation.md", "docs/compatibility/lua51/overview.md")) {
    if ($readme -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "README.md is missing technical documentation entry: $required"
    }
}

$testSummary = Get-TestRunSummary -Failures $failures -ExecutablePath (Resolve-TestExecutablePath $TestExecutable)
if ($null -ne $testSummary) {
    Assert-DocHasCurrentTestCounts -Failures $failures -RelativePath "README.md" -Summary $testSummary
}

$runtimeServicesDoc = Read-Text "docs/runtime/services.md"
foreach ($required in @("GlobalState& globalState", "StringPool& strings", "GarbageCollector& gc", "VM::DispatchStrategy* dispatchStrategy")) {
    if ($runtimeServicesDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/runtime/services.md is missing RuntimeServices field: $required"
    }
}

$startHereDoc = Read-Text "docs/index.md"
foreach ($required in @("architecture/overview.md", "compiler/lexer.md", "compiler/parser.md", "compiler/bytecode-generation.md", "vm/instruction-set.md", "vm/runtime/overview.md", "runtime/value/overview.md", "gc/implementation.md", "stdlib/overview.md", "compatibility/lua51/overview.md", "testing/testing-strategy.md", "debugging/overview.md", "knowledge/source-document-map.md")) {
    if ($startHereDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/index.md is missing technical module reference: $required"
    }
}

$glossaryDoc = Read-Text "docs/glossary.md"
foreach ($required in @("RuntimeServices", "GCStrategy", "GlobalState", "LuaState", "Proto", "SymbolRef", "ValueResult", "CondResult", "LValueRef", "TMS", "RK")) {
    if ($glossaryDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "glossary.md is missing required term: $required"
    }
}

$sourceMapDoc = Read-Text "docs/knowledge/source-document-map.md"
foreach ($required in @("Lexer", "Parser/AST", "CodeGen", "Opcode ABI", "VM call/dispatch", "Value/object", "GC", "Standard library", "Compatibility", "Diagnostics", "instruction-set.md")) {
    if ($sourceMapDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/knowledge/source-document-map.md is missing source/document map fact: $required"
    }
}

$bytecodeDoc = Read-Text "docs/compiler/bytecode-generation.md"
foreach ($required in @("AST", "SymbolRef", "ValueResult", "CondResult", "LValueRef", "CallResultInfo", "Proto", "BytecodeBuilder")) {
    if ($bytecodeDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/compiler/bytecode-generation.md is missing current pipeline term: $required"
    }
}

$traceSystemDoc = Read-Text "docs/vm/trace-system.md"
foreach ($required in @("Trace JSONL Plain Golden", "Trace JSONL Diff Golden", "changedRegisters", "funcName", "registers", "VM Trace Debug")) {
    if ($traceSystemDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/vm/trace-system.md is missing current trace system fact: $required"
    }
}

foreach ($forbiddenPath in @("docs2", "docs/roadmap", "docs/status", "docs/agents", "docs/ai", "docs/guides", "docs/projects", "docs/superpowers")) {
    if (Test-Path -LiteralPath (Join-RepoPath $forbiddenPath)) {
        Add-Failure $failures "Non-technical documentation path must not exist: $forbiddenPath"
    }
}

if ($failures.Count -gt 0) {
    Write-Host "[FAIL] Documentation drift checks failed:"
    foreach ($failure in $failures) {
        Write-Host " - $failure"
    }
    exit 1
}

Write-Host "[OK] Documentation drift checks passed"
