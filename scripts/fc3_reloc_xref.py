#!/usr/bin/env python3
"""
fc3_reloc_xref.py — cross-reference an absolute global VA in a relocated MSVC binary.

Why this exists instead of a plain byte search: FC3.dll / FC3_d3d11.dll are relocated PE images.
A `cmp eax, [0x12829424]` (a1 / 8b 05 disp32) does NOT contain the literal bytes of the target VA in
the file, because the relocation table adds the image base at load time. Searching raw bytes for the VA
finds only relocation *entries*, never the accesses. So the correct route is: walk .reloc, keep entries
whose target RVA is the global, then map each entry's file offset back to a VA and disassemble the
function that contains it.

Reads the target PE only; writes nothing.

Usage:
  python -I tools/fc3_reloc_xref.py <pe path> <VA> [<VA> ...]
"""
import sys, struct
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
sys.path.insert(0, r"D:/Projects/Co-op Anything/FarCryCoop/games/far-cry-3/tools")
sys.path.insert(0, r"D:/Projects/Co-op Anything/FarCryCoop/tools")
from fc3_addrmap import PE  # canonical section/VA mapping (hub branch owns this file)
from fc3_iat_reverse import build_map  # IAT slot -> "DLL!func"

try:
    import capstone
except Exception:
    print("capstone missing: python -m pip install capstone"); sys.exit(1)

RELOC_TYPES = {0: "ABS", 3: "HIGH_ABS", 5: "DIR64", 6: "HIGH_ABS32", 10: "DIR32"}


def reloc_targets(pe):
    """(file offset of the patched field, target VA) for every relocation entry."""
    d = pe.d
    out = []
    # .reloc is at the end of the header area; find it by name
    for nm, (va, ptr, size) in pe.sec.items():
        if nm != ".reloc":
            continue
        blob = d[ptr:ptr + size]
        pos = 0
        while pos + 8 <= len(blob):
            page_rva, block_size = struct.unpack_from("<II", blob, pos)
            if block_size < 8 or block_size > len(blob) - pos:
                break
            n = (block_size - 8) // 2
            for i in range(n):
                entry = struct.unpack_from("<H", blob, pos + 8 + i * 2)[0]
                typ = entry >> 12
                off = entry & 0xFFF
                out.append((ptr + pos + 8 + i * 2, pe.base + page_rva + off, typ))
            pos += block_size
    return out


def main(path, targets):
    pe = PE(path)
    d = pe.d
    _, iat = build_map(path)
    targets = [int(t, 16) for t in targets]
    rl = reloc_targets(pe)
    print("%d relocation entries" % len(rl))
    # map file offset -> VA (for the field that gets patched)
    for target in targets:
        sites = sorted({va for off, va, typ in rl if va == target})
        print("== global 0x%08x %s" % (target, repr(_cstr(pe, target))))
        print("   %d relocation entry/entries point at it (self-referential storage)" % len(sites))
        # who loads the *address* of the global (mov reg, offset / lea): immediates in .text
        refs = pe.code_refs(target)
        print("   in-file direct references (push/mov imm32): %d" % len(refs))
        for off in refs[:12]:
            print("      at %s" % hex(pe.off2va(off) or off))
        # who accesses it via reloc: scan .text for a1/8b xx disp32 where disp32 == target
        tva, tend, tptr, tsize = pe.text
        text = d[tptr:tptr + tsize]
        hits = []
        i = 0
        while True:
            p = text.find(b"\xa1", i)
            if p < 0:
                break
            v = struct.unpack_from("<I", text, p + 1)[0]
            if v == target:
                hits.append(tva + p)
            i = p + 1
        import re as _re
        for m in _re.finditer(rb"\x8b[\x05\x0d\x15\x1d\x2d\x35\x3d]\x24\x82\x12", text):
            pass  # placeholder; see below
        pat = struct.pack("<I", target)
        for m in _re.finditer(rb"\x8b([\x05\x0d\x15\x1d\x2d\x35\x3d])" + pat, text):
            hits.append(tva + m.start())
        for m in _re.finditer(rb"\x89\x05" + pat, text):
            hits.append(tva + m.start())
        for m in _re.finditer(rb"\x83[\x0d\x15\x2d\x3d]" + pat, text):
            hits.append(tva + m.start())
        hits = sorted(set(hits))
        print("   direct disp32 accesses in .text: %d" % len(hits))
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        md.detail = True
        for va in hits[:40]:
            off = pe.va2off(va)
            ins = next(md.disasm(d[off:off + 12], va), None)
            print("      0x%08x  %s" % (va, ("%s %s" % (ins.mnemonic, ins.op_str)) if ins else "?"))


def _cstr(pe, va, maxlen=80):
    off = pe.va2off(va)
    if off is None:
        return None
    raw = pe.d[off:off + maxlen].split(b"\0", 1)[0]
    if 0 < len(raw) and all(0x20 <= b < 0x7f for b in raw):
        return raw.decode("latin-1")
    return None


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2:])
