@echo off
rem Opens UDP 9000-9005 both directions and allows the game exe. Run on BOTH PCs as Administrator.
setlocal EnableExtensions
set "GAME="
if defined FC3_GAME set "GAME=%FC3_GAME%"
if "%GAME%"=="" if exist "%~dp0game_dir.txt" set /p "GAME=<%~dp0game_dir.txt"
if "%GAME%"=="" set /p "GAME=Far Cry 3 install folder (contains bin\farcry3_d3d11.exe): "
if not exist "%GAME%\bin\farcry3_d3d11.exe" ( echo ERROR: not a Far Cry 3 folder: "%GAME%" & exit /b 1 )

netsh advfirewall firewall add rule name="FC3 coop LAN in"  dir=in  action=allow protocol=UDP localport=9000-9005
netsh advfirewall firewall add rule name="FC3 coop LAN out" dir=out action=allow protocol=UDP remoteport=9000-9005
netsh advfirewall firewall add rule name="FC3 coop exe in"  dir=in action=allow program="%GAME%\bin\farcry3_d3d11.exe" enable=yes
echo Done. Remove later:
echo   netsh advfirewall firewall delete rule name="FC3 coop LAN in"
echo   netsh advfirewall firewall delete rule name="FC3 coop LAN out"
echo   netsh advfirewall firewall delete rule name="FC3 coop exe in"
endlocal
