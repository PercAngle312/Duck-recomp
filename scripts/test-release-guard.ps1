$ErrorActionPreference = "Stop"
# A smoke-test EXE must never be published as a playable game.
$temp = Join-Path $env:RUNNER_TEMP "duck-release-guard"
New-Item -ItemType Directory -Path $temp -Force | Out-Null
try {
  & "$PSScriptRoot/package-release.ps1" -Exe (Join-Path $temp "missing-DuckDodgers.exe") -Staging (Join-Path $temp "stage") -OutputZip (Join-Path $temp "release.zip")
  throw "FAIL: packaging a missing game binary was permitted"
} catch {
  if ($_.Exception.Message -match "FAIL:") { throw }
  Write-Host "PASS: missing binary correctly rejected"
}
$dummy = Join-Path $temp "memory_test.exe"
[IO.File]::WriteAllBytes($dummy, [byte[]](0x4d,0x5a))
try {
  & "$PSScriptRoot/package-release.ps1" -Exe $dummy -Staging (Join-Path $temp "stage") -OutputZip (Join-Path $temp "release.zip")
  throw "FAIL: smoke test was accepted as playable binary"
} catch {
  if ($_.Exception.Message -match "FAIL:") { throw }
  Write-Host "PASS: test EXE correctly rejected"
}
if (Test-Path (Join-Path $temp "release.zip")) { throw "FAIL: invalid release artifact was created" }
