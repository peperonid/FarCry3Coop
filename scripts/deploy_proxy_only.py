# deploy_proxy_only.py - install/rollback ONLY the loader proxy (NOT the coop_shim) in a FC3
# working copy's bin/, so farcry3_d3d11.exe can be launched directly OUTSIDE Ubisoft Connect with
# FC3COOP_FAKE_ONLINE=1 (boots to menu; NOTES 28bh). The proxy forwards every UPLAY export to the
# real loader, which it loads from <bin>\real\uplay_r1_loader.dll, and only fakes the online-service
# overlapped op when FC3COOP_FAKE_ONLINE is set (or coop_fakeonline.flag exists next to it).
# Reversible: `restore` puts the original bin\uplay_r1_loader.dll back.
#   python -I deploy_proxy_only.py deploy   [GAME_DIR=H:\\FarCryCoop\\TestFC3_105]
#   python -I deploy_proxy_only.py restore  [GAME_DIR=...]
#   python -I deploy_proxy_only.py status   [GAME_DIR=...]
import sys, os, shutil, time, glob

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROXY_SRC = os.path.join(REPO, "src", "uplay_proxy", "uplay_r1_loader.dll")
REFUSE = os.path.normcase(r"H:\Uplay\Far Cry 3")

def resolve(argv):
    g = argv[0] if argv and not argv[0].lower() in ("deploy", "restore", "status") else "H:\\FarCryCoop\\TestFC3_105"
    return os.path.normpath(g)

def binp(game): return os.path.join(game, "bin")

def status(game):
    b = binp(game); loader = os.path.join(b, "uplay_r1_loader.dll")
    real = os.path.join(b, "real", "uplay_r1_loader.dll")
    print("loader bin :", loader, (os.path.getsize(loader) if os.path.exists(loader) else "MISSING"))
    print("loader real:", real, (os.path.getsize(real) if os.path.exists(real) else "MISSING"))
    print("baks       :", [os.path.basename(x) for x in glob.glob(loader + ".orig-*")])
    flag = os.path.join(b, "coop_fakeonline.flag")
    print("fakeonline.flag:", os.path.exists(flag))

def deploy(game):
    b = binp(game); loader = os.path.join(b, "uplay_r1_loader.dll")
    if os.path.normcase(game) == REFUSE:
        print("ERROR: refusing the real install"); return
    if not os.path.exists(PROXY_SRC):
        print("ERROR: built proxy missing:", PROXY_SRC, "\n  build: cd src\\uplay_proxy && build_x86.bat"); return
    if not os.path.exists(loader):
        print("ERROR: no uplay_r1_loader.dll in", b); return
    realdir = os.path.join(b, "real"); os.makedirs(realdir, exist_ok=True)
    realdst = os.path.join(realdir, "uplay_r1_loader.dll")
    # if real loader is not yet stashed, the current bin loader IS the real one -> stash + backup it
    if not os.path.exists(realdst):
        ts = time.strftime("%Y%m%d-%H%M%S")
        shutil.copy2(loader, os.path.join(b, "uplay_r1_loader.dll.orig-" + ts))
        shutil.copy2(loader, realdst)
        print("stashed real loader -> real/uplay_r1_loader.dll (+ .orig backup)")
    shutil.copy2(PROXY_SRC, loader)
    print("deployed proxy -> bin/uplay_r1_loader.dll (size %d)" % os.path.getsize(loader))
    print("now launch with env FC3COOP_FAKE_ONLINE=1 (see RUN note). Rollback: python -I deploy_proxy_only.py restore")

def restore(game):
    b = binp(game); loader = os.path.join(b, "uplay_r1_loader.dll")
    realdst = os.path.join(b, "real", "uplay_r1_loader.dll")
    if os.path.exists(realdst):
        shutil.copy2(realdst, loader)
        print("restored real loader from real/uplay_r1_loader.dll")
    else:
        baks = sorted(glob.glob(loader + ".orig-*"))
        if not baks:
            print("no real copy or backup to restore from"); return
        shutil.copy2(baks[-1], loader); print("restored from", os.path.basename(baks[-1]))
    # leave bin/real in place (harmless)

if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "status"
    game = resolve(sys.argv[2:])
    {"deploy": deploy, "restore": restore, "status": status}.get(cmd, status)(game)
