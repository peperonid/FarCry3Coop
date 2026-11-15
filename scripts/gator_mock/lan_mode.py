# lan_mode.py - backup and toggle Far Cry 3's GamerProfile.xml <NetworkProfile SessionProvider="...">
# to try the in-engine LAN session provider (no Ubisoft / no Quazal RV) for local 2-4p co-op.
# FC3 has explicit "Online" vs "Lan" session providers (see NOTES 28bu); empty SessionProvider
# defaults to online/RdV. Usage:
#   python -I lan_mode.py find                       # locate GamerProfile.xml
#   python -I lan_mode.py status [profile.xml]       # show current SessionProvider
#   python -I lan_mode.py lan    [profile.xml]       # backup + set SessionProvider="Lan"
#   python -I lan_mode.py online [profile.xml]       # backup + set SessionProvider="" (default/online)
#   python -I lan_mode.py restore [profile.xml]      # restore the last backup
# An explicit path always wins; otherwise the profile is searched under common Documents roots.
import sys, os, re, glob, time, shutil

CANDIDATE_ROOTS = []
for envv in ("USERPROFILE", "HOME"):
    if os.environ.get(envv):
        CANDIDATE_ROOTS.append(os.environ[envv])
for drv in ("C:", "D:", "E:"):
    CANDIDATE_ROOTS.append(drv + "\\")

def find_profile():
    # prefer common Documents locations before a deeper drive scan
    prefs = []
    for root in CANDIDATE_ROOTS:
        for pat in (
            os.path.join(root, "Documents", "My Games", "Far Cry 3", "GamerProfile.xml"),
            os.path.join(root, "OneDrive", "Documents", "My Games", "Far Cry 3", "GamerProfile.xml"),
            os.path.join(root, "*", "Documents", "My Games", "Far Cry 3", "GamerProfile.xml"),
            os.path.join(root, "Мои документы", "Documents", "My Games", "Far Cry 3", "GamerProfile.xml"),
        ):
            prefs += glob.glob(pat)
    for p in prefs:
        if os.path.exists(p):
            return p
    # bounded deep search on user roots only
    for root in [r for r in CANDIDATE_ROOTS if os.environ.get("USERPROFILE") and r.lower() == os.environ["USERPROFILE"].lower()]:
        for p in glob.glob(os.path.join(root, "**", "My Games", "Far Cry 3", "GamerProfile.xml"), recursive=True):
            if os.path.exists(p):
                return p
    return None

def resolve(argv):
    return argv[0] if argv and argv[0].lower().endswith(".xml") else find_profile()

def read(p): return open(p, "r", encoding="utf-8", errors="ignore").read()

def get_provider(t):
    m = re.search(r'SessionProvider="([^"]*)"', t)
    return m.group(1) if m else None

def backup(p):
    bk = p + ".bak-" + time.strftime("%Y%m%d-%H%M%S")
    shutil.copy2(p, bk)
    return bk

def set_provider(p, value):
    t = read(p)
    if 'SessionProvider=' not in t:
        print("ERROR: no SessionProvider attribute found in", p); return
    cur = get_provider(t)
    t2 = re.sub(r'SessionProvider="[^"]*"', 'SessionProvider="%s"' % value, t, count=1)
    if t2 == t:
        print("already SessionProvider=\"%s\"" % cur); return
    bk = backup(p)
    open(p, "w", encoding="utf-8").write(t2)
    print("SessionProvider: \"%s\" -> \"%s\"  (backup: %s)" % (cur, value, bk))

def restore(p):
    bks = sorted(glob.glob(p + ".bak-*"))
    if not bks:
        print("no backups for", p); return
    shutil.copy2(bks[-1], p)
    print("restored from", bks[-1])

if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "status"
    if cmd == "find":
        print(find_profile() or "not found"); sys.exit(0)
    p = resolve(sys.argv[2:])
    if not p or not os.path.exists(p):
        print("GamerProfile.xml not found; pass its full path explicitly"); sys.exit(2)
    if cmd == "status":
        print("file:", p); print("SessionProvider=\"%s\"" % get_provider(read(p)))
    elif cmd == "lan":
        set_provider(p, "Lan")
    elif cmd == "online":
        set_provider(p, "")
    elif cmd == "restore":
        restore(p)
    else:
        print(__doc__)
