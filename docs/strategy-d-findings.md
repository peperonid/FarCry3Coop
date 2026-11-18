# Strategy D — local RendezVous (RV) / `prudps` co-op emulation: consolidated findings

**Scope:** sessions **28f5 → 28g8**. **Status:** the online multi-human RV-emulation
frontier is **PARKED as research**; the shipped, owner-confirmed deliverable remains
**mod-262 SOLO offline co-op** (see `docs/fc3-coop-mod-setup.md`).

English (`strategy-d-findings.md`) · Русский (`strategy-d-findings.ru.md`).

> Privacy: the owner's LAN address, MAC and account identifiers are deliberately **not**
> reproduced here. Public Ubisoft RendezVous addresses are kept (they are public service
> endpoints). This is a research log, not a shipping artifact.

---

## 1. What Strategy D was

Make two-to-four real human players join a **local/private co-op session** on the offline
1.05 build by **emulating Ubisoft's RendezVous service**: redirect FC3's RV client
(DNS → our host) to a local server we implement, so the game believes it is in an online
co-op lobby and lets several instances meet — without port forwarding or Ubisoft's live
lobby. It is the in-process successor to the old "GATOR/online emulation" plan (see
`docs/coop-online-routes.md` and `docs/strategy-d-rv-emulation-plan.md`).

Every earlier "Strategy D is dead" verdict (28f0 and before) rested on one observation:
with **fake** login credentials the RendezVous client never fired. 28f5 broke that.

---

## 2. The reversal — RV IS reached, but only on a genuine login (28f5)

Running the observer-only harness (`run_d0_reallogin.ps1`) on the clean 1.05
(`FC3_clean`, `FC3_d3d11.dll` md5 `46bcc097`) with a **real** Ubisoft Connect account:

- Login advanced to code **9 (LoggingInSucceeded)** then 13 (login stayed stuck).
- **RV-PROBE fired 17×**, all on the RV `WSASendTo` wrapper `FUN_11214630` — i.e. FC3
  actively **sends RendezVous packets**.
- The co-op manager pointer was non-null (the party/online state was alive).

**Conclusion:** fake credentials reach `LoggingInSucceeded` but never arm the deeper
online-service/party state that wakes the RV client; **only a real Ubisoft session does.**
That single fact reopened Strategy D: the RV client's outbound traffic is now reachable
and capturable, which was the "golden reference" the whole plan was blocked on.

---

## 3. Real RV traffic captured; the Ubisoft RV server answers (28f7)

A `pktmon` capture under a genuine login (harness `pktmon_rv_capture.ps1`) produced
**1 400 RendezVous-direction packets** (`scripts/rv_capture/rv.pcap`).

- Destination = the Ubisoft RendezVous cluster:
  - `lb-rdv-as-prod01.ubisoft.com` `203.132.26.146:21251` (x760)
  - `ne1-z3-as-rdv04.ubisoft.com` `203.132.26.143:21251` (x208)
  - port **21251** = the Quazal RendezVous service.
- **The RV server replies** (inbound packets present) ⇒ the RV transport is *alive* at
  Ubisoft; only the higher co-op **lobby/session** logic fails elsewhere.
- Outbound sizes 11/12/15/86 B; inbound 18 B. The **86-byte** outbound packets
  (~176 of them) are the substantive RV commands (auth/create) with ~70-byte encrypted
  bodies. Transport is **not** plain PRUDP v1 (no `0xD0` EA magic) — it is the Quazal
  **`prudps`** secured RV (RC4 + HMAC), not the plain codec our early
  `scripts/rv_server/` spoke.

---

## 4. The `prudps` wire format, decoded offline (28g0)

Re-parsing `rv.pcap` gave a complete byte layout (all observed sizes
11/12/13/14/15/17/18/29/86 B):

| Bytes | Meaning |
|---|---|
| `b0..b1` | magic/direction pair — OUT `3e 31`/`3f 31`, IN `31 3e`/`31 3f` (FC3's obfuscated PRUDP magic, **not** EA's `D0`) |
| `b2` | type/flags — `0x32` = reliable **payload (encrypted)**; `0x24`/`0x0a`/`0x31`/`0x20` = **control** (low entropy; a 15 B type-`0x20` body is **all zeros** = a cleartext keepalive, so *not everything* is encrypted) |
| `b3` | per-packet field that varies across **retransmits** (retransmit/nonce byte) |
| `b4..b7` | **connection identifier — CLEARTTEXT in both directions** (e.g. `3a da dd f9`, `3a da bf db`; inbound shows `8f/92 1a 87 db`) |
| `b8` | sequence byte (increments across distinct messages, constant across retransmits) |
| `b9..` | payload |
| last | **part of the ciphertext**, *not* a checksum — `sum`/`negsum`/`len+sum` all fail (0 of 1384); it is the RC4 keystream byte XOR plaintext, which is why it tracks the sequence counter |

