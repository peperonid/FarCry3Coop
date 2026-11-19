# Far Cry 3 campaign co-op - project status (locked snapshot)

> Bilingual: English here - [русская версия](project-status.ru.md)
> Snapshot date: 2026-11-14. This file records **what works today, what the frontier is, and the
> structural wall we hit**, so any future session can resume without re-deriving anything. Private
> local paths stay as placeholders (`<GAME_DIR>`, `<UPC_DIR>`).

## 1. What works today (the shipped deliverable)

**Solo offline co-op on vanilla 1.06 + mod 262 works** (owner-confirmed 2026-11-13).
Reproduce it with the operational recipe in [fc3-coop-mod-setup.md](fc3-coop-mod-setup.md)
(bilingual). This is a real, playable offline co-op baseline on the current build
(1.06 install + 1.05-era FC3 binaries, `FC3_d3d11.dll` md5 `46bcc097`).

## 2. The frontier we were chasing

**Multi-human (2-4 players) join to a local/private co-op session** without Ubisoft services and
without port forwarding. This needs the game's own online flow (create/join) to run against an
emulated RendezVous (RV)/GATOR back-end (route **A2**), because every in-process shortcut was
proven structurally fragile.

## 3. The structural wall (final, traced end to end)

The RV dial is **not a callable routine**:
- The RV back-end connect (`JobBackEndServicesLogin::ConnectToAuthenticationService`) is a Quazal
  **job-vtable method** run by the JobManager only when FCX's login state machine reaches
  `LoggingIn`.
- FCX reaches `LoggingIn` only when it holds valid **UbiAuth passport credentials**, which
  **Ubisoft Connect** fetches at boot via `gconnect`/`agora`. Those strings are **not in any
  game-folder DLL** - the code lives in the Ubisoft Connect install.
- Our offline boot (`FC3COOP_FAKE_ONLINE`) starts the game but never authenticates, so FCX never
  reaches `LoggingIn`, no login job is enqueued, RV is never dialled, and the online session
  manager's RV sub-object (`manager+0x110`) stays null -> a deterministic null-deref.
- Forcing any online gate in-process only relocates the null-deref to the next unpopulated RV
  object (netconfig, menu-init, ServiceMaster). The boundary is **outside our binary**.

Key evidence: NOTES sessions 28cg, 28ci, 28ck, 28cl, 28co, 28cq; Ghidra artifacts
`rv_connect.txt`, `rv_trigger.txt`, `gamestartlogin.txt`, `dis_fault.txt`, `online_init.txt`.

## 4. Decision (2026-11-14)

1. **Lock in** the mod-262 solo offline co-op baseline above as the practical deliverable.
2. **Open the Ubisoft Connect frontier**: reverse-engineer how Ubisoft Connect boot-auth populates
   FCX's online credentials, to find a way to make FCX reach `LoggingIn` and dial a local RV host.
   This is a larger, externally-gated effort (the code is not in the game folder) and needs owner
   approval to read files under the Ubisoft Connect install location. See
   [coop-gator-emulation-plan.md](coop-gator-emulation-plan.md) for the RV protocol design that the
   local emulator would target.

## 5. Safety state at this snapshot

- Working game copy `TestFC3_105\bin` is **rolled back to a clean mod-262** install (real loaders,
  no experiment shim). Game plays normally.
- `hosts` is clean (no redirects). `GamerProfile.xml` session provider is default.
- All experiments live under ignored `local/` and `scripts/` only.


## Final gate (sessions 28df-28dh, 2026-11-16)

