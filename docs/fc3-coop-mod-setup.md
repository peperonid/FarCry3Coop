# Far Cry 3 co-op mod setup — the recipe that actually works (and the version truth)

> Bilingual: English here · [Русская версия](fc3-coop-mod-setup.ru.md)
> This is an **operational recipe** for reproducing a working offline co-op baseline on a PC, and it
> resolves the long-standing "which version is our build?" question. Private local paths are written
> as placeholders (`<GAME_DIR>`, `<DOWNLOADS>`, `<UPC_DIR>`) — keep real paths out of the PR.

> **TL;DR / status.** mod 262 gives **SOLO offline co-op** — one human plays the co-op maps
> offline; this works and is the shipped result. **True multi-human co-op is NOT delivered**: the
> in-process online route (emulating Ubisoft's RendezVous) is **parked as research** — the RV
> transport works and the server answers, but the per-packet `prudps` cipher is applied by an
> OpenSSL-RC4/vtable routine we cannot isolate statically or hook live. Full technical write-up:
> [Strategy D findings](strategy-d-findings.md).

## 0. What this document is

This is the **release guide** for the **mod-262 SOLO offline co-op** baseline: it tells you how to
reproduce the one working setup, what it does and does *not* give you, and the version truth behind
it. It is paired with [Strategy D findings](strategy-d-findings.md) (why the online multiplayer
frontier was parked) and `NOTES.md` (the raw session log).

## 1. The working recipe (confirmed by the owner on 2026-11-13)

Build the offline co-op setup **in this exact order**, on a **clean** copy:

1. **Fresh 1.06 base** installed normally through Ubisoft Connect (`upc`).
2. **`Far CRy3 FIX Co-oP.rar`** applied on top — this installs the **1.05-version game files**
   (`FC3.dll`, `FC3_d3d11.dll`, `farcry3.exe`, `farcry3_d3d11.exe`) over the 1.06 base. This is the
   step that makes co-op-capable binaries run.
3. **`The Dependencies-262-1-01-1737371190.7z`** — the mod-262 dependency loaders
   (`uplay_r1_loader.dll`, `ubiorbitapi_r2_loader.dll`, and `uplay_r14.dll` which goes into
   `<UPC_DIR>` = the Ubisoft Connect launcher directory).
4. **`patch_solo_offline_coop_vanilla-262-1-01-1658304414.7z`** (Nexus "mod 262 Solo Offline Coop")
   — content (`data_win32\patch.dat/fat`) + the mod's patched `bin` files.

After these four steps the offline co-op works (this is the state the owner re-created after our
earlier attempts failed).

## 2. Why the version was confusing — resolved

The repo (and `NOTES.md`) had variously labelled the running build "1.06". The truth:

- **Retail 1.06** FC3 DLLs are **~40 MB**: `FC3.dll` 40,720,424 B (SHA-256 `71d58a7a…`),
  `FC3_d3d11.dll` 40,869,416 B (SHA-256 `e541594e…`) — these match the hub hashes
  (`coop-anything/games/far-cry-3/README.md`) and are preserved in the local backup
  `_backup_106_before_solo262`.
- The **co-op setup runs 1.05-era FC3 DLLs**: ~30 MB (`FC3.dll` 29,994,856 B, `FC3_d3d11.dll`
  30,057,832 B, internal dates 2022-07), installed by **`Far CRy3 FIX Co-oP.rar`** (step 2).

So: a **1.06 base**, but the **FC3 binaries are the 1.05 build**. That is why:
- the in-game banner shows **"V1.05"**,
- the co-op patch-site addresses (single-instance mutex `0x100041bb`/`0x10004092`, party loop,
  end-session loc-id table) match **Bantila's 1.05** notes,
- yet the base/install plumbing is Ubisoft Connect 1.06.

The earlier confusion ("is our build 1.05 or 1.06?") is because it is **both**: 1.06 install + 1.05
FC3 binaries brought in by the co-op fix. Address notes must be labelled by **which FC3 binary**, not
by the game patch.

## 3. Reference-install caveat

`<REAL_UPLAY_INSTALL>` (the Ubisoft Connect game install folder) was used as a "reference/known-good"
copy in earlier sessions. The owner reports that **its files were not fully correct** — restoring a
"mod 262" state from it did NOT reproduce a working co-op. Treat it as unreliable for the co-op
stack; rebuild from the four archives in §1 instead.

## 4. Ubisoft Connect auto-update hazard

mod 262's upc-side patch (`uplay_r14.dll` in `<UPC_DIR>`) is dated 2024, while Ubisoft Connect
self-updates. When upc updates, the mod's offline-coop hook can break (observed: co-op start crashes
in `MSVCR100.dll` 0xc0000005 with everything else correct — see `NOTES.md` 28bj). A clean re-run of
§1 restores it. Keep this in mind if the mod "worked before and crashes now".

