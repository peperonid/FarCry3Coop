# Route A — Emulating a local fake RendezVous / GATOR session service for FC3 campaign co-op (static plan)

> Status: **recon / plan only** — no game launched for this document, no game binary modified.
> Scope: what has to be built so the campaign co-op op cascade completes against a **local fake
> server** instead of Ubisoft's live services, then hands off to the native `storm::echo` LAN
> session (the actual data transport). This is the only route left after all hook-level levers
> were falsified live (see `NOTES.md` 28bd / 28be).
> The detailed literal internal strings (Ubisoft build paths, the runtime online-config host, the
> crash-telemetry FQDN) are intentionally **not** reproduced here; they live in `NOTES.md`
> (sessions 28bc, 28be) and must not go into a public PR.

English (this file) · [Русская версия](coop-gator-emulation-plan.ru.md)

## 1. Why this is the only remaining route

The campaign co-op Host action is hard-wired to the UbiAuth + RendezVous/GATOR path:
- The join op (`FUN_11279f90`, op vtable at RVA `0x116cf86c`) always issues a RendezVous/UbiAuth
  login; there is **no NetEngineType branch** to a LAN backend for campaign co-op
  (`NOTES` 28aq / 28bc).
- Live probes proved the game actually **sends the RV login and receives the callback**
  (`RVLOGIN-SEND` / `RVLOGIN-CB`), i.e. the co-op path talks to the network service and does not
  short-circuit (`NOTES` 28bd).
- But driving the real RV session flow any deeper **crashes**, even with our hooks neutralised —
  the crash is the game's own deep RV/GATOR session handling, not our code (`NOTES` 28be).

Conclusion: hook-level patching of the client cannot get past the UbiAuth/RV → GATOR session
establishment cascade. To host/join a campaign co-op session we must make the client believe it is
talking to a real RendezVous/GATOR service — i.e. **emulate that service locally** and satisfy the
whole op-result cascade, then let the client bring up its own `storm::echo` session for actual play.

## 2. Evidence inventory (static)

All addresses are RVAs (add the runtime module base; ASLR is active). Internal Ubisoft build-path
prefixes and the literal online-config FQDN are kept only in `NOTES.md`.

| Artifact | What it tells us |
|---|---|
| `prudp:/`, `prudps:/`, `prudp`, `prudps` (`.rdata`) | Transport is **PRUDP** (UDP-based, reliable layer). `prudps` = the secure/encrypted variant. A PRUDP packet-rate counters block exists (reliable tx / retransmit) → full PRUDP implementation is statically linked. |
| `getaddrinfo`, `freeaddrinfo`, `getnameinfo`, `WSASocketA`, `WSASendTo`, `WSARecvFrom`, `WSAIoctl`, `WSACreateEvent`, `WSAWaitForMultipleEvents`, `WSAGetOverlappedResult` (WS2_32 imports) | The RV socket driver resolves the server host via **`getaddrinfo`** and does UDP I/O with **overlapped `WSASendTo`/`WSARecvFrom`**. These named imports are hookable at the IAT — the clean interception point. |
| `onlineconfigservice.ubi.com` + `cert-` / `uat-` variants | FC3 fetches its **online configuration over HTTPS**; the RendezVous/GATOR connection endpoints are delivered at runtime by this config service, not baked into the binary. This is the reason there is no static RV host. |
| `clusterResId` | RV sessions are placed into **clusters**; cluster routing is config-driven. |
| `CRendezVousLoginOperation`, `CRendezVousLocalizationOperation`, `CRendezVousConfigureUPnPOperation`, `CSessionCreateGameOperation`, `CSessionSearchOperation`, `CSessionAcceptInviteOperation`, `COnlineGameConfigOperation`, `CSessionCreateOperation(_RdV)`, `CGameMatchmakingProvider(_RdV)` | The **op catalog** the client runs during co-op bring-up (see §4). |
| `JobUbiAuthenticationLogin`, `JobLaunchUPnPThread` (+ `ThreadPerformUPnP`, `StepCheckForCompletion`) | Async **jobs** behind the login and UPnP ops. |
| UPnP strings: `InternetGatewayDevice:1`, `WANIPConnection:1`, `WANPPPConnection:1`, `upnp:rootdevice` | The client opens a **UPnP port mapping** when hosting → the fake must tolerate UPnP (or UPnP is stubbed/failed gracefully). |
| `DummyUbiUser@quazal.com` / `UbiDummyPwd` | The RV guest login uses stub credentials → suggests the client can reach RV login without a fully authenticated Ubi user (confirmed it sends + gets callback live). |
| `127.0.0.1`, `0.0.0.0` present | Loopback/wildcard binds exist → a local fake server needs no special stack. |