We made FCX hold a stable online login offline (proxy credential success + fake online +
block LogoutOperation): the login reaches LoggingIn(6) + LoggingInSucceeded(9) and stays. That
was the long-believed-impossible step. But opening Co-op -> Host still issues no session/RV call,
and the game never resolves any DNS name - FCX's online network engine never initialises under any
loader-proxy configuration (even forwarding the real loader's 14k events). The offline (mod-262)
loader cannot emit the genuine "authenticated/connected to Ubisoft" event that wakes the FCX online
session, because that event only comes from the live Ubisoft Connect (upc.exe) backend.

So the offline co-op frontier is now pinned to one thing: waking FCX's online net engine, which is
gated on real Ubisoft Connect authentication. Emulating that is a separate, large undertaking
(route A3b - Ubisoft Connect / upc.exe backend emulator). Reaching a real RV session from the
loader-proxy side is not possible without it.

Locked practical deliverable: mod-262 SOLO offline co-op (working). The RV/PRUDP tooling
(prudp codec, rv_server handshake, config_server, getaddrinfo DNS-redirect) is committed and ready
for any future route that can produce a genuine authenticated event.


## Final verdict (sessions 28dg-28dq) / Final verdict

After exhausting every offline-callable seam, the campaign-co-op question is settled for the
hook/proxy layer:

- Login CAN be faked and made to stick (FC3COOP_FAKE_CREDS + FC3COOP_BLOCK_LOGOUT); the net engine
  comes up without FAKE_ONLINE (LAN UDP:9000, UPnP); the online session-manager singleton
  (DAT_11e42c90) is NON-NULL already at the menu (the old "Host gate == 0" idea was wrong).
- But Co-op -> Host still hangs on "connecting to Far Cry 3 servers", polling UPLAY_USER_IsConnected
  forever. It NEVER reaches RendezVous: no RV connect (ConnectToAuthenticationService), no
  RVCreateGameSession/SetGameSession, no DNS, no packets to our local RV/PRUDP emulator.
- Forcing the online-enable global, forcing the session-manager object, forcing every overlapped
  async op to "completed + success" (FC3COOP_FORCE_FO), faking party/leader/IsConnected, and
  faking credentials do NOT move it past that point.

The gate is a genuine Ubisoft co-op BACKEND state - "online service connected" plus the
party / matchmaking / session events delivered through the Uplay event pump - that only a LIVE
Ubisoft co-op service produces. Online, that service is dead (OW15 and vanilla online both say
"servers unavailable"). Offline, the mod-262 loader fakes Uplay for SOLO only and never emits those
events. No proxy/hook seam we have can synthesize them.

Conclusion: the only achievable FC3 co-op today is mod-262 SOLO (one human, offline). 2+ human
co-op (offline or online) requires reimplementing the Ubisoft co-op backend (auth + party +
matchmaking + session + the RV login that follows), which is a large, separate emulation project.
The RV/PRUDP tooling (codec, rv_server handshake, config_server, getaddrinfo DNS-redirect) and the
shim/proxy experiment flags remain committed as the groundwork for that project.

See also: docs/ubisoft-coop-emulation-recon.md (open-source Ubisoft/Quazal online emulator landscape + FC3 co-op-backend emulation effort/risk, 28ds-28dt).


## Update 2026-11-17 (NOTES 28f0) - Strategy D live-tested and CLOSED

The last open route, Strategy D (local RendezVous/NetConnect emulator), was tested end-to-end via the
Stage D0 capture experiment (scripts/run_d0.ps1). With the harness provably engaged (proxy + shim live,
FCX login stuck at LoggingInSucceeded, session-manager non-null, config DNS-redirect active), FC3 made
**zero** RendezVous attempts (RV-PROBE runtime 0, rv_capture 0 packets, no SetGameSession/CreateSession).
The co-op "connecting to servers" wait is gated upstream of RendezVous on the FCX online-service /
party-session state, so there is no RV traffic for any local emulator to serve. **Strategy D is dead;
Strategy B' (LAN provider) shares the same NetConnect/RV gate and is dead too.** This closes every
multi-human route. **The only working Far Cry 3 co-op remains mod-262 SOLO (1 human).**


## UPDATE 2026-11-17 (NOTES 28f5) - the D0 "FAIL / D dead" above is SUPERSEDED

The D0 FAIL (28f0) used FAKE credentials. A control on a clean 1.05 with a REAL Ubisoft Connect
login (run_d0_reallogin.ps1) showed the FC3 RendezVous client FIRES: RV-PROBE hit 17 times on
RV WSASendTo (FUN_11214630) while attempting Co-op. So FC3 DOES dial RendezVous when the login is
genuine - the "gate is above RV / D is dead" reading was an artifact of the fake login. Strategy D /
A2 (local RendezVous emulator) is REOPENED: the next step is to capture the real RV handshake
(redirect the RV destination to rv_server under a real login) to obtain the golden reference traffic.
Risks unchanged: prudps crypto, full RV service replies, and the separate high-level Ubisoft party/
matchmaking layer. Netstat watch (28f2/28f4) only saw gconnect:80 (web) because it cannot see the
RV UDP; the in-process RV-PROBE does, and it is positive.


## UPDATE 2026-11-17 (NOTES 28f7) - golden RV reference OBTAINED (was the last blocker)

The pktmon capture of a genuine-login Co-op attempt captured FC3 dialing Ubisoft's real RendezVous
servers: lb-rdv-as-prod01.ubisoft.com:21251 and ne1-z3-as-rdv04.ubisoft.com:21251, and the RV server
REPLIES (transport alive). Transport is prudps-secured Quazal RV (header: magic-pair + cleartext
connectionId + seq + RC4/HMAC body + checksum), NOT plain PRUDP v1. So the old "no golden RV reference"
killer risk is partly lifted: we now have real RV vectors both ways (scripts/rv_capture/, gitignored).
Decisive next steps: (A) offline RE the FC3 prudps key exchange (Ghidra 0x112xxxxx) to decrypt the
captured payloads and read the real RV methods; (B) hosts-redirect lb-rdv-*.ubisoft.com -> rv_server and
replay the captured handshake to see if the client advances. A2 feasibility now rests on prudps key
derivation, not on "we will never see the traffic".

## 6. Status update 2026-11-18 - Ubisoft shut FC3 online servers 2024-03-06 (external, decisive)

Independent community sources confirm Ubisoft **shut down Far Cry 3's online servers on 2024-03-06**; multiplayer and
online co-op are no longer supported. This makes the entire RV/PRUDP online route (A2/A3b) moot for a
playable co-op: there is no server to reach, and NOTES 28i9 showed the `+0x618` game-session cipher
handler is only installed post-matchmaking, so the encrypted co-op traffic never even starts. The
**online co-op goal is therefore permanently out of reach**; the RV-cipher work is archived as research
only (NOTES 28g8-28i9). The **only** remaining path to 2+ human co-op is the game's own LAN/Storm
provider over a virtual LAN (Radmin/Hamachi) - the community-proven method and the subject of
[r2-lan-provider-plan.md](r2-lan-provider-plan.md). mod 262 solo offline co-op stays the shipped baseline.