---

## 5. The crypto model (28g0 / 28g1)

- **Per-connection stream cipher.** For each `connId`, the 86 B payload is **byte-identical
  across all ~88 retransmits** of that connection (only `b3` and body offsets 19/20 vary).
  ⇒ the keystream is **constant per connection** and resets per packet.
- **No ephemeral Diffie–Hellman.** No 128-byte DH packets appear anywhere in the capture,
  so — contrary to the 28f8 first guess — `prudps` here runs in **static-key mode**:
  key = `f(connId, STATIC_SECRET)`, not a fresh DH agreement.
- The whole co-op failure is visible here: one RMC message is **retransmitted 88× and never
  ACKed** — that repetition *is* the "servers unavailable" symptom the UI shows.

---

## 6. Where the cipher is — and where it is emphatically not (28g1 · 28g4 · 28g6 · 28g8)

The send path was mapped and every candidate was decompiled (Ghidra project
`FC3d3d11_live`, fully analysed; scripts `DecompSendPath.java`, `DumpVtbl2.java`):

```
FUN_1123cd20 (pump)
  → FUN_1123a870 → FUN_1123a780 (send by connId)
  → FUN_11231860 (reliable-layer fragment/queue)
  → FUN_112390f0 → … → FUN_1121d980 → FUN_112161b0 (data send)
  → FUN_11214630 (RV WSASendTo) → socket
```

- **None of these send-side functions contain a byte cipher.** All `^` XOR operations in
  `FUN_11231860 / 3a780 / 390f0 / 3d140 / 2fd60 / 3cd20` are **zero**; the only XOR-bearing
  functions in the region (`FUN_11236020`, `FUN_11218d40`) use it solely for the
  **`__security_check_cookie` /GS stack guard**.
- The **packet-object vtable at RVA `0x016bebbc`** (all 24 slots) also has **zero real XOR**;
  slot 18 (`FUN_112201e0`) is just a size/fragment serializer (its `0xFF` is a mask).
- Live hooks confirm it three independent times: at `FUN_11214630` (28g4, RV-ENC fired 77×)
  **and** at `FUN_112161b0` entry (28g6 + the 28g8 run) the buffer is **already ciphertext**.

⇒ **The stream cipher is applied by a vtable-dispatched routine that sits below every
observable layer** — i.e. the OpenSSL-RC4 (28f8: statically linked **OpenSSL 0.9.8l**)
reached indirectly through the connection's socket-driver vtable, inside/below
`WSASendTo`. It is not reachable by hooking the Quazal send code above it.

---

## 7. The capture attempts, and the one crash

| Session | Hook | Result |
|---|---|---|
| 28g2 | `RV-ENC` inline on `FUN_11214630` | built; offline extractor (`rv_enc_extract.py`) recovers `keystream = PRE ^ POST` and can `--apply` to the pcap |
| 28g3 | driver-vtable slot swap (`conn+0x9c` `+0x10`/`+0x1c`) | built as a `payload_changed=0` fallback |
| 28g4 | live | RV-ENC fired **77×** ⇒ RV *is* reached, but the captured buffer is **already encrypted**; driver slot read as `0x0` |
| 28g5 | packet-object vtable `+0x4` swap | **CRASHED FC3 on load** — swapping an *unknown* vtable method corrupts the ABI |
| 28g6 | fix | removed the `+0x4` swap; entry dump reconfirms data is already ciphertext at `FUN_112161b0` |

**Hard-won rule:** inline observers on verified `55 8B EC` prologues are safe; **never
blind-swap the vtable slot of an unknown method** — the ABI guess crashes the game.
A tooling bug was also seen and left documented: two inline hooks on the **same address**
(`RV-ENC` and `RV-PROBE` both on `FUN_11214630`) collide, only the first wins.

---

## 8. Why the offline crack (many-time-pad) is data-starved (28g7)

