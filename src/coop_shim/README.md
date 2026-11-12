# coop_shim - Route B session-establishment shim (SCAFFOLD)

**Status: SKELETON, untested, NOT deployed, NOT written to the game.** This is the design scaffold for the offline
campaign-co-op plan in [`docs/coop-online-routes.md`](../../docs/coop-online-routes.md) (Route B), backed by the reverse
engineering in `NOTES.md` 28r-28ad. Build with `build_x86.bat` (x86); it produces `coop_shim.dll` here only.

## What it does (intent)
Far Cry 3 campaign co-op session establishment is RendezVous-bound end to end (NOTES 28ac/28ad): the gate `FUN_11279ba0`
requires the online flag (`FUN_11245090`) and an RV `SandboxUrl` (`FUN_112492a0`), and the handlers it releases into are RV
services (host -> SubAccountClient `FUN_11275d90`, join -> UbiAuthentication `FUN_1127a1d0`). The `SessionTicket` is MD5-only
(NOTES 28z). This shim redirects that path to a local `storm::echo` LAN session:

| Site (RVA) | Original | Shim intent |
|---|---|---|
| `online_flag` `FUN_11245090` | reads online-enable flag | return 1 |
| `rv_sandboxurl` `FUN_112492a0` | RV net-op, needs `SandboxUrl` | fill a LAN address, return 1 |
| `gate` `FUN_11279ba0` | RV gate, out = address+port | succeed with LAN address+port |
| `host_handler` `FUN_11275d90` | `SubAccountClient` session | `CFCXCreateSessionOperation` (`NetEngineType=LAN`) |
| `join_handler` `FUN_1127a1d0` | UbiAuthentication | `CFCXJoinSessionOperation` to the LAN address |
| `ticket_validate` `FUN_11254be0` | MD5 validate | accept a locally-derived MD5 ticket |
| `ticket_prepare` `FUN_1124e5e0` | AcquireTicket network | skip network, feed the local ticket |

MD5 is implemented locally (no asymmetric crypto anywhere); both peers running the same shim derive the same key.

## What is real vs TODO
- Real: the RVA map, the 5-byte inline-detour plumbing, the MD5, the config/env plumbing, inert-by-default guard.
- TODO (need a running client to fill/validate - the agent cannot attach; FC3 blocks injection):
  1. Exact struct ABIs for the gate out-address (`FUN_11232e20`/`FUN_11232ef0`) and the `rv_sandboxurl` out struct.
  2. The `storm::echo` session-op registry calls (`CFCXCreate/JoinSessionOperation`, op registered by name via `FUN_1008bf50`).
  3. Per-hook saved bytes + trampolons (currently a single shared 5-byte slot) and calling-convention / stack balance per site.
  4. Ticket input fields to mix into the MD5 so it matches the shared session identity (NOTES 28aa InitiateLogin bundle).
  5. A module-md5 guard (patch only if `FC3_d3d11.dll` md5 == `46bcc097`).
  6. An injection/loading path (DLL-load-order hijack or reuse the `src/uplay_proxy` loader to call `init()`).

## Config (env)
`FC3COOP_SHIM=1` enable (else inert) - `FC3COOP_SECRET=<str>` shared ticket seed - (TODO) `FC3COOP_HOST_IP`, `FC3COOP_PORT`.

## Safety
Read-only to the game: this folder builds a DLL here; copying it into a game install is a deliberate owner step and the
real install `H:\Uplay\Far Cry 3\` is read-only by project rule. Test only on the working copy `H:\FarCryCoop\TestFC3_105\`.

## Build
```
build_x86.bat   -> coop_shim.dll   (x86; needs VS 18 + vcvars32)
```


## 28af refinement (ABI now concrete)
- Online flag = global `DAT_11f07e48` (RVA `0x1f07e48`); the scaffold writes 1 at init (the hook is kept too).
- Gate out-struct = `sockaddr_in`: family@+0 (AF_INET=2), port@+2 (network), IPv4@+4 (network) - NOTES 28af. `hk_gate` builds this (htons/htonl).
- Added hook `rv_connected` (`FUN_11237670` -> 1), required by AcquireTicket + the ticket builder.
- `derive_local_ticket` still hashes only `FC3COOP_SECRET`; the exact shared fields (session id / playlist / account, NOTES 28aa) need a live run.

## Runtime test checklist (owner-side; the agent cannot attach - FC3 blocks injection)
1. Build: `build_x86.bat` -> `coop_shim.dll` (x86).
2. Load it into the working copy `H:\FarCryCoop\TestFC3_105\` (NOT the real install): DLL-load-order hijack, or have the existing
   `src/uplay_proxy` loader call the shim's `init()` (it already sits in the load path).
3. Set env before launch: `FC3COOP_SHIM=1`, `FC3COOP_SECRET=<same on every peer>`, (TODO) host IP/port.
4. Launch 2 instances (see `scripts/fc3_lan_coop_launch.bat`). Watch `OutputDebugString` (DebugView) for `shim:` lines:
   `online global ...=1`, `gate -> success (LAN ...)`, `host -> CFCXCreate...`, `join -> CFCXJoin...`, `ticket_validate -> accept`.
5. Success signal: a co-op session forms with NO "service unavailable"/ticket error and both clients share one world.
6. Crash at a hook => that site's saved-bytes/calling-convention is wrong (trampoline TODO): re-disassemble that RVA and fix.


## 28ag correction (op registry + correct host/join lever)
- storm::echo session ops are created by NAME through a C++ op-type registry (`FUN_1008bf50` -> `FUN_1008bff8`; descriptors `DAT_11e6b208`/`DAT_11e74ee0`/`DAT_11e42de8`) with an async op lifecycle (factory vtable + param block + refcount). A 5-byte detour cannot hand-build them, so `hk_host`/`hk_join` do NOT build CFCX ops.
- Correct lever: (1) force `NetEngineType=LAN` (net-engine `FUN_1092d256` `param[0xe]=0`; op-param field id `0x3c`) and (2) short-circuit the RV auth handlers `FUN_11275d90` (SubAccountClient) / `FUN_1127a1d0` (UbiAuthenticationClient) to no-op success; the game's own storm::echo LAN op then forms the session. Join precondition `FUN_1127a0d0` is unlocked by the `rv_connected` hook.


## Status (NOTES 28ah) - static analysis EXHAUSTED
Route B static work is complete: everything derivable from `FC3_d3d11.dll` (md5 46bcc097) by static RE is in NOTES 28r-28ag. The scaffold
compiles and encodes the design. **Nothing further can be closed statically.** The only remaining step is a human-run live test of `coop_shim`
on `H:\FarCryCoop\TestFC3_105\` (the agent cannot attach - FC3 anti-injection). Open items live-test resolves: the `NetEngineType`/
`SearchIdAddress` write site, per-hook trampolines/calling-convention, "does the LAN storm::echo op form a session once RV auth is short-circuited",
and the shared ticket field set.

Full step-by-step run procedure (build, load via the proxy, DebugView, smoke + join, record/rollback): see [RUN.md](RUN.md).
