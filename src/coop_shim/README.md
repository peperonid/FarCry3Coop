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
