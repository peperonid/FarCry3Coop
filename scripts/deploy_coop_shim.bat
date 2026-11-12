@echo off
rem deploy_coop_shim.bat - copy the BUILT coop_shim.dll + proxy uplay_r1_loader.dll into the FC3 working copy's bin\,
rem with a timestamped backup of whatever was there, then drop a launcher run_coop_shim.bat with the env stub.
rem RUN MANUALLY. Build the two DLLs first:
rem     cd src\uplay_proxy  & build_x86.bat
rem     cd src\coop_shim    & build_x86.bat
rem
rem The launcher turns on FC3COOP_SHIM=1 + FC3COOP_FAKE_ONLINE=1 (proxy fakes the Uplay online-service overlapped op,
rem NOTES 28al). Comment out FC3COOP_FAKE_ONLINE in run_coop_shim.bat for a plain run.
rem
rem SAFETY: this only writes into GAME_DIR (default the working copy). It NEVER touches the real install
rem (H:\Uplay\Far Cry 3), which is read-only by project rule. Edit GAME_DIR or pass it as the 1st argument.
setlocal EnableExtensions

set "REPO=%~dp0.."
set "GAME_DIR=H:\FarCryCoop\TestFC3_105"
if not "%~1"=="" set "GAME_DIR=%~1"
set "BIN=%GAME_DIR%\bin"
set "SRC_PROXY=%REPO%\src\uplay_proxy\uplay_r1_loader.dll"
set "SRC_SHIM=%REPO%\src\coop_shim\coop_shim.dll"
set "EXE=%BIN%\farcry3.exe"

echo === deploy_coop_shim ===
echo   repo      : %REPO%
echo   game dir  : %GAME_DIR%

rem ---- guards ----
if /I "%GAME_DIR%"=="H:\Uplay\Far Cry 3" ( echo ERROR: GAME_DIR is the real install; refusing. & exit /b 1 )
if /I "%GAME_DIR%"=="H:\Uplay\Far Cry 3\" ( echo ERROR: GAME_DIR is the real install; refusing. & exit /b 1 )
if not exist "%BIN%\" ( echo ERROR: bin folder not found: "%BIN%" & exit /b 1 )
if not exist "%SRC_PROXY%" ( echo ERROR: built proxy missing: "%SRC_PROXY%" & echo   build: cd src\uplay_proxy ^& build_x86.bat & exit /b 1 )
if not exist "%SRC_SHIM%"  ( echo ERROR: built shim missing: "%SRC_SHIM%"  & echo   build: cd src\coop_shim   ^& build_x86.bat & exit /b 1 )

rem ---- timestamp for backups ----
set "TS=backup"
for /f %%t in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd-HHmmss"') do set "TS=%%t"

rem ---- backup + deploy proxy ----
if exist "%BIN%\uplay_r1_loader.dll" copy /y "%BIN%\uplay_r1_loader.dll" "%BIN%\uplay_r1_loader.dll.bak-%TS%" >nul && echo   backup : uplay_r1_loader.dll.bak-%TS%
copy /y "%SRC_PROXY%" "%BIN%\uplay_r1_loader.dll" >nul && echo   deploy : uplay_r1_loader.dll

rem ---- backup + deploy shim ----
if exist "%BIN%\coop_shim.dll" copy /y "%BIN%\coop_shim.dll" "%BIN%\coop_shim.dll.bak-%TS%" >nul && echo   backup : coop_shim.dll.bak-%TS%
copy /y "%SRC_SHIM%" "%BIN%\coop_shim.dll" >nul && echo   deploy : coop_shim.dll

rem ---- launcher with the env stub (edit FC3COOP_SECRET before the join test; %* forwards extra args) ----
> "%GAME_DIR%\run_coop_shim.bat" (
  echo @echo off
  echo rem Launch FC3 with the co-op shim enabled. Set FC3COOP_SECRET the SAME on every peer.
  echo set FC3COOP_SHIM=1
  echo rem proxy: fake the Uplay online-service overlapped op so co-op Host passes service unavailable - NOTES 28al
  echo set FC3COOP_FAKE_ONLINE=1
  echo set FC3COOP_SECRET=change-me-same-on-all-peers
  echo start "FC3" "%EXE%" %%*
)
echo   launcher: %GAME_DIR%\run_coop_shim.bat

echo.
echo Done. Manual alternative (if you launch the game yourself, set these in the SAME console):
echo     set FC3COOP_SHIM=1
echo     set FC3COOP_FAKE_ONLINE=1
echo     set FC3COOP_SECRET=^<same on all peers^>
echo Then: the shim logs to bin\coop_shim.log (RUN step 5); run run_coop_shim.bat, go to co-op Host.
echo Rollback: delete bin\coop_shim.dll and restore the .bak proxy.

endlocal
