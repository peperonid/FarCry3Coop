# run_d0_reallogin.ps1 - definitive "does FC3's RendezVous client fire on a GENUINE Ubisoft login?"
# control. Deploys the proxy (transparent forwarder to the genuine loader) + the coop_shim OBSERVER
# ONLY (RV-PROBE / MGR_WATCH / login watcher) on the clean install, and launches the game with NO
# fakes, NO DNS redirect, NO local rv_server/config_server - so the game does a REAL Ubisoft Connect
# login and uses the REAL OnlineConfig/SandboxUrl. Then RV-PROBE (which sits on the RV client
# functions INSIDE FC3_d3d11.dll) tells us, protocol-independently, whether a real login ever makes
# FC3 enter RendezVous - which the netstat watcher (d0_reallogin_watch.ps1) cannot tell us, because
# RV is unconnected UDP that netstat never shows. Companion to NOTES 28f0/28f3.
#
# Run from an ELEVATED PowerShell, from the repo root. The game's genuine online loader is stashed to
# bin\real\ and restored by stop_d0_reallogin.ps1.
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_d0_reallogin.ps1

$ErrorActionPreference = "Stop"
$Repo = Split-Path -Parent $PSScriptRoot
$Game = if ($env:GAME_DIR) { $env:GAME_DIR } else { "H:\FarCryCoop\FC3_clean" }
$Bin  = Join-Path $Game "bin"
$ShimSrc = Join-Path $Repo "src\coop_shim\coop_shim.dll"
if (-not (Test-Path $ShimSrc)) { throw "shim not built: $ShimSrc" }

$cmdPy = Get-Command python.exe -ErrorAction SilentlyContinue | Where-Object { $_.Source -notlike "*\WindowsApps\*" } | Select-Object -First 1
if (-not $cmdPy) { $cmdPy = Get-Command python.exe -ErrorAction SilentlyContinue | Select-Object -First 1 }
if (-not $cmdPy) { throw "python.exe not found" }
$Py = $cmdPy.Source
Write-Host ("[env] python: {0} ; game: {1}" -f $Py, $Game)

# 1) close the game (both names hold bin/uplay_r1_loader.dll)
foreach ($proc in @("farcry3_d3d11","farcry3")) { Get-Process $proc -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue }
Start-Sleep -Seconds 2

# 2) deploy proxy (forwarder) + shim (observer) onto the clean install; verify by size + retry
$ProxySrc = Join-Path $Repo "src\uplay_proxy\uplay_r1_loader.dll"
$BinLoader = Join-Path $Bin "uplay_r1_loader.dll"
$want = (Get-Item $ProxySrc).Length
$landed = $false
for ($i = 1; $i -le 6; $i++) {
    & $Py -I "$Repo\scripts\deploy_proxy_only.py" deploy $Game
    if ((Test-Path $BinLoader) -and ((Get-Item $BinLoader).Length -eq $want)) { $landed = $true; break }
    foreach ($proc in @("farcry3_d3d11","farcry3")) { Get-Process $proc -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue }
    Start-Sleep -Seconds 3
}
if (-not $landed) { throw "DEPLOY FAILED: bin loader != proxy ($want bytes). Close the game + Ubisoft Connect fully and retry." }
Copy-Item $ShimSrc (Join-Path $Bin "coop_shim.dll") -Force
Write-Host ("[verify] bin/uplay_r1_loader.dll = proxy ({0} bytes)" -f (Get-Item $BinLoader).Length)

# 3) clear observer logs
foreach ($f in @("coop_shim.log","uplay_api.log")) { Set-Content -Path (Join-Path $Bin $f) -Value ("# cleared " + (Get-Date -Format HH:mm:ss)) }

# 4) launch with OBSERVER-ONLY flags: NO FAKE_ONLINE, NO FAKE_CREDS, NO DNS_REDIRECT, NO BLOCK_LOGOUT,
#    and do NOT start rv_server/config_server. Real Ubisoft Connect login + real OnlineConfig.
$env:FC3COOP_SHIM        = "1"
$env:FC3COOP_WATCH_LOGIN = "1"
$env:FC3COOP_RV_PROBE    = "1"
$env:FC3COOP_RV_MD5      = "1"
$env:FC3COOP_RV_ENC      = "1"
$env:FC3COOP_MGR_WATCH   = "1"
$env:FC3COOP_LOG_PUMP    = "1"
Start-Process -FilePath (Join-Path $Bin "farcry3_d3d11.exe") -ArgumentList @("-skipintro","-quickstartup")

Start-Sleep -Seconds 15
$alive = Get-Process farcry3_d3d11 -ErrorAction SilentlyContinue
$shimSz = (Get-Item (Join-Path $Bin "coop_shim.log")).Length
Write-Host ""
Write-Host ("[engagement] game : {0}" -f $(if ($alive) {"RUNNING pid $($alive.Id)"} else {"NOT RUNNING"}))
Write-Host ("[engagement] coop_shim.log bytes : {0}  (>11 => observer LOADED)" -f $shimSz)
if ($shimSz -le 11) { Write-Host "[engagement] WARNING: shim not logging - RV-PROBE will be silent; run is INCONCLUSIVE." }
Write-Host ""
Write-Host "== real-login RV-PROBE harness up =="
Write-Host "UI STEPS:"
Write-Host "  1) Log in to Ubisoft Connect for real; reach the main menu."
Write-Host "  2) Multiplayer -> Co-op -> Host -> connect; wait ~60s on whatever screen shows."
Write-Host "  3) Then: powershell -NoProfile -ExecutionPolicy Bypass -File scripts\stop_d0_reallogin.ps1"
Write-Host "     (it restores the clean loader) and send me the coop_shim.log RV-PROBE / RV-ENC lines."
Write-Host "VERDICT: any 'RV-PROBE' RUNTIME hit during Co-op => FC3 enters RendezVous on a real login."
Write-Host "         'RV-ENC' lines = each RV packet buffer BEFORE and AFTER the driver encrypts it."
Write-Host "         RV-ENC ... payload_changed=1 => the PRE/POST pair is a plaintext/ciphertext pair => the RC4 keystream = pt^ct (NO secret)."
Write-Host "         RV-ENC ... payload_changed=0 => the driver encrypts a copy; we then add a driver-side capture (one more hook, same harness)."
