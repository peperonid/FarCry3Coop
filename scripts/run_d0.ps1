# run_d0.ps1 - Strategy-D Stage D0 capture harness (docs/strategy-d-rv-emulation-plan.md, NOTES 28eb).
#
# PURPOSE: decisive go/no-go for Strategy D. With the net-on config (NO FAKE_ONLINE so FCX's
# net engine + RendezVous client initialise) and the OnlineConfig seam pointing SandboxUrl at our
# local PRUDP server, drive the game to Co-op->Host and find out whether FC3 EVER dials an RV
# server at all. Either we capture >=1 PRUDP SYN reaching rv_server and handshaking to
# `established` (D is tractable), or we prove zero inbound UDP (D and B' are dead; only mod-262
# SOLO remains, per NOTES 28dj which already saw RV-PROBE=0 on a real co-op-host attempt).
#
# MUST run from an ELEVATED PowerShell: config_server binds :80 (the port FC3 fetches OnlineConfig
# on) and the shim redirects onlineconfigservice.ubi.com -> 127.0.0.1 by an IAT hook (no hosts edit).
#
# Usage (from repo root):
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_d0.ps1
# Then in-game do the UI steps printed at the end. Roll back with:
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\stop_a2_livecap.ps1

$ErrorActionPreference = "Stop"
$Repo = Split-Path -Parent $PSScriptRoot
$Game = if ($env:GAME_DIR) { $env:GAME_DIR } else { "H:\FarCryCoop\TestFC3_105" }
$Bin  = Join-Path $Game "bin"
$RvDir = Join-Path $Repo "scripts\rv_server"
$ShimSrc = Join-Path $Repo "src\coop_shim\coop_shim.dll"

if (-not (Test-Path $ShimSrc)) { throw "shim not built: $ShimSrc (run scripts\deploy_coop_shim.bat or build first)" }

# 1) stop any running game
Get-Process farcry3_d3d11 -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2

# 2) deploy proxy (over bin/uplay_r1_loader.dll) + copy shim; clear logs
python -I "$Repo\scripts\deploy_proxy_only.py" deploy
Copy-Item $ShimSrc (Join-Path $Bin "coop_shim.dll") -Force
foreach ($f in @("coop_shim.log","uplay_api.log")) { $p = Join-Path $Bin $f; if (Test-Path $p) { Set-Content -Path $p -Value "" } }
foreach ($f in @("rv_capture.log","config_server.out","rv_server.out")) { $p = Join-Path $RvDir $f; if (Test-Path $p) { Set-Content -Path $p -Value "" } }

# 3) start rv_server (:50000) + config_server (:80, advertises SandboxUrl -> prudp://127.0.0.1:50000)
Start-Process -FilePath "python" -ArgumentList @("-I","rv_server.py","--host","127.0.0.1","--port","50000") -WorkingDirectory $RvDir -WindowStyle Hidden
Start-Process -FilePath "python" -ArgumentList @("-I","config_server.py","--host","0.0.0.0","--port","80","--rv-port","50000") -WorkingDirectory $RvDir -WindowStyle Hidden
Start-Sleep -Seconds 2

# 4) launch the game NET-ON (NO FAKE_ONLINE) with RV + manager observers on
$env:FC3COOP_SHIM        = "1"
$env:FC3COOP_WATCH_LOGIN = "1"
$env:FC3COOP_BLOCK_LOGOUT= "1"
$env:FC3COOP_FAKE_CREDS  = "1"
$env:FC3COOP_DNS_REDIRECT= "1"
$env:FC3COOP_LOG_PUMP    = "1"
$env:FC3COOP_RV_PROBE    = "1"   # log every RV connect/send attempt in the 0x112xxxxx block
$env:FC3COOP_MGR_WATCH   = "1"   # poll the online session-manager global DAT_11e42c90 / RV sub-object this+0x110
# deliberately NOT setting FC3COOP_FAKE_ONLINE (it suppresses the net engine - NOTES 28di)
Start-Process -FilePath (Join-Path $Bin "farcry3_d3d11.exe") -ArgumentList @("-skipintro","-quickstartup")

Write-Host ""
Write-Host "== D0 harness up =="
Write-Host "rv_server udp :50000 ; config_server http :80 ; game launching (menu)."
Write-Host ""
Write-Host "UI STEPS (report exactly where you get stuck):"
Write-Host "  1) Wait for the main menu (login should stick: LOGIN-EV 3..9..13, no 15 loop)."
Write-Host "  2) Multiplayer -> Co-op -> Host (any map/playlist) -> start/connect."
Write-Host "  3) Wait ~30-60s on whatever screen it shows (connecting / servers unavailable / etc)."
Write-Host ""
Write-Host "READ THE RESULT:"
Write-Host "  PASS (D tractable) : scripts\rv_server\rv_capture.log shows a SYN + 'CONNECTION ESTABLISHED'"
Write-Host "                     and/or coop_shim.log shows RV-PROBE connect/sendto hits."
Write-Host "  FAIL (D dead)      : rv_capture.log has ZERO inbound and RV-PROBE never fires => record + stop."
Write-Host "  Did the game even fetch OnlineConfig? scripts\rv_server\config_server.out (200 lines)."
Write-Host ""
Write-Host "Collect logs with: powershell -NoProfile -ExecutionPolicy Bypass -File scripts\d0_status.ps1"
Write-Host "Tear down with   : powershell -NoProfile -ExecutionPolicy Bypass -File scripts\stop_a2_livecap.ps1"
