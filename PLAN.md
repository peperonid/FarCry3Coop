# Far Cry 3 — campaign co-op for 4 players: plan

Russian version: [PLAN.ru.md](PLAN.ru.md)

Status: plan, approved by the owner 2026-10-22. Branch `game/far-cry-3`.

## Goal

Four players pass the whole single-player story of Far Cry 3 together, with cutscenes, side
activities (outposts, towers, hunting, Path of the Hunter, Wanted Dead, trials, races) and
collectibles (relics, letters, memory cards). Friends join through a **Steam invite**.

**Game versions (owner decision 2026-10-24):** both are supported: **1.05** (Steam depot 220241,
manifest `7362101836779063707`, DLL build 2018-05-02, the last version with multiplayer, the one
scubrah's mod is built on) and **1.06** (current Steam / Ubisoft Connect, build 2022-10-03; hard to
downgrade to 1.05). Each version has its own addresses; a shared address map is built by the script
`scripts\fc3_addrmap.py`. Ubisoft servers for FC3 are shut down, so joining goes only
through our own transport.

The game has no campaign co-op: only a separate 6-mission co-op campaign in multiplayer.
Prior art: "Open World Co-op" by scubrah (https://www.nexusmods.com/farcry3/mods/220, alpha,
no story missions).

## Architecture

Same shape as the Far Cry (2004) mod: **the host is the authority**. The host runs the real
single-player game (story, AI, mission scripts, save). Joined players run their own copy of the
game and get the host's world replicated.

Two candidate transports; the Phase 2 spike picks one:

- **A. The engine's own network layer:** run the single-player island inside a co-op network
  session. Most sync for free. Risk: single-player systems (missions, shops, skills) may not be
  network-aware. First learn how scubrah's mod starts the island in co-op.
- **B. Own netcode in the ASI:** every player runs single-player; the ASI sends the host's state
  over its own UDP; other players appear as puppet characters; the joined players' AI and mission
  scripts are switched off and driven by host events. More work, full control. Fallback if A
  breaks on story maps.

Parts in both cases:

- `FarCry3Coop.asi` loaded by Ultimate ASI Loader; hooks in `FC3.dll` / `FC3_d3d11.dll` by
  address, found with REA/Ghidra. Pattern: the Far Cry (2004) `FarCryCoopNet.asi`.
- Injection for the first experiments does **not** need the ASI loader: the H1 probe is a
  self-contained `dinput8.dll` proxy (`mod/fc3coop_probe/`, built 2026-11-07) that needs no
  external loader. The ASI + Ultimate ASI Loader path stays the eventual shipping form.
  **Caveat (2026-11-08, NOTES session 5):** the probe's poke of `0x12891ca0/0x12891ca8` is now known
  to be a *diagnostic only* — those are `static const` lookup keys, not the live mode state; the real
  co-op switch is the mode-manager field `[this+0x4c]` and the live playlist container
  `[service+0x8c0]` (`CFCXPlaylistService`, instance global `0x12863890`), which need a runtime trace.
- Script layer: changed mission scripts in our own archive; original files are not replaced.
- **Steam invite:** the game has no `steam_api.dll` (Steam only starts it, Ubisoft Connect runs
  it), so the Far Cry (2004) Steam bridge (Spacewar, AppID 480) is reused: "Join" in the Steam
  friends list, connection relayed through localhost. Direct IP stays as a fallback.
- Launcher and lobby: the Far Cry (2004) launcher (Rust + Tauri) gets a game profile, not a fork.

## Phases

0. **Game identity.** Done 2026-10-22, see [NOTES.md](NOTES.md).
1. **Research (read-only).** Unpack the data with Gibbed.Dunia into `E:\FarCryCoop\Work\fc3`.
   Find mission scripts, the cutscene system, the save format, collectible and progress tables.
   Study scubrah's mod. Open the game DLL in REA: network layer, player spawn, entity replication,
   player cap.
2. **Spike, choose A or B.** Two game windows on one PC (host left, joined player right): both walk
   the island, see each other, shoot the same enemy.
3. **Free roam, 2 players.** Movement, weapons, damage, AI reacts to any player, vehicles, death
   and respawn near the host, outposts captured for everyone.
