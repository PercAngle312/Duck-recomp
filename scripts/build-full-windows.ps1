param(
  [Parameter(Mandatory=$true)][string]$GameSource,
  [Parameter(Mandatory=$true)][string]$N64Recomp,
  [Parameter(Mandatory=$true)][string]$N64ModernRuntime,
  [Parameter(Mandatory=$true)][string]$RT64,
  [string]$BuildDirectory = "build-full-windows",
  [switch]$ConfigureOnly
)
$ErrorActionPreference = "Stop"
$game = (Resolve-Path -LiteralPath $GameSource -ErrorAction Stop).Path
$recomp = (Resolve-Path -LiteralPath $N64Recomp -ErrorAction Stop).Path
$modern = (Resolve-Path -LiteralPath $N64ModernRuntime -ErrorAction Stop).Path
$rt64 = (Resolve-Path -LiteralPath $RT64 -ErrorAction Stop).Path

$required = @(
  (Join-Path $game "CMakeLists.txt"),
  (Join-Path $game "build/modules/funcs_0.c"),
  (Join-Path $game "build/resident/funcs_0.c"),
  (Join-Path $game "build/runtime/funcs_0.c"),
  (Join-Path $game "build/runtime/bindings.cpp"),
  (Join-Path $game "build/runtime/audio_rsp.cpp"),
  (Join-Path $recomp "include/recomp.h"),
  (Join-Path $modern "CMakeLists.txt"),
  (Join-Path $rt64 "CMakeLists.txt")
)
$missing = @($required | Where-Object { !(Test-Path -LiteralPath $_ -PathType Leaf) })
if ($missing.Count) {
  Write-Host "Missing inputs required for native Windows game build:"
  $missing | ForEach-Object { Write-Host "  $_" }
  throw "Full Duck Dodgers game compile aborted before CMake: $($missing.Count) missing input(s)."
}
# The public game/CMakeLists.txt is intentionally a test-only scaffold.
# A real build requires the imported Phase 12 CMake which actually defines duckdodgers.
$gameCmake = Get-Content -LiteralPath (Join-Path $game "CMakeLists.txt") -Raw
if ($gameCmake -notmatch '(?m)^\s*add_executable\s*\(\s*duckdodgers\b') {
  throw "Not a full-game CMake source: duckdodgers executable target is missing. Import the actual Phase 12 project to a private checkout; the public CI preflight stub cannot build the game."
}
$build = [IO.Path]::GetFullPath($BuildDirectory)
$arguments = @(
  "-S", $game,
  "-B", $build,
  "-A", "x64",
  "-DDUCK_BUILD_GAME=ON",
  "-DDUCK_ENABLE_BOOT_PROBE=ON",
  "-DN64RECOMP_ROOT=$recomp",
  "-DN64MODERN_ROOT=$modern",
  "-DDUCK_RT64_ROOT=$rt64"
)
Write-Host "Configuring full native game target (not test-only)..."
& cmake @arguments
if ($LASTEXITCODE -ne 0) { throw "Full Windows CMake configuration failed (exit $LASTEXITCODE)." }
if ($ConfigureOnly) { Write-Host "Full-game configuration passed."; exit 0 }
Write-Host "Building actual duckdodgers target..."
& cmake --build $build --config Release --target duckdodgers --parallel 2
if ($LASTEXITCODE -ne 0) { throw "DuckDodgers.exe compilation/link failed (exit $LASTEXITCODE)." }
$exe=Join-Path $build "Release/DuckDodgers.exe"
if (!(Test-Path -LiteralPath $exe -PathType Leaf)) {
  # Existing upstream target is lower-case; CMake output name must be explicitly aligned
  $exe=Join-Path $build "Release/duckdodgers.exe"
}
if (!(Test-Path -LiteralPath $exe -PathType Leaf)) { throw "No game executable produced by duckdodgers target." }
Write-Host "Full native executable produced at $exe (gameplay still requires separate validation)"
