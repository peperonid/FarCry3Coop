# Offline campaign co-op: route A (emulate the service) vs route B (patch the client) / Офлайн-кооп: путь A (эмуляция сервиса) против пути B (патч клиента)

Status: decision pending one dynamic fact. Sources: NOTES 28r-28y (source of truth), upstream `Bantila/FarCry3Coop` NOTES,
PR #1. Target build: Uplay 1.06, running module `FC3_d3d11.dll` md5 `46bcc097` (x86, base `0x10000000`).

## Established facts
- Campaign co-op **menu** host/join is hard-bound to the online stack: dispatcher `FUN_10441fb6` -> `FUN_1044123b`/`FUN_10443663`
  -> RendezVous gate `FUN_11279ba0` -> RV `BackEndServices` session (`FUN_112536a0`). No LAN branch in the menu path (28t/28v).
- The engine still carries a **`storm::echo` LAN/loopback** layer: `lan::inEvent::DiscoverSession` / `SetDiscoveryHandler` /
  `BroadcastLanDiscovery2` registered, tunables `LanMatchmakingDiscoveryOnLoopbackEnable`/`ReuseAddress` runtime-writable,
  and session ops `CFCXCreate/JoinSessionOperation`, `CSessionSearchIPOperation`. Session join params (`NetEngineType` id
  `0x3c`, `SessionType`, `SearchIdAddress`, `JoinSessionId`, `MatchMakingEnabled`) are **settable data** (28w).
- **The `SessionTicket` / `GameSessionKey` is minted by the SERVICE (RendezVous `JobTicketManager` + `TicketGrantingProtocol`),
  not the host** (28x). Patch-relevant functions (28y): PrepareCall `FUN_1124e5e0`, ProcessResponse `FUN_1124d660`
  (ticket-inject at `**(param_1+0x78)=*(local_c+0x10)`), ValidateKey `FUN_1124d7b0` (gate `DAT_1174d35a`), DeriveKey
  `FUN_1124e670`; crypto dispatched through a **vtable backend `PTR_PTR_117531f4`** (pluggable).

## The one fact that decides the route
**Does the `storm::echo` LAN/loopback join path require the service-minted `SessionTicket`, or does LAN trust bypass it?**
- If LAN join is **ticket-exempt** -> cheapest route: a session-establishment shim (drive `NetEngineType=LAN` +
  `CFCXCreate/JoinSessionOperation`/`SearchIP`, enable `LanMatchmakingDiscoveryOnLoopbackEnable`). No ticket work.
