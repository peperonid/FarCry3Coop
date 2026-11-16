@echo off
rem ============================================================================
rem run_a2_stage0.bat - A2 Stage 0 (NOTES 28cg+): reach the co-op menu WITHOUT the in-process
rem fake-online flag. We do NOT enable coop_shim (its forced online_flag=1 woke netconfig
rem FUN_1126dc60 which crashed offline - see 28cg). Instead: the loader proxy fakes ONLY the
rem Ubisoft SDK account presence (FC3COOP_FAKE_ONLINE) so the game boots outside upc, and the
rem game does its OWN online-init against our local servers (hosts -> 127.0.0.1, run_capture
rem serves fc3_config.json on :80). Goal: see how far the REAL client gets and capture the exact
rem login/gconnect/RV request it makes (gator_http_log.txt / gator_udp_log.txt).
rem
rem PRECONDITIONS (chat steps): hosts_patch.py apply (admin) + run_capture.py (admin, running).
rem Deploy the loader proxy first: python -I scripts\deploy_proxy_only.py deploy
rem ============================================================================
setlocal EnableExtensions
set "BIN=H:\FarCryCoop\TestFC3_105\bin"
if not "%~1"=="" set "BIN=%~1\bin"

rem guard: coop_shim must NOT be present (its hooks would force online=1 and crash on netconfig)
if exist "%BIN%\coop_shim.dll" ( echo ERROR: coop_shim.dll is in bin - REMOVE it for Stage 0 & exit /b 1 )
if not exist "%BIN%\farcry3_d3d11.exe" ( echo ERROR: farcry3_d3d11.exe missing & exit /b 1 )

set FC3COOP_FAKE_ONLINE=1
rem (deliberately NO FC3COOP_SHIM)

echo A2 Stage 0: proxy FAKE_ONLINE only (no coop_shim).
echo   loader proxy must be deployed; hosts applied; gator_mock\run_capture.py running in another admin window.
echo   Launching... then go to Multiplayer - Co-op and note the EXACT behaviour + any error text.
start "FC3-a2s0" "%BIN%\farcry3_d3d11.exe" -language=Russian -skipintro -quickstartup
endlocal
