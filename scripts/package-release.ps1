param(
  [Parameter(Mandatory=$true)][string]$Exe,
  [string]$Staging = "dist/DuckDodgers-Windows",
  [string]$OutputZip = "dist/DuckDodgers-Windows.zip",
  [string[]]$RuntimeDlls = @(),
  [string[]]$Notices = @()
)
$ErrorActionPreference = "Stop"
if (!(Test-Path -LiteralPath $Exe -PathType Leaf)) { throw "Playable game EXE not found: $Exe. Refusing to publish a test-only archive." }
if ([IO.Path]::GetFileName($Exe) -ne "DuckDodgers.exe") { throw "Release requires DuckDodgers.exe, not a smoke-test executable." }
if (Test-Path $Staging) { Remove-Item -LiteralPath $Staging -Recurse -Force }
New-Item -ItemType Directory -Force -Path $Staging | Out-Null
Copy-Item -LiteralPath $Exe -Destination (Join-Path $Staging "DuckDodgers.exe")
foreach ($dll in $RuntimeDlls) {
  if (!(Test-Path -LiteralPath $dll -PathType Leaf)) { throw "Missing runtime dependency: $dll" }
  Copy-Item -LiteralPath $dll -Destination (Join-Path $Staging ([IO.Path]::GetFileName($dll)))
}
foreach ($notice in $Notices) {
  if (!(Test-Path -LiteralPath $notice -PathType Leaf)) { throw "Missing license notice: $notice" }
  $noticeDir = Join-Path $Staging "licenses"
  New-Item -ItemType Directory -Force -Path $noticeDir | Out-Null
  Copy-Item -LiteralPath $notice -Destination (Join-Path $noticeDir ([IO.Path]::GetFileName($notice)))
}
New-Item -ItemType Directory -Force -Path (Join-Path $Staging "roms") | Out-Null
Copy-Item -LiteralPath "docs/ROM-SETUP.md" -Destination (Join-Path $Staging "README-ROM.txt")
$forbidden = Get-ChildItem -LiteralPath $Staging -Recurse -File | Where-Object { $_.Extension -match '(?i)^\.(z64|n64|v64|rom|sav|eep|sra|fla)$' }
if ($forbidden) { throw "Proprietary game files or saves found in release staging. Refusing packaging." }
# Include no external installation steps. Dependency closure must be verified by the release job.
if (Test-Path $OutputZip) { Remove-Item -LiteralPath $OutputZip -Force }
Compress-Archive -LiteralPath $Staging -DestinationPath $OutputZip -CompressionLevel Optimal
Write-Host "Package created: $OutputZip"
