@echo off
rem ============================================================================
rem run_a2_rvcapture.bat - live-capture harness for condition B (S0).
rem Brings up the two halves the live capture needs:
rem   (1) rv_server.py  - PRUDP RV emulator + full hex capture on 127.0.0.1:50000
rem   (2) config_server.py - serves a crafted OnlineConfig whose SandboxUrl points at that port
rem Then YOU (admin) redirect onlineconfigservice.ubi.com -> 127.0.0.1 and launch the game.
rem EXPECTED today: zero inbound UDP, because the game only dials RV once FCX reaches LoggingIn
rem (condition A). That zero-UDP is itself the clean proof the gate is FCX login, not the socket layer.
rem ============================================================================
setlocal EnableExtensions
set "REPO=D:\Projects\Co-op Anything\FarCry3Coop-bantila"
set "RVS=%REPO%\scripts\rv_server"

echo [1/3] starting rv_server (PRUDP capture) on 127.0.0.1:50000 ...
start "FC3-rv-capture" cmd /k "python -I "%RVS%\rv_server.py" --host 127.0.0.1 --port 50000"

echo [2/3] starting config_server on 127.0.0.1:8081 (SandboxUrl -> 127.0.0.1:50000) ...
start "FC3-config-seam" cmd /k "python -I "%RVS%\config_server.py" --port 8081 --rv-port 50000"

echo [3/3] OWNER STEPS (admin):
echo   a) redirect the config host to localhost (or use gator_mock/hosts_patch.py apply):
echo        echo 127.0.0.1 onlineconfigservice.ubi.com ^>^> %WINDIR%\System32\drivers\etc\hosts   (as admin)
echo      NOTE: hosts redirect changes front-end behavior; revert after (gator_mock/hosts_patch.py restore).
echo   b) if you bind the config server to :80 you must run config_server --port 80 as admin;
echo      otherwise the game hits the real (dead) config service and will not pick up the SandboxUrl.
echo   c) launch the game, open Multiplayer - Co-op, then inspect the rv_server window log
echo      and scripts\rv_server\rv_capture.log for any PRUDP SYN (Connection Request).
echo.
echo Rollback: gator_mock\hosts_patch.py restore ; close the two python windows.
endlocal
