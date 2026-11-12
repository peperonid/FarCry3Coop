# Running `coop_shim` on the test copy - step-by-step (NOTES 28ah)

**What this run actually is (read first).** The scaffold in `src/coop_shim/` compiles and, when enabled, patches 8 game functions
and forces the online global. But it is still a **smoke test**, not a working co-op yet: the per-hook saved-bytes / trampolines are
shared in one slot (see `patch_5`/`install_hooks` TODO) and the hook shims do not call the original function, so a hook whose calling
convention (thiscall/fastcall vs the shim's cdecl) mismatches can corrupt the stack. Expect to gather **evidence** (does it load, do the
`shim:` lines print, where does it crash) rather than a finished co-op session. That evidence is exactly what closes the remaining TODOs.

Safety: work only on the working copy `H:/FarCryCoop/TestFC3_105`. Never touch the real install `H:/Uplay/Far Cry 3` (read-only by rule).

---

## 0. Prerequisites
- Visual Studio 18 (any Community) on `PATH` via `vcvars32` (x86). The build script calls it.
- The test copy present at `H:/FarCryCoop/TestFC3_105` with the loader proxy already deployed at
  `.../bin/uplay_r1_loader.dll` (the ABI-fixed proxy; real Ubisoft loader lives in `.../bin/real/`).
- **DebugView** (Sysinternals `dbgview.exe`) - `coop_shim` logs with `OutputDebugStringA`, so DebugView sees it without a debugger.
- Optional: a second PC on the same LAN for the real join test (otherwise use loopback + the tunable, see step 7).

## 1. Build the shim DLL
```
cd src/coop_shim
build_x86.bat        -> produces coop_shim.dll   (expect: BUILD_DONE 0)
```

## 2. Load path (already wired into the proxy)
The one-shot loader is already in `src/uplay_proxy/uplay_proxy.cpp`: `tryLoadShim()` runs from `log_entry` (called by every export thunk,
outside DllMain/loader-lock) and, **only when `FC3COOP_SHIM=1`**, does a single `LoadLibraryA("<proxy-dir>\coop_shim.dll")`. Nothing to edit -
just rebuild the proxy and deploy it:
```
cd src/uplay_proxy
build_x86.bat          -> uplay_r1_loader.dll   (expect: BUILD_DONE 0)
```
Redeploy that `uplay_r1_loader.dll` over `H:/FarCryCoop/TestFC3_105/bin/uplay_r1_loader.dll` (the real Ubisoft loader stays in `bin/real/`).
On the first `UPLAY_*` call DebugView shows `uplay_proxy: LoadLibrary coop_shim ... -> OK`, then the `shim:` init lines. To revert, rebuild
the proxy without the deploy, or just delete `bin/coop_shim.dll`.

## 3. Deploy
- Copy `coop_shim.dll` to `H:/FarCryCoop/TestFC3_105/bin/` (same folder as `uplay_r1_loader.dll`).
- Reverted/rollback = delete that `coop_shim.dll`; the proxy's co-op fake stays OFF by default, so the game runs normally.

## 4. Environment (set in the SAME shell that starts the game)
```
set FC3COOP_SHIM=1
set FC3COOP_SECRET=whateverSameOnAllPeers
```
`FC3COOP_SHIM` must be exactly `1` or the shim loads inert. `FC3COOP_SECRET` must match on every peer (the local MD5 ticket seed).

## 5. Where the log goes
`coop_shim` writes every `shim:` line to **`bin/coop_shim.log`** (next to the DLL; override with env `FC3COOP_LOG=<path>`). Open or tail
that file before launching - no listener and no admin needed. (`OutputDebugStringA`/DebugView still works too, but the file is simpler.)

## 6. Smoke test (single instance) - do this first
1. `H:/FarCryCoop/TestFC3_105/bin/farcry3.exe` (normal launch, solo).
2. In `bin/coop_shim.log` expect, in order:
   - `shim: FC3_d3d11 base=0x... enable=1`
   - `shim: online global DAT_11f07e48=1`
   - eight `shim: patch <name> @0x...` lines, then `shim: hooks installed (SCAFFOLD - untested)`
3. Reach the main menu. **If the game boots to menu with those lines, the patches installed without a boot crash** - good baseline.
4. Try to **Host** a co-op session. Watch which hook fires (`shim: host -> ...`) and whether it crashes. A crash here is *useful data*:
   note the faulting module+offset (the game's exception dialog or a `.dmp`) so the calling-convention of that specific hook can be fixed.

## 7. Two-instance join test (after smoke passes)
- On one PC you also need the echo loopback tunable ON in the game config (NOT on the cmdline): `LanMatchmakingDiscoveryOnLoopbackEnable=1`,
  `LanMatchmakingReuseAddress=1`, and the "LAN Match Making Port" option. Then:
```
scripts/fc3_lan_coop_launch.bat        (host first, wait for world, then client -ip 127.0.0.1)
```
- On two PCs: on the client use the **host's LAN IP** instead of `127.0.0.1`; keep `FC3COOP_SECRET` identical on both.

## 8. Record and hand back (this is the point of the run)
Capture and send back:
- the full `bin/coop_shim.log` (or `FC3COOP_LOG`) tail;
- for any crash: faulting **module + offset** and which `shim: ...` line printed last (tells which hook/calling-convention to fix);
- for a join attempt: whether the world loaded or an error popped, and any `Ticket`/`INVALIDSESSIONKEY`/`GameSessionKey` text.
That closes the four runtime-only TODOs in NOTES 28ah: the `NetEngineType`/`SearchIdAddress` write site, per-hook trampolines/calling-conventions,
"does the native LAN storm::echo op form a session once RV auth is short-circuited", and the shared ticket field set.

## 9. Rollback
Delete `bin/coop_shim.dll` and (if you added it) the `tryLoadShim` proxy edit; the game is back to normal.
