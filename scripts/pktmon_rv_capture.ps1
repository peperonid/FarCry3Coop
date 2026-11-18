# pktmon_rv_capture.ps1 - capture the REAL RendezVous traffic FC3 sends during a Co-op attempt on a
# GENUINE Ubisoft login (NOTES 28f5 follow-up: we proved the RV client fires; this records WHERE it
# dials and WHAT it sends, the golden reference the local-RV-emulator plan needs). Uses Windows'
# built-in pktmon - NO game patch, NO proxy/shim, NO hosts change. Elevated PowerShell.
#
#   1) launch the clean game normally, log in to Ubisoft for real
#   2) run: powershell -NoProfile -ExecutionPolicy Bypass -File scripts\pktmon_rv_capture.ps1 [seconds]
#   3) the moment it says "GO", in-game do Multiplayer -> Co-op -> Host and stay ~the full window
#   4) when it stops, it writes an .etl, a .pcap and a .txt to scripts\rv_capture\ and prints paths;
#      send me those paths (I parse the pcap to list RV UDP destinations + packet bytes).
#
# Output dir: scripts\rv_capture\ . pktmon needs admin; if pktmon start fails, the cause prints here.

param([int]$Seconds = 120)
$ErrorActionPreference = "Stop"
$Repo = Split-Path -Parent $PSScriptRoot
$OutDir = Join-Path $Repo "scripts\rv_capture"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$etl  = Join-Path $OutDir "rv-$stamp.etl"
$pcap = Join-Path $OutDir "rv-$stamp.pcap"
$txt  = Join-Path $OutDir "rv-$stamp.txt"

# best effort: clear any old filter so we capture all traffic
pktmon filter remove 2>$null | Out-Null

# pktmon build on this PC has no --net-data and rejects --comp here; the plain 'start --capture -f'
# is the supported form (NOTES 28i7.1: --net-data returned exit 87 and --comp nics made start fail).
Write-Host "[pktmon] starting capture ..."
& pktmon start --capture -f $etl
if ($LASTEXITCODE -ne 0) {
    Write-Host "[pktmon] start -f failed (exit $LASTEXITCODE); retrying minimal 'pktmon start --capture'"
    & pktmon start --capture
    if ($LASTEXITCODE -ne 0) { throw "pktmon start failed - run this script from an ELEVATED PowerShell" }
}

Write-Host ""
Write-Host "================ GO NOW ================"
Write-Host "In-game: Multiplayer -> Co-op -> Host, keep it trying for the whole $Seconds s window."
Write-Host "Capturing..."
Start-Sleep -Seconds $Seconds

Write-Host "[pktmon] stopping ..."
& pktmon stop | Out-Null
Start-Sleep -Seconds 2

# pktmon writes the .etl to the path given with -f, or to the configured logger file (often the CWD / repo
# root PktMon.etl, or %USERPROFILE%). Search all of them for the newest .etl written in the last 15 minutes.
$cut = (Get-Date).AddMinutes(-15)
$search = @($OutDir, $Repo, (Get-Location).Path, $env:USERPROFILE) | Select-Object -Unique
$cand = foreach ($d in $search) { Get-ChildItem -Path $d -Filter "*.etl" -ErrorAction SilentlyContinue }
$src = $cand | Where-Object { $_.LastWriteTime -gt $cut } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $src) { throw "no .etl produced; check that pktmon ran elevated (looked in: $($search -join '; '))" }
Write-Host ("[pktmon] etl: {0}" -f $src.FullName)

& pktmon etl2pcap $src.FullName -o $pcap | Out-Null
& pktmon etl2txt  $src.FullName | Out-Null   # writes <etl>.txt next to the etl
$txtpath = "$($src.FullName).txt"

Write-Host ""
Write-Host "================ capture done ================"
Write-Host ("pcap : {0}" -f $pcap)
Write-Host ("txt  : {0}" -f $txtpath)
if (Test-Path $pcap) { Write-Host ("pcap size: {0} bytes" -f (Get-Item $pcap).Length) }
Write-Host "Send me the pcap path (I parse RV UDP destinations + payload bytes)."
