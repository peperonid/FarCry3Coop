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

set "ROLE=%~1"
set "CLI=%~2"
set "SAVE=%~dp0save_lan"
if not exist "%SAVE%" mkdir "%SAVE%"
rem Same physical LAN (no VPN). Host: fc3_lan_same_subnet.bat host  Client: ... client HOST-LAN-IPv4
if /I "%ROLE%"=="host" (
  echo HOST. Find this PC LAN IPv4 in ipconfig for the client.
  start "FC3-host" "%EXE%" -host coop_vidar -login Host -coop -SavePath "%SAVE%" -xres 1280 -yres 720 -skipintro -quickstartup
  goto :end
)
if /I "%ROLE%"=="client" (
  if "%CLI%"=="" (
    echo CLIENT no -ip: in-game Co-op, Local Session, Browse to join via LAN broadcast.
    start "FC3-client" "%EXE%" -client -login Guest -coop -SavePath "%SAVE%" -xres 1280 -yres 720 -skipintro -quickstartup
  ) else (
    echo CLIENT direct-connect to %CLI% ...
    start "FC3-client" "%EXE%" -client -login Guest -ip %CLI% -coop -SavePath "%SAVE%" -xres 1280 -yres 720 -skipintro -quickstartup
  )
  goto :end
)
echo usage:
echo   fc3_lan_same_subnet.bat host
echo   fc3_lan_same_subnet.bat client [HOST-LAN-IPv4]
:end
endlocal
