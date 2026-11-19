@echo off
rem ============================================================================
rem  FC3 co-op - HOST ONLY (instance A) / ТОЛЬКО ХОСТ (инстанс A)
rem  Start just the host to verify the Local/LAN host session comes up on this
rem  build, BEFORE any client. Uses the Bantila -host recipe on farcry3_d3d11.exe.
rem  Usage:
rem     fc3_lan_host_only.bat            (default map coop_vidar)
rem     fc3_lan_host_only.bat coop_skadi (pick a co-op map)
rem  After the game is at the menu, do:  Co-op > Local/LAN Session > Host.
rem  Then in ANOTHER console run  scripts\fc3_host_engine_check.ps1  to confirm
rem  the LAN/net engine bound its ports (9000 / 9004). See the check script.
rem ============================================================================
setlocal
set A=H:\FarCryCoop\TestFC3_105
set SAVEA=H:\FarCryCoop\save_A
set MAP=%1
if "%MAP%"=="" set MAP=coop_vidar
if not exist "%SAVEA%" mkdir "%SAVEA%"
echo HOST ONLY  map=%MAP%   profile=%SAVEA%
echo Window at 0,0 900x640. In-game: Co-op ^> Local/LAN Session ^> Host.
echo Verify the engine with:  powershell -File H:\...\scripts\fc3_host_engine_check.ps1
start "FC3-A-host" "%A%\bin\farcry3_d3d11.exe" -host %MAP% -login Host -coop -SavePath "%SAVEA%" -xpos 0 -ypos 0 -xres 900 -yres 640 -skipintro -quickstartup
echo.
echo If the game ignores -host (retail 1.06 sometimes does), just navigate the menu
echo to Co-op ^> Local/LAN Session ^> Host manually - mod-262 unlocked it.
endlocal
