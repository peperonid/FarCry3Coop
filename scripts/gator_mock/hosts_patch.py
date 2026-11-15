# Add / remove a hosts-file redirect that points Ubisoft FC3 online hosts to 127.0.0.1 so the
# local gator_mock HTTP logger can intercept them. MUST be run elevated (hosts is a system file).
#   python -I hosts_patch.py apply     (backs up hosts first, then adds redirects)
#   python -I hosts_patch.py restore   (restores the backup)
#   python -I hosts_patch.py status
import sys, os
HOSTS = os.path.join(os.environ.get("SystemRoot", r"C:\Windows"), r"System32\drivers\etc\hosts")
BK = os.path.join(os.path.dirname(os.path.abspath(__file__)), "hosts.backup")
MARK = "# --- FC3Coop gator redirect ---"
ENTRIES = [
    "127.0.0.1 gconnect.ubi.com",
    "127.0.0.1 onlineconfigservice.ubi.com",
    "127.0.0.1 cert-onlineconfigservice.ubi.com",
    "127.0.0.1 uat-onlineconfigservice.ubi.com",
]
def read(): return open(HOSTS, encoding="utf-8", errors="ignore").read()
def apply():
    open(BK, "w", encoding="utf-8").write(read())
    t = read()
    if MARK in t:
        print("already applied (backup written)"); return
    t += ("\n" + MARK + "\n" + "\n".join(ENTRIES) + "\n")
    open(HOSTS, "w", encoding="utf-8").write(t)
    print("APPLIED redirects; hosts backup: %s" % BK)
def restore():
    if not os.path.exists(BK): print("no backup at %s" % BK); return
    open(HOSTS, "w", encoding="utf-8").write(open(BK, encoding="utf-8").read())
    print("RESTORED hosts from %s" % BK)
def status(): print("applied" if MARK in read() else "not applied")
if __name__ == "__main__":
    {"apply": apply, "restore": restore, "status": status}.get(sys.argv[1] if len(sys.argv) > 1 else "status")()
