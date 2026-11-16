@echo off
rem ============================================================================
rem run_coop_shim_sandbox.bat - A2/LAN de-risk EXPERIMENT (NOTES 28ca).
rem Launch FC3 directly (loader proxy fakes Ubisoft online, NOTES 28by) with the coop_shim
rem in DEFAULT mode: it short-circuits the co-op Host RV-auth (hk_host) so the game's OWN
rem storm::echo op forms the co-op session (target: native LAN, NetEngineType=LAN) with no
rem Ubisoft RendezVous. We then watch coop_shim.log + a UDP sniffer to see if a real session
rem forms (the storm::echo packets are what we would relay). NOT HOOK_PROVIDER (that probes
rem the real RV path instead of short-circuiting).
rem
rem PREREQS (do these first, see the chat steps):
rem   1) scripts\deploy_proxy_only.py deploy   (loader proxy in bin)   [already done]
rem   2) coop_shim.dll present in bin          [already done]
rem   3) scripts\gator_mock\hosts_patch.py apply   (admin)  -> redirects online-config to 127.0.0.1
rem   4) scripts\gator_mock\run_capture.py         (admin)  -> HTTP :80 serves fc3_config.json (SandboxUrl 127.0.0.1:50000) + UDP sniffer on 50000
rem
rem This is an UNTESTED SCAFFOLD (RUN.md): the hooks may crash the game. That is still useful:
rem report the faulting module+offset and the last "shim:" line in bin\coop_shim.log.
rem ============================================================================
setlocal EnableExtensions
set "BIN=H:\FarCryCoop\TestFC3_105\bin"
if not "%~1"=="" set "BIN=%~1\bin"

if not exist "%BIN%\coop_shim.dll"      ( echo ERROR: coop_shim.dll missing in "%BIN%" & exit /b 1 )
if not exist "%BIN%\farcry3_d3d11.exe"  ( echo ERROR: farcry3_d3d11.exe missing in "%BIN%" & exit /b 1 )

set FC3COOP_SHIM=1
set FC3COOP_FAKE_ONLINE=1
set FC3COOP_SECRET=fc3coop-experiment
rem optional probes (uncomment if needed):
rem set FC3COOP_FAKE_MGR=1        rem force a fake RV manager
rem set FC3COOP_HOOK_PROVIDER=1   rem PROBE real RV provider vtable (do NOT use with the LAN short-circuit test)

echo Launching: %BIN%\farcry3_d3d11.exe  (SHIM=1 FAKE_ONLINE=1)
echo   shim log: %BIN%\coop_shim.log
echo   Now go to Multiplayer - Co-op - HOST a session. Watch coop_shim.log for 'shim: host -> ...'
echo   and whether the world loads (LAN session formed) or it crashes.
start "FC3-shim" "%BIN%\farcry3_d3d11.exe" -language=Russian -skipintro -quickstartup
endlocal
