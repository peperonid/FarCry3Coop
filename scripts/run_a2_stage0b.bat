@echo off
rem ============================================================================
rem run_a2_stage0b.bat - A2 Stage0b (NOTES 28ci): minimal collateral-free test of the co-op gate.
rem The co-op Host action FUN_10443663 returns the "service unavailable" error (code 4) IF AND ONLY IF
rem the online session-manager global [0x11e42c90] == 0 (null offline). coop_shim in FAKE_MGR_ONLY mode
rem forces ONLY that global to a valid fake object (vptr = real manager class vtable) and does NOT write
rem the online flag nor patch any code - so the 28cg netconfig crash (which the online_flag=1 wake caused)
rem is avoided. If this clears "service unavailable" the gate is confirmed and the next step is to get the
rem endpoint getter FUN_11279ba0 to attempt RV (which needs online flag, handled carefully later).
rem PRECONDITIONS: loader proxy deployed (boot outside upc) + coop_shim.dll deployed in bin.
rem ============================================================================
setlocal EnableExtensions
set "BIN=H:\FarCryCoop\TestFC3_105\bin"
if not "%~1"=="" set "BIN=%~1\bin"
if not exist "%BIN%\coop_shim.dll" ( echo ERROR: coop_shim.dll not deployed to "%BIN%" & exit /b 1 )
if not exist "%BIN%\farcry3_d3d11.exe" ( echo ERROR: game exe missing & exit /b 1 )

set FC3COOP_SHIM=1
set FC3COOP_FAKE_MGR_ONLY=1
set FC3COOP_FAKE_ONLINE=1
set FC3COOP_SECRET=fc3coop-experiment

echo A2 Stage0b: coop_shim FAKE_MGR_ONLY (only [0x11e42c90] forced; NO online flag, NO code hooks).
echo   shim log: "%BIN%\coop_shim.log"
echo   Launching... then go Multiplayer - Co-op. NOTE whether "service unavailable" is GONE and what happens on Host.
start "FC3-a2s0b" "%BIN%\farcry3_d3d11.exe" -language=Russian -skipintro -quickstartup
endlocal