## 3. Interception architecture (where the fake sits)

Two sub-problems: (a) make the client's RV traffic land on our fake; (b) speak the protocol and
author the right results.

**(a) Redirect RV → localhost.** The lowest clean hook is **`getaddrinfo`** in the WS2_32 IAT (the
proxy already sits in-process via the loader auto-load, `NOTES` 28ai). Strategy:
- Hook `getaddrinfo` in the proxy. For the RV/GATOR host(s) returned by the online-config service,
  return `127.0.0.1` instead of the resolved Ubisoft address; pass everything else through.
- Optional belt-and-braces: hook `WSASocketA`/`WSASendTo`/`WSARecvFrom` to (i) force destination
  `127.0.0.1:<fake port>` and (ii) log the exact PRUDP frames for RE.

This bypasses the runtime config host entirely — we do **not** need to fake `onlineconfigservice`
HTTPS/TLS, because we redirect after name resolution. (The alternative — a fake HTTPS config
service — needs cert trust or hooking before TLS and is strictly harder; avoid it.)

**(b) Local PRUDP/GATOR server.** A small UDP listener on `127.0.0.1:<port>` that:
- decodes PRUDP packets (header, ack/reliable framing, ping/pong, fragmentation),
- demultiplexes the application service calls inside,
- replies with the results the client's op cascade expects (§4),
- eventually returns the peer/address information that lets the client start its `storm::echo`
  session.

## 4. The op cascade to satisfy (what the fake must answer)

Reconstructed order (client-side, from the op classes + the live login trace). Each op completes
when the client posts a **result-event** for it; the fake must produce those results.

1. `COnlineGameConfigOperation` / online-config fetch — normally from the HTTPS config service.
   With the `getaddrinfo` redirect we mostly skip this, but the client may still expect a game-config
   blob (playlist, max players, rules). **Unknown:** whether co-op needs a successful config op or
   tolerates a cached/empty one.
2. `CRendezVousLoginOperation` (backed by `JobUbiAuthenticationLogin`) — **confirmed to run and get
   a callback live**. Fake must answer with a successful RV "connected / logged-in" result (ticket /
   session ticket the client accepts). This is the piece we could observe (`RVLOGIN-*`) but the
   exact success payload is not yet captured (`NOTES` 28be: STATUS not read).
3. `CRendezVousConfigureUPnPOperation` (`JobLaunchUPnPThread`) — hosting opens a UPnP mapping. The
   fake path can let UPnP simply fail/be skipped (many RV clients proceed with a public-address
   fallback). **Unknown:** whether the co-op host path hard-requires a successful mapping.
4. `CRendezVousLocalizationOperation` — locates the user/session in a cluster. Fake returns a
   cluster/localization result (cluster routing info is normally from config).
5. `CSessionCreateGameOperation` (host) / `CSessionSearchOperation` (join/browse) /
   `CSessionAcceptInviteOperation` (accept) — the **session bring-up**. This is the core: the fake
   must create/return a session descriptor (session id, host address + port, the peer list, and
   whatever `session ticket`/attributes the client validates).
6. **storm::echo handoff** — once the RV layer hands the client a session descriptor with the peers'
   addresses, the client starts its native `storm::echo` game session. That data path is **local /
   LAN / loopback** and is NOT our fake's job — but our fake must feed it a descriptor that makes it
   bind/connect to the real peers (the other game instances). This is the payoff of the whole
   approach and also its biggest unknown: how the RV session descriptor maps onto the storm::echo
   session bring-up (`NOTES` 28ay: co-op uses `join_handler`, not `host_handler`).

**Result-events / payload fields are only partially known.** The op class names and the login trace
are grounded; the exact wire schema of each result (field layout, the `session ticket` contents, the
cluster descriptor) is **not** yet RE'd. That capture is the main work of the first build stages.

## 5. Decomposition, deliverables, risk, effort

Effort = focused weeks by one competent RE/dev; ranges assume a public PRUDP reference exists for
the framing (Ubisoft Connect PRUDP is publicly documented and may share the packet framing with the
in-game RV stack — verify, do not assume). Risk = can this stage stall the whole route.

