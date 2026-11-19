@echo off
rem ============================================================================
rem  FC3 co-op TWO PCs over RADMIN / ДВА ПК ПО RADMIN
rem  Run this on EACH PC after both are in one Radmin VPN network.
rem  PC that hosts:   fc3_lan_radmin.bat host
rem  PC that joins:   fc3_lan_radmin.bat client <HOST-Radmin-IPv4>
rem  Join with the HOST's Radmin IP (the 26.x / 21.x address from Radmin).
rem  Allow Windows Firewall for the game on UDP 9000-9005 (both directions).
rem  See docs/fc3-lan-coop-test.md for the full checklist.
rem ============================================================================
setlocal
set GAME=H:\FarCryCoop\TestFC3_105
set ROLE=%1
set CLI=%2
set MAP=coop_vidar
set SAVE=H:\FarCryCoop\save_net
if not exist "%SAVE%" mkdir "%SAVE%"
if /I "%ROLE%"=="host" (
  echo HOST on this PC. Find your Radmin IP (Radmin window / IPCONFIG 26.*).
  echo On the OTHER PC run:  fc3_lan_radmin.bat client ^<this-PC-radmin-ip^>
  start "FC3-host" "%GAME%\bin\farcry3_d3d11.exe" -host %MAP% -login Host -coop -SavePath "%SAVE%" -xres 1280 -yres 720 -skipintro -quickstartup
  goto :end
)
if /I "%ROLE%"=="client" (
  if "%CLI%"=="" (
    echo usage: fc3_lan_radmin.bat client ^<HOST-Radmin-IPv4^>
    exit /b 1
  )
  echo CLIENT joining %CLI% ...
  start "FC3-client" "%GAME%\bin\farcry3_d3d11.exe" -client -login Guest -ip %CLI% -coop -SavePath "%SAVE%" -xres 1280 -yres 720 -skipintro -quickstartup
  goto :end
)
echo usage:
echo   fc3_lan_radmin.bat host
echo   fc3_lan_radmin.bat client ^<HOST-Radmin-IPv4^>
:end
endlocal
