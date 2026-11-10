param(
    [Parameter(Mandatory = $true)]
    [string] $InputPath,

    [Parameter(Mandatory = $true)]
    [string] $OutputPath
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $InputPath -PathType Leaf)) {
    throw "English language file not found: $InputPath"
}

$jsonBytes = [System.IO.File]::ReadAllBytes($InputPath)
if ($jsonBytes.Length -ge 3 -and $jsonBytes[0] -eq 0xEF -and $jsonBytes[1] -eq 0xBB -and $jsonBytes[2] -eq 0xBF) {
    $jsonWithoutBom = [byte[]]::new($jsonBytes.Length - 3)
    [Array]::Copy($jsonBytes, 3, $jsonWithoutBom, 0, $jsonWithoutBom.Length)
    $jsonBytes = $jsonWithoutBom
}
$utf8 = New-Object System.Text.UTF8Encoding($false, $true)
$jsonText = $utf8.GetString($jsonBytes)
$document = ConvertFrom-Json -InputObject $jsonText
if ($null -eq $document -or $document -isnot [pscustomobject]) {
    throw "English language file must contain a JSON object: $InputPath"
}

$byteValues = [System.Collections.Generic.List[string]]::new()
foreach ($byte in $jsonBytes) {
    $byteValues.Add(('0x{0:X2}' -f $byte))
}
$byteValues.Add('0x00')

$lines = [System.Collections.Generic.List[string]]::new()
for ($index = 0; $index -lt $byteValues.Count; $index += 16) {
    $count = [Math]::Min(16, $byteValues.Count - $index)
    $lineValues = $byteValues.GetRange($index, $count) -join ', '
    $lines.Add("    $lineValues,")
}

$header = @"
// Generated from $([System.IO.Path]::GetFileName($InputPath)); do not edit.
#pragma once

namespace Language::Embedded
{
inline constexpr unsigned char EnglishCatalogJson[] =
{
$($lines -join "`n")
};
} // namespace Language::Embedded
"@

$outputDirectory = [System.IO.Path]::GetDirectoryName($OutputPath)
[System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null
$outputBytes = [System.Text.UTF8Encoding]::new($false).GetBytes($header)
$outputIsCurrent = $false
if (Test-Path -LiteralPath $OutputPath -PathType Leaf) {
    $existingBytes = [System.IO.File]::ReadAllBytes($OutputPath)
    $outputIsCurrent = [Convert]::ToBase64String($existingBytes) -ceq [Convert]::ToBase64String($outputBytes)
}
if (-not $outputIsCurrent) {
    [System.IO.File]::WriteAllBytes($OutputPath, $outputBytes)
}
