# One-time per clone: installs the versioned git hooks from tools/ into .git/hooks/.
param()

$ErrorActionPreference = 'Stop'

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$gitDir = (& git -C $RepoRoot rev-parse --git-dir).Trim()
if (-not [System.IO.Path]::IsPathRooted($gitDir)) { $gitDir = Join-Path $RepoRoot $gitDir }
$hooksDir = Join-Path $gitDir 'hooks'

Copy-Item (Join-Path $PSScriptRoot 'pre-commit') (Join-Path $hooksDir 'pre-commit') -Force
Write-Host "Installed pre-commit hook to $hooksDir"
