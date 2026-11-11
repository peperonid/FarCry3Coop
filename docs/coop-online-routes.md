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
