Set-StrictMode -Version Latest

function Get-SingleReleaseIdentityMatch {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text,
        [Parameter(Mandatory = $true)]
        [string]$Pattern,
        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    $matches = [regex]::Matches($Text, $Pattern)
    if ($matches.Count -ne 1) {
        throw "Release identity requires exactly one $Label; found $($matches.Count)"
    }
    return $matches[0].Groups[1].Value
}

function Get-LuaCppReleaseIdentity {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)]
        [string]$CMakeText,
        [Parameter(Mandatory = $true)]
        [string]$VersionHeaderText
    )

    $projectVersion = Get-SingleReleaseIdentityMatch `
        -Text $CMakeText `
        -Pattern '(?m)^\s*project\(lua_cpp\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)\s+LANGUAGES\b' `
        -Label "CMake project version"
    $cmakeAbiText = Get-SingleReleaseIdentityMatch `
        -Text $CMakeText `
        -Pattern '(?m)^\s*set\(LUA_CPP_ABI_VERSION\s+([0-9]+)\s*\)\s*$' `
        -Label "CMake ABI version"

    $soversionMatches = [regex]::Matches(
        $CMakeText,
        '(?m)^\s*SOVERSION\s+\$\{LUA_CPP_ABI_VERSION\}\s*$'
    )
    if ($soversionMatches.Count -ne 1) {
        throw (
            "Release identity requires shared SOVERSION to use " +
            "LUA_CPP_ABI_VERSION exactly once; found $($soversionMatches.Count)"
        )
    }

    $majorText = Get-SingleReleaseIdentityMatch `
        -Text $VersionHeaderText `
        -Pattern '(?m)^\s*#define\s+LUA_CPP_VERSION_MAJOR\s+([0-9]+)\s*$' `
        -Label "header major version"
    $minorText = Get-SingleReleaseIdentityMatch `
        -Text $VersionHeaderText `
        -Pattern '(?m)^\s*#define\s+LUA_CPP_VERSION_MINOR\s+([0-9]+)\s*$' `
        -Label "header minor version"
    $patchText = Get-SingleReleaseIdentityMatch `
        -Text $VersionHeaderText `
        -Pattern '(?m)^\s*#define\s+LUA_CPP_VERSION_PATCH\s+([0-9]+)\s*$' `
        -Label "header patch version"
    $headerVersion = Get-SingleReleaseIdentityMatch `
        -Text $VersionHeaderText `
        -Pattern '(?m)^\s*#define\s+LUA_CPP_VERSION\s+"([^"]+)"\s*$' `
        -Label "header string version"
    $headerAbiText = Get-SingleReleaseIdentityMatch `
        -Text $VersionHeaderText `
        -Pattern '(?m)^\s*#define\s+LUA_CPP_ABI_VERSION\s+([0-9]+)\s*$' `
        -Label "header ABI version"

    $componentVersion = "$majorText.$minorText.$patchText"
    if ($componentVersion -cne $headerVersion) {
        throw (
            "Header component version $componentVersion does not match " +
            "LUA_CPP_VERSION $headerVersion"
        )
    }
    if ($headerVersion -cne $projectVersion) {
        throw (
            "Header version $headerVersion does not match CMake project " +
            "version $projectVersion"
        )
    }
    if ($headerAbiText -cne $cmakeAbiText) {
        throw (
            "Header ABI version $headerAbiText does not match CMake shared " +
            "SOVERSION $cmakeAbiText"
        )
    }

    return [pscustomobject][ordered]@{
        ProjectVersion = $projectVersion
        AbiVersion = [int]$headerAbiText
    }
}

Export-ModuleMember -Function Get-LuaCppReleaseIdentity
