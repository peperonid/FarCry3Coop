@echo off
rem ============================================================================
rem  FC3 co-op TWO PCs ON THE SAME REAL LAN / ДВА ПК В ОДНОЙ РЕАЛЬНОЙ ЛОКАЛЬНОЙ СЕТИ
rem  No VPN needed. LAN mode uses broadcast discovery on UDP 9004/9005.
rem  Host PC : fc3_lan_same_subnet.bat host
rem  Client PC: fc3_lan_same_subnet.bat client <HOST-LAN-IPv4 e.g. 192.168.1.10>
rem            (or just:  fc3_lan_same_subnet.bat client  -> relies on in-game
rem             LAN browser, you may not even need the -ip for a real LAN)
rem  Open the firewall BOTH ways on BOTH PCs first: run fc3_lan_firewall.bat (admin).
rem  Checklist: docs/fc3-lan-coop-test.md
rem ============================================================================
setlocal
set GAME=H:\FarCryCoop\TestFC3_105
set ROLE=%1
set CLI=%2
set MAP=coop_vidar
set SAVE=H:\FarCryCoop\save_lan
if not exist "%SAVE%" mkdir "%SAVE%"
if /I "%ROLE%"=="host" (
  echo HOST. Show this PC's LAN IP (run:  ipconfig  ^| findstr IPv4) for the client.
  start "FC3-host" "%GAME%\bin\farcry3_d3d11.exe" -host %MAP% -login Host -coop -SavePath "%SAVE%" -xres 1280 -yres 720 -skipintro -quickstartup
  goto :end
)
if /I "%ROLE%"=="client" (
  if "%CLI%"=="" (
    echo CLIENT: launching without -ip. In-game use Co-op ^> Local Session ^> Browse
    echo         and join the session shown via LAN broadcast. To direct-connect run:
    echo             fc3_lan_same_subnet.bat client ^<HOST-LAN-IPv4^>
    start "FC3-client" "%GAME%\bin\farcry3_d3d11.exe" -client -login Guest -coop -SavePath "%SAVE%" -xres 1280 -yres 720 -skipintro -quickstartup
  ) else (
    echo CLIENT direct-connect to %CLI% ...
    start "FC3-client" "%GAME%\bin\farcry3_d3d11.exe" -client -login Guest -ip %CLI% -coop -SavePath "%SAVE%" -xres 1280 -yres 720 -skipintro -quickstartup
  )
  goto :end
)
echo usage:
echo   fc3_lan_same_subnet.bat host
echo   fc3_lan_same_subnet.bat client [HOST-LAN-IPv4]
:end
endlocal
