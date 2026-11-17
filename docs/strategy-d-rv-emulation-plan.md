# Strategy D — local RendezVous / NetConnect backend emulator for Far Cry 3: plan + assessment

Standalone plan. Prerequisites: NOTES 28cw/28cx/28cz (PRUDP v1 + RV scaffold), 28dq/28do (offline
co-op host uses storm::echo, not RV; FCX waits on the online-service/party/session event), 28ea
(provider create/search bottom out in the NetConnect/RV RPC client), docs/r2-lan-provider-interface.md.
Existing scaffold: `scripts/rv_server/` (PRUDP v1 codec, `rv_server.py` handshake + RMC dispatch,
`config_server.py` SandboxUrl seam, `test_prudp.py`).

## 0. What D is, and the layer model (be precise)

FC3 PC online co-op sits on **two** Ubisoft/Quazal layers; D targets the lower one:
1. **FCX online layer** (in `FC3_d3d11.dll`): login state machine (28db/28dd), party/matchmaking
   orchestration, and the "online-service connected" state that gates everything (28dq). This layer
   is what the OW15 "servers unavailable" error came from (28dm).
2. **Quazal NetConnect / RendezVous layer** (the `0x112xxxxx` block + `storm::echo`): the actual
   rendezvous + session + ticket service the game dials via a PRUDP/RV connection to the URL the
   OnlineConfig advertises (`SandboxUrl = prudp://address=…;port=…`). The `CGameMatchmakingProvider_RdV`
   create/search (provider slots 15/13 → `FUN_107925a8` → `FUN_11258f20`, and `FUN_11279ae0` →
   `FUN_11279920`, 28ea) speak RMC-over-PRUDP to this service. `storm::echo` is the game data plane
   (the UDP:9000 session a co-op host forms, 28do).

**D's premise:** stand up a local server that answers the NetConnect/RV service the game dials, so
its RendezVu client can authenticate, create/search a session and exchange the session ticket, after
which the two game clients connect over storm::echo. D replaces the DEAD Ubisoft RV/GATOR backend.

**The central unknown that frames all of D (from 28dj/28dq):** in every offline net-on test the FC3
client NEVER emitted an RV dial (RV-PROBE = 0). It only formed a local storm::echo session for a
co-op host, and otherwise waited on the FCX online-service state. So before writing any RV service,
we must first answer: **can FC3 be driven to actually dial an RV server at all, and under what
FCX state?** If not, D has no entry point and is moot. This is why Stage 0 below is a capture
experiment, not a server feature.

## 1. Inventory of what already works (offline, tested, no game, no C:)

From `scripts/rv_server/` (Stage S1):
- `prudp.py`: PRUDP **v1** packet `decode/encode`, options (`id, (type<<4|size), value`, LE),
  SYN/CONNECT/DATA/PING/DISCONNECT, tolerant parser. Header signature/checksum is a **placeholder**.
- `rv_server.py`: UDP server, `SYN → CONNECT(+connid) → CONNECT-ack → DATA` state machine; logs every
  packet in hex; `services[(protocol_id,method_id)] → handler` dispatch on `RMCMessage` payloads.
- `test_prudp.py`: offline proof — codec round-trip + synthetic client completing the handshake and
  an RMC request/response. Prints `ALL OK`.
- `config_server.py`: serves OnlineConfig with `SandboxUrl → prudp://address=127.0.0.1;port=50000`
  (the config seam that would point the client at us).

Not present: prudps (encrypted) transport; real RV option IDs (conn-id/pid/cid are NEX-table
guesses); any real RV service (auth, create, search, ticket). All RV protocol/method IDs are
**unknown** (no golden capture).

## 2. What the game is expected to call (static evidence + the gap)

From the RV connect chain (28cq) and the provider bottom-out (28ea), the client side calls, in order:
- **ConnectEndPoint / DNS + SYN** to the `SandboxUrl` host:port (Quazal `JobConnectEndPoint`).
- **ConnectToAuthenticationService** (`FUN_11252f70`, 28cq): the RV/NetConnect **login/auth** service
  (`RendezVousBackEndServices` / `JobBackEndServicesLogin`). On success the manager builds its RV
  sub-object (`this+0x110`, `FUN_1041f965`, 28cp) — i.e. "online-service connected".
- **RegisterURLs / CompleteConnect** (`BackEndServicesConnect::*` strings, 28da): service-URL registration.
- **Session create** (provider slot 15 → `FUN_107925a8` → final `FUN_11258f20`): RV CreateSession RMC.
- **Session search / join-by-id** (provider slot 13 → `FUN_11279ae0` → `FUN_11279920`): RV search RMC,
  async callback.
- **Player/service enumeration** (provider slots 16/17), **observer add/remove** (22/23).
- **Ticket / GameSessionKey** exchange: required to authorize the two clients into the same game
  session (the anti-cheat session key). Its message(s) live in the same RV RMC layer; **not located**
  because it is only reached after create/search succeeds.

**Gap:** the concrete RV `protocol_id/method_id` numbers, their request/response binary layouts, the
prudps key-exchange, and the ticket format are all unconfirmed — every one of them currently lives
only in Ubisoft's (dead) RV server. There is **no open-source FC3-era Quazal RendezVous server** and
**no golden FC3 RV capture** (28dq/28ds). This is the whole difficulty of D.

## 3. PRUDP / prudps format

- PRUDP v1 transport: implemented (S1). Magic `EA D0`, per-packet seq/session/substream/source/dest,
  option TLVs. Confirmed enough to hand the handshake.