- If LAN join **demands a ticket** -> route A or B below.
Decisive test: `scripts/fc3_lan_coop_launch.bat` (`-coop` host + `-client -ip`, loopback tunable on) and read for any
`*TICKET*`/`INVALIDSESSIONKEY` error. (Live run is owner-side; Frida attach is blocked by the game's anti-injection.)

## Route A - emulate the Ubisoft online service
Stand up a local service that answers the Rendezvous/NetPlay jobs the client already issues: `JobTicketManagerAcquireTicket`
(TicketGrantingProtocol), login (`JobTicketManagerLogin` DeriveKey/ValidateKey), `SandboxUrl`/OnlineConfig, GameSessionKey.
- Effort: **large, multi-protocol** (PRUDP transport with its crypto framing, ticket signing, session server semantics).
- Risk: high (many unknowns in the wire protocol; upstream flagged this as the heavy option).
- Upside: no game-code patching; a self-contained server a player runs.

## Route B - narrow client patch (leading candidate if LAN is not ticket-exempt)
Make the client satisfy itself without the service:
1. Stop `PrepareCall` (`FUN_1124e5e0`) from networking; fill the out-ticket buffer in `ProcessResponse` (`FUN_1124d660`)
   locally (from the session id / a shared password).
2. Make `ValidateKey` (`FUN_1124d7b0`) / `DeriveKey` (`FUN_1124e670`) accept it: neutralize the `FUN_112547c0` crypto check
   or swap the `PTR_PTR_117531f4` backend for a deterministic local one.
- **Why it works for friends-only co-op:** the ticket crypto only needs to MATCH between peers (both derive the same
  `GameSessionKey`), not be authentic to Ubisoft. All co-op players run the same mod, so the locally-derived key matches on all peers.
- Effort: **small, localized** (4 functions + 1 vtable backend), all in the running module.
- Risk: medium (need to confirm every ticket/key path is covered; the mod must be on every peer).
- Next drill: dump the `PTR_PTR_117531f4` backend vtable to pick the exact swap / the single validate-check to neutralize.

## Recommendation
1. Run the LAN experiment first (may make ticket work moot).
2. If a ticket is needed on LAN, prefer **Route B** (patch the client), and only fall back to Route A if B proves too broad.
3. Keep the work on the upstream 1.05 leading route; cross-map addresses with `scripts/fc3_addrmap.py`.


## Update (28z): Route B crypto is MD5-only (risk LOW)
Backend vtable `0x116c6e48` (object `0x11759160`; its pointer lives only in `PTR_PTR_117531f4`, set only in `FUN_112547f0`/`FUN_112548b0`).
Validate slot `+4` = `FUN_11254be0`, derive slot `+8` = `FUN_11254ae0` - both MD5 (`OnlineCore/src/Core/Checksum/MD5/MD5Checksum.h`),
mode `FUN_112378a0(...,2)`. **No asymmetric key and no baked Ubisoft secret.** Neutralize the MD5 compare in `FUN_11254be0`, or
repoint `PTR_PTR_117531f4` to a deterministic local backend (derive +8 / validate +4). A patched host derives the ticket and every
patched peer validates the same MD5 -> no RV service, no key to break. See NOTES 28z for the full slot map.


## Update (28ac): co-op session layer is uniformly RV-gated; add route C (overlay) with caveat
Both co-op join flavors call the gate `FUN_11279ba0` unconditionally (= online-flag `FUN_11245090` AND RV `FUN_112492a0`/SandboxUrl);
the net-engine LAN/Online/Offline mode (`FUN_1092d256`) is transport-only and cannot bypass it. So:
- **Route B is a session-establishment SHIM** (not just the ticket patch): short-circuit `FUN_11279ba0` to return success with a
  locally-created `storm::echo` session (`NetEngineType=LAN` + `CFCXCreate/JoinSessionOperation`/`SearchIP`), then satisfy the MD5 ticket (28z).
- **Route C (new): VPN overlay (Tailscale/ZeroTier) or HXCoop local-proxy+relay** for "friends without port forwarding" - zero-code,
  but ONLY after Route B makes the game form LAN sessions; overlay alone does not work because the session gate blocks before any LAN session.
- Order of work: prove/build the Route-B session shim first (it is the only real blocker); Route C rides on top of it.


## Implementation status (as of NOTES 28ae/28af)
- A compiling scaffold exists at `src/coop_shim/` (x86 DLL, inert by default; **NOT deployed, NOT runtime-tested**).
- Route B patch points (4): session gate `FUN_11279ba0` (plus online global `DAT_11f07e48`, rv-connected `FUN_11237670`, rv `FUN_112492a0`);
  host `FUN_11275d90` -> `CFCXCreateSessionOperation`; join `FUN_1127a1d0` -> `CFCXJoinSessionOperation`; ticket `FUN_11254be0`/`FUN_1124e5e0`.
- Known ABI (28af): gate out = `sockaddr_in`; online = a global byte; ticket = MD5-only.
- Done: full static RV map, decision doc + Route C, PR #1 shared upstream, scaffold compiles clean. Blocked: the runtime test
  (FC3 anti-injection blocks agent attach) to pin struct ABIs and the ticket field set, and to confirm the native LAN storm::echo op forms once the RV
  auth handlers are short-circuited and NetEngineType is forced to LAN (28ag: the CFCX op registry is a C++ name factory, not hand-callable).
