# Far Cry 3 co-op over a virtual LAN (Radmin VPN / Hamachi) — 2-PC host & join procedure

> Bilingual: English here · [Русская версия](fc3-coop-radmin-setup.ru.md)
> Operational recipe for trying **2-human co-op** on Far Cry 3 (Uplay 1.06 + 1.05 binaries) across two
> PCs that are **not on the same physical LAN**, by joining them into one virtual LAN (Radmin VPN or
> Hamachi) and using the game's engine **direct-connect** CLI. Private local paths are written as
> placeholders (`<GAME_DIR>`); keep real paths out of any PR.

> **TL;DR / status.** mod 262 ships **SOLO offline co-op** (one human) — see
> [fc3-coop-mod-setup](fc3-coop-mod-setup.md). Ubisoft **shut down FC3 online servers on 2024-03-06**, so
> online co-op / the RendezVous lobby is permanently dead; the only route to 2+ humans is the game's own
> **LAN/Storm** provider over a virtual LAN (the community-proven method). **This build has NO in-game LAN
> session browser**: the Co-op → *Local Session* menu only ever opens your **own** "ЛОКАЛЬНАЯ ИГРА" host
> lobby (identical on host and guest). A second player therefore **joins by a separate launch with
> `-client -ip <host-IP>`**, not by a menu Browse. **This 2-PC flow is the decisive unverified test**; it
> is grounded in binary evidence (NOTES 28i22), not yet confirmed end-to-end by the owner.

## 0. Prerequisite — the working solo baseline

Both PCs must already reproduce the mod-262 **solo offline co-op** baseline and boot to the menu offline.
If that does not work on a PC, stop and fix it first ([fc3-coop-mod-setup](fc3-coop-mod-setup.md)).

## 1. Identical patched build on BOTH PCs

The two machines must be bit-for-bit compatible or the join is rejected with a version/session error:

- Base game **1.06** with FC3 binaries at the **1.05** level (stock `FC3_d3d11.dll` md5 `46bcc097`).
- The **multiplayer-fix MOD chain** that unlocks the Local co-op session: `Far CRy3 FIX Co-oP` →
  `The Dependencies-262` → `patch_solo_offline_coop_vanilla-262` (same recipe as the solo baseline).
- Run **`scripts/patch_dll.py`** on `<GAME_DIR>\bin\FC3_d3d11.dll` on **each** PC → md5 becomes
  **`03ff4a9b`** (removes the single-instance mutex + party-loop + end-session guards).

Verify on each PC:

```
certutil -hashfile "<GAME_DIR>\bin\FC3_d3d11.dll" MD5      # must be 03ff4a9b... on BOTH
```

## 2. The virtual LAN (Radmin VPN / Hamachi)

- Install **Radmin VPN** (or Hamachi) on both PCs and join **one** network (same Network ID + password).
- In Radmin enable **Direct Data Exchange** (peer-to-peer; otherwise relayed traffic breaks the P2P ports).
- Put the VPN interface in the **Private** network profile (Domain/Private, not Public) so the firewall is friendly.
- Note the **host's** VPN IPv4 (Radmin default `26.x.x.x`; Hamachi `25.x.x.x`) from the VPN window or `ipconfig`.

## 3. ⚠️ Disable the extra virtual NICs (the #1 failure cause)

FC3 binds its net engine to the **first** adapter it finds. On this PC family it grabbed
`172.19.128.1` (a `vEthernet (WSL)`/Hyper-V/Docker NAT adapter), **not** the VPN `26.x` address — so the
guest's `-ip 26.x.x.x` reached nothing. Do this on **both** PCs:

1. `Win+R` → `ncpa.cpl`.
2. **Disable** every `vEthernet (WSL)`, `vEthernet (Hyper-V ...)`, Docker/`Ethernet*`, VMware/VirtualBox adapter.
   Leave only the real physical NIC + the VPN adapter.
3. Start the host, then verify the engine bound the VPN IP:

```
powershell -File scripts\fc3_host_engine_check.ps1     # expect UDP 9000/9004 on the 26.x IP, NOT 172.19.128.1
```

If the host is on `172.19.128.1`, the guest can never reach it — fix the adapter before testing further.

## 4. Firewall (both PCs, run as Administrator)

```
scripts\fc3_lan_firewall.bat
```

Opens **UDP 9000–9005** both directions and allows the game exe. Minimum manual equivalent: allow the
game through Windows Firewall and keep the VPN interface on **Private**.

## 5. Host (on the host PC)

```
scripts\fc3_lan_radmin.bat host
```

This runs `farcry3_d3d11.exe -host coop_vidar -login Host -coop ...` and **starts a hosted co-op session**
(the net engine listens on its ports). Let it load fully to the lobby/into the game before the guest connects.
`coop_vidar` is a co-op map; `fc3_lan_radmin.bat` lets you keep the same one — pick any co-op map you like.

## 6. Join (on the guest PC) — the part that is NOT in the menu

Because there is no LAN browser, the guest **must** be launched by CLI so the engine direct-connects into
the host's session. On the guest:

```
scripts\fc3_lan_radmin.bat client <HOST-VPN-IPv4  e.g. 26.x.x.x>
```

This runs `farcry3_d3d11.exe -client -login Guest -ip <host-IP> -coop ...`, which dials the host session
directly and bypasses the menu entirely. **Do not** just open Co-op → Local Session on the guest — that
only self-hosts the guest's own lobby (you will see "only myself in slot 1", the classic wrong path).

Order: host fully loaded → **then** launch the guest with `-ip`. If it works, the **host's lobby shows a
second name in slot 2**, then both press Ready.

## 7. Checklist (run top to bottom if it fails)

1. Host listens on the **VPN 26.x** IP (step 3), not `172.19.128.1` — the usual culprit.
2. Both PCs in the **same** VPN network; Radmin **Direct Data Exchange** on.
3. Firewall UDP 9000–9005 open, VPN profile = **Private**.
4. `FC3_d3d11.dll` md5 = **`03ff4a9b`** on both; same game version.
5. Distinct `-login` (`Host` / `Guest`) and distinct `-SavePath`.
6. From the guest, `ping <host 26.x IP>` succeeds (if not: VPN not Private / blocked).

## 8. What to report after the test

- Did a **second name appear in the HOST lobby slot 2**?
- If not, the **guest's error text** (`INVALIDSESSIONKEY` / timeout / "cannot connect" / nothing):
  - network/port failure → adapter binding or firewall (steps 3–4);
  - join-rejected → the session provider itself (needs the R2 join work, see below).

## 9. Why the CLI join, in the binary

FC3_d3d11.dll exposes a host/client direct-connect CLI and session-join objects, no UI browser:
`-host` (near `missing (-login) .. localhost`), `-client`, `-ip` (`-ip .. for -client`), `-coop`,
`-login`; and `CFCXJoinSessionOperation`, `CJoinSessionParam` / `CJoinGameSessionParam`, `JoinSessionId`,
`CSessionImpl::RemoveClient` ("Non-host leaving"). Recorded in NOTES 28i22. Deeper LAN-provider design:
[R2 LAN-provider plan](r2-lan-provider-plan.md). Raw session log: `NOTES.md`.


## Game folder is auto-detected

The LAN scripts (`fc3_lan_radmin.bat`, `fc3_lan_host_only.bat`, `fc3_lan_same_subnet.bat`,
`fc3_lan_firewall.bat`) do **not** hard-code the install folder. They resolve it by priority:
1. environment variable `FC3_GAME`;
2. a file `game_dir.txt` next to the `.bat` (a single line with the game path);
3. an interactive prompt on first run.
On the 2nd PC just run the script and type the game path when asked (or set `FC3_GAME`, or put the
path into `game_dir.txt`). The script checks that `bin\farcry3_d3d11.exe` exists. Save folders are
created next to the script (`save_coop` / `save_host` / `save_lan`).
