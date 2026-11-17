# R2 plan — substitute a LAN session provider for `_RdV` in Far Cry 3 (PC 1.05/1.06, `FC3_d3d11.dll` md5 46bcc097)

Standalone project plan, follow-on to NOTES 28du-28dv. Static-only recon (capstone over the raw
DLL; the Ghidra project copy is `-noanalysis` and a full 40 MB headless re-analysis is impractical
in the current harness). This doc lists concrete patch anchors and a staged build plan with
verification gates and kill criteria. All addresses are file-image VAs (add the runtime module base;
ASLR is on). This is a reverse-engineering + code project, NOT a config toggle.

## 0. Goal and ground truth

Goal: get FC3 to host/join a campaign-map co-op session over LAN/loopback (Storm), so 2+ humans
can play, WITHOUT the dead Ubisoft RendezVous matchmaking service.

Ground truth from prior work (NOTES 28dm-28dv):
- FC3 PC wires ONLY the RendezVous (`_RdV`) matchmaking/session providers; campaign co-op has never
  run as a network game; the Ubisoft co-op lobby is dead online; offline the game waits for a live
  backend "online-service connected + party/session" state.
- The command-line dev path (`-host -coop` / `-client -ip`) does NOT engage on the 1.06 retail build
  via direct launch (28du).
- Multiplayer/Storm is NOT stripped from 1.06: the base (non-RV) session classes and Storm LAN
  primitives are present in the binary.

The whole R2 thesis rests on that last point: **the non-`_RdV` session machinery exists; we need to
make the game select and run it.**

## 1. Concrete anchors (this session + hub)

Session/provider classes (RTTI/log strings, with the code that references them):
| Symbol | string VA | referenced from |
|---|---|---|
| `CSessionCreateOperation` (base, non-RV) | 0x11523fa8 | 0x10425885 (in factory ~0x10425822) |
| `CSessionCreateOperation_RdV` | 0x11527a78 | 0x1044b650 (in factory ~0x1044b2cb) |
| `CSessionJoinOperation` (base, non-RV) | 0x11524030 | 0x10425961 (same base factory) |
| `CSessionJoinOperation_RdV` | 0x11527b88 | 0x1044bfd4 (same RdV factory) |
| `SessionSearchIPOperation` (IP-driven session op) | 0x11524015 | (RTTI/log only) |
| `MatchmakingProvider` (base) | 0x11589218 | (RTTI/log only) |
| `MatchmakingProvider_RdV` | 0x1158b43d | (RTTI/log only) |
| `RendezVousBackEndServices_native.cpp` / `RendezVousGuest...` | 0x115265f9 / 0x11526901 | (auth backend) |

Key structural finding: the base-op factory (0x10425822) and the `_RdV`-op factory (0x1044b2cb) are
**virtual methods** — `callers_of()` finds zero direct `E8` callers, so the provider is selected by
**which provider object is instantiated and installed**, not by a call-site branch. That is the one
anchor we have NOT yet pinned (needs RTTI/vtable analysis).

