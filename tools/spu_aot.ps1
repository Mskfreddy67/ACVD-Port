# Recompile collected SPU records into C++ chunks.
#   .\tools\spu_aot.ps1 [-Config Release|Debug]
param(
  [string]$Config = "Release"
)
$ErrorActionPreference = "Continue"
. (Join-Path $PSScriptRoot "env.ps1")
$root = Split-Path -Parent $PSScriptRoot
$spurecomp = Join-Path $root ("local\build\" + $Config.ToLower() + "\acvd-spurecomp.exe")
if (-not (Test-Path $spurecomp)) {
  $spurecomp = Join-Path $root ("local\build\" + $Config.ToLower() + "\ac4-spurecomp.exe")
}
$outDir = Join-Path $root "local\gen\spu"
$cacheDir = Join-Path $root "local\spu_cache"

if (-not (Test-Path $spurecomp)) {
  Write-Error "acvd-spurecomp.exe not found in $spurecomp. Build target acvd-spurecomp first."
  exit 1
}
if (-not (Test-Path $cacheDir)) {
  Write-Error "SPU cache directory not found: $cacheDir"
  exit 1
}
if (-not (Test-Path $outDir)) {
  New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

Write-Host "Recompiling SPU records from $cacheDir into $outDir..."
& $spurecomp $outDir $cacheDir
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "SPU code recompiled successfully. Rebuilding project..."
& (Join-Path $PSScriptRoot "build.ps1") -Config $Config
exit $LASTEXITCODE
