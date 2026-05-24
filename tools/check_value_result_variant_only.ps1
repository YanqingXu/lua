param(
    [string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

$forbiddenPatterns = @(
    "LegacyFields",
    "legacyFields",
    "ValueResultLegacyMirrorProbe",
    "LUA_VALUE_RESULT_PRIVATE_LEGACY_FIELDS",
    "LUA_SUPPRESS_DEPRECATED_DECLARATIONS",
    "setPayload\s*\("
)

$scanFiles = @()
foreach ($relativeRoot in @("src", "tests")) {
    $path = Join-Path $Root $relativeRoot
    if (Test-Path -LiteralPath $path) {
        $scanFiles += Get-ChildItem -LiteralPath $path -Recurse -File |
            Where-Object { $_.Extension -in @(".cpp", ".hpp", ".h") }
    }
}

foreach ($relativePath in @(
    "CMakeLists.txt",
    "lua.vcxproj",
    "lua_app.vcxproj",
    "lua_bytecode.vcxproj",
    "lua_test.vcxproj"
)) {
    $path = Join-Path $Root $relativePath
    if (Test-Path -LiteralPath $path) {
        $scanFiles += Get-Item -LiteralPath $path
    }
}

$violations = New-Object System.Collections.Generic.List[string]

foreach ($file in $scanFiles) {
    $content = Get-Content -LiteralPath $file.FullName
    for ($index = 0; $index -lt $content.Count; $index++) {
        $line = $content[$index]
        foreach ($pattern in $forbiddenPatterns) {
            if ($line -match $pattern) {
                $relative = [System.IO.Path]::GetRelativePath($Root, $file.FullName)
                $lineNo = $index + 1
                $violations.Add("${relative}:${lineNo}: forbidden ValueResult legacy symbol matched '$pattern'")
            }
        }
    }
}

if ($violations.Count -gt 0) {
    $violations | ForEach-Object { Write-Error $_ }
    throw "ValueResult variant-only check failed"
}

Write-Host "[OK] ValueResult is variant-only at the source/config boundary"