The constant-per-connection keystream invites a textbook **many-time-pad** crack: for one
connection, `CT₁ ^ CT₂ = PT₁ ^ PT₂` (keystream cancels), so two different messages leak
their plaintext XOR. It does **not** work here for a structural reason:

- In `rv.pcap`, the type-`0x32` payload per `connId` is **ONE logical message
  retransmitted** (72 of 74 body columns identical across the "11 distinct" 86 B bodies;
  only `b3` + offsets 19/20 vary). **One message ⇒ nothing to XOR.**
- Control types `0x0a`/`0x24` are effectively plaintext (last byte = a sequence checksum)
  but carry **no RMC payload**.

The richer 28g4 log *did* hold multiple distinct `0x32` messages, but `coop_shim.log` was
overwritten before it could be persisted. Tooling was hardened after that (the stop-script
now copies the log to `local/scratch/coop_shim_last.log` and auto-runs the extractor), and
`rv_enc_extract.py` gained an `mtp()` mode — but the fundamental data limit remains: **the
broken co-op join only ever sends one distinct message**, so the second message the pad
needs is exactly the thing the bug prevents.

---

## 9. Decision (28g8)

To read RV plaintext offline we would need one of:

- **(a)** a much deeper **OpenSSL-RC4 key-schedule / key-derivation** reverse
  engineering of the static OpenSSL in `FC3_d3d11.dll` (it has repeatedly resisted; high
  cost, low odds), or
- **(b)** a peer that **ACKs** so the client emits *diverse* RMC messages — which is
  chicken-and-egg with the very join that is broken.

Both are high-cost/low-probability against a game whose RV transport works but whose
**co-op lobby logic is dead server-side**. So the online multi-human frontier is **parked**.
The realistic, owner-confirmed result stays **mod-262 SOLO offline co-op**.

---

## 10. Reopen playbook (only if the frontier is revisited)

1. **Merge the duplicate hook**: make `RV-ENC`/`RV-PROBE` a single hook on `FUN_11214630`
   so the capture reliably fires.
2. **Fix the `RV-PKT` POST-read fault** (`rc=-1`) so an entry→exit pair can be dumped.
3. **Pin the socket-driver vtable** at `conn+0x9c` (the `driver=0x0` read was the blocker);
   read the real driver pointer before swapping its `+0x10`/`+0x1c` send slots.
4. **Crack the OpenSSL RC4 key schedule** statically: locate the RC4 KSA/PRGA in the
   static OpenSSL and the per-connection key material (key = `f(connId, STATIC_SECRET)`).
5. With any one of these, decrypt `rv.pcap`, read the RV RMC method IDs/bodies, then adapt
   `scripts/rv_server/` (plain PRUDP v1 → `prudps`) and redirect RV to localhost.

---

## 11. Key anchors (reference)

| Symbol (RVA / name) | Role |
|---|---|
| `0x01214630` `FUN_11214630` | RV `WSASendTo` wrapper; **receives already-encrypted** buffer |
| `0x012161b0` `FUN_112161b0` | data send path; **entry buffer already encrypted** |
| `0x016bebbc` | packet-object vtable; **no cipher in any slot** |
| `0x012201e0` `FUN_112201e0` | packet size/fragment serializer (slot 18) |
| `0x01231860 / 3a780 / 390f0 / 3d140 / 2fd60 / 3cd20` | Quazal reliable send path; **no cipher** |
| `0x01236020 / 0x01218d40` | only XOR here is the **/GS stack guard** |
| `0x01219270` `FUN_11219270` | MD5_Update — **0 RV-range hits** (28fe) |
| OpenSSL 0.9.8l (static) | DH + RC4 + MD5 + SHA1; the cipher is **RC4**, dispatched via vtable |

**Endpoints (public):** `203.132.26.146` / `203.132.26.143`, UDP **21251** (Quazal RendezVous).
**RV URI (in-binary string):** `prudps:/address=%s;port=%d;stream=3;sid=1;PID=2;CID=1;type=2`.

**Artifacts:** `scripts/rv_capture/` (`rv.pcap`, `rv_enc_extract.py`),
`scripts/pktmon_rv_capture.ps1`, `scripts/rv_server/` (plain PRUDP v1, needs `prudps`),
`scripts/run_d0_reallogin.ps1` / `stop_d0_reallogin.ps1`, `src/coop_shim/` (observers),
Ghidra `FC3d3d11_live` + `scripts_live/DecompSendPath.java`, `DumpVtbl2.java`.
