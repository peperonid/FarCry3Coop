# H3 live-test runbook (co-op mode switch)

A safe, ordered procedure to try the two session-7 Phase-2 levers (`switchCoop`, `populate`) against a
running Far Cry 3 1.06 using `scripts/fc3_h3_trace.js`. Only the owner runs this: it touches a live game
on the owner's machine. See NOTES.md "Session 7" for the mechanism and the disasm-confirmed ABI. Russian:
[h3-live-test.ru.md](h3-live-test.ru.md).

## Safety

- Use the **working copy** `H:\FarCryCoop\TestFC3\` only. Never the real install `H:\Uplay\Far Cry 3\`.
- The game must be in **offline mode**. Frida injection into an online / Ubisoft Connect session risks an
  anti-cheat flag; the process also has an `OpenMutexA("FarCry3Instance")` guard. One instance, offline.
- Make sure your single-player save is where you expect, and accept that a live call can **crash** the
  game (we call the engine switch-virtual outside the dispatcher's context). Offline, so no ban risk.
- Back up the working copy's save dir before the first live (`false`) call.

## Prerequisites

Frida on PATH (`frida`, `frida-ps`), game running offline to the main menu, `FC3.dll` at its 1.06 build
(the script re-anchors from `base`; addresses are 1.06, base `0x10000000`).

**Frida is NOT installed on the owner's PC yet** (checked 2026-11-08: no `frida`/`frida-ps` on PATH, no
`frida`/`frida-tools` in the active Python, no `frida.exe` in the project or `H:\FarCryCoop`). Install it
before step 2, e.g. `python -m pip install frida-tools` (this pulls the matching `frida` core and puts
`frida`, `frida-ps`, `frida-trace` on PATH). Installing is a download, so the owner runs it (or approves
it). Confirm afterwards with `frida --version` and `frida-ps -a`.

**Capture the log.** The script prints all evidence with `console.log` (the `[dispatch]` lines, and the
return values of every `rpc.exports.*`). Save the whole REPL session so it can be reviewed afterwards.
In PowerShell: `frida -n FC3 -l scripts\fc3_h3_trace.js 2>&1 | Tee-Object -FilePath H:\FarCryCoop\scratch\h3_live.log`.

## Steps

1. Launch the game offline and reach the **main menu** (do NOT start a normal single-player mission).
2. Attach:
   ```
   frida -n FC3 -l scripts/fc3_h3_trace.js
   ```
   You should see `attached dispatch @ ...`, `attached installer/callerA/callerB`, and
   `fc3_h3_trace loaded. base=...`. If `FC3` is not found, get the pid with `frida-ps -a` and use
   `frida -p <pid> -l scripts/fc3_h3_trace.js`.
3. **Read-only check.** In the REPL:
   ```
   rpc.exports.dumpService()   // service this + [service+0x8c0] container + install-globals
   rpc.exports.lastMode()      // should be null so far (dispatch hook has not fired)
   ```
4. **Cache the mode-manager `this`.** In the game, enter and leave a menu/lobby once (e.g. open the
   pause menu, or start + quit a mission) so the dispatch hook at `0x113405a0` fires. Then:
   ```
   rpc.exports.lastMode()      // thisPtr must be non-null now; id = current mode (0x43521e6f = CO-OP)
   ```
   The `[dispatch] this(ebx)=... [this+0x4c]=...` lines in the log are the live evidence. If `thisPtr`
   stays null, the hook is not firing (wrong build / not reached) — stop and report, do not poke.
5. **H3a dry run.**
   ```
   rpc.exports.switchCoop(true)   // prints modeManager this, curMode, the vtable+0x5c addr, the id
   ```
   Sanity: `vtable5c` should look like code in `FC3.dll`, `curMode` a plausible current mode.
6. **H3a live.**
   ```
   rpc.exports.switchCoop(false)  // writes [this+0x4c]=0x43521e6f and calls vtable[+0x5c](&id)
   ```
   Watch the game window and the Frida log. Record the return value and what happened on screen.
7. **H3b (if H3a is not enough).**
   ```
   rpc.exports.populate(true)     // dry run: prints service this + FUN_1127a0f0 addr
   rpc.exports.populate(false)    // calls FUN_1127a0f0(service) to build the co-op entry
   rpc.exports.dumpService()      // re-read [service+0x8c0] to see if the container changed
   ```

## Record results

Paste the Frida log (especially the `[dispatch]` lines and any return values), the screen behaviour, and
whether the game crashed, back into NOTES.md (new dated sub-section) and update the supermemory "current
state" record. A crash with the exact `this`/`vtable5c` values is still a useful result (it tells us the
context problem, see the NOTES session-7 ABI addendum).