## 5. Related co-op mods present locally (leads, not yet analysed)

- **`Far Cry 3 Open World Co-op (Patch 1.5)`** (scubrah's mod, ~2.7 GB) — a full **open-world**
  co-op mod. This is closer to the campaign-co-op goal than mod 262 (which is solo-offline) and is a
  strong candidate to study for multi-human co-op. Not yet analysed.
- `FC3No-IntroFix` — minor quality-of-life.

## 6. Status of our own tooling in this stack

Our `uplay_proxy` (`uplay_r1_loader.dll` fake-online) and the Route-B `coop_shim` were built against
the **vanilla menu → RV** path and the 1.06/1.05 FC3 binaries. With mod 262 installed, **mod 262's own
loaders are the ones that must be in `<GAME_DIR>\bin`** — our proxy is an alternative implementation
of the same Uplay-fake idea and was NOT the working config (mixing them caused the `MSVCR100` crash).
Keep mod 262's loaders in place for the mod-262 baseline; our proxy/shim are for the separate
custom-bridge experiments. Everything we wrote is backed up (`.orig` / `.bak-<stamp>`).

**Online tooling status (updated 2026-11-14).** After the full RV investigation (sessions 28f5–28g10,
summarised in [Strategy D findings](strategy-d-findings.md)) the custom online/co-op-emulation tooling
(`coop_shim`, `rv_server`, the RV observers) is **parked**: RV is reached on a real login and the
Ubisoft server answers, but the `prudps` per-packet cipher could not be decrypted offline and could not
be captured live (it sits below every hookable layer). This does **not** affect the mod-262 SOLO
baseline, which uses mod 262's own loaders only.

## 7. What this gives you, and its limits

**Gives you:** a fully playable **solo offline** Far Cry 3 with the co-op maps/levels reachable without
Ubisoft services — the stable baseline this repo ships.

**Limits (by design, not a bug):**
- **One player only.** mod 262 is *Solo Offline Co-op*: it lets the co-op content run offline for a
  single human. It does **not** bring in a second/third/fourth real player.
- **No online multiplayer.** Joining other humans requires the RendezVous/`prudps` online stack, which
  is Ubisoft-server-side and, for us, parked — see [Strategy D findings](strategy-d-findings.md).
- **Auto-update fragility.** Ubisoft Connect updating can break the mod's hook (§4); re-run §1.

**Not a shortcut — OW15 is NOT the answer.** scubrah's **Open World Co-op** (§5) does real multi-human
co-op, but it has no netcode of its own: it drives FC3's *native* RendezVous co-op lobby on Ubisoft's
servers, which are dead (kill-switched), so it fails with the same **"servers unavailable"** error and
**does not work offline** (owner-confirmed; see [OW15 checklist](ow15-online-coop-checklist.md) and
`NOTES.md` 28dm). It is therefore not a workaround for our blocker — it *shares* it. Its only value is
as a **reference** for how FC3's native co-op lobby is driven (a map of what an RV emulator would have
to reproduce). The only route to offline multi-human remains emulating the dead RV service ourselves —
parked (see [Strategy D findings](strategy-d-findings.md)).

Internal Ubisoft paths and the owner's real paths stay LOCAL; artifact names above are public Nexus
names.
