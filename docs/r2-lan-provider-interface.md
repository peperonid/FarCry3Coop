# `CGameMatchmakingProvider` interface spec — what a LAN provider (R2 route B') must implement

Basis: the shipped `CGameMatchmakingProvider_RdV` vtable at image VA **0x1158b458** (RdV singleton 0x11e684e8,
constructor 0x10786c8d). The base `CGameMatchmakingProvider` vtable is 0x11589198 (its virtuals are abstract
stubs — see NOTES 28dy; do NOT reuse it). A custom LAN provider = author a new vtable shaped like 0x1158b458 and
install it (patch the vptr write at 0x10786c94, or runtime-swap the vptr of singleton 0x11e684e8).

All addresses are file-image VAs (add runtime module base; ASLR on). x86 __thiscall: `ecx` = provider `this`;
extra args pushed right-to-left. "Slot" = index in the vtable (offset = slot*4).

## Slot map (RdV vtable 0x1158b458)

| Slot | RdV VA | RdV func | Role (best-evidence) | LAN must? |
|---|---|---|---|---|
| 0 | 0x10786cc3 | FUN_10786cc3 | **constructor** (calls 0x10786c8d; sets vptr; optional free on flag) | yes (boilerplate) |
| 1 | 0x10786e1d | FUN_10786e1d | **destructor** (FUN_1125efd0 teardown, delete on flag) | yes (boilerplate) |
| 2 | 0x10789b11 | FUN_10789b11 | returns a **sub-interface/accessor** (*out = &PTR_FUN_1158cc08) — a related provider/service handle | maybe (return your own) |
| 3 | 0x10dbfd1a | `mov eax,[ecx+4];ret` | getter this+4 | trivial |
| 4 | 0x107c9561 | `mov eax,[ecx+8];ret` | getter this+8 | trivial |
| 5 | 0x10787949 | FUN_10787949 | **operation dispatch/message handler**: reads session-manager `*0x11e42c90` (vtable+0xc), dispatches on `code=(arg+8)/1000` in {1,5,1000} and `subtype=(arg+0x10)` in {0,1,2} → create(075ae/078a2)/join(072c6)/search(07318/07376) handlers | **YES (core)** |
| 6 | 0x10787aee | FUN_10787aee | member destructor / cleanup (FUN_107876d8) | yes (boilerplate) |
| 7 | 0x10787ae9 | `jmp FUN_10443193` | thunk → shared destructor | boilerplate |
| 8 | 0x10d75d32 | (empty) | **no-op** virtual (Update/Pump-style or unused) | no (stub) |
| 9 | 0x10d75d32 | (empty) | no-op (same as 8) | no (stub) |
| 10 | 0x10786f19 | FUN_10786f19 | `return 0` (locked stub / not-implemented) | trivial |
| 11 | 0x10786f2a | FUN_10786f2a | `return 0` (locked stub) | trivial |
| 12 | 0x1073026a | `xor eax,eax;inc;ret 4` | `return 1` ("supported"/always-true) | trivial |
| 13 | 0x10786f66 | FUN_10786f66 | **async join/search-by-id**: session-manager `*0x11e42c90` vtable+0xc, calls FUN_11279ae0(id=(arg+0x10), callback=&PTR_FUN_1158b460, this+0xb0,+0xb4, timeout 250) → bool | **YES (core)** |
| 14 | 0x1073026a | `return 1` | "supported" (same as 12) | trivial |
| 15 | 0x10787103 | FUN_10787103 | **create session**: builds a session-description list then calls FUN_107925a8(arg+0x10, list) → bool | **YES (core)** |
| 16 | 0x10787745 | FUN_10787745 | **enumerate session results**: walk linked list this+0xb4, map state {1→3,2→1,3→2,else 0}, fill output array (stride 0x1c) via FUN_1077cf40 | **YES** |
| 17 | 0x10787268 | FUN_10787268 | **enumerate players/slots**: walk list this+0xa0, copy fields, fill output (stride 0x10) via FUN_10b4cbc4 | **YES** |
| 18 | 0x1022ecf8 | (empty) | no-op | no (stub) |
| 19 | 0x101e6fd2 | (empty) | no-op | no (stub) |
| 20 | 0x10cec419 | FUN_10cec419 | `return 0` (stub predicate) | trivial |
| 21 | 0x113a842a | FUN_113a842a | lifecycle/init forwarder → FUN_1041f2f5 (iterates this+0x14 list) | maybe (lifecycle) |
| 22 | 0x113a84c5 | FUN_113a84c5 | **add observer/listener**: append param to vector at this+0x2c (count this+0x30) if absent | **YES (events)** |
| 23 | 0x113a8444 | FUN_113a8444 | **remove observer/listener**: swap-remove param from that vector | **YES (events)** |

## What a working LAN provider actually needs to implement

The network-specific behaviour is concentrated in **5 slots: 5, 13, 15, 16, 17** (plus the event plumbing 22/23
and object lifecycle 0/1/6/7/21). Slots 3,4,8,9,10,11,12,14,18,19,20 can stay as the shipped trivial stubs.

- **Slot 15 CreateSession** — instead of FUN_107925a8 (which builds an RV/GATOR request), open a **Storm listen
  session** on ports 9000-9004 and publish a local session descriptor (name/slots/playlist). Return true on
  success. This is the single most important method to rewrite.
- **Slot 13 join/search-by-id** — instead of FUN_11279ae0 (RV rendezvous query with async callback
  PTR_FUN_1158b460 + timeout), do LAN discovery (the engine already has `storm::echo::lan` /
  `BroadcastLanDiscovery2`) and resolve the id to a local host, then deliver via the same callback contract.
- **Slot 5 dispatch** — map the game's create/join/search codes (the {1,5,1000} x {0,1,2} matrix) onto your
  Storm operations; keep the same entry contract so the callers above keep working.
- **Slots 16/17 enumerate** — return your in-memory session/player lists in the exact output layouts
  (stride 0x1c for sessions with the state mapping shown; stride 0x10 for players). The UI/lobby reads these.
- **Slots 22/23 observers** — keep the vector semantics; the game uses these to get join/result/error events, so
  your Storm session events must fire into the registered observers.
- The session-manager global `*0x11e42c90` (used by 5 and 13 via vtable+0xc) is the FCX online session-manager; a
  LAN provider must either keep it alive enough to satisfy those calls or route around it. This is the same
  online-service object that gated everything in NOTES 28dj-28dq — so B' does not magically dodge that gate, it
  moves the problem into these 5 methods.

## Effort / risk (unchanged by this spec, now made concrete)

- 24-slot interface, ~5 network-specific methods + lifecycle + events to author against an undocumented vtable
  ABI; the create path in particular must correctly drive `storm::echo` session creation offline (ports 9000-9004)
  AND satisfy the `*0x11e42c90` session-manager contract that callers already depend on.
- Killer risk stands: no golden reference of FC3 ever running a non-RV session, so every method is validated
  against our own reconstruction. Recommend a single spike: implement slots 15+13+5+22/23 minimal, install via
  vptr swap on singleton 0x11e684e8, and check whether a Storm listen session appears on 9000-9004 — that is the
  go/no-go for the whole B' route.
