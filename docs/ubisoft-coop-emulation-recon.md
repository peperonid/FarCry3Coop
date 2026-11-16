# Ubisoft co-op / RendezVous emulation — research recon (FC3 1.05/1.06, PRUDP v1)

Purpose: inventory the known open-source efforts that touch the Ubisoft / Quazal online stack,
judge how much of each is reusable for Far Cry 3 campaign co-op on this build (1.05-era binary
`FC3_d3d11.dll`, md5 46bcc097, PRUDP v1, Quazal RendezVous), and give an effort/risk estimate for
emulating the missing co-op backend as a standalone project. All findings are read-only / web.

## 1. What FC3 actually needs (the target)

From this repo's own static + live analysis (NOTES 28cw, 28dm-28dq):
- Co-op sessions are **Quazal RendezVous**, statically linked into `FC3_d3d11.dll`, talking to
  Ubisoft over its own PRUDP socket stack (not the `uplay_r1_loader` API).
- The create/join cascade is: online session-manager present -> `CFCXGatorLoginCheckOperation` ->
  `CRendezVousLoginOperation` -> `CRendezVousConfigureUPnPOperation` / `CRendezVousInitPunchOperation`
  -> `CSessionCreateOperation_RdV` -> host handler (`SubAccountClient`), with a **SessionTicket**
  minted by the service (RendezVous `JobTicketManager*`, `GameSessionKey`).
- Offline, FC3 never even dials RendezVous: it hangs "connecting to servers" waiting for a genuine
  *online-service-connected + party/session* state the offline loader cannot emit.

So the missing piece is not the transport (we already speak PRUDP v1) — it is the **RendezVous
service application layer** for FC3's version: auth login, party/`Uparty`, matchmaking, GameSession
create/join, JobTicket (`AcquireTicket`/`ValidateKey`), `GameSessionKey` derivation, UPnP/NAT — as
RMC/RendezVous messages.

## 2. The transport is a solved, shared problem

- **PRUDP** is documented and implemented independently of Ubisoft:
  - kinnay/NintendoClients "PRUDP Protocol" wiki and 3dbrew "PRUDP" — full V0/V1 framing:
    magic `0xEA 0xD0`, header, packet signature, sequence id, types SYN/CONNECT/DATA/DISCONNECT/PING,
    options negotiated in SYN/CONNECT, RC4/AES option encoding.
  - AnthomStahl `prudp` (Python, on PyPI) — a working PRUDP v1/v2 client/server library
    (extracted from a Steam reverse-engineering effort).
- FC3 uses the same PRUDP (the `EA D0` magic and SYN/CONNECT handshake are identical to what our own
  `scripts/rv_server/prudp.py` codec already implements and tested). Conclusion: **the transport
  layer for a local RV emulator is low-risk and largely done** (our codec + handshake work).

## 3. Open-source inventory and applicability to FC3

