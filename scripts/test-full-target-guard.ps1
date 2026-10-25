$ErrorActionPreference = "Stop"
$root = Join-Path $env:RUNNER_TEMP "duck-game-cmake-target-guard"
if(Test-Path $root){Remove-Item -LiteralPath $root -Recurse -Force}
$game = Join-Path $root "game"
$recomp = Join-Path $root "recomp"
$modern = Join-Path $root "modern"
$rt64 = Join-Path $root "rt64"
@("game/build/modules","game/build/resident","game/build/runtime","recomp/include","modern","rt64") | ForEach-Object {
  New-Item -ItemType Directory -Path (Join-Path $root $_) -Force | Out-Null
}
@(
  "game/build/modules/funcs_0.c",
  "game/build/resident/funcs_0.c",
  "game/build/runtime/funcs_0.c",
  "game/build/runtime/bindings.cpp",
  "game/build/runtime/audio_rsp.cpp",
  "recomp/include/recomp.h",
  "modern/CMakeLists.txt",
  "rt64/CMakeLists.txt"
) | ForEach-Object {Set-Content -LiteralPath (Join-Path $root $_) -Value "/* synthetic test fixture */"}
Set-Content -LiteralPath (Join-Path $game "CMakeLists.txt") -Value 'cmake_minimum_required(VERSION 3.23)'
$rejected=$false
try {
  & "$PSScriptRoot/build-full-windows.ps1" -GameSource $game -N64Recomp $recomp -N64ModernRuntime $modern -RT64 $rt64 -ConfigureOnly
} catch {
  $message = $_.Exception.Message
  if ($message -notmatch "duckdodgers executable target is missing") { throw }
  $rejected=$true
  Write-Host "PASS: test-only CMake was rejected: $message"
}
if (!$rejected) {throw "FAIL: build launcher accepted game source with no duckdodgers target"}
