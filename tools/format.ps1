# Formats (or checks) MenyooSP C++ sources with the pinned clang-format.
#
#   .\tools\format.ps1          # rewrite files in place
#   .\tools\format.ps1 -Check   # verify only, exit 1 on violations (same as CI)
#   .\tools\format.ps1 -Path Solution/source/Menu
#
# Pinned version lives in tools/clang-format.version (single source of truth,
# also read by .github/workflows/clang-format.yml). No hardcoded install paths:
# the binary is resolved via -ClangFormat, then the VS install (vswhere), then PATH.
param(
    [switch]$Check,
    [string]$Path = '',
    [string]$ClangFormat = ''
)

$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$ExpectedVersion = (Get-Content (Join-Path $PSScriptRoot 'clang-format.version') -TotalCount 1).Trim()

function Resolve-ClangFormat {
    param([string]$Override)

    if ($Override -ne '') {
        if (-not (Test-Path $Override)) { throw "clang-format not found: $Override" }
        return (Resolve-Path $Override).Path
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (Test-Path $vswhere) {
        # ponytail: x64 first; the ARM64 binary exists side-by-side but won't run on x64 Windows.
        $found = & $vswhere -latest -find 'VC/Tools/Llvm/x64/bin/clang-format.exe' 2>$null |
            Where-Object { $_ -ne '' } |
            Select-Object -First 1
        if (($found -eq $null) -or (-not (Test-Path $found))) {
            $found = & $vswhere -latest -find 'VC/Tools/Llvm/*/bin/clang-format.exe' 2>$null |
                Where-Object { $_ -ne '' } |
                Select-Object -First 1
        }
        if ($found -ne $null -and (Test-Path $found)) { return $found }
    }

    $onPath = Get-Command 'clang-format' -ErrorAction SilentlyContinue
    if ($onPath -ne $null) { return $onPath.Source }

    throw ('clang-format {0} not found. Install the "C++ Clang tools for Windows" VS component ' +
        '(it ships with VS), or pass -ClangFormat <path>.' -f $ExpectedVersion)
}

$Exe = Resolve-ClangFormat -Override $ClangFormat
$actual = (& $Exe --version) | Select-Object -First 1
if ($actual -notmatch 'clang-format version (\d+\.\d+\.\d+)') { throw "Cannot parse version from: $actual" }
if ($Matches[1] -ne $ExpectedVersion) {
    throw ('{0} (expected {1}). Update VS / your install, or bump tools/clang-format.version ' +
        'in lockstep with CI.' -f $actual, $ExpectedVersion)
}
Write-Host "$actual ($Exe)"

$excludeDirs = @('_Build', 'bin', 'tmp')
if ($Path -ne '') {
    $target = if ([System.IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $RepoRoot $Path }
} else {
    $target = Join-Path $RepoRoot 'Solution/source'
}
$files = @()
if (($Path -ne '') -and (Test-Path $target -PathType Leaf)) {
    $files = @((Resolve-Path $target).Path)
} else {
    $files = @(Get-ChildItem -Path $target -Include '*.cpp', '*.h', '*.hpp' -Recurse -File -ErrorAction Stop |
    Where-Object {
        $full = $_.FullName
        ($excludeDirs | Where-Object { $full -like ('*' + [System.IO.Path]::DirectorySeparatorChar + $_ + [System.IO.Path]::DirectorySeparatorChar + '*') }).Count -eq 0 -and
        $_.Name -ne 'build_version.h'
    } |
    Select-Object -ExpandProperty FullName)
}
if ($files.Count -eq 0) { Write-Host 'No C++ files found.'; exit 0 }

if ($Check) {
    & $Exe --dry-run --Werror --style=file $files
    if ($LASTEXITCODE -ne 0) { throw 'clang-format check failed. Run .\tools\format.ps1 to fix.' }
    Write-Host ('OK: {0} files clean.' -f $files.Count)
} else {
    & $Exe -i --style=file $files
    Write-Host ('Formatted {0} files.' -f $files.Count)
}
