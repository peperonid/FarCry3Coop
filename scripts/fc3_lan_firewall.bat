@echo off
rem ============================================================================
rem  FC3 co-op LAN firewall helper (run from an ELEVATED prompt) / Помощник фаервола
rem  Opens UDP 9000-9005 both directions for the FC3 game dir. Run on BOTH PCs.
rem ============================================================================
setlocal
set GAME_DIR=H:\FarCryCoop
netsh advfirewall firewall add rule name="FC3 coop LAN in"  dir=in  action=allow protocol=UDP localport=9000-9005
netsh advfirewall firewall add rule name="FC3 coop LAN out" dir=out action=allow protocol=UDP remoteport=9000-9005
netsh advfirewall firewall add rule name="FC3 coop exe in"  dir=in  action=allow program="%GAME_DIR%\TestFC3_105\bin\farcry3_d3d11.exe" enable=yes
netsh advfirewall firewall add rule name="FC3 coop exe in B" dir=in action=allow program="%GAME_DIR%\TestFC3_105_B\bin\farcry3_d3d11.exe" enable=yes
echo Done. To remove later:
echo   netsh advfirewall firewall delete rule name="FC3 coop LAN in"
echo   netsh advfirewall firewall delete rule name="FC3 coop LAN out"
echo   netsh advfirewall firewall delete rule name="FC3 coop exe in"
echo   netsh advfirewall firewall delete rule name="FC3 coop exe in B"
endlocal
