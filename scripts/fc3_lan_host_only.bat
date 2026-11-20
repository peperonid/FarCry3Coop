@echo off
setlocal EnableExtensions
rem ==========================================================================
rem  Game folder resolution (no hard-coded path). Priority:
rem    1) environment variable FC3_GAME
rem    2) file game_dir.txt next to this bat (one line with the path)
rem    3) interactive prompt
rem  To make a permanent choice, create a text file game_dir.txt in this bat
rem  folder whose single line is your Far Cry 3 install path.
rem ==========================================================================
set "GAME="
if defined FC3_GAME set "GAME=%FC3_GAME%"
if "%GAME%"=="" if exist "%~dp0game_dir.txt" set /p "GAME=<%~dp0game_dir.txt"
if "%GAME%"=="" set /p "GAME=Far Cry 3 install folder (contains bin\farcry3_d3d11.exe): "
if not exist "%GAME%\bin\farcry3_d3d11.exe" ( echo ERROR: not a Far Cry 3 folder: "%GAME%" & exit /b 1 )
set "EXE=%GAME%\bin\farcry3_d3d11.exe"
set "SAVE=%~dp0save_coop"
if not exist "%SAVE%" mkdir "%SAVE%"

set "MAP=%~1"
if "%MAP%"=="" set "MAP=coop_vidar"
set "SAVE=%~dp0save_host"
if not exist "%SAVE%" mkdir "%SAVE%"
rem Host-only boot to confirm the Local/LAN host engine comes up. Then Co-op, Local Session, Host.
echo HOST ONLY  map=%MAP%  profile=%SAVE%
start "FC3-host" "%EXE%" -host %MAP% -login Host -coop -SavePath "%SAVE%" -xres 900 -yres 640 -skipintro -quickstartup
endlocal
