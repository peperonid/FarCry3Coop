#!/usr/bin/env python3
# rv_enc_extract.py - offline recovery of the Quazal prudps RC4 keystream from the coop_shim RV-ENC log.
#
# The shim hook (FC3COOP_RV_ENC=1, NOTES 28g2) prints, for every RV send-wrapper call on
# FUN_11214630, the packet buffer BEFORE the socket driver encrypts (PRE = plaintext) and AFTER
# (POST). If the driver encrypts IN PLACE (payload_changed=1), then for the encrypted region:
#     keystream[i] = PRE[i] XOR POST[i]
# Because the prudps keystream is CONSTANT per connection (proven in NOTES 28g0 from the 88x identical
# retransmits), one such capture yields the whole keystream for that connection's connId -> decrypt any
# captured pcap packet of the same connId offline, with NO static secret.
#
# Usage:
#   python -I rv_enc_extract.py <coop_shim.log>            # show recovered keystreams
#   python -I rv_enc_extract.py <coop_shim.log> --apply rv.pcap out.txt   # also decrypt matching pcap pkts
#
# Log line grammar (shim LOGF):
#   shim: RV-ENC #N this=... driver=... slotSel=... len=L connId=XXXXXXXX
#   shim:   PRE(plaintext):
#     0000: aa bb ...
#   shim: RV-ENC #N POST rc=.. payload_changed=.. ...
#   shim:   POST(after):
#     0000: .. .. ..
import sys, re, glob

HDR = re.compile(r"RV-(ENC-ST|ENC|DRV10|DRV1C|PKT4|PKT) #(\d+) .*len=(\d+) connId=([0-9a-fA-F]{8})")
POSTH = re.compile(r"RV-(ENC-ST|ENC|DRV10|DRV1C|PKT4|PKT) #(\d+) POST rc=(-?\d+) payload_changed=(\d+)")
ROW = re.compile(r"([0-9a-fA-F]{4}):\s+((?:[0-9a-fA-F]{2}\s*)+)")

def parse_rows(lines, i, want):
    """consume hexdump rows starting at index i until we've read `want` bytes; return (bytes,newi)."""
    out = {}
    while i < len(lines) and len(out) < want + 16:
        m = ROW.search(lines[i])
        if not m:
            if out: break
            i += 1; continue
        off = int(m.group(1), 16)
        for j, hx in enumerate(m.group(2).split()):
            out[off + j] = int(hx, 16)
        i += 1
    if not out: return None, i
    top = max(out); b = bytearray((out.get(k, 0)) for k in range(top + 1))
    return b, i

def extract(logpath):
    lines = open(logpath, encoding="utf-8", errors="replace").read().splitlines()
    caps = {}  # n -> dict
    i = 0
    while i < len(lines):
        m = HDR.search(lines[i])
        if m:
            src = m.group(1); n = int(m.group(2)); L = int(m.group(3)); cid = m.group(4).lower()
            d = caps.setdefault((src, n), {"len": L, "cid": cid, "src": src})
            i += 1
            # PRE label?
            while i < len(lines) and "PRE(" not in lines[i] and HDR.search(lines[i]) is None:
                i += 1
            if i < len(lines) and "PRE(" in lines[i]:
                i += 1
                b, i = parse_rows(lines, i, L)
                if b is not None: d["pre"] = b
            # POST
            while i < len(lines) and not POSTH.search(lines[i]) and not HDR.search(lines[i]):
                i += 1
            pm = POSTH.search(lines[i]) if i < len(lines) else None
            if pm and (pm.group(1), int(pm.group(2))) == (src, n):
                d["rc"] = int(pm.group(3)); d["changed"] = int(pm.group(4)); i += 1
                while i < len(lines) and "POST(" not in lines[i] and HDR.search(lines[i]) is None:
                    i += 1
                if i < len(lines) and "POST(" in lines[i]:
                    i += 1
                    b, i = parse_rows(lines, i, L)
                    if b is not None: d["post"] = b
            continue
        i += 1
    return caps

def keystreams(caps):
    """Return {connId: (start_offset, keystream_bytes)} from changed PRE/POST pairs."""
    res = {}
    for n, d in caps.items():
        if d.get("changed") != 1 or "pre" not in d or "post" not in d: continue
        pre, post = d["pre"], d["post"]
        m = min(len(pre), len(post))
        first = None
        for k in range(m):
            if pre[k] != post[k]:
                first = k; break
        if first is None: continue
        ks = bytes(pre[k] ^ post[k] for k in range(first, m))
        cid = d["cid"]
        # keep the longest keystream seen per connId, aligned to a common start
        if cid not in res or len(ks) > len(res[cid][1]):
            res[cid] = (first, ks)
    return res


