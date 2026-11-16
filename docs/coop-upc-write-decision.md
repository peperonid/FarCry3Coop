# Condition A - should we WRITE into Ubisoft Connect (upc.exe / uplay_r14.dll on C:)?

Decision record + patch plan. Status: **recommend NOT writing to C: now** (see "Recommendation").
No C: file was modified for this decision; the analysis is read-only. Companion:
`docs/project-status.md`, NOTES sessions 28cj/28ck/28cl/28cq/28cu/28cv/28cy.

## What condition A is
FC3 shows "co-op service unavailable" purely from the game's own FCX online LOGIN STATE (NOT a
server round-trip). The co-op Host action FUN_10443663 returns "unavailable" exactly when the
game-wide online session-manager singleton `DAT_11e42c90 == 0` (null offline). To make multi-human
LAN join work we need the FCX login state machine to reach `LoggingIn`, which runs
`JobBackEndServicesLogin -> JobConnectEndPoint ->` the RV (PRUDP) dial. Today that never happens.

## Key facts that decide the C: write question (all read-only RE)
1. **The auth is NOT a Ubisoft-signed passport gate.** FC3 login is the Quazal RendezVous
   UbiAuthentication service over PRUDP using DUMMY credentials from config
   (`DummyUbiUser@quazal.com` / `UbiDummyPwd`, key `h7fyctiuucf`). We control BOTH endpoints, so
   our RV server accepts whatever the client presents; no Ubisoft private key is involved (28cj/28ck).
2. **The real upstream is already dead.** The launcher's `public-ubiservices...?productId=46`
   returns no content (server-side kill, 28cv). Patching upc.exe cannot make a dead server mint a
   real ticket, and the game never demanded one anyway.
3. **Every input upc.exe sends already lands on H:.** Game <-> upc.exe talk over named pipes
   (`PolledPipeSocket` + `PolledPipeProtoBufHandler<uplaydll@protocol@mg>`); the game-side endpoint
   of that pipe is our `uplay_r1_loader` proxy, which we already own and patch on H:.
4. **The loader account-presence surface is already fully fakeable and is NOT the gate.** Our proxy
   forces `UPLAY_USER_IsConnected == 1`, yet the game STILL never advances / never calls the
   friends/session/party APIs (28cu). So "what upc tells the loader" is not what's blocking us.
5. **The whole login->RV bring-up chain is inside FC3_d3d11.dll (H:)** and is Quazal job plumbing:
   - FUN_1041936b (online-init entry): builds the manager (FUN_10420a77 -> DAT_11e42c90), then
     calls the manager vtable+0x10 online-init op.
   - FUN_1127a0d0 (UbiAuthenticationClient.cpp): creates the RV ServiceMaster at `manager+0x110`
     ONLY after the pending connect op at `manager+0x108` reaches state 5 and FUN_11237670 passes.
   - FUN_11275f40 -> FUN_11279ba0 (gated by FUN_11245090) is the co-op Host endpoint/RV path.
   None of this reads or validates a Ubisoft passport signature; it is state machines FCX drives.

## Conclusion
A limited WRITE into C: (upc.exe / Ubisoft Connect's uplay_r14.dll) **would change only what upc
puts on the pipe, and that pipe's game-side end is already ours on H:.** It reaches no lever the
H: side does not already control, the auth has no signature to satisfy, and the actual blocking
trigger (the FCX predicate that decides to enqueue the login op) lives in FC3_d3d11.dll on H:.
Patching C: only adds brick-risk to the owner's whole Ubisoft launcher install (big, anti-tamper,
shared by every Ubisoft game) for zero new capability.

## Recommendation (my consent as the agent)
I do NOT consent to writing to C: at this time, because the analysis shows it targets the wrong
layer. The correct next step is H:-only:
- Find the FCX predicate that decides to enqueue the GatorLogin / advance the login state past
  `FetchOnlineConfigRdy -> LoggingIn` (inside FC3_d3d11.dll), i.e. where FCX consumes the loader
  signal beyond IsConnected (the autologin / account-flags field, not just the bool).
- That consumption point is on H: and is the true gate; satisfying it (or driving the FCX login op
  directly from coop_shim AFTER understanding why the shallow force AV'd in 28cg/28ci) is the path.

## If a C: write ever became warranted (kept as a controlled plan, NOT executed)
Only revisit if a future read-only finding proves the ONE missing input can be produced nowhere
but inside upc.exe. Then, and only then, with a SEPARATE explicit owner go-ahead per AGENTS rule 2:
1. Full backup first: copy the entire Ubisoft Game Launcher directory off C: to H: (size-matched,
   hash-verified) before touching anything.
2. Patch a COPY, never the live file: build a candidate uplay_r14.dll / upc.exe patch offline, and
   test it against a disposable launcher copy, never the one the owner's real account runs.
3. Minimal, single, reversible byte patch with a written before/after diff and a one-command
   restore from the backup; keep the original file hash in the repo.
4. Never patch upc.exe anti-tamper paths; uplay_r14.dll (the loader lib) is a smaller, safer target
   than the upc.exe main binary.
Even so, expectations should be near-zero: facts 1-5 above say the capability is not in that file.

## What this decision does NOT change
- Mod-262 SOLO offline co-op remains the locked, working deliverable.
- Condition B (RV emulator: PRUDP v1 codec + handshake + scaffolded RMC layer) stays built and
  unit-tested; its live capture (S0) still waits on condition A.


## Update 28db - concrete force-point map (found by read-only RE, all on H:)
- Loader credential read: FUN_107ac658 calls UPLAY_USER_GetCredentials(accountObj+0x70, +0x1f8) + GetCdKeys, guarded by FUN_104191f4 (manager-ready, -> FUN_10420a1e; NOT a signature check). This is the FCX-side read of the loader autologin/credential signal.
- Account-flags combiner: FUN_1064acf3 = (bit 0x10 AND UPLAY_USER_IsConnected()) AND (bit 0x40 AND FUN_10418c95->FUN_10421c46 entitlement). Explains why IsConnected alone was not enough (28cu).
- Login-event state machine: FUN_107db689(handle, code) with codes 6=LoggingIn, 9=LoggingInSucceeded, 0x11=Failed, 0xf/0x10=Out/OutSucceeded; gated by FUN_107dea4c (handle must be in the online-login session table, stride 0x50).
Force-point = the loader side (make uplay_r1_loader proxy return valid UPLAY_USER_GetCredentials/GetCdKeys + satisfy entitlement 0x40); then FCX itself should advance the enum via FUN_107db689. No C: write is involved. Next is a shim experiment (proxy returns fabricated credentials + entitlement=1) observed for FUN_107db689 firing code 6/9.