| Stage | Deliverable | Risk | Effort |
|---|---|---|---|
| **S0. PRUDP wire capture** | Proxy hooks `WSASendTo`/`WSARecvFrom`/`getaddrinfo`; capture the **real** co-op bring-up PRUDP frames against live servers (1 host + 1 join, internet ON). Produce a frame log + host/port used. | Medium (must not crash; capture path must be transparent) | **1–2 wk** |
| **S1. PRUDP codec** | Decode/encode PRUDP header + reliable framing + ping/pong from the S0 capture. Round-trip a captured session offline. | High (prudps encryption/signature) | **2–4 wk** |
| **S2. Redirect to fake** | `getaddrinfo`→`127.0.0.1`; fake UDP listener on the captured port; client reaches it and starts PRUDP handshake. Client must not crash at the handshake. | Medium | **1–2 wk** |
| **S3. Login + localization ops** | Fake answers `CRendezVousLoginOperation` + `...Localization` results so the client reports RV connected. | High (exact result schema) | **2–3 wk** |
| **S4. Session create/search + descriptor** | Fake returns a session descriptor that satisfies `CSessionCreateGameOperation`/`CSessionSearchOperation`/`CSessionAcceptInviteOperation`. Client reaches "session joined" without crash. | **Highest** (session ticket / peer mapping / what storm::echo consumes) | **3–5 wk** |
| **S5. storm::echo bring-up on 2 peers** | Two game instances each redirected to the fake; after S4 both get the peer descriptor and start storm::echo **against each other** (loopback/LAN). This is the real go/no-go. | **Extreme** (whole theory depends on this) | **2–4 wk** |
| **S6. 3–4 players + robustness** | Extend descriptors to 4 peers; packet-loss/late-join; make it repeatable. | High | **2–4 wk** |

**Total: ~13–24 focused weeks, dominated by S1 (PRUDP crypto), S4 (session schema) and S5 (the
storm::echo handoff, which is the single make-or-break unknown).** These three are also where the
route can die outright even after the others succeed.

## 6. Key risks & open unknowns

- **prudps encryption/signature** — the secure PRUDP variant needs the packet signature key/scheme.
  Public Ubisoft Connect PRUDP may cover auth but **not** the in-game GATOR session channel. If the
  game-session channel uses a different key/agreement, S1 can be much harder or blocked.
- **online-config dependency** — some ops may need a non-empty game-config/cluster blob; if the
  client rejects a missing one, we must also fake that payload (not the HTTPS transport, but the
  decoded blob).
- **session ticket contents** — `NOTES` 28x/28z: the session ticket is service-minted and (for the
  observed path) MD5-based with no asymmetric key — encouraging, but the exact field set the client
  validates is still unknown.
- **storm::echo mapping (S5)** — the crux: does an RV-delivered session descriptor actually let the
  client open a storm::echo session to arbitrary peers we choose? If storm::echo itself validates
  the ticket/ownership against a live authority, the route fails at the last step.
- **UPnP hard-requirement** — if hosting refuses to proceed without a successful port mapping, the
  fake must also satisfy UPnP (or we stub the UPnP job's completion).

## 7. Recommended sequencing & de-risking

1. Do **S0 (capture)** first and cheaply: it answers "which host/port + what the frames look like",
   de-risks S1, and costs little. This needs one live 2-instance co-op attempt (internet ON) with
   the capture hooks — the first thing worth doing if this route is chosen.
2. Before S1, **check a public PRUDP reference** against the S0 capture (share of framing/keying) —
   this bounds S1 cost.
3. **De-risk S5 conceptually as early as possible** (it is the kill-switch): once we can decode a
   real 2-player co-op session (S0/S1), inspect the session descriptor / storm::echo bring-up
   arguments to confirm the client will connect to peers we specify. If storm::echo needs a live
   authority, abandon the route before building S3/S4.
4. Only then build S2→S4 in order, testing the client reaches each next op.

## 8. Relationship to the live probe harness

All of S0/S2 use the existing in-process proxy + shim (`src/uplay_proxy`, `src/coop_shim`;
auto-load via the loader proxy, `NOTES` 28ai; detached launch via WMI). The new work is (a)
`getaddrinfo`/Winsock IAT hooks added to the proxy for capture/redirect, and (b) a standalone
local PRUDP server binary (new `tools/` component). No game-binary writes are required at any stage.

## 9. Bottom line

Route A is **feasible in principle but expensive and gated by two hard unknowns** (PRUDP
game-session crypto in S1, and the storm::echo peer-mapping in S5). The single most valuable cheap
step — if the owner chooses to invest at all — is **S0: capture a real 2-player co-op bring-up
trace**, which converts most of the guesses above into facts and bounds the rest of the estimate.
If Bantila's "1.05 LAN" co-op turns out to operate below the GATOR layer (see PR #1), it may
bypass this entire build; confirm that before committing weeks.