| Project | What it emulates | FC3 relevance | Notes |
|---|---|---|---|
| **Detanup01/UplayServer** (archived) -> **ServerEmus / Release.Uplay** (C#) | Uplay **Demux + API** (launcher), auth, DRM, achievements, store; DLL shims using named pipes like real Uplay; modified UplayKit/UbiServices/CoreLib | Medium, but for the *boot/login/DRM* layer ("Condition A"), **not** co-op sessions | The closest launcher/auth emulator; proves the Uplay boot-auth we fake on the client can also be served. Does not do RendezVous party/lobby. |
| **michal-kapala/ubi-gs** (Python) | Ubisoft **Game Service (GS)**, ca. 2000-2005 (`gsconnect.ubisoft.com`): auth, friends, matchmaking, chat, CD-key validation | Low: different, older SDK — FC3 uses Quazal RendezVous, not GS | Useful as an architecture precedent for "auth + friends + matchmaking" service emulation. |
| **Modern Ubisoft online server-emulator scene** — The Crew 1 offline emulator, PlayBlock / OpenCrew / SwallowBlock forks, Wild-Ones, Ghost-Recon/Breakpoint tooling | The **new** Ubisoft Online stack (Anvil games): login/UbiAuth, **party / Uparty**, session/matchmaking, PRUDP v4+ | Medium (architecture inspiration) but **not directly reusable**: FC3 is Quazal RendezVous 6 / PRUDP v1, a different protocol generation | These prove a full Ubisoft party/lobby/session backend CAN be emulated by the community, and publish real message flows for the modern stack. |
| **PretendoNetwork/nex-go** (Go), **pyprudp** (Python) | Nintendo **NEX** server over PRUDP (transport + NEX RMI/RMC service layer) | Low-medium: same PRUDP transport is reusable; the NEX service schema differs from Ubisoft RendezVous | Good reference for PRUDP server structure; do NOT expect the service layer to transfer. |

Bottom line: **no open project emulates FC3's specific Quazal RendezVous co-op service.** The
transport is reusable; the launcher/auth layer has a solid reference (UplayServer); the party/lobby
session emulation exists only for the *newer* Ubisoft stack, not FC3's generation.

## 4. Effort / risk estimate (as a standalone project)

Work packages for a local FC3 co-op backend:
1. **PRUDP v1 transport + RV handshake** — DONE here (codec + SYN/CONNECT + RMC framing). Effort:
   small; risk: low.
2. **Config / SandboxUrl seam** (serve OnlineConfig with `SandboxUrl` -> our server; DNS-redirect)
   — prototyped here. Effort: small; risk: low-medium.
3. **FC3 client bring-up to actually dial RV** — the current hard blocker (FC3 never reaches the RV
   connect offline). Needs either a client patch to skip the backend-state wait, or feeding the exact
   events FCX polls. Effort: medium; risk: HIGH (many prior attempts failed).
4. **RendezVous service application layer for FC3** — implement the specific RMC service
   classes/methods FCX invokes (backend-services login, JobTicketManager login/derive/validate,
   party, GameSession create/join, UPnP/NAT config) and their marshalling, AND the
   `GameSessionKey`/ticket crypto. Effort: LARGE (reverse a statically-linked Quazal build,
   reconstruct semantics); risk: HIGHEST.
5. **Session/party sync + in-world co-op** — the classic netcode surface after the session forms.
   Effort: large; risk: medium-high.

Killer risk (call it out): **there is no golden reference traffic.** A co-op session that worked
against Ubisoft was only ever captured while the service was live; now it is dead. Without a
working capture, package 4 must be reconstructed purely from static disassembly with no oracle to
validate against — and crypto/RMC-marshalling mistakes are silent and crash-prone. That single fact
dominates cost and uncertainty.

Realistic sizing: this is a **multi-month, team-scale reverse-engineering + emulation project**, not
a hook/proxy change. A single contributor (or an agent) can credibly land packages 1-2 (already
have) and maybe 3 with luck, but package 4 with no reference capture is the kind of problem that
has historically needed a community with several people sharing captured traffic.

## 5. Recommendation

- Keep mod-262 SOLO as the shipped deliverable (the only working FC3 co-op).
- Treat "FC3 co-op backend emulation" as a separate, high-effort / high-risk project; do not start it
  without either (a) a captured live FC3 co-op session from before the service shutdown, or (b) a
  team pooling that kind of traffic.
- If it is ever started, the reusable assets are: this repo's PRUDP codec/rv_server/config_server,
  the kinnay + AnthomStahl PRUDP references, the Detanup01/ServerEmus Uplay emulator for the
  launcher/auth side, and the modern Ubisoft online emulator scene for party/lobby architecture.

## 6. Sources (fetched/consulted)

- kinnay/NintendoClients wiki, "PRUDP Protocol" (github.com/kinnay/NintendoClients)
- 3dbrew, "PRUDP"
- AnthomStahl `prudp` Python library (PyPI / github) — PRUDP v1/v2
- Detanup01/UplayServer (archived) and the ServerEmus org (Release.Uplay)
- michal-kapala/ubi-gs (Ubisoft Game Service emulator)
- PretendoNetwork/nex-go; pyprudp (Nintendo NEX over PRUDP)
- modern Ubisoft online emulator scene (The Crew 1 offline; PlayBlock / OpenCrew / SwallowBlock /
  Wild-Ones tooling for Ghost Recon Wildlands / Breakpoint)
- coop-anything hub entry `games/far-cry-3` + issue "Far Cry 3 Co-op Support"
