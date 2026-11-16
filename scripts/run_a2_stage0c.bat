@echo off
rem ============================================================================
rem run_a2_stage0c.bat - A2 Stage0c (NOTES 28cn): let FCX build a REAL online manager.
rem Instead of writing a zeroed fake into [0x11e42c90] (which crashed the menu, 28ci), coop_shim in
rem FAKE_MGR=3 mode waits ~20s then CALLS the game's own online-manager creator FUN_10420a77, so FCX
rem constructs a properly built manager. Clean mode: NO online_flag write and NO code hooks (avoids
rem the 28cg netconfig crash). Outcome is read from coop_shim.log ("MGR_FORCER global after=...").
rem If a real manager appears -> co-op gate FUN_10443663 may pass without a crash (big step).
rem If still NULL -> online-init is truly gated behind a live RV connection (must emulate RV first).
rem PRECONDITIONS: loader proxy deployed (boot outside upc) + coop_shim.dll deployed in bin.
rem ============================================================================
setlocal EnableExtensions
set "BIN=H:\FarCryCoop\TestFC3_105\bin"
if not "%~1"=="" set "BIN=%~1\bin"
if not exist "%BIN%\coop_shim.dll" ( echo ERROR: coop_shim.dll not deployed to "%BIN%" & exit /b 1 )
if not exist "%BIN%\farcry3_d3d11.exe" ( echo ERROR: game exe missing & exit /b 1 )

set FC3COOP_SHIM=1
set FC3COOP_FAKE_MGR=3
set FC3COOP_FAKE_ONLINE=1
set FC3COOP_SECRET=fc3coop-experiment

echo A2 Stage0c: coop_shim FAKE_MGR=3 (call FUN_10420a77 to build a REAL manager; no online flag, no hooks).
echo   shim log: "%BIN%\coop_shim.log"   - wait ~20s, look for 'MGR_FORCER global after='
echo   Launching... then wait ~25s, THEN try Multiplayer - Co-op -> Host. Report game behaviour + log tail.
start "FC3-a2s0c" "%BIN%\farcry3_d3d11.exe" -language=Russian -skipintro -quickstartup
endlocal
