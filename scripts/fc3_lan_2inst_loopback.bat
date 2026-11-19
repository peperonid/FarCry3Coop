@echo off
rem ============================================================================
rem  FC3 co-op TWO INSTANCES ON ONE PC (loopback) / ДВА ИНСТАНСА НА ОДНОМ ПК (loopback)
rem  Hosts instance A, then a 2nd instance B as client pointed at 127.0.0.1.
rem  Uses Bantila -host/-client/-coop recipe on farcry3_d3d11.exe + separate
rem  -SavePath profiles so the two installs do not fight over one GamerProfile.
rem  Requires the 2nd instance folder TestFC3_105_B (a clone of TestFC3_105).
rem  CAVEAT: single-PC loopback is the RISKY path; the ROBUST path is two PCs
rem          over Radmin (see fc3_lan_radmin.bat). If B cannot see A, use Radmin.
rem ============================================================================
setlocal
set A=H:\FarCryCoop\TestFC3_105
set B=H:\FarCryCoop\TestFC3_105_B
set SAVEA=H:\FarCryCoop\save_A
set SAVEB=H:\FarCryCoop\save_B
if not exist "%SAVEA%" mkdir "%SAVEA%"
if not exist "%SAVEB%" mkdir "%SAVEB%"
set MAP=coop_vidar
if not exist "%B%\bin\farcry3_d3d11.exe" (
  echo [ERROR] 2nd instance not found: %B%
  echo Run the clone first. Aborting.
  exit /b 1
)
echo Launching HOST (A) ... map=%MAP%
start "FC3-A-host" "%A%\bin\farcry3_d3d11.exe" -host %MAP% -login Host -coop -SavePath "%SAVEA%" -xpos 0 -ypos 0 -xres 900 -yres 640 -skipintro -quickstartup
echo.
echo  >>> In the HOST window: reach Co-op -> Local Session -> Host, wait until the
echo  >>> session is up. THEN come back here and press any key to launch the CLIENT.
pause
echo Launching CLIENT (B) -> 127.0.0.1 ...
start "FC3-B-client" "%B%\bin\farcry3_d3d11.exe" -client -login Guest -ip 127.0.0.1 -coop -SavePath "%SAVEB%" -xpos 920 -ypos 0 -xres 900 -yres 640 -skipintro -quickstartup
echo Done. In the CLIENT window: Co-op -> Local Session -> Browse/Join.
endlocal
