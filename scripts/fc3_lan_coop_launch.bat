@echo off
REM fc3_lan_coop_launch.bat - EXPERIMENTAL LAN/loopback co-op probe for Far Cry 3 (Uplay 1.06 copy).
REM
REM Purpose: the decisive test for NOTES 28x. Does the storm::echo LAN/loopback join
REM path (lan::inEvent::DiscoverSession) work WITHOUT the RendezVous JobTicket, or does
REM it still require the service-minted SessionTicket? If it joins -> same-LAN co-op is
REM ticket-exempt and much cheaper. If it fails with a ticket/INVALIDSESSIONKEY error ->
REM the LAN path is NOT exempt and we need the client-side ticket patch (B) or emulation.
REM
REM Run this from a FULL COPY of the game (never the real install). Edit GAME_DIR first.
REM Multi-instance on one PC needs the loopback tunable ON and separate profiles; on a
REM second physical PC use its LAN IP instead of 127.0.0.1 and skip the loopback note.
REM
REM Set the echo tunable in the game settings/config (NOT on the cmdline; it is a runtime
REM tunable per NOTES 28w FUN_107a227d): LanMatchmakingDiscoveryOnLoopbackEnable=1,
REM LanMatchmakingReuseAddress=1, and the "LAN Match Making Port" option (OPTIONS_NETWORK_LANMATCHMAKINGPORT).

setlocal
set GAME_DIR=H:\FarCryCoop\TestFC3_105
set EXE=%GAME_DIR%\bin\farcry3.exe

echo [1/2] Launch HOST instance (-coop). Bindings: UDP 9000 on the first adapter.
start "FC3-HOST" "%EXE%" -coop
echo     ... then in the menu: Host / LAN / create a Local LAN co-op session.
echo     Wait until the world is loaded and the session is open BEFORE launching the client.
timeout /t 45 /nobreak >nul

echo [2/2] Launch CLIENT instance (-client -ip ^<host-ip^>). Binds UDP 9001.
REM Use the HOST machine's LAN IP. For one-PC loopback with the tunable on, 127.0.0.1 may work.
start "FC3-CLIENT" "%EXE%" -client -ip 127.0.0.1

echo.
echo OBSERVE AND RECORD (this is the experiment's output):
echo  - Does the client load the host's world, or pop "service unavailable" / "Invalid ticket"?
echo  - Any SessionTicket / GameSessionKey error (E_SESSION_ERROR_*TICKET*, INVALIDSESSIONKEY)?
echo  - If a log/uplay_api.log is produced, grep it for: Ticket, JobTicketManager, AcquireTicket,
echo    INVALIDSESSIONKEY, LanMatchmaking, DiscoverSession.
echo Decision: join OK with NO ticket error == LAN path is ticket-exempt (cheap route).
echo           ticket error == LAN path NOT exempt; do the (B) client patch (see NOTES 28x targets).
endlocal