def parse_pcap_rv(pcap):
    import struct
    data = open(pcap, "rb").read(); i = 0; n = len(data); out = []
    while i + 8 <= n:
        bt, bl = struct.unpack_from("<II", data, i)
        if bl < 8 or i + bl > n: break
        if bt == 0x00000006:
            cl = struct.unpack_from("<I", data, i + 20)[0]; pk = data[i + 28:i + 28 + cl]
            b = rv_udp(pk)
            if b: out.append(b)
        i += bl
    return out

def rv_udp(pkt):
    import struct
    if len(pkt) < 14: return None
    et = struct.unpack_from(">H", pkt, 12)[0]; off = 14
    if et == 0x8100: off = 18
    ip = pkt[off:]
    if len(ip) < 20 or (ip[0] >> 4) != 4 or ip[9] != 17: return None
    ihl = (ip[0] & 0xf) * 4; u = ip[ihl:]
    if len(u) < 8: return None
    sp, dp = struct.unpack_from(">HH", u, 0); pl = struct.unpack_from(">H", u, 4)[0] - 8
    body = u[8:8 + max(pl, 0)]
    if sp != 21251 and dp != 21251: return None
    return body

def apply_to_pcap(ks, pcap):
    seen = {}
    for body in parse_pcap_rv(pcap):
        if len(body) < 12: continue
        cid = body[4:8].hex()
        if cid not in ks: continue
        start, ksb = ks[cid]
        pt = bytearray(body)
        for o in range(start, min(len(body), start + len(ksb))):
            pt[o] = body[o] ^ ksb[o - start]
        key = (cid, len(body))
        if key in seen: continue
        seen[key] = pt
    print("\nDecrypted pcap packets whose connId matches a recovered keystream:", len(seen))
    for (cid, ln), pt in sorted(seen.items()):
        print(f"  connId {cid} len={ln}  header={bytes(pt[:12]).hex(' ')}")
        ascii_tail = "".join(chr(c) if 32 <= c < 127 else "." for c in pt[12:])
        print("    plaintext-ish:", ascii_tail[:120])

def mtp(caps, src_pref=("ENC",)):
    """Many-time-pad: data packets (type 0x32) of the SAME connection share a constant per-packet
    keystream, so CT1 ^ CT2 = pt1 ^ pt2. Group PRE buffers by connId, drop retransmit variants
    (bytes differing only at offsets {3,19,20}), and XOR genuinely different messages."""
    groups = {}
    for key, d in caps.items():
        if "pre" not in d: continue
        if not key[0].startswith(src_pref): continue
        pre = d["pre"]
        if len(pre) < 13 or pre[2] != 0x32: continue
        cid = d["cid"]
        groups.setdefault(cid, []).append(pre)
    VARIANT = {3, 19, 20}
    for cid, bufs in groups.items():
        # canonicalize retransmit variants: zero out the variant columns, dedup
        canon = {}
        for b in bufs:
            c = bytearray(b)
            for o in VARIANT:
                if o < len(c): c[o] = 0
            canon.setdefault(bytes(c), b)
        distinct = list(canon.values())
        print(f"\n[MTP] connId {cid}: {len(bufs)} data pkts -> {len(distinct)} distinct messages")
        if len(distinct) < 2:
            print("     only one distinct message (all retransmits) -> no MTP leverage here")
            continue
        for ai in range(len(distinct)):
            for bj in range(ai+1, len(distinct)):
                A, B = distinct[ai], distinct[bj]
                L = min(len(A), len(B))
                x = bytes(A[k] ^ B[k] for k in range(12, L))
                asc = "".join(chr(c) if 32 <= c < 127 else "." for c in x)
                runs = re.findall(r"[ -~]{3,}", asc)
                print(f"   {len(A)}B ^ {len(B)}B -> xor-runs {runs[:5]}")
                if runs:
                    print("     (printable runs in pt_i^pt_j: crib-drag candidates)")


