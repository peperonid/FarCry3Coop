import struct
# read-only: dump first bytes at candidate send-path RVAs of FC3_d3d11.dll to check for 55 8B EC prologues
dll = r"H:\FarCryCoop\FC3_clean\bin\FC3_d3d11.dll"
d = open(dll, "rb").read()
pe = struct.unpack_from("<I", d, 0x3c)[0]
assert d[pe:pe+4] == b"PE\0\0"
nsec = struct.unpack_from("<H", d, pe+6)[0]
sizeopt = struct.unpack_from("<H", d, pe+20)[0]
opt = pe+24
imgbase = struct.unpack_from("<I", d, opt+28)[0]
sp = pe+24+sizeopt   # section table
secs = []
for i in range(nsec):
    nm = d[sp+i*40:sp+i*40+8].rstrip(b"\0").decode("latin1","replace")
    vs = struct.unpack_from("<I", d, sp+i*40+8)[0]
    va = struct.unpack_from("<I", d, sp+i*40+12)[0]
    rs = struct.unpack_from("<I", d, sp+i*40+16)[0]
    raw = struct.unpack_from("<I", d, sp+i*40+20)[0]
    span = max(vs, rs)
    secs.append((nm, va, span, raw))
    print("%-9s va=0x%08x span=0x%08x raw=0x%08x" % (nm, va, span, raw))
def rva_to_off(r):
    for nm, va, span, raw in secs:
        if va <= r < va+span:
            return raw+(r-va)
    return None
cands = {"FUN_1123cd20":0x0123cd20,"FUN_1123a780":0x0123a780,"FUN_11231860":0x01231860,
         "FUN_112390f0":0x012390f0,"FUN_1122fd60":0x0122fd60,"FUN_1121d980":0x0121d980,
         "FUN_112161b0":0x012161b0,"FUN_11218510":0x01218510,"FUN_11214630":0x01214630}
print("\nimgbase 0x%x" % imgbase)
for name, r in cands.items():
    off = rva_to_off(r)
    b = d[off:off+8] if off is not None else b""
    tag = "558BEC-OK" if b[:3] == b"\x55\x8b\xec" else "other"
    print("%-14s rva=0x%08x off=%s bytes=%-16s %s" % (name, r, hex(off) if off else "-", b.hex(" "), tag))
