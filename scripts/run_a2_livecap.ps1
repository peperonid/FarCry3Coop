# run_a2_livecap.ps1 - one-shot A2 LIVE-capture harness (NOTES 28di).
# Brings up the net-on experiment and launches the game WITHOUT FAKE_ONLINE so FCX's online/net
# engine initialises (SSDP/UPnP + storm::echo LAN UDP:9000). Captures OnlineConfig + RV dial.
# Net-on key finding (28di): FC3COOP_FAKE_ONLINE SUPPRESSES the net engine; omit it and the network
# stack comes up offline. DNS of onlineconfigservice.ubi.com is redirected to 127.0.0.1 by the shim's
# IAT hook (FC3COOP_DNS_REDIRECT=1) - no C:\Windows hosts edit needed.
#
# Usage (from the repo root):
#   powershell -NoProfile -ExecutionPolicy Bypass -File scripts\run_a2_livecap.ps1
# Then in-game: Multiplayer -> Co-op -> Host, and report. Read the logs:
#   scripts\rv_server\config_server.out   (did the game fetch OnlineConfig?)
#   scripts\rv_server\rv_capture.log       (did it dial RendezVous?)
#   H:\FarCryCoop\TestFC3_105\bin\coop_shim.log   (DNS-LOOKUP / LOGIN-EV / LOGOUT-BLOCK)
#   H:\FarCryCoop\TestFC3_105\bin\uplay_api.log    (UPLAY calls incl RVCreateGameSession tag)
# Roll everything back with: powershell -NoProfile -ExecutionPolicy Bypass -File scripts\stop_a2_livecap.ps1

$ErrorActionPreference = "Stop"
$Repo = Split-Path -Parent $PSScriptRoot
$Game = if ($env:GAME_DIR) { $env:GAME_DIR } else { "H:\FarCryCoop\TestFC3_105" }
$Bin  = Join-Path $Game "bin"
$RvDir = Join-Path $Repo "scripts\rv_server"
$ShimSrc = Join-Path $Repo "src\coop_shim\coop_shim.dll"

# 1) stop any running game (PS-native, non-terminating even with ErrorActionPreference=Stop)
Get-Process farcry3_d3d11 -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2

# 2) deploy proxy + shim
python -I "$Repo\scripts\deploy_proxy_only.py" deploy
Copy-Item $ShimSrc (Join-Path $Bin "coop_shim.dll") -Force
foreach ($f in @("coop_shim.log","uplay_api.log")) { $p = Join-Path $Bin $f; if (Test-Path $p) { Set-Content -Path $p -Value "" } }
foreach ($f in @("rv_capture.log","config_server.out","rv_server.out")) { $p = Join-Path $RvDir $f; if (Test-Path $p) { Set-Content -Path $p -Value "" } }

# 3) start rv_server (:50000) + config_server (:80) detached
Start-Process -FilePath "python" -ArgumentList @("-I","rv_server.py","--host","127.0.0.1","--port","50000") -WorkingDirectory $RvDir -WindowStyle Hidden
Start-Process -FilePath "python" -ArgumentList @("-I","config_server.py","--host","0.0.0.0","--port","80","--rv-port","50000") -WorkingDirectory $RvDir -WindowStyle Hidden
Start-Sleep -Seconds 2

# 4) launch the game in the NET-ON config (NO FAKE_ONLINE)
$env:FC3COOP_SHIM = "1"
$env:FC3COOP_WATCH_LOGIN = "1"
$env:FC3COOP_BLOCK_LOGOUT = "1"
$env:FC3COOP_FAKE_CREDS = "1"
$env:FC3COOP_DNS_REDIRECT = "1"
$env:FC3COOP_LOG_PUMP = "1"
# deliberately NOT setting FC3COOP_FAKE_ONLINE (it suppresses the net engine - NOTES 28di)
Start-Process -FilePath (Join-Path $Bin "farcry3_d3d11.exe") -ArgumentList @("-skipintro","-quickstartup")
Write-Host "A2 live-capture running: game at menu, config_server :80, rv_server :50000."
Write-Host "Now open Multiplayer -> Co-op -> Host and watch the logs. Stop with stop_a2_livecap.ps1."
