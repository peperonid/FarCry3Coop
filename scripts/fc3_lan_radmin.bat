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
set "MAP=%~3"
if "%MAP%"=="" set "MAP=coop_vidar"
rem ==========================================================================
rem  Radmin/Hamachi 2-PC co-op.  Host PC: fc3_lan_radmin.bat host
rem  Join PC: fc3_lan_radmin.bat client  HOST-Radmin-IPv4
rem  Join uses the engine direct-connect (-client -ip): there is no LAN browser.
rem  Also: open firewall UDP 9000-9005 on both (fc3_lan_firewall.bat) and
rem  disable extra virtual NICs (vEthernet WSL/Hyper-V) so the host binds 26.x.
rem ==========================================================================
if /I "%ROLE%"=="host" (
  echo HOST. Find your Radmin IP in ipconfig - a 26.x address - for the other PC.
  echo On the OTHER PC run:  fc3_lan_radmin.bat client ^<this-PC-radmin-ip^>
  start "FC3-host" "%EXE%" -host %MAP% -login Host -coop -SavePath "%SAVE%" -xres 1280 -yres 720 -skipintro -quickstartup
  goto :end
)
if /I "%ROLE%"=="client" (
  if "%CLI%"=="" ( echo usage: fc3_lan_radmin.bat client ^<HOST-Radmin-IPv4^> & exit /b 1 )
  echo CLIENT joining %CLI% ...
  start "FC3-client" "%EXE%" -client -login Guest -ip %CLI% -coop -SavePath "%SAVE%" -xres 1280 -yres 720 -skipintro -quickstartup
  goto :end
)
echo usage:
echo   fc3_lan_radmin.bat host [map]
echo   fc3_lan_radmin.bat client ^<HOST-Radmin-IPv4^> [map]
:end
endlocal
