@echo off
rem ============================================================================
rem fc3_splitscreen_coop.bat - EXPERIMENT: Far Cry 3 local split-screen co-op on ONE PC
rem (no network, no Ubisoft session provider). Uses the engine's hidden dev switches
rem -splitscreen / -splitscreen4 (found in FC3.dll, NOTES 28bw).
rem
rem We do NOT deploy the loader proxy here (that risks the known-good mod-262 boot).
rem PRIMARY path: launch the game's OWN local launcher bin\farcry3.exe WITH the switches.
rem   farcry3.exe relaunches farcry3_d3d11.exe through upc.exe and (per NOTES 809-819)
rem   PRESERVES the arguments. upc must be running and logged in. This keeps the real
rem   loader and a legit Ubisoft startup, so it is the safe first try.
rem FALLBACK path (commented): if the args get dropped, boot OUTSIDE upc with the loader
rem   proxy + FC3COOP_FAKE_ONLINE=1 (deploy it first with scripts\deploy_coop_shim.bat).
rem
rem HARDWARE NOTE: FC3 split-screen wants XInput gamepads (Xbox 360-style). 2-player
rem split usually needs 1-2 pads, 4-player wants up to 4. Keyboard+mouse is the host
rem only. Start with 2-player (-ss2) to prove the mode, then try 4 (-ss4).
rem ============================================================================
setlocal EnableExtensions

set "GAME_DIR=H:\FarCryCoop\TestFC3_105"
if not "%~1"=="" set "GAME_DIR=%~1"
set "BIN=%GAME_DIR%\bin"
set "LAUNCHER=%BIN%\farcry3.exe"

if not exist "%LAUNCHER%" ( echo ERROR: not found "%LAUNCHER%" & exit /b 1 )

if /i "%~2"=="-ss4" ( set "SS=-splitscreen4" ) else ( set "SS=-splitscreen" )

echo.
echo  Split-screen co-op probe:  %SS%  (players via XInput pads; host = KB+mouse)
echo  Launcher: %LAUNCHER%
echo  Close Ubisoft Connect popups if any; keep Ubisoft Connect running and logged in.
echo.

start "FC3-SS" "%LAUNCHER%" %SS% -coop -language=Russian -skipintro -quickstartup

echo.
echo  Launched. Watch for: does the game start a LOCAL co-op with multiple players, or
echo  still show "Far Cry 3 service is not available"? Report which, plus a screenshot.
echo.
echo  If the switches were IGNORED (normal single-player co-op only), use the fallback:
echo     scripts\deploy_coop_shim.bat    (installs the loader proxy into bin, with backups)
echo     then in a console:  set FC3COOP_FAKE_ONLINE=1
echo     run: "%BIN%\farcry3_d3d11.exe" %SS% -coop -language=Russian -skipintro -quickstartup
echo ============================================================================
endlocal