4. **Story missions.** Start, objectives, failure and completion from the host to all; move
   everyone to the mission start; mission triggers fire for any player; interiors and linear
   levels (Hoyt's ship, dream sequences) load for all.
5. **Cutscenes.** The host triggers, every player plays the same cutscene locally; players are
   frozen and hidden during it; skip by the host. Scripted takedowns and QTE: the host plays,
   others watch.
6. **Side activities.** Towers, outposts, hunts, Path of the Hunter, Wanted Dead, supply drops,
   trials, races; world state comes from the host.
7. **Collectibles and progress.** Owner decision 2026-10-22: campaign progress and statistics are
   shared. Story, side activities, towers, outposts and collectibles count for **every player in
   the lobby**, wherever they stand; the item disappears for all. Personal per player: weapons,
   skills, crafting, money, loot and other personal data.
8. **4 players.** Raise caps, bandwidth and priority limits, test with 4 windows / 4 PCs.
9. **Release.** Launcher profile, Steam invite, install guide (EN + RU), beta checklist.

## Main risks

- Story scripts expect one Jason: forced animations, takedowns, vehicle seats, chases. Plan: the
  host is Jason in scripted parts; others are moved or watch for a short time.
- The save may be tied to the Ubisoft account: joined players may need state sync instead of a
  copied save.
- If the engine's network play needs Ubisoft servers for matchmaking, transport A needs direct
  join (as in Far Cry 2004) or the project falls back to B.

## Checks

Each phase ends with a check in game on one PC, two windows, written into NOTES. Phase 2: see and
shoot each other. Phase 4: the first three story missions together. Phase 5: the Vaas intro
cutscene on all players. Phase 7: a relic taken by a joined player counts and disappears for the
host. Phase 8: 4 windows, one full mission. Phase 9: join through a Steam invite.

---

## Critical path v2 (agent, updated 2026-11-09)

Agent addendum per the owner's standing instruction to keep PLAN current; the numbered plan above
is unchanged. Authoritative evidence: `NOTES.md`.

**What the read-only reverse engineering established (no code shipped):**

1. The offline co-op host is the game's own Uplay / RendezVous session path. The solo-262
   `FC3.dll` is byte-identical to stock; it unlocks offline co-op with loader shims
   (`uplay_r1_loader.dll`, `ubiorbitapi_r2_loader.dll`, `upc_r1_loader.dll`) plus a data patch.
2. Offline co-op is already selectable on 1.05 and 1.06. No session-gate or version blocker is left.
3. Loading the island as a co-op map is a DATA problem (patch.dat world-asset expansion plus the
   mod's small one-shot content-spawn hooks), not a hosting hook. The mod's `FC3.dll` code is a
   content spawner (`FUN_108c4b09`) plus count bumps; it does not create a host session.
4. The one hard problem left for 2-4 players OFFLINE is the native HOST path: FC3's party
   controller (≈ `0x10620400`-`0x10623500`) drives `UPLAY_PARTY_*` (`SetGameSession` declares the
   host, `GetFullMemberList` enumerates peers, `IsPartyLeader` gates the host, `SetGuest` marks
   remotes). The data plane is the RendezVous stack inside `uplay_r14.dll`.

**New critical path (offline-host prototype), pending the owner's go for writing code:**

- A. Map offline, in Ghidra FC3_106, the exact Rv / party call flow a second peer must satisfy.
- B. Prototype a shim that reports a synthesized second party member
  (`UPLAY_PARTY_GetFullMemberList` thunk `0x10CFD84C`, `UPLAY_PARTY_IsPartyLeader` thunk `0x10CFD85E`)
  so the engine believes it hosts a two-player session.
- C. Only if the RendezVous data plane will carry a second OFFLINE instance with no Ubisoft relay,
  run two game windows (host + joined) on one loopback / LAN.

Until B/C, stay read-only: the engine's co-op session transport is online-only by the mod author's
own report (LAN = no), so offline multiplayer needs the host path, not more online joining.

## Critical path v3 — concrete hook points (agent, 2026-11-09, read-only)

Deeper pass over the party controller (FC3_106). The three `FUN_1056ec80 / 1056f3d0 / 103642f0`
that first looked like the logic are only lazy-singleton getters; the real co-op join/host logic is
below. Authoritative record: `NOTES.md` + supermemory (ids `nvjxCfpQn79eavDMBirw1P`, `chzyBsC2o83CHAcLdYT3Uu`).

Key objects (FC3.dll 1.06, Ghidra addresses):
- Master live session object = `*(DAT_12805180 + 4)`. `+0x118/+0x11c` identity args; `+0x120` = the
  host/principal id. The local player id comes from `FUN_1006f990()`; host check is
  `FUN_1006f990() == *(session + 0x120)`.
- Co-op join / reject gate = `FUN_11218480` (virtual). It trusts the current member list built by
  `FUN_11273650(provider, &list)`, where the provider is returned by `FUN_103660e0()`; it compares
  counts against the **config variable `MaxPartySize` at config-block offset `0xa88`**
  (registered `FUN_10065cd0("MaxPartySize",0xa88,0,3)` in `FUN_112250c0`), then emits the
  "Rejecting partyId … max allowed size / team size difference > 2" verdicts.
- Party netcode ctor = `FUN_11275750` (vtable `PTR_FUN_11faba78`) — ships the `CParty*NetMessage`.

**Path 1 (offline host: let the in-game party layer, not the Rv GameSession, decide membership).**
Minimal set of levers, all reversible, to be validated on solo-262 (1.05) first:
1. Raise config `MaxPartySize` (cfg offset `0xa88`) from 1 to the target (2/4).
2. Hook the member-list builder `FUN_11273650` (or its provider `FUN_103660e0`) so the host's
   session reports the desired number of members.
3. Relax/skip the reject branches in `FUN_11218480` so a locally-synthesized member is not refused.
4. Make `*(session+0x120)` equal the local principal so the host/leader gate passes.

**Open question that decides if Path 1 can give >1 real human:** a second game *instance* still has
to find and join a live session; that join goes through the RendezVous `GameSession`
(`CJoinGameSessionParam` `FUN_10207af0`). Synthesizing the host's member list lets one window behave
as a full 2-player session (good for solo, AI-like slots, testing), but two separate windows need
either the Rv GameSession to carry a real second participant (Rv relay/NAT — normally online) or a
second path that bypasses Rv join entirely. **Next read-only step:** decompile `FUN_11273650`,
`FUN_103660e0`, and the `CJoinGameSessionParam` path `FUN_10207af0` to see exactly which list the
joiner validates against, and whether a loopback/UDP Rv join is even reachable offline. No game code
is written until the owner approves step B.
