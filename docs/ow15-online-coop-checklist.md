# OW15 Open World Co-op — online 2-player checklist (GOAL 1)

How to play Far Cry 3 co-op with a friend ONLINE (no port forwarding) using the
"Far Cry 3 Open World Co-op (Patch 1.5)" mod (pack `H:\FarCryCoop\Downloads\OW15`, archive
`Far Cry 3 Open World Co-op (Patch 1.5)-220-1-5-...zip`, 2.7 GB).

This route needs live Ubisoft Connect. OW15 multiplayer runs through Ubisoft's own
co-op sessions, which relay through Ubisoft servers, so no port forwarding is required.
(For fully OFFLINE co-op this mod does NOT work — that is a separate unsolved goal.)

## 0. What OW15 is / is not
- It IS host/client multiplayer co-op over Ubisoft online (readme changelog: "host no longer kicked when CLIENTS have packet loss").
- It is NOT offline. It needs Ubisoft Connect logged in + online.
- It REPLACES the world: "You cannot play single player nor vanilla co-op maps while this mod is installed." Back up first.
- Play flow (readme): create a PRIVATE co-op session from the main menu, set the chapter to ROOK ISLANDS (or fc3_main).

## 1. Base version (the #1 gotcha) — 1.04/1.05, NOT 1.06
- OW15 (and mod-262) are built for the **1.04/1.05** FC3, whose `bin\` uses
  `uplay_r1_loader.dll` + `ubiorbitapi_r2_loader.dll` (the OW15 pack ships these).
- A genuine **1.06** install uses `upc_r1_loader.dll` instead and will NOT match the mod.
- The pack's `FC3_d3d11.dll` (md5 `07abaf66`, 30,057,832 bytes) is the 1.05-era binary.
- Action: start from a clean **1.04/1.05** FC3 install. Use the SAME base version on BOTH PCs.
- Do NOT reuse this repo's experiment copy `H:\FarCryCoop\TestFC3_105` — it holds the
  mod-262 OFFLINE loader and is not a clean OW15 base.

## 2. Each player's PC (do this on BOTH)
1. Back up your clean FC3 game folder first (OW15 overwrites many files).
2. Start from a clean 1.04/1.05 FC3 install (verified/installed via Ubisoft Connect).
3. Make sure NO other co-op mod is present (not mod-262, not Open World 1.0, no proxy/offline loader). `bin\` should have the genuine base files, nothing custom.
4. Copy the OW15 "Far Cry 3" folder from the extracted patch OVER the game install (readme step). If Ubisoft Connect later overwrites files, re-copy OW15 again after any "verify/repair".
5. Install Ubisoft Connect, log in with your OWN Ubisoft account (each player needs their OWN legit FC3 + account; no shared account).

## 3. Launch
- Launch the game the normal way through Ubisoft Connect (online). Do not use any offline/proxy loader.
- Make sure Ubisoft Connect has internet + firewall allows it (and the game). No game port forward needed (Ubisoft relays co-op).

## 4. Host (player 1)
1. Main menu -> Co-op -> Create game -> set it to PRIVATE.
2. Set the chapter to **ROOK ISLANDS** (EN) or **fc3_main** (if the language shows that name).
3. Wait until the session/world is loaded and you are in the open world.

## 5. Client (player 2)
1. In Ubisoft Connect (overlay) add each other as friends first.
2. Join the host's private session via the Ubisoft friends list ("Join game") or accept the host's invite.
3. When asked for the mission/chapter, pick the SAME one as the host (ROOK ISLANDS / fc3_main).

## 6. Verify it works (this is the confirmation the task asks for)
Both players are in the SAME open world at the same time and it stays synced:
- [ ] You can see each other's characters move in real time.
- [ ] Both can shoot / drive; the game does not kick either of you.
- [ ] Moving far apart does not drop the connection (this is the no-port-forward proof via Ubisoft relay).
- [ ] If one dies, the other can revive / the session does not end.
- [ ] Rejoin works if the client reconnects.

## 7. Troubleshooting
- Game won't start / "another instance": a stale lock — reboot or delete leftover process; do NOT mix the mod-262 multi-instance patch with OW15.
- "Servers unavailable"/can't create session: Ubisoft Connect not logged in / offline, or wrong base version, or OW15 files overwritten by a verify (re-copy OW15).
- One player can't join: version mismatch between the two PCs, OW15 sub-version mismatch, or not friends in Ubisoft Connect. Make both PCs identical (same base + same OW15 1.5).
- Antivirus/Defender blocking Ubisoft Connect: allow it.
- Player count: vanilla FC3 co-op is 2 players; this checklist targets 2. If you need 3-4, check the mod page whether OW15 Patch 1.5 supports more than 2 — do not assume it does.

## 8. Important honesty notes
- The OW15 pack bundles the same offline uplay loader that mod-262 uses, but OW15 multiplayer
  still requires live Ubisoft servers (confirmed by the owner). Treat it as an ONLINE-only mod.
- If the goal later becomes fully OFFLINE co-op, OW15 does not help; that needs the local
  RendezVous/auth emulation tracked separately (see NOTES 28cw-28dl).