Session parameters (from the hub's PR + our ref):
| Param | string VA | registered in |
|---|---|---|
| `MatchMakingEnabled` | 0x11561628 | ~0x1061e581 (0x1061e633 ref) |
| `NetEngineType` (id 0x3c) | hub-noted | same param-schema function |
| `SearchIdAddress`, `JoinSessionId`, `SessionType`, `PlaylistId` | hub-noted | same |

Command-line / mode path (28dv):
- `sessionTypeByName` mapper (prologue 0x107ab377): `-coop` -> mode enum from *[0x1171a99c];
  `-host`-ish -> *[0x1171a9a4]; others 0x1171a994/0x1171a98c.
- startup CLI/ini parser ~0x100044d0-0x10004830 stores mode/flags into globals
  0x117f5d84..0x117f5dd4.
- Storm LAN: `storm::echo::session`, `storm::echo::lan`, `BroadcastLanDiscovery2`, ports 9000-9004.

## 2. Patch strategies (candidate, ordered by expected tractability)

Strategy A — **flip the provider selection to the base (non-RV) provider.**
Find where the game instantiates/installs `CGameMatchmakingProvider_RdV` (and the `_RdV`
create/join factories), and install the base provider instead (or make the `_RdV` factory forward to
the base factory 0x10425822). If the base ops build a Storm LAN session, this is the cleanest win.
Needs: RTTI walk to find the `_RdV` provider vtable and its construction site.

Strategy B — **vtable override of the create/join operation factory slots** (shim).
Instead of finding the install site, hook the virtual slots so the `_RdV` object returns the
BASE `CSessionCreateOperation` / `CSessionJoinOperation` (built by 0x10425822). Our shim already does
vtable trampolines; add the provider vtable to the tracer (read `vtable_rva` of the session-manager /
provider object), then detour the two slots.

Strategy C — **force LAN/direct session parameters.**
Set the session params `MatchMakingEnabled=0` and `NetEngineType=LAN` at creation (the param-schema
function ~0x1061e581), plus `SearchIdAddress`/`JoinSessionId` for direct join. Cheapest to try, but
likely insufficient because the provider object is still `_RdV` (per hub), so the params may be
ignored. Treat as a quick probe, not the fix.

Strategy D — **local RendezVous emulator** (the other "emend the join path" option).
Keep the `_RdV` provider but answer its login/ticket/session RPCs with a local RV/PRUDP server
(our existing `scripts/rv_server` codec groundwork). Highest fidelity, but it is the big emulation
project (auth + party + ticket + GameSessionKey), not a provider swap. Out of scope for R2 proper.

## 3. Staged plan with gates

Stage R2-1 (analyze, ~1-2 focused Ghidra sessions): run a full auto-analysis of `FC3_d3d11.dll`
in a proper Ghidra session (give it 20-40 min; use a longer REA_GHIDRA_STARTUP_TIMEOUT_MS and run
truly detached with file-redirected logs). Produce: RTTI of `CGameMatchmakingProvider_RdV` +
`MatchmakingProvider` base + the two `_RdV` factories; the vtable of the online session-manager
(global at 0x11f07e48) and where the provider is constructed/installed. GATE: a named construction /
install site for the `_RdV` provider. Kill criterion: if the `_RdV` provider is hard-wired with no
virtual/registration seam and the base provider cannot run standalone, R2-A/B are dead and the game
effectively has no usable LAN path without full RV emulation (-> Strategy D).

Stage R2-2 (provider swap spike, shim): implement Strategy B — trace the session-manager/provider
vtable, detour the create/join factory slots to the base factory (0x10425822). GATE (bot-only, per
AGENTS "test with bots not your owner"): a single launched instance reaches a REAL Storm listen
session (Storm binds ports 9000-9004 with an active session, NOT just the idle menu UDP:9000 we
already saw) and logs a created session. No gate pass => provider still gated upstream.

Stage R2-3 (second instance join): launch instance B as client with the direct join params /
`SessionSearchIPOperation` path pointed at A's LAN address; observe Storm player add
(`storm::echo::session::resultEvent::AddPlayer`) and 2 slots populated in `CFCXPlayerService`.
GATE: two players in the same world with movement synced (the hub's `GetPlayerInSlot` /
`CoopTeleportPlayer` Lua confirms slot 1 populates). Kill criterion: if joins never form a Storm
session even with the base provider, the LAN path is not actually functional in the retail build.

Stage R2-4 (campaign run + sync QA): once 2 players are in a map, verify the story/campaign scripts
handle co-op (the hub notes the scripts are only "partly co-op aware"; `GRI_Coop_*.lua` + co-op
spawn entities exist). This is unexplored and may need script/data patches. GATE: an automated 2-bot
playtest with a desync/ownership harness (hub `tools/` chaos/ownership/fuzz) at an honest QA level.

## 4. Effort / risk

- R2-1 analysis: days, medium risk (Ghidra flakiness in this harness is a real friction; run
  detached with long timeouts).
- R2-2 provider swap: days-weeks, HIGH risk (the provider selection anchor is not yet pinned; the
  base provider may itself assume an online service at construction).
- R2-3 join: weeks, HIGH risk (unverified that the base/SessionSearchIP path is wired to anything
  reachable in retail 1.06; likely also blocked by the same "online-service connected" gate at a
  higher layer that we already hit from the menu path in 28di/28dj).
- R2-4 campaign co-op: weeks-months, HIGH risk (campaign scripts not network-tested; host migration,
  AI ownership, save/respawn all need sync).
- Cross-cutting killer risk (unchanged): there is no golden reference of FC3 campaign co-op ever
  running; every stage is validated against nothing but our own reconstruction.

Overall: R2 is a **multi-week-to-month, high-risk, multi-person-scale research project**, and its
first gate (a real Storm listen session offline) is itself uncertain because we have already seen the
game wait on the online-session state from the menu path. R2-A/B is the most promising ordering, but
treat Stage R2-1 (does a usable non-RV provider seam even exist?) as the go/no-go.

## 5. Recommendation

Do R2-1 first and treat it as a hard gate. If R2-1 finds a clean provider seam and the base provider
can run offline, continue to R2-2. If R2-1 shows the `_RdV` provider is hard-wired and the base
provider cannot stand alone, stop R2 and the only remaining path is Strategy D (full RendezVous
co-op backend emulation), which is strictly larger. Meanwhile mod-262 SOLO remains the only working
FC3 co-op.


## 6. R2-1 RESULTS (session 28dx; full Ghidra analysis done)

Confirmed by auto-analysis (NOTES 28dx): both provider families and both op families are present
with name-registration singles: base `CGameMatchmakingProvider` 0x1158922c / `_RdV` 0x1158b438 /
`CSessionMatchmakingProvider` 0x11589210; `CSessionCreateOperation` 0x11523fa8 (init FUN_10425868) /
`_RdV` 0x11527a78; `CSessionJoinOperation` 0x11524030 / `_RdV` 0x11527b88; `CSessionSearchIPOperation`
0x11524014. `_RdV` create Execute = FUN_1044b2cb is a vtable slot (only DATA xref from 0x11527a64,
0 E8 callers), gated on FUN_1042c988(this)=*(this+0x3c)!=1,3. Full session-param schema in
FUN_1061e581 (NetEngineType id 0x3c, MatchMakingEnabled 0x4d, SessionType 0x38, SearchIdAddress 8,
JoinGame 0x53, JoinService 0x54, JoinSessionId 0x18, PlaylistId 0x34, SearchSlots 0x48). Dev CLI
parser FUN_107ab377 handles -keyonline (key "a123456"), -login x4 + -password x4, -splitscreen
(4-slot login table, stride 0x24).

GO/NO-GO: GO (qualified). The non-RV seam exists and the LAN param schema is first-class. Still open:
prove the base provider builds a Storm LAN session. NEXT = R2-1b: (1) find where the game constructs
and installs `CGameMatchmakingProvider_RdV` (swap to base `CGameMatchmakingProvider`), (2) decompile
the base `CSessionCreateOperation::Execute` and confirm it opens a Storm session on ports 9000-9004.


## 7. R2-1b RESULTS (session 28dy) - Strategy A RULED OUT

Provider install + both vtables found. Constructors write provider vtables: BASE CGameMatchmakingProvider ctor
FUN_1077ac7e writes 0x11589198 (base vtable); CGameMatchmakingProvider_RdV ctor 0x10786c8d writes 0x1158b458
(RdV vtable, at 0x10786c94); RdV singleton 0x11e684e8 built by provider-init 0x10786c5c which registers the
base provider then layers RdV via FUN_1008bf50. Swap point = patch 0x10786c94 (or runtime vptr swap of
0x11e684e8). BUT the base provider vtable 0x11589198 virtuals are ABSTRACT STUBS (return 0x12/0x3e/0x3f/1,
empty dtors); create/join not implemented. => Strategy A dead: swapping to base gives error stubs. There is NO
working non-RV provider in the binary. R2 now = (B') author a new LAN provider implementing the full provider
vtable interface + Storm create, or (D) full RendezVous emulator. Custom-provider install anchor: vtable
0x1158b458 / singleton 0x11e684e8 / ctor 0x10786c8d.
