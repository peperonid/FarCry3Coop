# d0_status.ps1 - read the Strategy-D Stage D0 outcome after you drove the game (run after run_d0.ps1).
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\d0_status.ps1
# Prints a PASS/FAIL/UNKNOWN verdict for "did FC3 ever dial the local RendezVous server".

$Repo = Split-Path -Parent $PSScriptRoot
$Game = if ($env:GAME_DIR) { $env:GAME_DIR } else { "H:\FarCryCoop\TestFC3_105" }
$Bin  = Join-Path $Game "bin"
$RvDir = Join-Path $Repo "scripts\rv_server"

function Show([string]$label,[string]$path,[int]$tail=15){
    Write-Host ""
    Write-Host "===== $label : $path ====="
    if (Test-Path $path) {
        Get-Content $path -Tail $tail -ErrorAction SilentlyContinue
    } else { Write-Host "(missing)" }
}

$rvlog = Join-Path $RvDir "rv_capture.log"
$shim  = Join-Path $Bin   "coop_shim.log"
$cfg   = Join-Path $RvDir "config_server.out"
$api   = Join-Path $Bin   "uplay_api.log"

# 1) did the game fetch OnlineConfig?
$cfgHits = if (Test-Path $cfg) { (Select-String -Path $cfg -Pattern "GET" -SimpleMatch -ErrorAction SilentlyContinue).Count } else { 0 }
# 2) did RV traffic reach the PRUDP server?
$rvSyn = if (Test-Path $rvlog) { (Select-String -Path $rvlog -Pattern "SYN|ESTABLISHED" -ErrorAction SilentlyContinue).Count } else { 0 }
# 3) did the shim see RV connect/send attempts?
$rvProbe = if (Test-Path $shim) { (Select-String -Path $shim -Pattern "RV-PROBE" -ErrorAction SilentlyContinue).Count } else { 0 }
# 4) login state + manager
$login = if (Test-Path $shim) { (Select-String -Path $shim -Pattern "LOGIN-EV" -ErrorAction SilentlyContinue).Count } else { 0 }
$mgr   = if (Test-Path $shim) { (Select-String -Path $shim -Pattern "MGR-WATCH|RV sub|this\+0x110" -ErrorAction SilentlyContinue | Select-Object -Last 3) } else { $null }
# 5) storm::echo LAN session on UDP 9000 (the local-host data plane)
$lan9000 = (netstat -ano 2>$null | Select-String -Pattern ":9000\b" -ErrorAction SilentlyContinue)

Write-Host "=================== D0 RESULT ==================="
Write-Host ("OnlineConfig GETs   : {0}" -f $cfgHits)
Write-Host ("rv_server SYN/ESTAB : {0}" -f $rvSyn)
Write-Host ("shim RV-PROBE hits  : {0}" -f $rvProbe)
Write-Host ("shim LOGIN-EV lines : {0}" -f $login)
Write-Host ("UDP :9000 (storm echo) :" -f "")
if ($lan9000) { $lan9000 | Select-Object -First 4 | ForEach-Object { Write-Host ("   "+$_.ToString().Trim()) } } else { Write-Host "   (none)" }

Write-Host ""
if ($rvSyn -ge 1 -or $rvProbe -ge 1) {
    Write-Host ">>> PASS: FC3 dialed the local RendezVous server. Strategy D is TRACTABLE -> next stage D1 (prudps)."
} elseif ($rvSyn -eq 0 -and $rvProbe -eq 0) {
    Write-Host ">>> FAIL: FC3 never dialed RV (zero inbound + zero RV-PROBE). Strategy D (and B') DEAD; only mod-262 SOLO."
} else {
    Write-Host ">>> UNKNOWN: ambiguous counts; inspect the logs below."
}

Show "rv_capture.log (PRUDP packets)" $rvlog 25
Show "coop_shim.log tail (RV-PROBE / LOGIN-EV / DNS-LOOKUP)" $shim 30
if ($mgr) { Write-Host ""; Write-Host "MGR-WATCH sample:"; $mgr | ForEach-Object { Write-Host ("  "+$_.Line.Trim()) } }
Show "config_server.out" $cfg 10
Show "uplay_api.log tail" $api 12
