#!/usr/bin/env python3
"""check_clean_install.py - verify a freshly deployed FC3_clean install is a GENUINE, un-modded
build suitable for the real-login RendezVous control (NOTES 28f1 follow-up).

It does NOT install anything. Read-only. It checks:
  1) bin\\FC3_d3d11.dll exists and its MD5 == the known vanilla 1.05 hash (46bcc097...) and
     byte-equals the local vanilla reference TestFC3_105\\bin_backup_vanilla105\\FC3_d3d11.dll
     (mod-262 patches 7 bytes of this exact file, so equality == clean).
  2) no mod / our-harness artifacts are present in bin (coop_shim, uplay_api.log, fake-online flag,
     uplay_r1_loader.orig backups, bin\\real stash, ubiorbit from a mod drop).
  3) reports the loader set + sizes so a human can eyeball the genuine online loader.

Run (read-only): python -I scripts/check_clean_install.py ["H:\\FarCryCoop\\FC3_clean"]
Exit 0 = clean, 1 = problems.
"""
import sys, os, hashlib, glob

VANILLA_105_MD5_PREFIX = "46bcc097"  # known vanilla 1.05 FC3_d3d11.dll
REFERENCE = r"H:\FarCryCoop\TestFC3_105\bin_backup_vanilla105\FC3_d3d11.dll"
MOD_MARKERS = [
    "coop_shim.dll", "coop_shim.log", "uplay_api.log", "coop_fakeonline.flag",
    "uplay_proxy*", "*.orig-*", "bin_real_backup*",
]

def md5(path):
    h = hashlib.md5()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()

def main():
    clean = sys.argv[1] if len(sys.argv) > 1 else r"H:\FarCryCoop\FC3_clean"
    b = os.path.join(clean, "bin")
    dll = os.path.join(b, "FC3_d3d11.dll")
    problems = []
    print("=== check_clean_install:", clean, "===")
    if not os.path.isdir(b):
        print("FAIL: no bin dir at", b); return 1
    if not os.path.exists(dll):
        print("FAIL: no FC3_d3d11.dll in", b); return 1

    m = md5(dll)
    print("FC3_d3d11.dll md5 :", m, "(size %d)" % os.path.getsize(dll))
    if m.startswith(VANILLA_105_MD5_PREFIX):
        print("  md5 matches known vanilla 1.05 -> good")
    else:
        print("  md5 does NOT match vanilla 1.05 (46bcc097...). This may be 1.06 (916c6d4b) or a modded build.")
        if not m.startswith("916c6d4b"):
            problems.append("FC3_d3d11.dll md5 is neither vanilla 1.05 nor vanilla 1.06")

    # byte compare to the local vanilla 1.05 reference (the gold test against the 7-byte mod patch)
    if os.path.exists(REFERENCE):
        same = open(dll, "rb").read() == open(REFERENCE, "rb").read()
        print("vs vanilla 1.05 reference (%s): %s" % (os.path.basename(os.path.dirname(REFERENCE)), "IDENTICAL (clean)" if same else "DIFFERS (modded?)"))
        if not same and m.startswith(VANILLA_105_MD5_PREFIX) is False:
            pass  # different version, byte diff expected
        elif not same:
            problems.append("FC3_d3d11.dll differs from the vanilla 1.05 reference (mod patch present?)")
    else:
        print("reference not present, skipped byte-compare:", REFERENCE)

    # artifacts / mod leftovers in bin
    print("scanning bin for mod / harness artifacts...")
    found = []
    for mk in MOD_MARKERS:
        for hit in glob.glob(os.path.join(b, mk)):
            found.append(os.path.basename(hit))
    real = os.path.isdir(os.path.join(b, "real"))
    if real:
        found.append("real/ dir (our proxy stash)")
    if found:
        print("  PRESENT (should be absent on a clean install):", sorted(set(found)))
        problems.append("mod/harness artifacts present: " + ", ".join(sorted(set(found))))
    else:
        print("  none found -> clean")

    # loaders
    loaders = [f for f in os.listdir(b) if "loader" in f.lower()]
    print("loaders:", {f: os.path.getsize(os.path.join(b, f)) for f in loaders})
    if "uplay_r1_loader.dll" in loaders:
        print("  uplay_r1_loader.dll present (genuine 1.05 uses this); size sanity: %d bytes" % os.path.getsize(os.path.join(b, "uplay_r1_loader.dll")))
    if "upc_r1_loader.dll" in loaders:
        print("  upc_r1_loader.dll present (genuine online loader)")

    print()
    if problems:
        print("RESULT: NOT CLEAN / SUSPICIOUS:")
        for pr in problems:
            print("  -", pr)
        return 1
    print("RESULT: LOOKS CLEAN (vanilla FC3_d3d11.dll, no mod/harness artifacts). Good to run the real-login control.")
    return 0

if __name__ == "__main__":
    sys.exit(main())
