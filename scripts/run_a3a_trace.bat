@echo off
rem ============================================================================
rem run_a3a_trace.bat - A3a ground-truth trace (NOTES 28cs/28ct).
rem Goal: capture the EXACT UPLAY_* call sequence + arguments the game makes while
rem driving Multiplayer -> Co-op (Host) with the WORKING mod-262 online stack, so the
rem proxy's local party/session/event emulation (A3a) can be built against the truth.
rem HOW: our uplay_r1_loader proxy in FRONT of the mod's real loader (real\uplay_r1_loader.dll),
rem forwarding everything (NO fake flags, so the game behaves exactly as a normal mod-262 session),
rem but FC3COOP_LOG_PUMP=1 makes it log EVERY UPLAY_* call incl the Update/GetNextEvent hot loop.
rem OUTPUT: TestFC3_105\bin\uplay_api.log  (also tags RVCreateGameSession/RVJoinGameSession/party region).
rem PRECONDITION: clean mod-262 in bin (this script deploys the proxy over it; rollback = restore).
rem ============================================================================
setlocal EnableExtensions
set "ROOT=H:\FarCryCoop\TestFC3_105"
set "REPO=D:\Projects\Co-op Anything\FarCry3Coop-bantila"

echo [1/3] deploying our uplay_r1_loader proxy in front of the real mod-262 loader (pure forward)...
python -I "%REPO%\scripts\deploy_proxy_only.py" deploy || ( echo deploy failed & exit /b 1 )

echo [2/3] starting the game in PURE FORWARD + FULL LOG mode (no fake online/party)...
set FC3COOP_LOG_PUMP=1
set FC3COOP_SHIM=
set FC3COOP_FAKE_ONLINE=
set FC3COOP_FAKE_PARTY=
start "FC3-a3a-trace" "%ROOT%\bin\farcry3_d3d11.exe" -language=Russian -skipintro -quickstartup

echo [3/3] NOW in the game: go to Multiplayer -> Co-op -> Host (create a co-op session), wait ~10s,
echo       then try to view/refresh the friends/join list, then quit to desktop.
echo       Log lands at: "%ROOT%\bin\uplay_api.log"
echo       After quitting, roll back: python -I "%REPO%\scripts\deploy_proxy_only.py" restore
endlocal