def pair_with_pcap(caps, pcap):
    """Pair a PRE dump from the shim log (plaintext at the send wrapper, e.g. RV-ENC-ST on sendto) with the
    matching on-wire CIPHERTEXT packet from a pcap captured in the SAME session. The header bytes b0..b8 (magic,
    type, retransmit, connId, seq) are cleartext in BOTH the plaintext dump and the ciphertext packet, so we match
    by (connId, length, seq) and require b0..b4 to agree, then keystream = PRE ^ CT over the body. If PRE is itself
    already ciphertext the xor is ~all-zero over the body and we skip it (no new info)."""
    import struct
    data = open(pcap, "rb").read(); i = 0; n = len(data); ct = []
    while i + 8 <= n:
        bt, bl = struct.unpack_from("<II", data, i)
        if bl < 8 or i + bl > n: break
        if bt == 0x00000006:
            cl = struct.unpack_from("<I", data, i + 20)[0]; pk = data[i + 28:i + 28 + cl]
            if len(pk) >= 14:
                et = struct.unpack_from(">H", pk, 12)[0]; off = 18 if et == 0x8100 else 14
                ip = pk[off:]
                if len(ip) >= 20 and (ip[0] >> 4) == 4 and ip[9] == 17:
                    ihl = (ip[0] & 0xf) * 4; u = ip[ihl:]
                    if len(u) >= 8:
                        dp = struct.unpack_from(">H", u, 2)[0]; pl = struct.unpack_from(">H", u, 4)[0] - 8
                        if dp == 21251:
                            body = u[8:8 + max(pl, 0)]
                            if len(body) >= 12: ct.append(body)
        i += bl
    idx = {}
    for b in ct:
        idx.setdefault((b[4:8].hex(), len(b), b[8]), []).append(b)
    print("\n[pair] pcap outbound RV packets to :21251 =", len(ct))
    res = {}
    for key, d in caps.items():
        if "pre" not in d: continue
        pre = d["pre"]; cid = d["cid"]
        if len(pre) < 13: continue
        cands = idx.get((cid, len(pre), pre[8] if len(pre) > 8 else 0), [])
        for cb in cands:
            # header must agree in cleartext bytes b0..b4 (magic/type/var/connId first byte)
            if pre[4] != cb[4] or pre[0] != cb[0]: continue
            m = min(len(pre), len(cb))
            ks = bytes(pre[k] ^ cb[k] for k in range(12, m))
            nz = sum(1 for x in ks if x)
            if nz < max(2, len(ks) // 4):  # PRE was already ciphertext (xor ~zero) -> no plaintext captured
                continue
            start = 12
            if cid not in res or len(ks) > len(res[cid][1]):
                res[cid] = (start, ks)
    if res:
        print("[pair] recovered keystreams by log-vs-pcap pairing for connIds:", ",".join(sorted(res)))
        for cid, (start, ksb) in sorted(res.items()):
            print(f"  connId {cid}: keystream from offset {start}, length {len(ksb)}")
            for o in range(0, min(len(ksb), 96), 16):
                print("   +%04x:" % (start + o), " ".join("%02x" % x for x in ksb[o:o+16]))
    else:
        print("[pair] no plaintext-vs-ciphertext match (no PRE dump was plaintext for a pcap packet, or connIds differ).")
    return res


def main():
    if len(sys.argv) < 2:
        print(__doc__); return
    caps = extract(sys.argv[1])
    print("parsed RV-ENC packets:", len(caps))
    ch = [n for n, d in caps.items() if d.get("changed") == 1]
    print("payload_changed=1 (in-place, yields keystream):", len(ch))
    ks = keystreams(caps)
    for cid, (start, ksb) in sorted(ks.items()):
        print(f"\nconnId {cid}: keystream starts at offset {start}, length {len(ksb)}")
        for o in range(0, min(len(ksb), 128), 16):
            print("  +%04x:" % (start + o), " ".join("%02x" % x for x in ksb[o:o+16]))
    pcap_arg = None
    if "--apply" in sys.argv:
        ai = sys.argv.index("--apply")
        if ai + 1 < len(sys.argv): pcap_arg = sys.argv[ai + 1]
    if pcap_arg:
        try:
            pk = pair_with_pcap(caps, pcap_arg)
            for cid, (start, ksb) in pk.items():
                if cid not in ks or len(ksb) > len(ks[cid][1]): ks[cid] = (start, ksb)
        except FileNotFoundError as e:
            print("[pair] pcap not found:", e)
    if not ks:
        mtp(caps)
        print("\nNo keystream recovered (no in-place pair and no log-vs-pcap plaintext match). If RV-ENC/RV-ENC-ST")
        print("PRE dumps are all already-ciphertext, the cipher sits upstream: use the THIS[] window offsets the")
        print("shim logged to pin the real driver vtable, then hook its send slot for an in-place pair (NOTES 28i2).")
        return
    print("\nOK: recovered keystreams for connIds:", ",".join(sorted(ks)))
    mtp(caps)
    if pcap_arg:
        apply_to_pcap(ks, pcap_arg)
    else:
        print("Next: re-run with --apply <rv.pcap> to decrypt captured packets of these connIds.")

if __name__ == "__main__":
    main()