- **prudps (the `s` in `prudp://...`)**: RC4 + HMAC payload encryption with a session key established
  at CONNECT (Quazal uses a signed "connection keyblock" — RSA on the server's key + a DH/derived
  session secret). We do NOT have FC3's RV server RSA private key or its keyblock format, and FC3
  may pin/verify the server key. Two ways in: (a) serve plain `prudp` (no encryption) if the client
  will accept the non-secure scheme at the config seam, or (b) recover the key-exchange/verify logic
  from the client (the NetConnect block) and either supply matching keys or patch the client to skip
  verification. **This is a hard, multi-day unknown either way.**

## 4. auth / party / session / ticket — what to fake

- **auth (login)**: FCX passes credentials fetched from the (proxied) launcher (28db/28dd); with
  `FC3COOP_FAKE_CREDS=1` the credential struct is fabricated. A local RV auth must accept fabricated
  credentials (no Ubisoft passport validation) and return a well-formed login OK with a player id /
  service master. Feasible to attempt, but the exact reply schema is unknown.
- **party**: co-op host is effectively single-session; FCX may require a party/leader state. The
  provider enumeration (slots 16/17) and observers (22/23) are how the UI reads players; a local
  session must populate these lists so the lobby shows a joinable slot.
- **session**: the RV create/search/ticket messages that let client B find and enter client A's
  session, then hand off to storm::echo for the data plane. Needs the real create/search/ticket
  layouts (unknown).
- **ticket / GameSessionKey**: must be produced by the RV service and accepted by both clients'
  anti-cheat; likely the single hardest unknown (cryptographic, client-verified).

## 5. Staged plan with gates + kill criteria

**Stage D0 — the decisive capture experiment (must do first; ~1-2 sessions; LOW code, HIGH info).**
Question: can FC3 be driven to dial an RV server, and does it emit the NetConnect auth RPC?
Method: net-on config WITHOUT fake-online (28di recipe), `SandboxUrl → prudp://127.0.0.1:50000`
(config_server), rv_server + RV-PROBE + FCX login probes on; drive the game to co-op and log all UDP.
- Gate A: at least one PRUDP SYN reaches rv_server and the handshake completes to `established`.
  - If PASS → FC3 can dial RV locally; proceed to D1 (and the earlier "FCX waits on party/session"
    finding is refined: it proceeds once RV auth responds).
  - If FAIL (zero inbound, as 28dj saw) → **KILL D**: the FC3 co-op flow will not dial RV, so there
    is no server to emulate into; the only offline co-op path is storm::echo (28do), which is the
    provider-swap route (B', itself blocked — 28ea). Record and stop.
This stage converts D from speculation to a known-tractable or known-dead path before any service is
written. Everything below is conditional on D0 PASS.

**Stage D1 — prudps transport (days-weeks).** Get an inbound RMC DATA frame to decrypt to something
parseable, and send a valid encrypted reply. Gate: the client accepts a CONNECT + first encrypted
response and does not disconnect (no immediate DISCONNECT / retry loop). Kill: if prudps pins an RSA
key we cannot reproduce and the client hard-fails key verification, D is dead unless we patch the
client to accept `prudp` (no encryption) — evaluate that fallback.

**Stage D2 — auth/login service (days-weeks).** Implement the ConnectToAuthenticationService RMC
reply so FCX marks online-service connected and builds the RV sub-object. Gate: FCX login sticks AND
the RV ServiceMaster sub-object (`this+0x110`) becomes non-null (observable via the existing shim
MGR_WATCH/observer). Kill: if the login reply schema cannot be reverse-engineered from the NetConnect
client parser (read the client's parse of our reply, drive it to "malformed" until we learn the
format), and no progress is made across a week, escalate to full packet-layout RE (large).

**Stage D3 — session create/search/ticket (weeks-months).** Implement RV CreateSession + Search +
the GameSessionKey/ticket so two clients rendezvous and start a storm::echo session. Gate: two FC3
instances on one LAN meet through rv_server, exchange a ticket, and a storm::echo session forms with
both players present (observe storm::echo::session AddPlayer + provider slot 17 enumeration shows 2).
Kill: if the ticket/GameSessionKey cannot be made acceptable to the client anti-cheat, the two
clients will not enter the same session → D dead.

**Stage D4 — campaign co-op QA (weeks-months).** Once two humans are in a world, run the
desync/ownership/chaos harness (hub `tools/`) and verify campaign scripts tolerate co-op. Gate: an
honest 2-bot QA pass. (Same stage as R2-4; independent of whether we got here via B' or D.)

## 6. Effort / risk / recommendation

- D0: cheap, decisive — **do it regardless**, it also re-tests the B' dead-end conclusion cleanly.
- D1-D3: **team-scale, months**, gated on unknowns we cannot bound without traffic: prudps crypto
  (RSA/keyblock we don't have), every RV RMC message layout, and the ticket/GameSessionKey.
- Killer risk (unchanged and decisive for D): **no golden reference.** No open-source FC3-era
  Quazal RendezVous server exists, and the FC3 RV traffic was only ever produced against Ubisoft's
  (now-dead) server, which we never captured while it answered. Every message layout must be
  inferred by interrogating the client parser blind.
- **Recommendation:** run D0 first. If D0 shows FC3 will not dial RV, stop D (and B') — only
  mod-262 SOLO remains. If D0 surprisingly PASSES, D1 (prudps) becomes the next go/no-go; commit
  there only if the key-exchange looks recoverable. As of current evidence (28dj: RV-PROBE zero on
  a real co-op-host attempt), the prior is that D0 FAILS, so treat D as low-probability and
  mod-262 SOLO as the deliverable.
