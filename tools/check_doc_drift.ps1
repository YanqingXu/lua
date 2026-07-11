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

function Get-FactHeaderStatus {
    param([string]$Text)

    $header = [regex]::Match($Text, "(?ms)\A---\r?\n.*?^status:\s*(current|historical|planned|active)\s*$.*?^---\s*$")
    if (-not $header.Success) {
        return ""
    }
    return [regex]::Match($header.Value, "(?m)^status:\s*(\w+)\s*$").Groups[1].Value
}

function Get-CurrentDocumentationPaths {
    $candidates = [System.Collections.Generic.List[object]]::new()
    $readmePath = Join-RepoPath "README.md"
    if (Test-Path -LiteralPath $readmePath) {
        $candidates.Add((Get-Item -LiteralPath $readmePath)) | Out-Null
    }

    foreach ($relativeRoot in @("docs", "docs2", "examples")) {
        $path = Join-RepoPath $relativeRoot
        if (-not (Test-Path -LiteralPath $path)) {
            continue
        }
        foreach ($file in Get-ChildItem -LiteralPath $path -Recurse -File -Filter "*.md") {
            $candidates.Add($file) | Out-Null
        }
    }

    foreach ($file in $candidates) {
        $text = [System.IO.File]::ReadAllText($file.FullName, [System.Text.Encoding]::UTF8)
        if ((Get-FactHeaderStatus $text) -eq "current") {
            Get-RepoRelativePath $file.FullName
        }
    }
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
        if ($relativePath -match "^[a-z][a-z0-9+.-]*://") {
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

$strictVerifiedAgainstDocs = @(
    "docs/roadmap/c-style-refactoring-roadmap.md",
    "docs/roadmap/modern-cpp-teaching-audit-report.md"
)

$coreDocs = @(
    "README.md",
    "docs/index.md",
    "docs/roadmap/current.md",
    "docs/roadmap/c-style-refactoring-roadmap.md",
    "docs/roadmap/modern-cpp-teaching-audit-report.md",
    "docs/status/project-status.md",
    "docs/guides/development.md",
    "docs/guides/repl-cli.md",
    "docs/guides/test-runner.md",
    "docs/guides/bytecode-tool.md",
    "docs/knowledge/README.md",
    "docs/knowledge/source-document-map.md",
    "docs/ai/rag-knowledge-base.md",
    "docs/agents/senior-lua-architect.md",
    "docs/glossary.md",
    "docs/walkthroughs/index.md",
    "docs/projects/lua-lib.md",
    "docs/projects/lua-app.md",
    "docs/projects/lua-test.md",
    "docs/projects/lua-bytecode.md",
    "docs/architecture/gc.md",
    "docs/compiler/bytecode-generation.md",
    "docs/architecture/overview.md",
    "docs/architecture/runtime-services.md",
    "docs/compiler/parser.md",
    "docs/compiler/register-allocation.md",
    "docs/compiler/codegen-responsibility-map.md",
    "docs/vm/instruction-set.md",
    "docs/vm/trace-system.md",
    "docs/stdlib/overview.md",
    "docs/archive/research/deep-research-report.md",
    "docs/archive/coroutine/design-analysis.md",
    "docs/architecture/coroutine.md",
    "docs/archive/debug/debug-notes.md",
    "docs/roadmap/future-architecture.md",
    "docs/archive/refactors/refactor-expdesc-plan.md",
    "docs/archive/refactors/refactor-expdesc-pr-checklist.md",
    "docs/archive/refactors/refactor-singlepass-cleanup-plan.md",
    "docs/archive/history/exprdesc.md",
    "examples/README.md",
    "docs2/README.md",
    "docs2/00-project-overview/00-architecture.md",
    "docs2/00-project-overview/02-current-status.md",
    "docs2/02-source-code-map/00-directory-map.md",
    "docs2/02-source-code-map/01-core-files.md",
    "docs2/02-source-code-map/05-change-location-guide.md",
    "docs2/03-lexer-parser/00-overview.md",
    "docs2/04-bytecode-compiler/00-overview.md",
    "docs2/05-vm-runtime/00-overview.md",
    "docs2/06-value-object-system/00-overview.md",
    "docs2/07-table-metatable/00-overview.md",
    "docs2/08-function-call-closure/00-overview.md",
    "docs2/10-stdlib/00-overview.md",
    "docs2/12-gc-memory/00-overview.md",
    "docs2/13-compatibility/00-lua51-compatibility-goal.md",
    "docs2/14-testing/00-testing-strategy.md",
    "docs2/15-engineering-guide/00-build-and-run.md"
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

    if ($strictVerifiedAgainstDocs -contains $doc) {
        Assert-VerifiedAgainstPathsExist -Failures $failures -DocPath $doc -Text $text
    }
}

function Assert-LiveFactsVerificationFreshness {
    param(
        [System.Collections.Generic.List[string]]$Failures,
        [string]$DocPath,
        [string]$Text
    )

    if ($Text -notmatch "<!--\s*live-facts:start\s*-->") {
        return
    }

    $lastCheckedMatch = [regex]::Match($Text, "(?m)^last_checked:\s*(\d{4}-\d{2}-\d{2})\s*$")
    if (-not $lastCheckedMatch.Success) {
        Add-Failure $Failures "$DocPath live facts require a parseable last_checked date"
        return
    }
    $lastChecked = [datetime]::ParseExact(
        $lastCheckedMatch.Groups[1].Value,
        "yyyy-MM-dd",
        [System.Globalization.CultureInfo]::InvariantCulture
    )

    $git = Get-Command git -ErrorAction SilentlyContinue
    if ($null -eq $git) {
        Add-Failure $Failures "$DocPath live facts require git to verify dependency freshness"
        return
    }

    foreach ($relativePath in Get-VerifiedAgainstPaths $Text) {
        if ($relativePath -match "^[a-z][a-z0-9+.-]*://") {
            continue
        }

        $commitDateText = (& $git.Source -C $Root log -1 --format=%cs -- $relativePath).Trim()
        if ([string]::IsNullOrWhiteSpace($commitDateText)) {
            continue
        }
        $commitDate = [datetime]::ParseExact(
            $commitDateText,
            "yyyy-MM-dd",
            [System.Globalization.CultureInfo]::InvariantCulture
        )
        if ($commitDate -gt $lastChecked) {
            Add-Failure $Failures "$DocPath live facts were checked $($lastChecked.ToString('yyyy-MM-dd')) but dependency changed later: $relativePath ($commitDateText)"
        }
    }
}

$currentDocs = @(Get-CurrentDocumentationPaths | Sort-Object -Unique)
foreach ($doc in $currentDocs) {
    $text = Read-Text $doc
    if (-not (Test-FrontMatter $text)) {
        Add-Failure $failures "Missing or invalid fact header: $doc"
        continue
    }
    Assert-VerifiedAgainstPathsExist -Failures $failures -DocPath $doc -Text $text
    Assert-LiveFactsVerificationFreshness -Failures $failures -DocPath $doc -Text $text
}

foreach ($requiredFile in @("CMakeLists.txt", "tools/run_cmake_smoke.ps1", "tools/add_source.ps1", "tools/build_rag_index.ps1", "tools/search_rag_index.ps1")) {
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
if ($readme -notmatch "docs/status/project-status\.md") {
    Add-Failure $failures "README.md must point readers to docs/status/project-status.md"
}

$guide = Read-Text "docs/guides/development.md"
if ($guide -notmatch "MSBuild" -or $guide -notmatch "\.vcxproj" -or $guide -notmatch "docs/status/project-status\.md") {
    Add-Failure $failures "docs/guides/development.md must name the current MSBuild/.vcxproj path and reference docs/status/project-status.md"
}

foreach ($required in @("CMakeLists.txt", "tools\run_cmake_smoke.ps1", "tools\add_source.ps1", "CMake", "CTest", "secondary")) {
    if ($guide -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/guides/development.md is missing CMake/CTest support fact: $required"
    }
}

$statusDoc = Read-Text "docs/status/project-status.md"
$statusDocPath = Join-RepoPath "docs/status/project-status.md"
foreach ($required in @("Visual Studio", "MSBuild", ".vcxproj", "CMake", "CTest", "secondary", "CMakeLists.txt", "tools/run_cmake_smoke.ps1", "tools/add_source.ps1", "--report=junit", "RuntimeServices", "GCStrategy", "Learning Path", "lua_bytecode", "decoded instructions", "constant table", "--cfg", "Mermaid")) {
    if (-not (Select-String -LiteralPath $statusDocPath -SimpleMatch -Pattern $required -Quiet)) {
        Add-Failure $failures "docs/status/project-status.md is missing required fact: $required"
    }
}

$testSummary = Get-TestRunSummary -Failures $failures -ExecutablePath (Resolve-TestExecutablePath $TestExecutable)
if ($null -ne $testSummary) {
    Assert-DocHasCurrentTestCounts -Failures $failures -RelativePath "README.md" -Summary $testSummary
    Assert-DocHasCurrentTestCounts -Failures $failures -RelativePath "docs/status/project-status.md" -Summary $testSummary
    Assert-DocHasCurrentTestCounts -Failures $failures -RelativePath "docs2/00-project-overview/02-current-status.md" -Summary $testSummary
    Assert-DocHasCurrentTestCounts -Failures $failures -RelativePath "docs2/14-testing/00-testing-strategy.md" -Summary $testSummary

    foreach ($doc in $currentDocs) {
        $text = Read-Text $doc
        $blocks = [regex]::Matches(
            $text,
            "(?ms)<!--\s*live-facts:start\s*-->(?<body>.*?)<!--\s*live-facts:end\s*-->"
        )
        foreach ($block in $blocks) {
            $body = $block.Groups["body"].Value
            if ($body -match "registered tests") {
                foreach ($required in @(
                    $testSummary.RegisteredTests.ToString(),
                    $testSummary.TotalResults.ToString(),
                    "$($testSummary.Failed) failures"
                )) {
                    if ($body -notmatch [regex]::Escape($required)) {
                        Add-Failure $failures "$doc live-facts block is missing current test summary value: $required"
                    }
                }
            }

            foreach ($removedClaim in @(
                "LUA_VALUE_RESULT_PRIVATE_LEGACY_FIELDS\s*(?:=|打开|ON|选项)",
                "legacyFields\(\).*?(?:主入口|访问器|保留|读取)",
                "ValueResultLegacyMirrorProbe.*?(?:使用|新增|保留)"
            )) {
                if ($body -match $removedClaim) {
                    Add-Failure $failures "$doc live-facts block presents a removed ValueResult compatibility surface as current: $($Matches[0])"
                }
            }
        }
    }
}

$runtimeServicesDoc = Read-Text "docs/architecture/runtime-services.md"
foreach ($required in @("GlobalState& globalState", "StringPool& strings", "GarbageCollector& gc", "VM::DispatchStrategy* dispatchStrategy")) {
    if ($runtimeServicesDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/architecture/runtime-services.md is missing RuntimeServices field: $required"
    }
}

$startHereDoc = Read-Text "docs/index.md"
foreach ($required in @("docs/status/project-status.md", "docs/guides/development.md", "docs/guides/repl-cli.md", "docs/guides/test-runner.md", "docs/guides/bytecode-tool.md", "docs/projects/lua-lib.md", "docs/projects/lua-app.md", "docs/projects/lua-test.md", "docs/projects/lua-bytecode.md", "docs/vm/instruction-set.md", "walkthroughs/index.md", "glossary.md", "examples/README.md", "bin\lua_test.exe --list")) {
    if ($startHereDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/index.md is missing learning path reference: $required"
    }
}

$replGuideDoc = Read-Text "docs/guides/repl-cli.md"
$replGuideFacts = @(
    ".help",
    ".bytecode <expr|chunk>",
    ".ast <expr|chunk>",
    ".gc [stats|collect|strategy|help]",
    "Tab completion",
    "line-numbered",
    "lua:1>",
    "GCStrategy",
    "collectgarbage(`"strategy`")",
    "colorized error output",
    "ErrorColorMode::Auto",
    "mode: expression",
    "RuntimeServices.gc",
    "string.sub",
    "REPL Commands"
)
foreach ($required in $replGuideFacts) {
    if ($replGuideDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/guides/repl-cli.md is missing current REPL command fact: $required"
    }
}

$glossaryDoc = Read-Text "docs/glossary.md"
foreach ($required in @("RuntimeServices", "GCStrategy", "GlobalState", "LuaState", "Proto", "SymbolRef", "ValueResult", "CondResult", "LValueRef", "TMS", "RK")) {
    if ($glossaryDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "glossary.md is missing required term: $required"
    }
}

$examplesDoc = Read-Text "examples/README.md"
foreach ($required in @("bin\lua_app.exe", "hello.lua", "control_flow.lua", "tables_and_methods.lua", "metamethods.lua")) {
    if ($examplesDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "examples/README.md is missing example reference: $required"
    }
}

$sourceMapDoc = Read-Text "docs/knowledge/source-document-map.md"
foreach ($required in @("Compiler", "VM", "Runtime", "GC", "docs/roadmap/current.md", "docs/vm/instruction-set.md", "tools/check_doc_drift.ps1")) {
    if ($sourceMapDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/knowledge/source-document-map.md is missing source/document map fact: $required"
    }
}

$ragDoc = Read-Text "docs/ai/rag-knowledge-base.md"
foreach ($required in @("tools/build_rag_index.ps1", "tools/search_rag_index.ps1", "docs/roadmap/current.md", "docs/vm/instruction-set.md", "tools/check_doc_drift.ps1", "4.0")) {
    if ($ragDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/ai/rag-knowledge-base.md is missing RAG fact: $required"
    }
}

$agentDoc = Read-Text "docs/agents/senior-lua-architect.md"
foreach ($required in @("docs/roadmap/current.md", "docs/guides/development.md", "tools/check_doc_drift.ps1", "RAII", "std::variant", "tools/run_quality_gate.ps1")) {
    if ($agentDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/agents/senior-lua-architect.md is missing agent workflow fact: $required"
    }
}

$bytecodeDoc = Read-Text "docs/compiler/bytecode-generation.md"
foreach ($required in @("AST", "SymbolRef", "ValueResult", "CondResult", "LValueRef", "CallResultInfo", "Proto", "BytecodeBuilder")) {
    if ($bytecodeDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/compiler/bytecode-generation.md is missing current pipeline term: $required"
    }
}

$bytecodeToolDoc = Read-Text "docs/guides/bytecode-tool.md"
foreach ($required in @("decoded instructions", "constant references", "constant table", "recursive child protos in full mode", "--diff", "side-by-side", "changed lines", "--cfg", "Mermaid", "basic blocks", "control-flow edges")) {
    if ($bytecodeToolDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/guides/bytecode-tool.md is missing current bytecode printer fact: $required"
    }
}

$traceSystemDoc = Read-Text "docs/vm/trace-system.md"
foreach ($required in @("Trace JSONL Plain Golden", "Trace JSONL Diff Golden", "changedRegisters", "funcName", "registers", "VM Trace Debug")) {
    if ($traceSystemDoc -notmatch [regex]::Escape($required)) {
        Add-Failure $failures "docs/vm/trace-system.md is missing current trace system fact: $required"
    }
}

$historyDoc = Read-Text "docs/archive/history/exprdesc.md"
if ($historyDoc -notmatch "status:\s*historical") {
    Add-Failure $failures "docs/archive/history/exprdesc.md must be marked historical"
}

if ($failures.Count -gt 0) {
    Write-Host "[FAIL] Documentation drift checks failed:"
    foreach ($failure in $failures) {
        Write-Host " - $failure"
    }
    exit 1
}

Write-Host "[OK] Documentation drift checks passed"
